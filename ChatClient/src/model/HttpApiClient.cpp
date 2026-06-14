#include "HttpApiClient.h"
#include "UserSession.h"

HttpApiClient* HttpApiClient::s_instance = nullptr;

HttpApiClient::HttpApiClient(QObject* parent)
    : QObject(parent), m_nam(new QNetworkAccessManager(this))
{
    connect(m_nam, &QNetworkAccessManager::finished,
        this, &HttpApiClient::onReplyFinished);
}

HttpApiClient* HttpApiClient::instance()
{
    if (!s_instance) {
        s_instance = new HttpApiClient();
    }
    return s_instance;
}

void HttpApiClient::setServerUrl(const QString& url)
{
    m_serverUrl = url;
}

void HttpApiClient::setAuthToken(const QString& token)
{
    m_authToken = token;
}

QString HttpApiClient::sendHttpRequest(const QString& method, const QString& endpoint,
    const QJsonObject& data, SuccessCallback onSuccess, ErrorCallback onError,
    bool authenticated)
{
    QString requestId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    if (onSuccess) m_successCallbacks[requestId] = onSuccess;
    if (onError) m_errorCallbacks[requestId] = onError;

    QUrl url(m_serverUrl + endpoint);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Connection", "close");

    if (authenticated) {
        addAuthHeader(request);
    }

    QNetworkReply* reply = nullptr;

    if (method == "GET") {
        reply = m_nam->get(request);
    }
    else if (method == "POST") {
        QJsonDocument doc(data);
        reply = m_nam->post(request, doc.toJson());
    }
    else if (method == "PUT") {
        QJsonDocument doc(data);
        reply = m_nam->put(request, doc.toJson());
    }
    else if (method == "DELETE") {
        reply = m_nam->deleteResource(request);
    }

    if (reply) {
        m_pendingReplies.append(reply);
        reply->setProperty("requestId", requestId);
    }

    return requestId;
}

void HttpApiClient::addAuthHeader(QNetworkRequest& request)
{
    request.setRawHeader("Authorization", QString("Bearer %1").arg(UserSession::instance()->token()).toUtf8());
}

void HttpApiClient::onReplyFinished(QNetworkReply* reply)
{
    m_pendingReplies.removeOne(reply);
    QString requestId = reply->property("requestId").toString();

    auto successIt = m_successCallbacks.find(requestId);
    auto errorIt = m_errorCallbacks.find(requestId);

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject obj = doc.object();

    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << reply->errorString();
        QString errorMessage = obj.contains("message") ? obj["message"].toString() : "Unknown Error";

        if (errorIt != m_errorCallbacks.end()) {
            ErrorCallback callback = errorIt.value();
            m_errorCallbacks.erase(errorIt);
            callback(errorMessage, reply->error());
        } else {
            emit errorOccurred(errorMessage, reply->error());
        }

        if (successIt != m_successCallbacks.end()) m_successCallbacks.erase(successIt);
        reply->deleteLater();
        return;
    }

    // 处理文件上传的特殊逻辑
    QString url = reply->url().toString();
    if (url.contains("/api/files/record")) {
        QString fileUuid = obj["uuid"].toString();
        QByteArray fileData = reply->property("fileData").toByteArray();

        if (!fileUuid.isEmpty() && !fileData.isEmpty()) {
            QUrl uploadUrl(m_serverUrl + "/api/files/upload");
            QNetworkRequest uploadRequest(uploadUrl);
            uploadRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream");
            uploadRequest.setRawHeader("Connection", "close");
            addAuthHeader(uploadRequest);
            uploadRequest.setRawHeader("fileUuid", fileUuid.toUtf8());

            QString uploadRequestId = QUuid::createUuid().toString(QUuid::WithoutBraces);
            if (successIt != m_successCallbacks.end()) {
                m_successCallbacks[uploadRequestId] = successIt.value();
                m_successCallbacks.erase(successIt);
            }
            if (errorIt != m_errorCallbacks.end()) {
                m_errorCallbacks[uploadRequestId] = errorIt.value();
                m_errorCallbacks.erase(errorIt);
            }

            QNetworkReply* uploadReply = m_nam->post(uploadRequest, fileData);
            if (uploadReply) {
                m_pendingReplies.append(uploadReply);
                uploadReply->setProperty("requestId", uploadRequestId);
            }
            reply->deleteLater();
            return;
        }
    }

    // 成功回调
    if (successIt != m_successCallbacks.end()) {
        SuccessCallback callback = successIt.value();
        m_successCallbacks.erase(successIt);
        callback(doc);
    }
    if (errorIt != m_errorCallbacks.end()) {
        m_errorCallbacks.erase(errorIt);
    }

    reply->deleteLater();
}

