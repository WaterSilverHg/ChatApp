#ifndef HTTPAPICLIENT_H
#define HTTPAPICLIENT_H

#include "../global.h"
#include <functional>
#include <QUuid>

class HttpApiClient : public QObject
{
    Q_OBJECT

public:
    using SuccessCallback = std::function<void(const QJsonDocument&)>;
    using ErrorCallback   = std::function<void(const QString&, int)>;

    static HttpApiClient* instance();

    void setServerUrl(const QString& url);
    QString serverUrl() const { return m_serverUrl; }

    void setAuthToken(const QString& token);
    QString authToken() const { return m_authToken; }

    void login(const QString& email, const QString& password,
               SuccessCallback onSuccess = nullptr, ErrorCallback onError = nullptr);
    void registerUser(const QString& username, const QString& email,
        const QString& password, const QString& verificationCode = "",
        SuccessCallback onSuccess = nullptr, ErrorCallback onError = nullptr);
    void logout(SuccessCallback onSuccess = nullptr, ErrorCallback onError = nullptr);
    void resetPassword(const QString& email, const QString& code,
                       const QString& oldPassword, const QString& newPassword,
                       SuccessCallback onSuccess = nullptr, ErrorCallback onError = nullptr);
    void sendVerificationCode(const QString& email,
                              SuccessCallback onSuccess = nullptr, ErrorCallback onError = nullptr);
    void getCurrentUser(SuccessCallback onSuccess = nullptr, ErrorCallback onError = nullptr);

    void getConversations(SuccessCallback onSuccess = nullptr, ErrorCallback onError = nullptr);
    void markPrivateConversationRead(const QString& targetUserUuid,
                                      SuccessCallback onSuccess = nullptr, ErrorCallback onError = nullptr);
    void markGroupConversationRead(const QString& groupUuid,
                                    SuccessCallback onSuccess = nullptr, ErrorCallback onError = nullptr);

    void getSentRequests(SuccessCallback onSuccess = nullptr, ErrorCallback onError = nullptr);
    void getReceivedRequests(SuccessCallback onSuccess = nullptr, ErrorCallback onError = nullptr);
    void getFriends(SuccessCallback onSuccess = nullptr, ErrorCallback onError = nullptr);
    void getBlockedUsers(SuccessCallback onSuccess = nullptr, ErrorCallback onError = nullptr);

    void getReceivedGroupRequests(SuccessCallback onSuccess = nullptr, ErrorCallback onError = nullptr);
    void getSentGroupRequests(SuccessCallback onSuccess = nullptr, ErrorCallback onError = nullptr);
    void updateFriendRemark(const QString& friendId, const QString& remark,
                            SuccessCallback onSuccess = nullptr, ErrorCallback onError = nullptr);
    void updateFriendGroup(const QString& friendId, const QString& group,
                           SuccessCallback onSuccess = nullptr, ErrorCallback onError = nullptr);
    void blockUser(const QString& targetUserId,
                   SuccessCallback onSuccess = nullptr, ErrorCallback onError = nullptr);
    void unblockUser(const QString& targetUserId,
                     SuccessCallback onSuccess = nullptr, ErrorCallback onError = nullptr);

    // 好友分组管理
    void getFriendGroups(SuccessCallback onSuccess = nullptr, ErrorCallback onError = nullptr);
    void createFriendGroup(const QString& name, const QJsonArray& memberUuids = QJsonArray(),
                           SuccessCallback onSuccess = nullptr, ErrorCallback onError = nullptr);
    void deleteFriendGroup(const QString& groupName,
                           SuccessCallback onSuccess = nullptr, ErrorCallback onError = nullptr);
    void renameFriendGroup(const QString& oldName, const QString& newName,
                           SuccessCallback onSuccess = nullptr, ErrorCallback onError = nullptr);

    // 消息免打扰
    void mutePrivateConversation(const QString& targetUserUuid,
                                 SuccessCallback onSuccess = nullptr, ErrorCallback onError = nullptr);
    void unmutePrivateConversation(const QString& targetUserUuid,
                                   SuccessCallback onSuccess = nullptr, ErrorCallback onError = nullptr);
    void muteGroupConversation(const QString& groupUuid,
                               SuccessCallback onSuccess = nullptr, ErrorCallback onError = nullptr);
    void unmuteGroupConversation(const QString& groupUuid,
                                 SuccessCallback onSuccess = nullptr, ErrorCallback onError = nullptr);

