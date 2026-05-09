#include"AppAuthInterceptor.h"

AppAuthInterceptor::AppAuthInterceptor()
	:oatpp::web::server::interceptor::RequestInterceptor()
{
	// 无需JWT认证的路由
	//// 注册登录相关路由
	//authEndpoints.route("GET", "/ws", false);
	//authEndpoints.route("POST", "/api/auth/register", false);
	//authEndpoints.route("POST", "/api/auth/login", false);
	//authEndpoints.route("POST", "/api/auth/refresh-token", false);
	//authEndpoints.route("GET", "/api/auth/verify-email", false);
	//authEndpoints.route("POST", "/api/auth/forgot-password", false);
	//authEndpoints.route("POST", "/api/auth/reset-password", false);
	//// 新增验证码相关路由
	//authEndpoints.route("POST", "/api/auth/send-verification-code", false);
	//authEndpoints.route("POST", "/api/auth/verify-code", false);
	//
#ifdef SWAGGER
	authEndpoints.route("GET", "swagger/*", false);
	authEndpoints.route("GET", "api-docs/oas-3.0.0.json", false);
#endif // SWAGGER
}

std::shared_ptr<AppAuthInterceptor::OutgoingResponse> AppAuthInterceptor::intercept(const std::shared_ptr<IncomingRequest>& request) {
	auto method = request->getStartingLine().method;
	auto path = request->getStartingLine().path;
	

	// 打印调试信息
	//OATPP_LOGD("AuthInterceptor", "Request: %s %s", method.std_str(), path.std_str());
	std::cout << method.std_str() << "     " << path.std_str() << '\n';

	auto r = authEndpoints.getRoute(method, path);
	//auto r = authEndpoints.getRoute(request->getStartingLine().method, request->getStartingLine().path);
	if (r && !r.getEndpoint()) {//如果未找到路径或该路径不需要认证，则不进行JWT验证，直接返回nullptr，表示继续处理请求。
		return nullptr; // Continue without Authorization
	}

	auto authHeader = request->getHeader(oatpp::web::protocol::http::Header::AUTHORIZATION);
	auto authObject = std::static_pointer_cast<Appjwt::Payload>(m_authHandler.handleAuthorization(authHeader));
	//if(authObject)
	if (authObject->userUuid) {
		//request->putBundleData("userId", authObject->userId);
		request->putHeader("userUuid", authObject->userUuid);
		return nullptr; // Continue - token is valid.
	}

	throw oatpp::web::protocol::http::HttpError(oatpp::web::protocol::http::Status::CODE_401, "Unauthorized", {});
}