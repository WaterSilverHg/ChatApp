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
        .set_payload_claim("sessionId", jwt::claim(payload->sessionId))
        .sign(jwt::algorithm::hs256{ m_secret });
    return token;
}

std::shared_ptr<Appjwt::Payload> Appjwt::readAndVerifyToken(const oatpp::String& token) {
    auto decoded = jwt::decode(token);
    m_verifier.verify(decoded);

    auto payload = std::make_shared<Payload>();
    payload->userUuid = decoded.get_payload_json().at("userUuid").to_str();
    payload->sessionId = decoded.get_payload_json().at("sessionId").to_str();

    return payload;
}

oatpp::String Appjwt::generateSessionId() {
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 999999);

    std::ostringstream oss;
    oss << timestamp << "-" << std::setw(6) << std::setfill('0') << dis(gen);

    return oss.str().c_str();
}