void HttpApiClient::login(const QString& email, const QString& password,
                          SuccessCallback onSuccess, ErrorCallback onError)
{
    QJsonObject data;
    data["email"] = email;
    data["password"] = password;
    sendHttpRequest("POST", "/api/auth/login", data, onSuccess, onError, false);
}

void HttpApiClient::registerUser(const QString& username, const QString& email,
                                 const QString& password, const QString& verificationCode,
                                 SuccessCallback onSuccess, ErrorCallback onError)
{
    QJsonObject data;
    data["username"] = username;
    data["email"] = email;
    data["password"] = password;
    if (!verificationCode.isEmpty()) data["verificationcode"] = verificationCode;
    sendHttpRequest("POST", "/api/auth/register", data, onSuccess, onError, false);
}

void HttpApiClient::logout(SuccessCallback onSuccess, ErrorCallback onError)
{
    sendHttpRequest("POST", "/api/auth/logout", QJsonObject(), onSuccess, onError, true);
}

void HttpApiClient::abortAllRequests()
{
    QList<QNetworkReply*> pending = m_pendingReplies;
    m_pendingReplies.clear();
    m_successCallbacks.clear();
    m_errorCallbacks.clear();

    for (QNetworkReply* reply : pending) {
        if (reply && reply->isRunning()) {
            reply->abort();
            reply->deleteLater();
        }
    }
}

void HttpApiClient::resetPassword(const QString& email, const QString& code,
                                 const QString& oldPassword, const QString& newPassword,
                                 SuccessCallback onSuccess, ErrorCallback onError)
{
    QJsonObject data;
    data["email"] = email;
    data["verificationcode"] = code;
    data["oldpassword"] = oldPassword;
    data["newpassword"] = newPassword;
    sendHttpRequest("POST", "/api/auth/reset-password", data, onSuccess, onError, false);
}

void HttpApiClient::sendVerificationCode(const QString& email,
                                         SuccessCallback onSuccess, ErrorCallback onError)
{
    QJsonObject data;
    data["email"] = email;
    sendHttpRequest("POST", "/api/auth/send-code", data, onSuccess, onError, false);
}

void HttpApiClient::getCurrentUser(SuccessCallback onSuccess, ErrorCallback onError)
{
    sendHttpRequest("GET", "/api/user/me", QJsonObject(), onSuccess, onError, true);
}

void HttpApiClient::getConversations(SuccessCallback onSuccess, ErrorCallback onError)
{
    sendHttpRequest("GET", "/api/conversations", QJsonObject(), onSuccess, onError, true);
}

void HttpApiClient::markPrivateConversationRead(const QString& targetUserUuid,
                                                 SuccessCallback onSuccess, ErrorCallback onError)
{
    sendHttpRequest("PUT", QString("/api/conversations/users/%1/read").arg(targetUserUuid),
                   QJsonObject(), onSuccess, onError, true);
}

void HttpApiClient::markGroupConversationRead(const QString& groupUuid,
                                               SuccessCallback onSuccess, ErrorCallback onError)
{
    sendHttpRequest("PUT", QString("/api/conversations/groups/%1/read").arg(groupUuid),
                   QJsonObject(), onSuccess, onError, true);
}

void HttpApiClient::getSentRequests(SuccessCallback onSuccess, ErrorCallback onError)
{
    sendHttpRequest("GET", "/api/friends/requests/sent", QJsonObject(), onSuccess, onError, true);
}

