#include "UserSession.h"

UserSession* UserSession::s_instance = nullptr;

UserSession::UserSession(QObject* parent)
    : QObject(parent)
{
}

UserSession* UserSession::instance()
{
    if (!s_instance) {
        s_instance = new UserSession();
    }
    return s_instance;
}

void UserSession::setUserData(const QJsonObject& user)
{
    if (user.contains("useruuid")) m_userUuid = user["useruuid"].toString();
    if (user.contains("username")) m_username = user["username"].toString();
    if (user.contains("email")) m_email = user["email"].toString();
    if (user.contains("avatarurl")) m_avatarUrl = user["avatarurl"].toString();
    if (user.contains("status")) m_status = user["status"].toString();
    if (user.contains("lastseen")) m_lastSeen = user["lastseen"].toString();
    emit sessionChanged();
}

QJsonObject UserSession::toJson() const
{
    QJsonObject obj;
    obj["token"] = m_token;
    obj["useruuid"] = m_userUuid;
    obj["username"] = m_username;
    obj["email"] = m_email;
    obj["avatarurl"] = m_avatarUrl;
    obj["status"] = m_status;
    obj["lastseen"] = m_lastSeen;
    return obj;
}

void UserSession::clear()
{
    m_token.clear();
    m_userUuid.clear();
    m_username.clear();
    m_email.clear();
    m_avatarUrl.clear();
    m_status.clear();
    m_lastSeen.clear();
    emit sessionChanged();
}
