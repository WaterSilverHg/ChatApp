#pragma once
#include"../../global.h"
#include "../../jwt/Appjwt.h"
//#include"../postgresql/AppPostgresql.hpp"
#include"../../redis/AppRedis.hpp"

class AppAuthHandler : public oatpp::web::server::handler::BearerAuthorizationHandler {
private:
	OATPP_COMPONENT(std::shared_ptr<Appjwt>, m_jwt);
	OATPP_COMPONENT(std::shared_ptr<AppRedis>, m_redis);
public:
	AppAuthHandler::AppAuthHandler();
	std::shared_ptr<AuthorizationObject> authorize(const oatpp::String& token) override;

};

