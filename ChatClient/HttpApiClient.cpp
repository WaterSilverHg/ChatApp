#include "HttpApiClient.h"
#include "UserSession.h"
#include <QUrlQuery>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QByteArray>

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
    const QJsonObject& data)
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
    else if (url.contains("/api/conversations")) {
        if(url.contains("/read")){
            emit conversationMarkedRead(true);

        }
        else{

            QJsonArray conversations = doc.isArray() ? doc.array() :
                                           (obj.contains("content") ? QJsonDocument::fromJson(obj["content"].toString().toUtf8()).array() : QJsonArray());
            emit conversationsReceived(conversations);
        }

    }
    else if (url.contains("/api/friends/requests/sent")) {
        QJsonArray requests = doc.isArray() ? doc.array() :
                              (obj.contains("content") ? QJsonDocument::fromJson(obj["content"].toString().toUtf8()).array() : QJsonArray());
        emit sentRequestsReceived(requests);
    }
    else if (url.contains("/api/friends/requests/received")) {
        QJsonArray requests = doc.isArray() ? doc.array() :
                              (obj.contains("content") ? QJsonDocument::fromJson(obj["content"].toString().toUtf8()).array() : QJsonArray());
        emit receivedRequestsReceived(requests);
    }
    else if (url.contains("/api/groups/requests/received")) {
        QJsonArray requests = doc.isArray() ? doc.array() :
                              (obj.contains("content") ? QJsonDocument::fromJson(obj["content"].toString().toUtf8()).array() : QJsonArray());
        emit receivedGroupRequestsReceived(requests);
    }
    else if (url.contains("/api/groups/requests/sent")) {
        QJsonArray requests = doc.isArray() ? doc.array() :
                              (obj.contains("content") ? QJsonDocument::fromJson(obj["content"].toString().toUtf8()).array() : QJsonArray());
        emit sentGroupRequestsReceived(requests);
    }
    else if (url.contains("/api/friends/list") && !url.contains("/remark") && !url.contains("/group") && !url.contains("/block") && !url.contains("/unblock")) {
        QJsonArray friends = doc.isArray() ? doc.array() :
                             (obj.contains("content") ? QJsonDocument::fromJson(obj["content"].toString().toUtf8()).array() : QJsonArray());
        emit friendsListReceived(friends);
    }
    else if (url.contains("/api/friends/") && url.contains("/remark")) {
        emit friendRemarkUpdated(true);
    }
    else if (url.contains("/api/friends/") && url.contains("/group")) {
        emit friendGroupUpdated(true);
    }
    else if (url.contains("/api/friends/block/")) {
        emit userBlocked(true);
    }
    else if (url.contains("/api/friends/unblock/")) {
        emit userUnblocked(true);
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
        emit groupDetailReceived(groupData);
    }
    else if (url.contains("/api/groups/") && url.contains("/members")) {
        QJsonArray members = doc.isArray() ? doc.array() :
                             (obj.contains("content") ? QJsonDocument::fromJson(obj["content"].toString().toUtf8()).array() : QJsonArray());
        emit groupMembersReceived(members);
    }
    else if (url.contains("/api/users/")) {
        QJsonObject userData = obj.contains("content") ? QJsonDocument::fromJson(obj["content"].toString().toUtf8()).object() : obj;
        emit userDetailReceived(userData);
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
        
        if (!fileUuid.isEmpty() && !m_pendingFileData.isEmpty()) {
            QUrl uploadUrl(m_serverUrl + "/api/files/upload");
            QNetworkRequest request(uploadUrl);
            request.setHeader(QNetworkRequest::ContentTypeHeader, "application/octet-stream");
            request.setRawHeader("Connection", "close");
            request.setRawHeader("Authorization", QString("Bearer %1").arg(UserSession::instance()->token()).toUtf8());
            request.setRawHeader("fileUuid", fileUuid.toUtf8());

            m_nam->post(request, m_pendingFileData);
            m_pendingFileData.clear();
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
    const QString& password, const QString& phone)
{
    QJsonObject data;
    data["username"] = username;
    data["email"] = email;
    data["password"] = password;
    if (!phone.isEmpty()) data["phone"] = phone;
    sendHttpRequest("POST", "/api/auth/register", data);
}

void HttpApiClient::logout()
{
    sendAuthenticatedRequest("POST", "/api/auth/logout", QJsonObject());
}

void HttpApiClient::abortAllRequests()
{
    for (QNetworkReply* reply : m_pendingReplies) {
        if (reply && reply->isRunning()) {
            reply->abort();
            reply->deleteLater();
        }
    }
    m_pendingReplies.clear();
}

void HttpApiClient::resetPassword(const QString& email, const QString& code, const QString& oldPassword, const QString& newPassword)
{
    QJsonObject data;
    data["email"] = email;
    data["verificationCode"] = code;
    data["oldPassword"] = oldPassword;
    data["newPassword"] = newPassword;
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

void HttpApiClient::markConversationRead(const QString& convid)
{
    sendAuthenticatedRequest("PUT", QString("/api/conversations/%1/read").arg(convid), QJsonObject());
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

void HttpApiClient::getGroupDetail(const QString& groupUuid)
{
    sendAuthenticatedRequest("GET", QString("/api/groups/%1").arg(groupUuid), QJsonObject());
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

void HttpApiClient::getUserInfo(const QString& userUuid)
{
    sendAuthenticatedRequest("GET", QString("/api/users/info/%1").arg(userUuid), QJsonObject());
}

void HttpApiClient::searchUsers(const QString& keyword)
{
    sendAuthenticatedRequest("GET", QString("/api/users/search?keyword=%1").arg(keyword), QJsonObject());
}

void HttpApiClient::searchGroups(const QString& keyword)
{
    sendAuthenticatedRequest("GET", QString("/api/groups/search?keyword=%1").arg(keyword), QJsonObject());
}

void HttpApiClient::uploadAvatarFile(const QString& fileName, const QString& mimeType,
                               qint64 fileSize, const QByteArray& fileData)
{
    m_pendingFileData = fileData;

    QJsonObject data;
    //QFileInfo fileInfo(fileName);
    data["filetype"] = "avatar";
    data["mimetype"] = mimeType;
    data["filesize"] = fileSize;

    sendAuthenticatedRequest("POST", "/api/files/record", data);
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
