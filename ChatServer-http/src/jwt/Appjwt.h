#pragma once
#include"../global.h"

class Appjwt {
public:
    struct Payload : public oatpp::web::server::handler::AuthorizationObject{
      oatpp::String userUuid;
    };
private:
    oatpp::String m_secret;
    oatpp::String m_issuer;
    jwt::verifier<jwt::default_clock, jwt::traits::kazuho_picojson> m_verifier;

public:
    ~Appjwt() = default;

    Appjwt(const oatpp::String& m_secret, const oatpp::String& issuer);

    oatpp::String createToken(const std::shared_ptr<Payload>& payload);

    std::shared_ptr<Payload> readAndVerifyToken(const oatpp::String& token);
};

