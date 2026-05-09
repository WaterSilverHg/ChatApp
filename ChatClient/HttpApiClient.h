#ifndef HTTPAPICLIENT_H
#define HTTPAPICLIENT_H

#include "global.h"

class HttpApiClient : public QObject
{
    Q_OBJECT

public:
    static HttpApiClient* instance();

    void setServerUrl(const QString& url);
    QString serverUrl() const { return m_serverUrl; }

    void setAuthToken(const QString& token);
    QString authToken() const { return m_authToken; }

    void login(const QString& email, const QString& password);
    void registerUser(const QString& username, const QString& email,
        const QString& password, const QString& phone = "");
    void logout();
    void resetPassword(const QString& email, const QString& code, const QString& oldPassword, const QString& newPassword);
    void sendVerificationCode(const QString& email);
    void getCurrentUser();

    void getConversations();
    void markConversationRead(const QString& convid);

    void getSentRequests();
    void getReceivedRequests();
    void getFriends();
    void updateFriendRemark(const QString& friendId, const QString& remark);
    void updateFriendGroup(const QString& friendId, const QString& group);
    void blockUser(const QString& targetUserId);
    void unblockUser(const QString& targetUserId);

    void getPrivateMessages(const QString& uid);
    void getPrivateMessagesPage(const QString& uid, int page, int size);
    //void markMessageRead(const QString& mid);
    void getGroupMessages(const QString& gid);
    void getGroupMessagesPage(const QString& gid, int page, int size);

    void getMyGroups();
    void getGroupDetail(const QString& groupUuid);
    void getGroupMembers(const QString& groupUuid);
    void getUserInfo(const QString& userUuid);

    void searchUsers(const QString& keyword);
    void searchGroups(const QString& keyword);

    void updateStatus(const QString& status);
    void getMultipleStatuses(const QJsonArray& userIds);

signals:
    void loginSuccess(const QJsonObject& data);
    void registerSuccess(const QJsonObject& data);
    void logoutSuccess();
    void userInfoReceived(const QJsonObject& user);
    void verificationCodeSent(const QJsonObject& data);
    void passwordResetSuccess(const QJsonObject& data);

    void conversationsReceived(const QJsonArray& conversations);
    void conversationMarkedRead(bool success);

    void sentRequestsReceived(const QJsonArray& requests);
    void receivedRequestsReceived(const QJsonArray& requests);
    void friendsListReceived(const QJsonArray& friends);
    void friendRemarkUpdated(bool success);
    void friendGroupUpdated(bool success);
    void userBlocked(bool success);
    void userUnblocked(bool success);

    void privateMessagesReceived(const QString& convUuid, const QJsonArray& messages);
    void privateMessagesPageReceived(const QJsonArray& messages);
    void messageMarkedRead(bool success);
    void groupMessagesReceived(const QString& convUuid, const QJsonArray& messages);
    void groupMessagesPageReceived(const QJsonArray& messages);

    void myGroupsReceived(const QJsonArray& groups);
    void groupDetailReceived(const QJsonObject& group);
    void groupMembersReceived(const QJsonArray& members);
    void userDetailReceived(const QJsonObject& user);

    void usersSearched(const QJsonArray& users);
    void groupsSearched(const QJsonArray& groups);

    void statusUpdated(const QJsonObject& statusInfo);
    void multipleStatusesReceived(const QJsonArray& statuses);

    void errorOccurred(const QString& errorMessage, int errorCode = 0);

private slots:
    void onReplyFinished(QNetworkReply* reply);

private:
    explicit HttpApiClient(QObject* parent = nullptr);
    ~HttpApiClient() = default;
    HttpApiClient(const HttpApiClient&) = delete;
    HttpApiClient& operator=(const HttpApiClient&) = delete;

    void sendHttpRequest(const QString& method, const QString& endpoint,
        const QJsonObject& data = QJsonObject());
    void sendAuthenticatedRequest(const QString& method, const QString& endpoint,
                                  const QJsonObject& data);
    void addAuthHeader(QNetworkRequest& request);

    static HttpApiClient* s_instance;

    QNetworkAccessManager* m_nam;
    QString m_serverUrl;
    QString m_authToken;
};

#endif // HTTPAPICLIENT_H
