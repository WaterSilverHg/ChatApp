#include "AppWebSocket.hpp"
#include "redis/RedisPubSubManager.hpp"

void AppWebSocket::setPubSubManager(const std::shared_ptr<RedisPubSubManager>& pubSubManager) {
    m_pubSubManager = pubSubManager;
}

bool AppWebSocket::sendMessageToUser(const oatpp::String& userUuid, const oatpp::String& message) {
    std::lock_guard<oatpp::concurrency::SpinLock> lock(m_lock);
    auto it = m_connections.find(userUuid);
    if (it != m_connections.end() && it->second.socket) {
        try {
            m_executor->execute<SendOneMessageCoroutine>(
                it->second.socket, message
            );
            OATPP_LOGD("AppWebSocket", "Message sent locally to user %s", userUuid->c_str());
            return true;
        } catch (const std::exception& e) {
            OATPP_LOGE("AppWebSocket", "Failed to send message to user %s: %s", userUuid->c_str(), e.what());
            return false;
        }
    }
    
    // 用户不在本服务器，通过 Redis Pub/Sub 广播
    OATPP_LOGD("AppWebSocket", "User %s not connected locally, publishing to Redis", userUuid->c_str());
    if (auto pubSubManager = m_pubSubManager.lock()) {
        pubSubManager->publishPrivateMessage(userUuid, message);
        return true;
    }
    
    OATPP_LOGW("AppWebSocket", "User not online and no Pub/Sub manager available: %s", userUuid->c_str());
    return false;
}

void AppWebSocket::batchPushMessage(const std::vector<oatpp::String>& targetUuids, const oatpp::String& message) {
    std::lock_guard<oatpp::concurrency::SpinLock> lock(m_lock);
    std::vector<oatpp::String> localTargets;
    std::vector<oatpp::String> remoteTargets;
    
    for (auto& uuid : targetUuids) {
        auto it = m_connections.find(uuid);
        if (it != m_connections.end() && it->second.socket) {
            localTargets.push_back(uuid);
        } else {
            remoteTargets.push_back(uuid);
        }
    }
    
    // 发送给本地连接的用户
    for (auto& uuid : localTargets) {
        auto it = m_connections.find(uuid);
        if (it != m_connections.end() && it->second.socket) {
            m_executor->execute<SendOneMessageCoroutine>(it->second.socket, message);
        }
    }
    OATPP_LOGD("AppWebSocket", "Batch message sent to %d local users", localTargets.size());
    
    // 通过 Redis Pub/Sub 发送给远程用户
    if (!remoteTargets.empty()) {
        OATPP_LOGD("AppWebSocket", "Publishing batch message to %d remote users via Redis", remoteTargets.size());
        if (auto pubSubManager = m_pubSubManager.lock()) {
            pubSubManager->publishGroupMessage(remoteTargets, message);
        }
    }
}