#pragma once

#include "global.h"

#include"../server/coroutine/Coroutines.hpp"





class AppWebSocket {
private:
    OATPP_COMPONENT(std::shared_ptr<oatpp::async::Executor>, m_executor, "ws-server-exec");
public:
    using AsyncWebSocket = oatpp::websocket::AsyncWebSocket;

    void registerConnection(const oatpp::String& userUuid, const std::shared_ptr<AsyncWebSocket>& socket) {
        std::lock_guard<oatpp::concurrency::SpinLock> lock(m_lock);

        auto it = m_connections.find(userUuid);
        if (it != m_connections.end()) {
            OATPP_LOGW("AppWebSocket", "User %s already connected, closing old connection and accepting new one", userUuid->c_str());

            // 关闭旧连接
            if (it->second) {
                //it->second->sendOneFrameTextAsync("{\"type\":\"error\",\"message\":\"new_login_detected\"}");
                m_executor->execute<SendCloseCoroutine>(
                    it->second
                );
            }

            // 移除旧连接
            m_connections.erase(it);
        }

        // 添加新连接
        m_connections[userUuid] = socket;
        OATPP_LOGI("AppWebSocket", "User connected: %s, total connections: %d",
            userUuid->c_str(), m_connections.size());
    }

    void removeConnection(const oatpp::String& userUuid) {
        std::lock_guard<oatpp::concurrency::SpinLock> lock(m_lock);
        m_connections.erase(userUuid);
        OATPP_LOGI("AppWebSocket", "User disconnected: %s, remaining connections: %d", userUuid->c_str(), m_connections.size());
    }

    bool isUserOnline(const oatpp::String& userUuid) {
        std::lock_guard<oatpp::concurrency::SpinLock> lock(m_lock);
        return m_connections.find(userUuid) != m_connections.end();
    }

    bool sendMessageToUser(const oatpp::String& userUuid, const oatpp::String& message) {
        std::lock_guard<oatpp::concurrency::SpinLock> lock(m_lock);
        auto it = m_connections.find(userUuid);
        if (it != m_connections.end() && it->second) {
            try {
                m_executor->execute<SendOneMessageCoroutine>(
                    it->second,message
                );
                return true;
            } catch (const std::exception& e) {
                OATPP_LOGE("AppWebSocket", "Failed to send message to user %s: %s", userUuid->c_str(), e.what());
                return false;
            }
        }
        OATPP_LOGW("AppWebSocket", "User not online: %s", userUuid->c_str());
        return false;
    }

    void sendMessageToUsers(const std::vector<oatpp::String>& userUuids, const oatpp::String& message) {
        std::lock_guard<oatpp::concurrency::SpinLock> lock(m_lock);
        for (const auto& userUuid : userUuids) {
            auto it = m_connections.find(userUuid);
            if (it != m_connections.end() && it->second) {
                try {
                    m_executor->execute<SendOneMessageCoroutine>(
                        it->second, message
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
                if (pair.second) {
                    m_executor->execute<SendOneMessageCoroutine>(
                        pair.second, message
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

    // 获取在线连接快照（线程安全，返回副本）
    std::unordered_map<oatpp::String, std::shared_ptr<AsyncWebSocket>> getConnectionsSnapshot() {
        std::lock_guard<oatpp::concurrency::SpinLock> lock(m_lock);
        return m_connections;
    }

    //// 批量分发（带回调）：在线推送 + 离线/失败回调
    //void batchSendWithOfflineFallback(
    //    const std::vector<oatpp::String>& targetUuids,
    //    const oatpp::String& message,
    //    OnMessageOffline  onOffline,
    //    OnMessageDelivered onDelivered)
    //{
    //    std::vector<std::pair<oatpp::String, std::shared_ptr<AsyncWebSocket>>> online;
    //    std::vector<oatpp::String> offline;

    //    {
    //        std::lock_guard<oatpp::concurrency::SpinLock> lock(m_lock);
    //        for (auto& uuid : targetUuids) {
    //            auto it = m_connections.find(uuid);
    //            if (it != m_connections.end() && it->second) {
    //                online.emplace_back(uuid, it->second);
    //            } else {
    //                offline.push_back(uuid);
    //            }
    //        }
    //    }

    //    m_executor->execute<BatchSendMessageCoroutine>(
    //        m_executor,
    //        std::move(online),
    //        std::move(offline),
    //        message,
    //        onOffline,
    //        onDelivered
    //    );
    //}

    // 纯推送（无回调），直接在线用户逐个 SendOneMessageCoroutine，不经过 BatchSendMessageCoroutine
    void batchPushMessage(
        const std::vector<oatpp::String>& targetUuids,
        const oatpp::String& message)
    {
        std::lock_guard<oatpp::concurrency::SpinLock> lock(m_lock);
        for (auto& uuid : targetUuids) {
            auto it = m_connections.find(uuid);
            if (it != m_connections.end() && it->second) {
                m_executor->execute<SendOneMessageCoroutine>(it->second, message);
            }
        }
    }

private:
    oatpp::concurrency::SpinLock m_lock;
    std::unordered_map<oatpp::String, std::shared_ptr<AsyncWebSocket>> m_connections;
};