    void getPrivateMessagesBefore(const QString& uid, const QString& msgUuid, int size,
                                  SuccessCallback onSuccess = nullptr, ErrorCallback onError = nullptr);
    void getPrivateMessagesPage(const QString& uid, int page, int size,
                                SuccessCallback onSuccess = nullptr, ErrorCallback onError = nullptr);
    void getGroupMessagesBefore(const QString& gid, const QString& msgUuid, int size,
                                SuccessCallback onSuccess = nullptr, ErrorCallback onError = nullptr);
    void getGroupMessagesPage(const QString& gid, int page, int size,
                              SuccessCallback onSuccess = nullptr, ErrorCallback onError = nullptr);

    void getMyGroups(SuccessCallback onSuccess = nullptr, ErrorCallback onError = nullptr);
    void getGroupDetail(const QString& groupUuid,
                        SuccessCallback onSuccess = nullptr, ErrorCallback onError = nullptr);
    void getGroupMembers(const QString& groupUuid,
                         SuccessCallback onSuccess = nullptr, ErrorCallback onError = nullptr);
    void getFriendDetail(const QString& friendUuid,
                         SuccessCallback onSuccess = nullptr, ErrorCallback onError = nullptr);
    void getUserInfo(const QString& userUuid,
                     SuccessCallback onSuccess = nullptr, ErrorCallback onError = nullptr);

    void searchUsers(const QString& keyword,
                     SuccessCallback onSuccess = nullptr, ErrorCallback onError = nullptr);
    void searchGroups(const QString& keyword,
                      SuccessCallback onSuccess = nullptr, ErrorCallback onError = nullptr);

    void updateStatus(const QString& status,
                      SuccessCallback onSuccess = nullptr, ErrorCallback onError = nullptr);
    void getMultipleStatuses(const QJsonArray& userIds,
                             SuccessCallback onSuccess = nullptr, ErrorCallback onError = nullptr);

    void updateProfile(const QString& username, const QString& avatarUrl,
                       SuccessCallback onSuccess = nullptr, ErrorCallback onError = nullptr);
    void updateGroupInfo(const QString& groupUuid, const QString& name, const QString& description,
                         const QString& avatarUrl, int maxMembers, bool isPublic,
                         SuccessCallback onSuccess = nullptr, ErrorCallback onError = nullptr);

    void uploadUserAvatarFile(const QString& fileName, const QString& mimeType,
                          qint64 fileSize, const QByteArray& fileData,
                          SuccessCallback onSuccess = nullptr, ErrorCallback onError = nullptr);
    
    void uploadGroupAvatarFile(const QString& groupUuid, const QString& fileName, const QString& mimeType,
                               qint64 fileSize, const QByteArray& fileData,
                               SuccessCallback onSuccess = nullptr, ErrorCallback onError = nullptr);
    
    void abortAllRequests();

signals:
    void errorOccurred(const QString& errorMessage, int errorCode = 0);

private slots:
    void onReplyFinished(QNetworkReply* reply);

private:
    explicit HttpApiClient(QObject* parent = nullptr);
    ~HttpApiClient() = default;
    HttpApiClient(const HttpApiClient&) = delete;
    HttpApiClient& operator=(const HttpApiClient&) = delete;

    QString sendHttpRequest(const QString& method, const QString& endpoint,
        const QJsonObject& data = QJsonObject(),
        SuccessCallback onSuccess = nullptr, ErrorCallback onError = nullptr,
        bool authenticated = false);
    void addAuthHeader(QNetworkRequest& request);

    static HttpApiClient* s_instance;

    QNetworkAccessManager* m_nam;
    QString m_serverUrl;
    QString m_authToken;
    QList<QNetworkReply*> m_pendingReplies;
    QMap<QString, SuccessCallback> m_successCallbacks;
    QMap<QString, ErrorCallback> m_errorCallbacks;
};

#endif // HTTPAPICLIENT_H
