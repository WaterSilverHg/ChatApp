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

void HttpApiClient::sendHttpRequest(const QString& method, const QString& endpoint,
    const QJsonObject& data)
{
    QUrl url(m_serverUrl + endpoint);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Connection", "close");

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
    }
}

void HttpApiClient::sendAuthenticatedRequest(const QString& method, const QString& endpoint,
    const QJsonObject& data, const QVariant& context)
{
    QUrl url(m_serverUrl + endpoint);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Connection", "close");

    request.setRawHeader("Authorization", QString("Bearer %1").arg(UserSession::instance()->token()).toUtf8());

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
        if (context.isValid()) {
            reply->setProperty("requestContext", context);
        }
    }
}

void HttpApiClient::onReplyFinished(QNetworkReply* reply)
{
    m_pendingReplies.removeOne(reply);
    
    QByteArray data = reply->readAll();
    QJsonDocument doc =QJsonDocument::fromJson(data);
    QJsonObject obj =  doc.object();
    //NoError就是0，或者说200，如果不是200就不是0
    if (reply->error() != QNetworkReply::NoError) {
        qDebug()<<reply->errorString();
        QString errorMessage = obj.contains("message")?obj["message"].toString():"UnKnow Error";
        emit errorOccurred(errorMessage, reply->error());
        reply->deleteLater();
        return;
    }



    QString url = reply->url().toString();

//    bool isError = obj.contains("status") && obj["status"].toString().toUpper() == "ERROR";
//    bool hasSuccessField = obj.contains("success");
//    bool isSuccess = hasSuccessField ? obj["success"].toBool() : true;

//    QString errorMessage = obj.contains("message") ? obj["message"].toString() :
//                           obj.contains("error") ? obj["error"].toString() :
//                           obj.contains("content") ? obj["content"].toString() : "操作失败";
//    int errorCode = obj.contains("code") ? obj["code"].toInt() : -1;

//    if (isError || (hasSuccessField && !isSuccess)) {
//        emit errorOccurred(errorMessage, errorCode);
//        reply->deleteLater();
//        return;
//    }

    if (url.contains("/api/auth/login")) {
        emit loginSuccess(obj);
    }
    else if (url.contains("/api/auth/register")) {
        emit registerSuccess(obj);
    }
    else if (url.contains("/api/auth/logout")) {
        emit logoutSuccess();
    }
    else if (url.contains("/api/auth/send-code")) {
        emit verificationCodeSent(obj);
    }
    else if (url.contains("/api/auth/reset-password")) {
        emit passwordResetSuccess(obj);
    }
    else if (url.contains("/api/conversations/users/") && (url.contains("/mute") || url.contains("/unmute"))) {
        if (url.contains("/mute") && !url.contains("/unmute")) {
            emit conversationMuted(true);
        } else if (url.contains("/unmute")) {
            emit conversationUnmuted(true);
        }
    }
    else if (url.contains("/api/conversations/groups/") && (url.contains("/mute") || url.contains("/unmute"))) {
        if (url.contains("/mute") && !url.contains("/unmute")) {
            emit conversationMuted(true);
        } else if (url.contains("/unmute")) {
            emit conversationUnmuted(true);
        }
    }
    else if (url.contains("/api/conversations/users/") && url.contains("/read")) {
        emit conversationMarkedRead(true);
    }
    else if (url.contains("/api/conversations/groups/") && url.contains("/read")) {
        emit conversationMarkedRead(true);
    }
    else if (url.contains("/api/conversations") && !url.contains("/users/") && !url.contains("/groups/")) {
        QJsonArray conversations = doc.isArray() ? doc.array() :
                                      (obj.contains("content") ? QJsonDocument::fromJson(obj["content"].toString().toUtf8()).array() : QJsonArray());
        emit conversationsReceived(conversations);
    }
    else if (url.contains("/api/friends/requests/sent")) {
        QJsonArray requests = doc.isArray() ? doc.array() :
                              (obj.contains("content") ? QJsonDocument::fromJson(obj["content"].toString().toUtf8()).array() : QJsonArray());
        emit sentRequestsReceived(requests);
    }
    else if (url.contains("/api/friends/requests/received")) {
        qDebug() << "[DEBUG] /api/friends/requests/received response:" << doc;
        QJsonArray requests = doc.isArray() ? doc.array() :
                              (obj.contains("content") ? QJsonDocument::fromJson(obj["content"].toString().toUtf8()).array() : QJsonArray());
        qDebug() << "[DEBUG] Friend requests count:" << requests.size();
        emit receivedRequestsReceived(requests);
    }
    else if (url.contains("/api/groups/requests/received")) {
        qDebug() << "[DEBUG] /api/groups/requests/received response:" << doc;
        QJsonArray requests = doc.isArray() ? doc.array() :
                              (obj.contains("content") ? QJsonDocument::fromJson(obj["content"].toString().toUtf8()).array() : QJsonArray());
        qDebug() << "[DEBUG] Group requests count:" << requests.size();
        emit receivedGroupRequestsReceived(requests);
    }
    else if (url.contains("/api/groups/requests/sent")) {
        QJsonArray requests = doc.isArray() ? doc.array() :
                              (obj.contains("content") ? QJsonDocument::fromJson(obj["content"].toString().toUtf8()).array() : QJsonArray());
        emit sentGroupRequestsReceived(requests);
    }
    else if (url.contains("/api/friends/blocked")) {
        QJsonArray users = doc.isArray() ? doc.array() :
                           (obj.contains("content") ? QJsonDocument::fromJson(obj["content"].toString().toUtf8()).array() : QJsonArray());
        emit blockedUsersReceived(users);
    }
    else if (url.contains("/api/friends/list") && !url.contains("/remark") && !url.contains("/group") && !url.contains("/block") && !url.contains("/unblock") && !url.contains("/detail")) {
        QJsonArray friends = doc.isArray() ? doc.array() :
                             (obj.contains("content") ? QJsonDocument::fromJson(obj["content"].toString().toUtf8()).array() : QJsonArray());
        emit friendsListReceived(friends);
    }
    else if (url.contains("/api/friends/") && url.contains("/detail")) {
        QJsonObject friendDetail = obj.contains("content") ? QJsonDocument::fromJson(obj["content"].toString().toUtf8()).object() : obj;
        QVariant context = reply->property("requestContext");
        emit friendDetailReceived(friendDetail, context);
    }
    else if (url.contains("/api/friends/") && url.contains("/remark")) {
        if (obj.contains("message") && obj["message"].toString() != "success") {
            emit errorOccurred(obj["message"].toString(), -1);
        } else {
            emit friendRemarkUpdated(true);
        }
    }
    else if (url.contains("/api/friends/") && url.contains("/group")) {
        if (obj.contains("message") && obj["message"].toString() != "success") {
            emit errorOccurred(obj["message"].toString(), -1);
        } else {
            emit friendGroupUpdated(true);
        }
    }
    else if (url.contains("/api/friends/block/")) {
        if (obj.contains("message") && obj["message"].toString() != "success") {
            emit errorOccurred(obj["message"].toString(), -1);
        } else {
            emit userBlocked(true);
        }
    }
    else if (url.contains("/api/friends/unblock/")) {
        if (obj.contains("message") && obj["message"].toString() != "success") {
            emit errorOccurred(obj["message"].toString(), -1);
        } else {
            emit userUnblocked(true);
        }
    }
    else if (url.contains("/api/friends/groups") && !url.contains("DELETE")) {
        if (url.contains("PUT")) {
            emit friendGroupRenamed(true);
        } else {
            QJsonArray groups = doc.isArray() ? doc.array() :
                                (obj.contains("content") ? QJsonDocument::fromJson(obj["content"].toString().toUtf8()).array() : QJsonArray());
            emit friendGroupsReceived(groups);
        }
    }
    else if (url.contains("/api/friends/groups") && url.contains("DELETE")) {
        emit friendGroupDeleted(true);
    }
    else if (url.contains("/api/conversations/") && url.contains("/mute") && !url.contains("/unmute")) {
        emit conversationMuted(true);
    }
    else if (url.contains("/api/conversations/") && url.contains("/unmute")) {
        emit conversationUnmuted(true);
    }
    else if (url.contains("/api/messages/private/") && url.contains("/before")) {
        QJsonArray messages = doc.isArray() ? doc.array() :
                              (obj.contains("content") ? QJsonDocument::fromJson(obj["content"].toString().toUtf8()).array() : QJsonArray());
        QString convUuid = extractUuidFromUrl(url, "/api/messages/private/");
        emit privateMessagesBeforeReceived(convUuid, messages);
    }
    else if (url.contains("/api/messages/private/") && !url.contains("/page") && !url.contains("/read")) {
        QJsonArray messages = doc.isArray() ? doc.array() :
                              (obj.contains("content") ? QJsonDocument::fromJson(obj["content"].toString().toUtf8()).array() : QJsonArray());
        QString convUuid = extractUuidFromUrl(url, "/api/messages/private/");
        emit privateMessagesReceived(convUuid, messages);
    }
    else if (url.contains("/api/messages/private/") && url.contains("/page")) {
        QJsonArray messages = doc.isArray() ? doc.array() :
                              (obj.contains("content") ? QJsonDocument::fromJson(obj["content"].toString().toUtf8()).array() : QJsonArray());
        QString convUuid = extractUuidFromUrl(url, "/api/messages/private/");
        emit privateMessagesPageReceived(convUuid, messages);
    }
    else if (url.contains("/api/messages/") && url.contains("/read")) {
        emit messageMarkedRead(true);
    }
    else if (url.contains("/api/messages/group/") && url.contains("/before")) {
        QJsonArray messages = doc.isArray() ? doc.array() :
                              (obj.contains("content") ? QJsonDocument::fromJson(obj["content"].toString().toUtf8()).array() : QJsonArray());
        QString convUuid = extractUuidFromUrl(url, "/api/messages/group/");
        emit groupMessagesBeforeReceived(convUuid, messages);
    }
    else if (url.contains("/api/messages/group/") && !url.contains("/page")) {
        QJsonArray messages = doc.isArray() ? doc.array() :
                              (obj.contains("content") ? QJsonDocument::fromJson(obj["content"].toString().toUtf8()).array() : QJsonArray());
        QString convUuid = extractUuidFromUrl(url, "/api/messages/group/");
        emit groupMessagesReceived(convUuid, messages);
    }
    else if (url.contains("/api/messages/group/") && url.contains("/page")) {
        QJsonArray messages = doc.isArray() ? doc.array() :
                              (obj.contains("content") ? QJsonDocument::fromJson(obj["content"].toString().toUtf8()).array() : QJsonArray());
        QString convUuid = extractUuidFromUrl(url, "/api/messages/group/");
        emit groupMessagesPageReceived(convUuid, messages);
    }
    else if (url.contains("/api/users/search")) {
        QJsonArray users = doc.isArray() ? doc.array() :
                           (obj.contains("content") ? QJsonDocument::fromJson(obj["content"].toString().toUtf8()).array() : QJsonArray());
        emit usersSearched(users);
    }
    else if (url.contains("/api/groups/search")) {
        QJsonArray groups = doc.isArray() ? doc.array() :
                            (obj.contains("content") ? QJsonDocument::fromJson(obj["content"].toString().toUtf8()).array() : QJsonArray());
        emit groupsSearched(groups);
    }
    else if (url.contains("/api/groups/list") && !url.contains("/members") && !url.contains("/search") && !url.contains("/requests")) {
        QJsonArray groups = doc.isArray() ? doc.array() :
                            (obj.contains("content") ? QJsonDocument::fromJson(obj["content"].toString().toUtf8()).array() : QJsonArray());
        emit myGroupsReceived(groups);
    }
    else if (url.contains("/api/groups/") && !url.contains("/members")) {
        QJsonObject groupData = obj.contains("content") ? QJsonDocument::fromJson(obj["content"].toString().toUtf8()).object() : obj;
        QVariant context = reply->property("requestContext");
        emit groupDetailReceived(groupData, context);
    }
    else if (url.contains("/api/groups/") && url.contains("/members")) {
        QJsonArray members = doc.isArray() ? doc.array() :
                             (obj.contains("content") ? QJsonDocument::fromJson(obj["content"].toString().toUtf8()).array() : QJsonArray());
        emit groupMembersReceived(members);
    }
    else if (url.contains("/api/users/")) {
        QJsonObject userData = obj.contains("content") ? QJsonDocument::fromJson(obj["content"].toString().toUtf8()).object() : obj;
        QVariant context = reply->property("requestContext");
        emit userInfoReceived(userData, context);
    }
    else if (url.contains("/api/status") && !url.contains("/batch")) {
        QJsonObject statusData = obj.contains("content") ? QJsonDocument::fromJson(obj["content"].toString().toUtf8()).object() : obj;
        emit statusUpdated(statusData);
    }
    else if (url.contains("/api/status/batch")) {
        QJsonArray statuses = doc.isArray() ? doc.array() :
                              (obj.contains("content") ? QJsonDocument::fromJson(obj["content"].toString().toUtf8()).array() : QJsonArray());
        emit multipleStatusesReceived(statuses);
    }
    else if (url.contains("/api/files/record")) {
        QString fileUuid = obj["uuid"].toString();
        
        // 从 reply 属性中获取该请求对应的文件数据
        QByteArray fileData = reply->property("fileData").toByteArray();
        
        if (!fileUuid.isEmpty() && !fileData.isEmpty()) {
            QUrl uploadUrl(m_serverUrl + "/api/files/upload");
            QNetworkRequest request(uploadUrl);
            request.setHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream");
            request.setRawHeader("Connection", "close");
            request.setRawHeader("Authorization", QString("Bearer %1").arg(UserSession::instance()->token()).toUtf8());
            request.setRawHeader("fileUuid", fileUuid.toUtf8());

            QNetworkReply* uploadReply = m_nam->post(request, fileData);
            if (uploadReply) {
                m_pendingReplies.append(uploadReply);
            }
        }
    }
    else if (url.contains("/api/files/upload")) {
        QJsonObject fileInfo = obj.contains("content") ? QJsonDocument::fromJson(obj["content"].toString().toUtf8()).object() : obj;
        emit fileUploaded(fileInfo);
    }

    reply->deleteLater();
}

