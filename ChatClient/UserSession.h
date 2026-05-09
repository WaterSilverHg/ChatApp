#ifndef USERSESSION_H
#define USERSESSION_H

#include "global.h"

class UserSession : public QObject
{
    Q_OBJECT

public:
    static UserSession* instance();

    QString token() const { return m_token; }
    void setToken(const QString& token) { m_token = token; }

    QString userUuid() const { return m_userUuid; }
    void setUserUuid(const QString& uuid) { m_userUuid = uuid; }

    QString username() const { return m_username; }
    void setUsername(const QString& username) { m_username = username; }

    QString email() const { return m_email; }
    void setEmail(const QString& email) { m_email = email; }

    QString avatarUrl() const { return m_avatarUrl; }
    void setAvatarUrl(const QString& url) { m_avatarUrl = url; }

    QString status() const { return m_status; }
    void setStatus(const QString& status) { m_status = status; }

    QString lastSeen() const { return m_lastSeen; }
    void setLastSeen(const QString& lastSeen) { m_lastSeen = lastSeen; }

    bool isLoggedIn() const { return !m_token.isEmpty(); }

    void setUserData(const QJsonObject& user);
    QJsonObject toJson() const;

    void clear();

signals:
    void sessionChanged();

private:
    explicit UserSession(QObject* parent = nullptr);
    ~UserSession() = default;
    UserSession(const UserSession&) = delete;
    UserSession& operator=(const UserSession&) = delete;

    static UserSession* s_instance;

    QString m_token;
    QString m_userUuid;
    QString m_username;
    QString m_email;
    QString m_avatarUrl;
    QString m_status;
    QString m_lastSeen;
};

#endif // USERSESSION_H