// LightweightAsyncWebSocketListener.hpp - 轻量级WebSocket监听器
#pragma once

#include "global.h"
#include "oatpp-websocket/AsyncWebSocket.hpp"
#include "SharedWebSocketResources.hpp"


class LightweightAsyncWebSocketListener : public oatpp::websocket::AsyncWebSocket::Listener {
private:
    OATPP_COMPONENT(std::shared_ptr<oatpp::async::Executor>, m_executor, "ws-server-exec");
    oatpp::data::stream::BufferOutputStream m_messageBuffer;
    oatpp::String m_userUuid;
    std::string m_connId;
    std::shared_ptr<SharedWebSocketResources> m_sharedResources;

public:
    LightweightAsyncWebSocketListener(const oatpp::String& userUuid,
        const std::shared_ptr<SharedWebSocketResources>& sharedResources)
        : m_userUuid(userUuid)
        , m_sharedResources(sharedResources)
    {
        m_connId = "conn_" + std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id())) + "_" + 
                   std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
        
        if (m_userUuid) {
            OATPP_LOGI("WebSocket", "User connected (lightweight): %s, connId: %s", m_userUuid->c_str(), m_connId.c_str());
        }
    }

    ~LightweightAsyncWebSocketListener() {
    }

    void initHeartbeat(const std::shared_ptr<AsyncWebSocket>& socket) {
        m_sharedResources->addHeartbeatRecord(m_connId, socket, m_userUuid->c_str());
    }

    CoroutineStarter onPing(const std::shared_ptr<AsyncWebSocket>& socket, const oatpp::String& message) override {
        return socket->sendPongAsync(message);
    }

    CoroutineStarter onPong(const std::shared_ptr<AsyncWebSocket>& socket, const oatpp::String& message) override {
        OATPP_LOGD("WebSocket", "Received pong from user %s", m_userUuid->c_str());
        m_sharedResources->updateActivityTime(m_connId);
        return nullptr;
    }

    CoroutineStarter onClose(const std::shared_ptr<AsyncWebSocket>& socket, v_uint16 code, const oatpp::String& message) override {
        m_sharedResources->removeHeartbeatRecord(m_connId);
        
        if (m_userUuid) {
            bool shouldSetOffline = m_sharedResources->webSocket->decrementAndRemoveIfZero(m_userUuid);

            if (shouldSetOffline) {
                try {
                    auto statusRequest = oatpp::Object<UpdateStatusRequestDTO>::createShared();
                    statusRequest->status = "offline";
                    m_sharedResources->statusService->updateStatus(m_userUuid, statusRequest);
                    OATPP_LOGI("WebSocket", "User %s status set to offline (all connections closed)", m_userUuid->c_str());
                    
                    // 删除 Redis 中的 sessionId
                    if (m_sharedResources->redis) {
                        m_sharedResources->redis->deleteSession(m_userUuid->c_str());
                        OATPP_LOGI("WebSocket", "Session deleted for user %s", m_userUuid->c_str());
                    }
                } catch (const std::exception& e) {
                    OATPP_LOGE("WebSocket", "Failed to set user %s status to offline: %s", m_userUuid->c_str(), e.what());
                }
            } else {
                OATPP_LOGI("WebSocket", "User %s has more connections, not setting offline", m_userUuid->c_str());
            }
        }

        return socket->sendCloseAsync();
    }

    CoroutineStarter readMessage(const std::shared_ptr<AsyncWebSocket>& socket, v_uint8 opcode, p_char8 data, oatpp::v_io_size size) override {
        if (size == 0) {
            oatpp::String wholeMessage = m_messageBuffer.toString();
            m_messageBuffer.setCurrentPosition(0);
            if (m_userUuid && wholeMessage) {
                m_sharedResources->updateActivityTime(m_connId);
                handleMessage(wholeMessage);
            }
        }
        else if (size > 0) {
            m_messageBuffer.writeSimple(data, size);
        }
        return nullptr;
    }

private:

    void handleMessage(const oatpp::String& message) {
        if (!message || message->empty()) {
            sendError("Empty message");
            return;
        }

        auto type = quickExtractType(message);
        if (type.empty()) {
            sendError("Missing message type");
            return;
        }

        auto it = m_sharedResources->handlers.find(type);
        if (it == m_sharedResources->handlers.end()) {
            OATPP_LOGW("WebSocket", "Unknown message type: %s", type.c_str());
            sendError("Unknown message type: " + type);
            return;
        }
        OATPP_LOGD("DEBUG", "%s", message->c_str());
        try {
            OATPP_LOGI("WebSocket", "Processing message type: %s from user: %s", type.c_str(), m_userUuid->c_str());
            it->second(m_userUuid, message);
        } catch (const std::exception& e) {
            OATPP_LOGE("WebSocket", "Error handling message: %s", e.what());
            sendError("Internal error");
        }
    }

    std::string quickExtractType(const oatpp::String& message) {
        if (!message || message->empty()) {
            return "";
        }

        const char* data = message->data();
        size_t len = message->size();
        const char* typePattern = "\"type\"";
        size_t typeLen = 6;

        for (size_t i = 0; i < len - typeLen; ++i) {
            if (memcmp(data + i, typePattern, typeLen) == 0) {
                size_t pos = i + typeLen;

                while (pos < len && (data[pos] == ' ' || data[pos] == '\t' || data[pos] == '\n' || data[pos] == '\r')) {
                    ++pos;
                }

                if (pos < len && data[pos] == ':') {
                    ++pos;

                    while (pos < len && (data[pos] == ' ' || data[pos] == '\t' || data[pos] == '\n' || data[pos] == '\r')) {
                        ++pos;
                    }

                    if (pos < len && data[pos] == '"') {
                        ++pos;

                        const char* start = data + pos;
                        const char* end = start;

                        while (end < data + len && *end != '"') {
                            if (*end == '\\' && (end + 1) < data + len) {
                                ++end;
                            }
                            ++end;
                        }

                        if (end > start) {
                            return std::string(start, end - start);
                        }
                    }
                }
                break;
            }
        }

        return "";
    }

    void sendError(const oatpp::String& errorMessage) {
        m_sharedResources->sendError(m_userUuid, errorMessage);
    }
};