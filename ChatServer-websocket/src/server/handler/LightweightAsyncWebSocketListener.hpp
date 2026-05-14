// LightweightAsyncWebSocketListener.hpp - 轻量级WebSocket监听器
#pragma once

#include "global.h"
#include "oatpp-websocket/AsyncWebSocket.hpp"
#include "SharedWebSocketResources.hpp"

#include <chrono>
#include <thread>
#include <atomic>

class LightweightAsyncWebSocketListener : public oatpp::websocket::AsyncWebSocket::Listener {
private:
    OATPP_COMPONENT(std::shared_ptr<oatpp::async::Executor>, m_executor, "ws-server-exec");
    oatpp::data::stream::BufferOutputStream m_messageBuffer;
    oatpp::String m_userUuid;
    std::shared_ptr<SharedWebSocketResources> m_sharedResources;
    std::atomic<std::chrono::steady_clock::time_point> m_lastPongTime;
    std::atomic<bool> m_running;
    std::thread m_heartbeatThread;
    static constexpr auto HEARTBEAT_INTERVAL = std::chrono::seconds(10);
    static constexpr auto PONG_TIMEOUT = std::chrono::seconds(12);

public:
    LightweightAsyncWebSocketListener(const oatpp::String& userUuid,
        const std::shared_ptr<SharedWebSocketResources>& sharedResources)
        : m_userUuid(userUuid)
        , m_sharedResources(sharedResources)
        , m_lastPongTime(std::chrono::steady_clock::now())
        , m_running(true)
    {
        if (m_userUuid) {
            OATPP_LOGI("WebSocket", "User connected (lightweight): %s", m_userUuid->c_str());
        }
        m_lastPongTime = std::chrono::steady_clock::now();
    }

    ~LightweightAsyncWebSocketListener() {
        stopHeartbeat();
    }

    CoroutineStarter onPing(const std::shared_ptr<AsyncWebSocket>& socket, const oatpp::String& message) override {
        return socket->sendPongAsync(message);
    }

    CoroutineStarter onPong(const std::shared_ptr<AsyncWebSocket>& socket, const oatpp::String& message) override {
        m_lastPongTime = std::chrono::steady_clock::now();
        OATPP_LOGD("WebSocket", "Received pong from user %s", m_userUuid->c_str());
        return nullptr;
    }

    CoroutineStarter onClose(const std::shared_ptr<AsyncWebSocket>& socket, v_uint16 code, const oatpp::String& message) override {
        m_running = false;
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
                handleMessage(wholeMessage);
            }
        }
        else if (size > 0) {
            m_messageBuffer.writeSimple(data, size);
        }
        return nullptr;
    }

    void startHeartbeatCheck(const std::shared_ptr<AsyncWebSocket>& socket) {
        m_heartbeatThread = std::thread([this, socket]() {
            while (m_running) {
                std::this_thread::sleep_for(std::chrono::seconds(1));

                auto now = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - m_lastPongTime.load()).count();

                if (elapsed >= PONG_TIMEOUT.count()) {
                    OATPP_LOGW("WebSocket", "Heartbeat timeout for user %s, closing connection", m_userUuid->c_str());
                    if (m_running) {
                        m_executor->execute<SendCloseCoroutine>(
                            socket
                        );
                    }
                    break;
                }

                if (elapsed >= HEARTBEAT_INTERVAL.count()) {
                    OATPP_LOGD("WebSocket", "Sending heartbeat to user %s", m_userUuid->c_str());
                    if (m_running) {
                        m_executor->execute<SendPingCoroutine>(
                            socket
                        );
                    }
                }
            }
            });
        m_heartbeatThread.detach();
    }

private:
    

    void stopHeartbeat() {
        m_running = false;
        if (m_heartbeatThread.joinable()) {
            m_heartbeatThread.join();
        }
    }

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