void HttpApiClient::getReceivedRequests(SuccessCallback onSuccess, ErrorCallback onError)
{
    sendHttpRequest("GET", "/api/friends/requests/received", QJsonObject(), onSuccess, onError, true);
}

void HttpApiClient::getReceivedGroupRequests(SuccessCallback onSuccess, ErrorCallback onError)
{
    sendHttpRequest("GET", "/api/groups/requests/received", QJsonObject(), onSuccess, onError, true);
}

void HttpApiClient::getSentGroupRequests(SuccessCallback onSuccess, ErrorCallback onError)
{
    sendHttpRequest("GET", "/api/groups/requests/sent", QJsonObject(), onSuccess, onError, true);
}

void HttpApiClient::getFriends(SuccessCallback onSuccess, ErrorCallback onError)
{
    sendHttpRequest("GET", "/api/friends/list", QJsonObject(), onSuccess, onError, true);
}

void HttpApiClient::getBlockedUsers(SuccessCallback onSuccess, ErrorCallback onError)
{
    sendHttpRequest("GET", "/api/friends/blocked", QJsonObject(), onSuccess, onError, true);
}

void HttpApiClient::updateFriendRemark(const QString& friendId, const QString& remark,
                                       SuccessCallback onSuccess, ErrorCallback onError)
{
    QJsonObject data;
    data["remark"] = remark;
    sendHttpRequest("PUT", QString("/api/friends/%1/remark").arg(friendId), data, onSuccess, onError, true);
}

void HttpApiClient::updateFriendGroup(const QString& friendId, const QString& group,
                                      SuccessCallback onSuccess, ErrorCallback onError)
{
    QJsonObject data;
    data["group"] = group;
    sendHttpRequest("PUT", QString("/api/friends/%1/group").arg(friendId), data, onSuccess, onError, true);
}

void HttpApiClient::blockUser(const QString& targetUserId,
                              SuccessCallback onSuccess, ErrorCallback onError)
{
    sendHttpRequest("POST", QString("/api/friends/block/%1").arg(targetUserId), QJsonObject(), onSuccess, onError, true);
}

void HttpApiClient::unblockUser(const QString& targetUserId,
                                SuccessCallback onSuccess, ErrorCallback onError)
{
    sendHttpRequest("POST", QString("/api/friends/unblock/%1").arg(targetUserId), QJsonObject(), onSuccess, onError, true);
}

void HttpApiClient::getFriendGroups(SuccessCallback onSuccess, ErrorCallback onError)
{
    sendHttpRequest("GET", "/api/friends/groups", QJsonObject(), onSuccess, onError, true);
}

void HttpApiClient::createFriendGroup(const QString& name, const QJsonArray& memberUuids,
                                      SuccessCallback onSuccess, ErrorCallback onError)
{
    QJsonObject data;
    data["name"] = name;
    if (!memberUuids.isEmpty()) data["memberuuids"] = memberUuids;
    sendHttpRequest("POST", "/api/friends/groups", data, onSuccess, onError, true);
}

void HttpApiClient::deleteFriendGroup(const QString& groupName,
                                      SuccessCallback onSuccess, ErrorCallback onError)
{
    sendHttpRequest("DELETE", QString("/api/friends/groups/%1").arg(groupName), QJsonObject(), onSuccess, onError, true);
}

void HttpApiClient::renameFriendGroup(const QString& oldName, const QString& newName,
                                      SuccessCallback onSuccess, ErrorCallback onError)
{
    QJsonObject data;
    data["newname"] = newName;
    sendHttpRequest("PUT", QString("/api/friends/groups/%1").arg(oldName), data, onSuccess, onError, true);
}

void HttpApiClient::mutePrivateConversation(const QString& targetUserUuid,
                                            SuccessCallback onSuccess, ErrorCallback onError)
{
    sendHttpRequest("PUT", QString("/api/conversations/users/%1/mute").arg(targetUserUuid),
                   QJsonObject(), onSuccess, onError, true);
}

