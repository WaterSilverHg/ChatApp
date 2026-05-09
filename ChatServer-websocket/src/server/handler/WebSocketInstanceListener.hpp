#pragma once

#include "global.h"
#include "LightweightAsyncWebSocketListener.hpp"
#include "SharedWebSocketResources.hpp"
#include "../postgresql/AppClient.hpp"
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
            m_sharedResources->webSocket->registerConnection(userUuid, socket);
            auto listener = std::make_shared<LightweightAsyncWebSocketListener>(userUuid, m_sharedResources);
            socket->setListener(listener);
            listener->startHeartbeatCheck(socket);
        } else {
            OATPP_LOGW("WebSocket", "No userUuid in connection parameters");
            //socket->close(1008, "Missing user authentication");
        }
    }
    
    void onBeforeDestroy_NonBlocking(const std::shared_ptr<AsyncWebSocket>& socket) override {
        // 连接关闭时会在 LightweightAsyncWebSocketListener 的 onClose 中处理
    }
};