#include"Appjwt.h"

Appjwt::Appjwt(const oatpp::String& secret,
    const oatpp::String& issuer)
    : m_secret(secret)
    , m_issuer(issuer)
    , m_verifier(jwt::verify()
        .allow_algorithm(jwt::algorithm::hs256{ m_secret })
        .with_issuer(m_issuer))
{}

oatpp::String Appjwt::createToken(const std::shared_ptr<Payload>& payload) {
    auto token = jwt::create()
        .set_issuer(m_issuer)
        .set_type("JWS")

        .set_payload_claim("userUuid", jwt::claim(payload->userUuid))

        .sign(jwt::algorithm::hs256{ m_secret });
    return token;
}

std::shared_ptr<Appjwt::Payload> Appjwt::readAndVerifyToken(const oatpp::String& token) {
    auto decoded = jwt::decode(token);
    m_verifier.verify(decoded);

    auto payload = std::make_shared<Payload>();
    payload->userUuid = decoded.get_payload_json().at("userUuid").to_str();

    return payload;
}

//std::pair<bool, oatpp::String> Appjwt::parseToken(const oatpp::String& token) {
//    try {
//        auto decoded = jwt::decode(token);
//        m_verifier.verify(decoded);
//        auto userUuid = decoded.get_payload_json().at("userUuid").to_str();
//        return {true, userUuid};
//    } catch (const std::exception& e) {
//        OATPP_LOGE("Appjwt", "Token parse error: %s", e.what());
//        return {false, ""};
//    }
//}