void HttpApiClient::login(const QString& email, const QString& password)
{
    QJsonObject data;
    data["email"] = email;
    data["password"] = password;
    sendHttpRequest("POST", "/api/auth/login", data);
}

void HttpApiClient::registerUser(const QString& username, const QString& email,
    const QString& password, const QString& verificationCode)
{
    QJsonObject data;
    data["username"] = username;
    data["email"] = email;
    data["password"] = password;
    if (!verificationCode.isEmpty()) data["verificationcode"] = verificationCode;
    sendHttpRequest("POST", "/api/auth/register", data);
}

void HttpApiClient::logout()
{
    sendAuthenticatedRequest("POST", "/api/auth/logout", QJsonObject());
}

void HttpApiClient::abortAllRequests()
{
    // 先收集需要 abort 的 reply（避免在遍历中修改容器）
    QList<QNetworkReply*> pending = m_pendingReplies;
    m_pendingReplies.clear();

    for (QNetworkReply* reply : pending) {
        if (reply && reply->isRunning()) {
            reply->abort();
            reply->deleteLater();
        }
    }
}

void HttpApiClient::resetPassword(const QString& email, const QString& code, const QString& oldPassword, const QString& newPassword)
{
    QJsonObject data;
    data["email"] = email;
    data["verificationcode"] = code;
    data["oldpassword"] = oldPassword;
    data["newpassword"] = newPassword;
    sendHttpRequest("POST", "/api/auth/reset-password", data);
}

