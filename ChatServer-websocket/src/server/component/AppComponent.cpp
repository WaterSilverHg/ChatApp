#pragma once
#include"global.h"
#include"../handler/AppErrorHandler.h"
#include "../interceptor/AppAuthInterceptor.h"
#include "../../websocket/AppWebSocket.hpp"
#include "../handler/SharedWebSocketResources.hpp"
#include "../handler/WebSocketInstanceListener.hpp"
#include "../postgresql/AppClient.hpp"
#include "../../jwt/Appjwt.h"

/**
 *  Class which creates and holds Application components and registers components in oatpp::base::Environment
 *  Order of components initialization is from top to bottom
 */
class AppComponent {
public:
    /*
     *  Create ObjectMapper component to serialize/deserialize DTOs in Contoller's API
     */
    OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::data::mapping::ObjectMapper>, objectMapper)([] {
        return oatpp::parser::json::mapping::ObjectMapper::createShared();
        }());

    /**
     *  Create ConnectionProvider component which listens on the port
     */
    OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::network::ServerConnectionProvider>, serverConnectionProvider)([] {
        OATPP_COMPONENT(std::shared_ptr<SettingController>, config);

        return oatpp::network::tcp::server::ConnectionProvider::createShared({ config->getString("server_host"),static_cast<v_uint16>(config->getNum("server_port")), oatpp::network::Address::IP_4});
        }());

    /**
     *  Create Router component
     */
    OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::web::server::HttpRouter>, httpRouter)([] {
        return oatpp::web::server::HttpRouter::createShared();
        }());

#ifdef SWAGGER
    /**
 * Swagger 文档信息配置
 */
    OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::swagger::DocumentInfo>, swaggerDocumentInfo)([] {
        oatpp::swagger::DocumentInfo::Builder builder;

        builder
            .setTitle("ChatAppServer API")
            .setDescription("聊天应用服务器 API 文档")
            .setVersion("1.0.0")
            .setContactName("开发团队")
            .setContactUrl("https://example.com")
            .setLicenseName("Apache 2.0")
            .setLicenseUrl("http://www.apache.org/licenses/LICENSE-2.0.html")
            .addServer("http://localhost:8080", "本地开发服务器");

        return builder.build();
        }());

    /**
     * Swagger UI 资源组件
     * 指定 Swagger UI 静态资源的路径
     */
    OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::swagger::Resources>, swaggerResources)([] {
        // 替换为你的实际资源路径
        OATPP_LOGD("Swagger", "Loading resources from: %s", OATPP_SWAGGER_RES_PATH);
        return oatpp::swagger::Resources::loadResources(OATPP_SWAGGER_RES_PATH);
        }());
#endif

    OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::network::ConnectionHandler>, httpConnectionHandler)("http" /* qualifier */, [] {
        OATPP_COMPONENT(std::shared_ptr<Appjwt>, jwt); // get JWT component
        OATPP_COMPONENT(std::shared_ptr<oatpp::web::server::HttpRouter>, router); // get Router component
        OATPP_COMPONENT(std::shared_ptr<oatpp::data::mapping::ObjectMapper>, objectMapper); // get ObjectMapper component

        auto connectionHandler = oatpp::web::server::AsyncHttpConnectionHandler::createShared(router);
        connectionHandler->addRequestInterceptor(std::make_shared<AppAuthInterceptor>());
        connectionHandler->setErrorHandler(std::make_shared<AppErrorHandler>(objectMapper));
        connectionHandler->addRequestInterceptor(std::make_shared<oatpp::web::server::interceptor::AllowOptionsGlobal>());
        connectionHandler->addResponseInterceptor(std::make_shared<oatpp::web::server::interceptor::AllowCorsGlobal>());

        return connectionHandler;
        }());

    /**
     *  Create async executor for HTTP server
     */
    //OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::async::Executor>, httpServerExecutor)("http-server-exec", [] {
    //    return std::make_shared<oatpp::async::Executor>(4, 2);
    //}());

    /**
     *  Create async executor for WebSocket server
     */
    OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::async::Executor>, wsServerExecutor)("ws-server-exec", [] {
        return std::make_shared<oatpp::async::Executor>(4, 2);
    }());



    OATPP_CREATE_COMPONENT(std::shared_ptr<AppWebSocket>, websocket)([] {
        return std::make_shared<AppWebSocket>();
        }());

    /**
     *  Create SharedWebSocketResources component
     */
    OATPP_CREATE_COMPONENT(std::shared_ptr<SharedWebSocketResources>, sharedWebSocketResources)([] {
        OATPP_COMPONENT(std::shared_ptr<AppClient>, appClient);
        OATPP_COMPONENT(std::shared_ptr<Appjwt>, jwt);
        OATPP_COMPONENT(std::shared_ptr<AppRedis>, redis);
        OATPP_COMPONENT(std::shared_ptr<AppWebSocket>, webSocket);
        OATPP_COMPONENT(std::shared_ptr<oatpp::data::mapping::ObjectMapper>, objectMapper);
        
        return std::make_shared<SharedWebSocketResources>(appClient, jwt, redis, webSocket, objectMapper);
        }());

    /**
     *  Create websocket connection handler
     */
    OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::websocket::AsyncConnectionHandler>, websocketConnectionHandler)("ws-server-handler", [] {
        OATPP_COMPONENT(std::shared_ptr<oatpp::async::Executor>, executor, "ws-server-exec");
        OATPP_COMPONENT(std::shared_ptr<SharedWebSocketResources>, sharedResources);
        
        auto connectionHandler = oatpp::websocket::AsyncConnectionHandler::createShared(executor);
        auto instanceListener = std::make_shared<WebSocketInstanceListener>(sharedResources);
        connectionHandler->setSocketInstanceListener(instanceListener);
        
        return connectionHandler;
    }());

    /**
     *  Create client connection provider
     */
    OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::network::ClientConnectionProvider>, clientConnectionProvider)([] {
        OATPP_COMPONENT(std::shared_ptr<SettingController>, config);
        return oatpp::network::tcp::client::ConnectionProvider::createShared({ 
            config->getString("server_host"), 
            static_cast<v_uint16>(config->getNum("server_port")) 
        });
    }());

    /**
     *  Create websocket connector
     */
    OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::websocket::Connector>, connector)([] {
        OATPP_COMPONENT(std::shared_ptr<oatpp::network::ClientConnectionProvider>, clientConnectionProvider);
        return oatpp::websocket::Connector::createShared(clientConnectionProvider);
    }());
};