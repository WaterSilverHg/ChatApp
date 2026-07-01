#pragma once
#include"global.h"
#include"server/handler/AppErrorHandler.h"
#include "tool/UuidIdCache.hpp"
#include"jwt/Appjwt.h"
//#include "server/interceptor/AppAuthInterceptor.h"
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
        auto ss = oatpp::swagger::DocumentInfo::SecuritySchemeBuilder::DefaultBearerAuthorizationSecurityScheme("JWT");
        ss->description = "输入你的JWT令牌";
        builder
            .setTitle("ChatAppServer API")
            .setDescription("聊天应用服务器 API 文档")
            .setVersion("1.0.0")
            .setContactName("开发团队")
            .setContactUrl("https://example.com")
            .setLicenseName("Apache 2.0")
            .setLicenseUrl("http://www.apache.org/licenses/LICENSE-2.0.html")
            .addServer("http://localhost:8080", "本地开发服务器")
            .addSecurityScheme("BearerAuth", ss);
                //.type("http")
                //.scheme("bearer")
                //.bearerFormat("JWT")
                //.description("输入你的JWT令牌")
                //.build());

        return builder.build();
        }());

    /**
     * Swagger UI 资源组件
     * 指定 Swagger UI 静态资源的路径
     */
    OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::swagger::Resources>, swaggerResources)([] {
        // 替换为你的实际资源路径
        //OATPP_LOGD("Swagger", "Loading resources from: %s", OATPP_SWAGGER_RES_PATH);
        return oatpp::swagger::Resources::loadResources(OATPP_SWAGGER_RES_PATH);
        }());
#endif

    OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::network::ConnectionHandler>, httpConnectionHandler)("http" /* qualifier */, [] {
        OATPP_COMPONENT(std::shared_ptr<Appjwt>, jwt); // get JWT component
        OATPP_COMPONENT(std::shared_ptr<oatpp::web::server::HttpRouter>, router); // get Router component
        OATPP_COMPONENT(std::shared_ptr<oatpp::data::mapping::ObjectMapper>, objectMapper); // get ObjectMapper component

        auto connectionHandler = oatpp::web::server::HttpConnectionHandler::createShared(router);

        connectionHandler->setErrorHandler(std::make_shared<AppErrorHandler>(objectMapper));
        connectionHandler->addResponseInterceptor(std::make_shared<oatpp::web::server::interceptor::AllowCorsGlobal>());
        connectionHandler->addRequestInterceptor(std::make_shared<oatpp::web::server::interceptor::AllowOptionsGlobal>());

        return connectionHandler;
        }());

    OATPP_CREATE_COMPONENT(std::shared_ptr<UuidIdCache>, uuidIdCache)([] {
        OATPP_COMPONENT(std::shared_ptr<AppRedis>, redis);
        OATPP_COMPONENT(std::shared_ptr<AppPostgresql>, appClient);
        return std::make_shared<UuidIdCache>(redis, appClient);
    }());
};