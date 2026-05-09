#pragma once
#include"../../global.h"
#include"../handler/AppAuthHandler.h"

class AppAuthInterceptor : public oatpp::web::server::interceptor::RequestInterceptor {
private:
	OATPP_COMPONENT(std::shared_ptr<AppRedis>, m_redis);
	AppAuthHandler m_authHandler;
	oatpp::web::server::HttpRouterTemplate<bool> authEndpoints;
public:

	AppAuthInterceptor();

	std::shared_ptr<OutgoingResponse> intercept(const std::shared_ptr<IncomingRequest>& request) override;
};

