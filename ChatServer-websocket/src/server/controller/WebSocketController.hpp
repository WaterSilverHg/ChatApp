#pragma once

#include "global.h"
#include "server/handler/SharedWebSocketResources.hpp"
#include "server/handler/WebSocketInstanceListener.hpp"
#include "oatpp-websocket/Handshaker.hpp"

#include OATPP_CODEGEN_BEGIN(ApiController)

class WebSocketController : public oatpp::web::server::api::ApiController {
private:
    typedef WebSocketController __ControllerType;
    OATPP_COMPONENT(std::shared_ptr<oatpp::websocket::AsyncConnectionHandler>, websocketConnectionHandler, "ws-server-handler");

public:
    WebSocketController(const std::shared_ptr<oatpp::data::mapping::ObjectMapper>& objectMapper)
        : oatpp::web::server::api::ApiController(objectMapper)
    {}

    static std::shared_ptr<WebSocketController> createShared(const std::shared_ptr<oatpp::data::mapping::ObjectMapper>& objectMapper) {
        return std::make_shared<WebSocketController>(objectMapper);
    }

    ENDPOINT_ASYNC("GET", "ws", wsEndpoint) {

        ENDPOINT_ASYNC_INIT(wsEndpoint)

        Action act() override {
            auto params = std::make_shared<oatpp::network::ConnectionHandler::ParameterMap>();
            auto userUuid = request->getHeader("userUuid");
            if (userUuid) {
                (*params)["userUuid"] = userUuid;
            }
            auto response = oatpp::websocket::Handshaker::serversideHandshake(request->getHeaders(), controller->websocketConnectionHandler);
            response->setConnectionUpgradeParameters(params);
            return _return(response);
        }
    };
};

#include OATPP_CODEGEN_END(ApiController)