void HttpApiClient::sendVerificationCode(const QString& email)
{
    QJsonObject data;
    data["email"] = email;
    sendHttpRequest("POST", "/api/auth/send-code", data);
}

void HttpApiClient::getCurrentUser()
{
    sendAuthenticatedRequest("GET", "/api/user/me", QJsonObject());
}

void HttpApiClient::getConversations()
{
    sendAuthenticatedRequest("GET", "/api/conversations", QJsonObject());
}

void HttpApiClient::markPrivateConversationRead(const QString& targetUserUuid)
{
    sendAuthenticatedRequest("PUT", QString("/api/conversations/users/%1/read").arg(targetUserUuid), QJsonObject());
}

void HttpApiClient::markGroupConversationRead(const QString& groupUuid)
{
    sendAuthenticatedRequest("PUT", QString("/api/conversations/groups/%1/read").arg(groupUuid), QJsonObject());
}

void HttpApiClient::getSentRequests()
{
    sendAuthenticatedRequest("GET", "/api/friends/requests/sent", QJsonObject());
}

void HttpApiClient::getReceivedRequests()
{
    sendAuthenticatedRequest("GET", "/api/friends/requests/received", QJsonObject());
}

void HttpApiClient::getReceivedGroupRequests()
{
    sendAuthenticatedRequest("GET", "/api/groups/requests/received", QJsonObject());
}

