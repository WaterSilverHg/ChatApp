#pragma once

#include "global.h"

#include"../server/coroutine/Coroutines.hpp"

// 前向声明，解决循环依赖
class RedisPubSubManager;

class AppWebSocket {
private:
    OATPP_COMPONENT(std::shared_ptr<oatpp::async::Executor>, m_executor, "ws-server-exec");
    std::weak_ptr<RedisPubSubManager> m_pubSubManager;
public:
    using AsyncWebSocket = oatpp::websocket::AsyncWebSocket;
    
    void setPubSubManager(const std::shared_ptr<RedisPubSubManager>& pubSubManager);
    
    bool sendMessageToUser(const oatpp::String& userUuid, const oatpp::String& message);
    
    void batchPushMessage(const std::vector<oatpp::String>& targetUuids, const oatpp::String& message);

    struct ConnectionInfo {
        std::shared_ptr<AsyncWebSocket> socket;
        int count;
        ConnectionInfo() : count(0) {}
        ConnectionInfo(const std::shared_ptr<AsyncWebSocket>& s, int c) : socket(s), count(c) {}
    };

    int registerConnection(const oatpp::String& userUuid, const std::shared_ptr<AsyncWebSocket>& socket) {
        std::lock_guard<oatpp::concurrency::SpinLock> lock(m_lock);

        auto it = m_connections.find(userUuid);
        if (it != m_connections.end()) {
            OATPP_LOGW("AppWebSocket", "User %s already has %d connection(s), incrementing count", userUuid->c_str(), it->second.count);

            it->second.count++;
            if (it->second.socket != socket) {
                it->second.socket = socket;
            }
            OATPP_LOGI("AppWebSocket", "User connected: %s, connection count: %d",
                userUuid->c_str(), it->second.count);
            return it->second.count;
        }

        ConnectionInfo info(socket, 1);
        m_connections[userUuid] = info;
        OATPP_LOGI("AppWebSocket", "User connected: %s, connection count: %d",
            userUuid->c_str(), info.count);
        return 0;
    }

    bool decrementAndRemoveIfZero(const oatpp::String& userUuid) {
        std::lock_guard<oatpp::concurrency::SpinLock> lock(m_lock);
        auto it = m_connections.find(userUuid);
        if (it == m_connections.end()) {
            OATPP_LOGW("AppWebSocket", "User %s not found in connections", userUuid->c_str());
            return true;
        }

        it->second.count--;
        OATPP_LOGI("AppWebSocket", "User %s connection count decremented to: %d", userUuid->c_str(), it->second.count);

        if (it->second.count <= 0) {
            m_connections.erase(it);
            OATPP_LOGI("AppWebSocket", "User %s disconnected (count reached 0), remaining connections: %d",
                userUuid->c_str(), m_connections.size());
            return true;
        }
        return false;
    }

    void removeConnection(const oatpp::String& userUuid) {
        std::lock_guard<oatpp::concurrency::SpinLock> lock(m_lock);
        auto it = m_connections.find(userUuid);
        if (it != m_connections.end()) {
            if (it->second.socket) {
                m_executor->execute<SendCloseCoroutine>(it->second.socket);
            }
            m_connections.erase(it);
        }
        OATPP_LOGI("AppWebSocket", "User disconnected: %s, remaining connections: %d", userUuid->c_str(), m_connections.size());
    }

    bool isUserOnline(const oatpp::String& userUuid) {
        std::lock_guard<oatpp::concurrency::SpinLock> lock(m_lock);
        return m_connections.find(userUuid) != m_connections.end();
    }

    void sendMessageToUsers(const std::vector<oatpp::String>& userUuids, const oatpp::String& message) {
        std::lock_guard<oatpp::concurrency::SpinLock> lock(m_lock);
        for (const auto& userUuid : userUuids) {
            auto it = m_connections.find(userUuid);
            if (it != m_connections.end() && it->second.socket) {
                try {
                    m_executor->execute<SendOneMessageCoroutine>(
                        it->second.socket, message
                    );
                } catch (const std::exception& e) {
                    OATPP_LOGE("AppWebSocket", "Failed to send message to user %s: %s", userUuid->c_str(), e.what());
                }
            }
        }
    }

    void broadcastMessage(const oatpp::String& message) {
        std::lock_guard<oatpp::concurrency::SpinLock> lock(m_lock);
        for (const auto& pair : m_connections) {
            try {
                if (pair.second.socket) {
                    m_executor->execute<SendOneMessageCoroutine>(
                        pair.second.socket, message
                    );
                }
            } catch (const std::exception& e) {
                OATPP_LOGE("AppWebSocket", "Failed to broadcast message to user %s: %s", pair.first->c_str(), e.what());
            }
        }
    }

    size_t getOnlineUserCount() {
        std::lock_guard<oatpp::concurrency::SpinLock> lock(m_lock);
        return m_connections.size();
    }

    std::vector<oatpp::String> getOnlineUsers() {
        std::lock_guard<oatpp::concurrency::SpinLock> lock(m_lock);
        std::vector<oatpp::String> users;
        users.reserve(m_connections.size());
        for (const auto& pair : m_connections) {
            users.push_back(pair.first);
        }
        return users;
    }

    void clearAllConnections() {
        std::lock_guard<oatpp::concurrency::SpinLock> lock(m_lock);
        m_connections.clear();
        OATPP_LOGI("AppWebSocket", "All connections cleared");
    }

    std::unordered_map<oatpp::String, std::shared_ptr<AsyncWebSocket>> getConnectionsSnapshot() {
        std::lock_guard<oatpp::concurrency::SpinLock> lock(m_lock);
        std::unordered_map<oatpp::String, std::shared_ptr<AsyncWebSocket>> snapshot;
        for (const auto& pair : m_connections) {
            snapshot[pair.first] = pair.second.socket;
        }
        return snapshot;
    }

private:
    oatpp::concurrency::SpinLock m_lock;
    std::unordered_map<oatpp::String, ConnectionInfo> m_connections;
};