void HttpApiClient::unmutePrivateConversation(const QString& targetUserUuid,
                                              SuccessCallback onSuccess, ErrorCallback onError)
{
    sendHttpRequest("PUT", QString("/api/conversations/users/%1/unmute").arg(targetUserUuid),
                   QJsonObject(), onSuccess, onError, true);
}

void HttpApiClient::muteGroupConversation(const QString& groupUuid,
                                          SuccessCallback onSuccess, ErrorCallback onError)
{
    sendHttpRequest("PUT", QString("/api/conversations/groups/%1/mute").arg(groupUuid),
                   QJsonObject(), onSuccess, onError, true);
}

void HttpApiClient::unmuteGroupConversation(const QString& groupUuid,
                                            SuccessCallback onSuccess, ErrorCallback onError)
{
    sendHttpRequest("PUT", QString("/api/conversations/groups/%1/unmute").arg(groupUuid),
                   QJsonObject(), onSuccess, onError, true);
}

void HttpApiClient::getPrivateMessagesPage(const QString& uid, int page, int size,
                                           SuccessCallback onSuccess, ErrorCallback onError)
{
    sendHttpRequest("GET", QString("/api/messages/private/%1/page?page=%2&size=%3").arg(uid).arg(page).arg(size),
                   QJsonObject(), onSuccess, onError, true);
}

void HttpApiClient::getPrivateMessagesBefore(const QString& uid, const QString& msgUuid, int size,
                                             SuccessCallback onSuccess, ErrorCallback onError)
{
    sendHttpRequest("GET", QString("/api/messages/private/%1/before?msgUuid=%2&size=%3").arg(uid).arg(msgUuid).arg(size),
                   QJsonObject(), onSuccess, onError, true);
}

void HttpApiClient::getGroupMessagesPage(const QString& gid, int page, int size,
                                         SuccessCallback onSuccess, ErrorCallback onError)
{
    sendHttpRequest("GET", QString("/api/messages/group/%1/page?page=%2&size=%3").arg(gid).arg(page).arg(size),
                   QJsonObject(), onSuccess, onError, true);
}

void HttpApiClient::getGroupMessagesBefore(const QString& gid, const QString& msgUuid, int size,
                                           SuccessCallback onSuccess, ErrorCallback onError)
{
    sendHttpRequest("GET", QString("/api/messages/group/%1/before?msgUuid=%2&size=%3").arg(gid).arg(msgUuid).arg(size),
                   QJsonObject(), onSuccess, onError, true);
}

void HttpApiClient::getMyGroups(SuccessCallback onSuccess, ErrorCallback onError)
{
    sendHttpRequest("GET", "/api/groups/list", QJsonObject(), onSuccess, onError, true);
}

void HttpApiClient::getGroupDetail(const QString& groupUuid,
                                   SuccessCallback onSuccess, ErrorCallback onError)
{
    sendHttpRequest("GET", QString("/api/groups/%1").arg(groupUuid), QJsonObject(), onSuccess, onError, true);
}

void HttpApiClient::getGroupMembers(const QString& groupUuid,
                                    SuccessCallback onSuccess, ErrorCallback onError)
{
    sendHttpRequest("GET", QString("/api/groups/%1/members").arg(groupUuid), QJsonObject(), onSuccess, onError, true);
}

void HttpApiClient::updateStatus(const QString& status,
                                 SuccessCallback onSuccess, ErrorCallback onError)
{
    QJsonObject data;
    data["status"] = status;
    sendHttpRequest("PUT", "/api/status", data, onSuccess, onError, true);
}

void HttpApiClient::updateProfile(const QString& username, const QString& avatarUrl,
                                  SuccessCallback onSuccess, ErrorCallback onError)
{
    QJsonObject data;
    data["username"] = username;
    data["avatarUrl"] = avatarUrl;
    sendHttpRequest("PUT", "/api/user/profile", data, onSuccess, onError, true);
}