void HttpApiClient::getSentGroupRequests()
{
    sendAuthenticatedRequest("GET", "/api/groups/requests/sent", QJsonObject());
}

void HttpApiClient::getFriends()
{
    sendAuthenticatedRequest("GET", "/api/friends/list", QJsonObject());
}

void HttpApiClient::getBlockedUsers()
{
    sendAuthenticatedRequest("GET", "/api/friends/blocked", QJsonObject());
}

void HttpApiClient::updateFriendRemark(const QString& friendId, const QString& remark)
{
    QJsonObject data;
    data["remark"] = remark;
    sendAuthenticatedRequest("PUT", QString("/api/friends/%1/remark").arg(friendId), data);
}

void HttpApiClient::updateFriendGroup(const QString& friendId, const QString& group)
{
    QJsonObject data;
    data["group"] = group;
    sendAuthenticatedRequest("PUT", QString("/api/friends/%1/group").arg(friendId), data);
}

void HttpApiClient::blockUser(const QString& targetUserId)
{
    sendAuthenticatedRequest("POST", QString("/api/friends/block/%1").arg(targetUserId), QJsonObject());
}

void HttpApiClient::unblockUser(const QString& targetUserId)
{
    sendAuthenticatedRequest("POST", QString("/api/friends/unblock/%1").arg(targetUserId), QJsonObject());
}

