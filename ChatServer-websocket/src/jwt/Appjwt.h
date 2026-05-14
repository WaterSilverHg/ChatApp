#pragma once
#include"../global.h"

class Appjwt {
public:
    struct Payload : public oatpp::web::server::handler::AuthorizationObject{
      oatpp::String userUuid;
      oatpp::String sessionId;  // 新增 sessionId 字段
    };
private:
    oatpp::String m_secret;
    oatpp::String m_issuer;
    jwt::verifier<jwt::default_clock, jwt::traits::kazuho_picojson> m_verifier;

    // 生成唯一的 sessionId
    oatpp::String generateSessionId();

public:
    ~Appjwt() = default;

    Appjwt(const oatpp::String& m_secret, const oatpp::String& issuer);

    // 创建 token，同时生成 sessionId
    std::pair<oatpp::String, oatpp::String> createTokenWithSession(const std::shared_ptr<Payload>& payload);

    std::shared_ptr<Payload> readAndVerifyToken(const oatpp::String& token);
};