void HttpApiClient::updateGroupInfo(const QString& groupUuid, const QString& name, const QString& description,
                                    const QString& avatarUrl, int maxMembers, bool isPublic,
                                    SuccessCallback onSuccess, ErrorCallback onError)
{
    QJsonObject data;
    data["name"] = name;
    data["description"] = description;
    data["avatarurl"] = avatarUrl;      // 修改为全小写
    data["maxmembers"] = maxMembers;    // 修改为全小写
    data["ispublic"] = isPublic;        // 修改为全小写
    sendHttpRequest("PUT", QString("/api/groups/%1").arg(groupUuid), data, onSuccess, onError, true);
}

void HttpApiClient::getMultipleStatuses(const QJsonArray& userIds,
                                        SuccessCallback onSuccess, ErrorCallback onError)
{
    QJsonObject data;
    data["useruuids"] = userIds;
    sendHttpRequest("POST", "/api/status/batch", data, onSuccess, onError, true);
}

void HttpApiClient::getFriendDetail(const QString& friendUuid,
                                    SuccessCallback onSuccess, ErrorCallback onError)
{
    sendHttpRequest("GET", QString("/api/friends/%1/detail").arg(friendUuid), QJsonObject(), onSuccess, onError, true);
}

void HttpApiClient::getUserInfo(const QString& userUuid,
                                SuccessCallback onSuccess, ErrorCallback onError)
{
    sendHttpRequest("GET", QString("/api/users/info/%1").arg(userUuid), QJsonObject(), onSuccess, onError, true);
}

void HttpApiClient::searchUsers(const QString& keyword,
                                SuccessCallback onSuccess, ErrorCallback onError)
{
    QString encoded = QUrl::toPercentEncoding(keyword);
    sendHttpRequest("GET", QString("/api/users/search?keyword=%1").arg(encoded), QJsonObject(), onSuccess, onError, true);
}

void HttpApiClient::searchGroups(const QString& keyword,
                                 SuccessCallback onSuccess, ErrorCallback onError)
{
    QString encoded = QUrl::toPercentEncoding(keyword);
    sendHttpRequest("GET", QString("/api/groups/search?keyword=%1").arg(encoded), QJsonObject(), onSuccess, onError, true);
}

void HttpApiClient::uploadUserAvatarFile(const QString& fileName, const QString& mimeType,
                                     qint64 fileSize, const QByteArray& fileData,
                                     SuccessCallback onSuccess, ErrorCallback onError)
{
    QJsonObject data;
    data["filetype"] = "avatar";
    data["mimetype"] = mimeType;
    data["filesize"] = fileSize;

    QString requestId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    if (onSuccess) m_successCallbacks[requestId] = onSuccess;
    if (onError) m_errorCallbacks[requestId] = onError;

    QUrl url(m_serverUrl + "/api/files/record");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Connection", "close");
    addAuthHeader(request);

    QNetworkReply* reply = m_nam->post(request, QJsonDocument(data).toJson());
    if (reply) {
        m_pendingReplies.append(reply);
        reply->setProperty("requestId", requestId);
        reply->setProperty("fileData", fileData);
    }
}

void HttpApiClient::uploadGroupAvatarFile(const QString& groupUuid, const QString& fileName, const QString& mimeType,
                                          qint64 fileSize, const QByteArray& fileData,
                                          SuccessCallback onSuccess, ErrorCallback onError)
{
    QJsonObject data;
    data["filetype"] = "group_avatar";
    data["mimetype"] = mimeType;
    data["filesize"] = fileSize;
    data["targetuuid"] = groupUuid;  // 传入群UUID，用于自动更新群聊头像

    QString requestId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    if (onSuccess) m_successCallbacks[requestId] = onSuccess;
    if (onError) m_errorCallbacks[requestId] = onError;

    QUrl url(m_serverUrl + "/api/files/record");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Connection", "close");
    addAuthHeader(request);

    QNetworkReply* reply = m_nam->post(request, QJsonDocument(data).toJson());
    if (reply) {
        m_pendingReplies.append(reply);
        reply->setProperty("requestId", requestId);
        reply->setProperty("fileData", fileData);
    }
}