// ==================== 好友分组管理 ====================

void HttpApiClient::getFriendGroups()
{
    sendAuthenticatedRequest("GET", "/api/friends/groups", QJsonObject());
}

void HttpApiClient::createFriendGroup(const QString& name, const QJsonArray& memberUuids)
{
    QJsonObject data;
    data["name"] = name;
    if (!memberUuids.isEmpty()) data["memberuuids"] = memberUuids;
    sendAuthenticatedRequest("POST", "/api/friends/groups", data);
}

void HttpApiClient::deleteFriendGroup(const QString& groupName)
{
    sendAuthenticatedRequest("DELETE", QString("/api/friends/groups/%1").arg(groupName), QJsonObject());
}

void HttpApiClient::renameFriendGroup(const QString& oldName, const QString& newName)
{
    QJsonObject data;
    data["newname"] = newName;
    sendAuthenticatedRequest("PUT", QString("/api/friends/groups/%1").arg(oldName), data);
}

// ==================== 消息免打扰 ====================

void HttpApiClient::mutePrivateConversation(const QString& targetUserUuid)
{
    sendAuthenticatedRequest("PUT", QString("/api/conversations/users/%1/mute").arg(targetUserUuid), QJsonObject());
}

void HttpApiClient::unmutePrivateConversation(const QString& targetUserUuid)
{
    sendAuthenticatedRequest("PUT", QString("/api/conversations/users/%1/unmute").arg(targetUserUuid), QJsonObject());
}

void HttpApiClient::muteGroupConversation(const QString& groupUuid)
{
    sendAuthenticatedRequest("PUT", QString("/api/conversations/groups/%1/mute").arg(groupUuid), QJsonObject());
}

void HttpApiClient::unmuteGroupConversation(const QString& groupUuid)
{
    sendAuthenticatedRequest("PUT", QString("/api/conversations/groups/%1/unmute").arg(groupUuid), QJsonObject());
}

void HttpApiClient::getPrivateMessagesPage(const QString& uid, int page, int size)
{
    sendAuthenticatedRequest("GET", QString("/api/messages/private/%1/page?page=%2&size=%3").arg(uid).arg(page).arg(size), QJsonObject());
}

void HttpApiClient::getPrivateMessagesBefore(const QString& uid, const QString& msgUuid, int size)
{
    sendAuthenticatedRequest("GET", QString("/api/messages/private/%1/before?msgUuid=%2&size=%3").arg(uid).arg(msgUuid).arg(size), QJsonObject());
}

