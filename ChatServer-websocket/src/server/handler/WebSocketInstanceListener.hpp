#pragma once

#include "global.h"
#include "LightweightAsyncWebSocketListener.hpp"
#include "SharedWebSocketResources.hpp"
#include "../postgresql/AppPostgresql.hpp"
#include "../../jwt/Appjwt.h"
#include "../../websocket/AppWebSocket.hpp"

class WebSocketInstanceListener : public oatpp::websocket::AsyncConnectionHandler::SocketInstanceListener {
private:
    std::shared_ptr<SharedWebSocketResources> m_sharedResources;

public:
    WebSocketInstanceListener(
        const std::shared_ptr<SharedWebSocketResources>& sharedResources
    ) : m_sharedResources(sharedResources) {}

    void onAfterCreate_NonBlocking(const std::shared_ptr<AsyncWebSocket>& socket, const std::shared_ptr<const ParameterMap>& params) override {

        OATPP_ASSERT(params);
        auto it = params->find("userUuid");
        std::string userUuid = (it != params->end()) ? it->second : "unknown";
        
        if (!userUuid.empty()) {
            int prevCount = m_sharedResources->webSocket->registerConnection(userUuid, socket);
            auto listener = std::make_shared<LightweightAsyncWebSocketListener>(userUuid, m_sharedResources);
            socket->setListener(listener);
            listener->startHeartbeatCheck(socket);

            if (prevCount == 0) {
                try {
                    auto statusRequest = oatpp::Object<UpdateStatusRequestDTO>::createShared();
                    statusRequest->status = "online";
                    m_sharedResources->statusService->updateStatus(userUuid, statusRequest);
                    OATPP_LOGI("WebSocket", "User %s status set to online (was offline)", userUuid.c_str());
                } catch (const std::exception& e) {
                    OATPP_LOGE("WebSocket", "Failed to set user %s status to online: %s", userUuid.c_str(), e.what());
                }
            } else {
                OATPP_LOGI("WebSocket", "User %s already online, connection count: %d", userUuid.c_str(), prevCount);
            }
        } else {
            OATPP_LOGW("WebSocket", "No userUuid in connection parameters");
        }
    }
    
    void onBeforeDestroy_NonBlocking(const std::shared_ptr<AsyncWebSocket>& socket) override {
        // 连接关闭时会在 LightweightAsyncWebSocketListener 的 onClose 中处理
    }
};