#ifndef HTTPAPICLIENT_H
#define HTTPAPICLIENT_H

#include "../global.h"

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
        const QString& password, const QString& verificationCode = "");
    void logout();
    void resetPassword(const QString& email, const QString& code, const QString& oldPassword, const QString& newPassword);
    void sendVerificationCode(const QString& email);
    void getCurrentUser();

    void getConversations();
    void markPrivateConversationRead(const QString& targetUserUuid);
    void markGroupConversationRead(const QString& groupUuid);

    void getSentRequests();
    void getReceivedRequests();
    void getFriends();
    void getBlockedUsers();

    void getReceivedGroupRequests();
    void getSentGroupRequests();
    void updateFriendRemark(const QString& friendId, const QString& remark);
    void updateFriendGroup(const QString& friendId, const QString& group);
    void blockUser(const QString& targetUserId);
    void unblockUser(const QString& targetUserId);

    // 好友分组管理
    void getFriendGroups();
    void createFriendGroup(const QString& name, const QJsonArray& memberUuids = QJsonArray());
    void deleteFriendGroup(const QString& groupName);
    void renameFriendGroup(const QString& oldName, const QString& newName);

    // 消息免打扰
    void mutePrivateConversation(const QString& targetUserUuid);
    void unmutePrivateConversation(const QString& targetUserUuid);
    void muteGroupConversation(const QString& groupUuid);
    void unmuteGroupConversation(const QString& groupUuid);

    void getPrivateMessagesBefore(const QString& uid, const QString& msgUuid, int size);
    void getPrivateMessagesPage(const QString& uid, int page, int size);
    //void markMessageRead(const QString& mid);
    void getGroupMessagesBefore(const QString& gid, const QString& msgUuid, int size);
    void getGroupMessagesPage(const QString& gid, int page, int size);

    void getMyGroups();
    void getGroupDetail(const QString& groupUuid, const QVariant& context = QVariant());
    void getGroupMembers(const QString& groupUuid);
    void getFriendDetail(const QString& friendUuid, const QVariant& context = QVariant());
    void getUserInfo(const QString& userUuid, const QVariant& context = QVariant());

    void searchUsers(const QString& keyword);
    void searchGroups(const QString& keyword);

    void updateStatus(const QString& status);
    void getMultipleStatuses(const QJsonArray& userIds);

    void uploadAvatarFile(const QString& fileName, const QString& mimeType, qint64 fileSize, const QByteArray& fileData);
    void abortAllRequests();

signals:
    void loginSuccess(const QJsonObject& data);
    void registerSuccess(const QJsonObject& data);
    void logoutSuccess();
    void friendDetailReceived(const QJsonObject& friendDetail, const QVariant& context = QVariant());
    void userInfoReceived(const QJsonObject& user, const QVariant& context = QVariant());
    void verificationCodeSent(const QJsonObject& data);
    void passwordResetSuccess(const QJsonObject& data);

    void conversationsReceived(const QJsonArray& conversations);
    void conversationMarkedRead(bool success);

    void sentRequestsReceived(const QJsonArray& requests);
    void receivedRequestsReceived(const QJsonArray& requests);
    void friendsListReceived(const QJsonArray& friends);
    void blockedUsersReceived(const QJsonArray& users);

    void receivedGroupRequestsReceived(const QJsonArray& requests);
    void sentGroupRequestsReceived(const QJsonArray& requests);
    void friendRemarkUpdated(bool success);
    void friendGroupUpdated(bool success);
    void userBlocked(bool success);
    void userUnblocked(bool success);

    void friendGroupsReceived(const QJsonArray& groups);
    void friendGroupCreated(bool success);
    void friendGroupDeleted(bool success);
    void friendGroupRenamed(bool success);
    void conversationMuted(bool success);
    void conversationUnmuted(bool success);

    void privateMessagesReceived(const QString& convUuid, const QJsonArray& messages);
    void privateMessagesBeforeReceived(const QString& convUuid, const QJsonArray& messages);
    void privateMessagesPageReceived(const QString& convUuid, const QJsonArray& messages);
    void messageMarkedRead(bool success);
    void groupMessagesReceived(const QString& convUuid, const QJsonArray& messages);
    void groupMessagesBeforeReceived(const QString& convUuid, const QJsonArray& messages);
    void groupMessagesPageReceived(const QString& convUuid, const QJsonArray& messages);

    void myGroupsReceived(const QJsonArray& groups);
    void groupDetailReceived(const QJsonObject& group, const QVariant& context = QVariant());
    void groupMembersReceived(const QJsonArray& members);
    void userDetailReceived(const QJsonObject& user);

    void usersSearched(const QJsonArray& users);
    void groupsSearched(const QJsonArray& groups);

    void statusUpdated(const QJsonObject& statusInfo);
    void multipleStatusesReceived(const QJsonArray& statuses);
    void fileUploaded(const QJsonObject& fileInfo);

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
                                  const QJsonObject& data, const QVariant& context = QVariant());
    QString extractUuidFromUrl(const QString& url, const QString& prefix) const;
    void addAuthHeader(QNetworkRequest& request);

    static HttpApiClient* s_instance;

    QNetworkAccessManager* m_nam;
    QString m_serverUrl;
    QString m_authToken;
    QList<QNetworkReply*> m_pendingReplies;
};

#endif // HTTPAPICLIENT_H