//void HttpApiClient::markMessageRead(const QString& mid)
//{
//    sendAuthenticatedRequest("PUT", QString("/api/messages/%1/read").arg(mid), QJsonObject());
//}

void HttpApiClient::getGroupMessagesPage(const QString& gid, int page, int size)
{
    sendAuthenticatedRequest("GET", QString("/api/messages/group/%1/page?page=%2&size=%3").arg(gid).arg(page).arg(size), QJsonObject());
}

void HttpApiClient::getGroupMessagesBefore(const QString& gid, const QString& msgUuid, int size)
{
    sendAuthenticatedRequest("GET", QString("/api/messages/group/%1/before?msgUuid=%2&size=%3").arg(gid).arg(msgUuid).arg(size), QJsonObject());
}

void HttpApiClient::getMyGroups()
{
    sendAuthenticatedRequest("GET", "/api/groups/list", QJsonObject());
}

void HttpApiClient::getGroupDetail(const QString& groupUuid, const QVariant& context)
{
    sendAuthenticatedRequest("GET", QString("/api/groups/%1").arg(groupUuid), QJsonObject(), context);
}

void HttpApiClient::getGroupMembers(const QString& groupUuid)
{
    sendAuthenticatedRequest("GET", QString("/api/groups/%1/members").arg(groupUuid), QJsonObject());
}

void HttpApiClient::updateStatus(const QString& status)
{
    QJsonObject data;
    data["status"] = status;
    sendAuthenticatedRequest("PUT", "/api/status", data);
}

void HttpApiClient::getMultipleStatuses(const QJsonArray& userIds)
{
    QJsonObject data;
    data["useruuids"] = userIds;
    sendAuthenticatedRequest("POST", "/api/status/batch", data);
}

void HttpApiClient::getFriendDetail(const QString& friendUuid, const QVariant& context)
{
    sendAuthenticatedRequest("GET", QString("/api/friends/%1/detail").arg(friendUuid), QJsonObject(), context);
}

void HttpApiClient::getUserInfo(const QString& userUuid, const QVariant& context)
{
    sendAuthenticatedRequest("GET", QString("/api/users/info/%1").arg(userUuid), QJsonObject(), context);
}

void HttpApiClient::searchUsers(const QString& keyword)
{
    QString encoded = QUrl::toPercentEncoding(keyword);
    sendAuthenticatedRequest("GET", QString("/api/users/search?keyword=%1").arg(encoded), QJsonObject());
}

void HttpApiClient::searchGroups(const QString& keyword)
{
    QString encoded = QUrl::toPercentEncoding(keyword);
    sendAuthenticatedRequest("GET", QString("/api/groups/search?keyword=%1").arg(encoded), QJsonObject());
}

void HttpApiClient::uploadAvatarFile(const QString& fileName, const QString& mimeType,
                               qint64 fileSize, const QByteArray& fileData)
{
    QJsonObject data;
    data["filetype"] = "avatar";
    data["mimetype"] = mimeType;
    data["filesize"] = fileSize;

    // 直接发送请求，不使用 sendAuthenticatedRequest，以便绑定文件数据到 reply
    QUrl url(m_serverUrl + "/api/files/record");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Connection", "close");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(UserSession::instance()->token()).toUtf8());

    QNetworkReply* reply = m_nam->post(request, QJsonDocument(data).toJson());
    if (reply) {
        m_pendingReplies.append(reply);
        // 将文件数据绑定到 reply 对象，每个请求携带自己的文件数据
        reply->setProperty("fileData", fileData);
        // 不需要单独连接信号，m_nam 的 finished 信号已经连接到 onReplyFinished
    }
}

QString HttpApiClient::extractUuidFromUrl(const QString& url, const QString& prefix) const
{
    int startIndex = url.indexOf(prefix) + prefix.length();
    int endIndex = url.indexOf("/", startIndex);
    if (endIndex == -1) {
        endIndex = url.indexOf("?", startIndex);
    }
    if (endIndex == -1) {
        return url.mid(startIndex);
    }
    return url.mid(startIndex, endIndex - startIndex);
}
