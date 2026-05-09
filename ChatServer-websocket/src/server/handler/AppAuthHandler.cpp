#include"AppAuthHandler.h"

AppAuthHandler::AppAuthHandler()
	: oatpp::web::server::handler::BearerAuthorizationHandler("API" /* Realm */)
{}

std::shared_ptr<AppAuthHandler::AuthorizationObject> AppAuthHandler::authorize(const oatpp::String& token) {
	// 先验证JWT令牌
	auto authObj = m_jwt->readAndVerifyToken(token);
	if (!authObj) {
		return nullptr;
	}

	// 从JWT中获取用户ID
	std::string userUuid = authObj->userUuid;

	// 验证Redis中的令牌
	if (!m_redis->validateToken(userUuid, token.getValue(""))) {
		return nullptr;
	}

	return authObj;
}