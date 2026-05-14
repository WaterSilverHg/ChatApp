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

	// 从JWT中获取用户ID和sessionId
	std::string userUuid = authObj->userUuid;
	std::string sessionId = authObj->sessionId;

	// 验证Redis中的sessionId
	if (!m_redis->validateSession(userUuid, sessionId)) {
		return nullptr;
	}

	return authObj;
}