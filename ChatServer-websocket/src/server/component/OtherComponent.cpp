#pragma once
#include"global.h"
#include"../../jwt/Appjwt.h"
#include"../../redis/AppRedis.hpp"
// #include"../../smtp/AppEmail.hpp"
#include"../../websocket/AppWebSocket.hpp"

class OtherComponent {
public:
    OATPP_CREATE_COMPONENT(std::shared_ptr<SettingController>, config)([] {
        auto cf = std::make_shared<SettingController>();
        if(cf->loadFromFile(CONF_PATH))
            return cf;
        throw "setting config fail";
        }());

    OATPP_CREATE_COMPONENT(std::shared_ptr<AppRedis>, redis)([] {
        OATPP_COMPONENT(std::shared_ptr<SettingController>, config);
        auto rs = std::make_shared<AppRedis>();
        rs->initRedis(config->getString("redis_url").c_str());
        return rs;
        }());

    // OATPP_CREATE_COMPONENT(std::shared_ptr<AppEmail>, email)([] {
    //     OATPP_COMPONENT(std::shared_ptr<SettingController>, config);
    //     auto em = std::make_shared<AppEmail>(config->getString("smtp_host"),config->getNum("smtp_port"), config->getString("sender_name"), config->getString("sender_email"), config->getString("sender_password"));
    //     return em;
    //     }());


    OATPP_CREATE_COMPONENT(std::shared_ptr<Appjwt>, jwt)([] {
        OATPP_COMPONENT(std::shared_ptr<SettingController>, config);
        return std::make_shared<Appjwt>(config->getString("jwt_secret"),config->getString("jwt_issuer"));
        }());
};