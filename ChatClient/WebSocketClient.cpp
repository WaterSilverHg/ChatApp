#include "WebSocketClient.h"
#include "UserSession.h"
#include <QDebug>

WebSocketClient* WebSocketClient::s_instance = nullptr;

WebSocketClient::WebSocketClient(QObject* parent)
    : QObject(parent), m_webSocket(nullptr)
{
    initResponseHandlers();
}

WebSocketClient* WebSocketClient::instance()
{
    if (!s_instance) {
        s_instance = new WebSocketClient();
    }
    return s_instance;
}

void WebSocketClient::setWsUrl(const QString& url)
{
    m_wsUrl = url;
}

void WebSocketClient::connectWebSocket()
{
    if (m_webSocket) {
        m_webSocket->deleteLater();
    }

    m_webSocket = new QWebSocket();

    connect(m_webSocket, &QWebSocket::connected, this, &WebSocketClient::onConnected);
    connect(m_webSocket, &QWebSocket::disconnected, this, &WebSocketClient::onDisconnected);
    connect(m_webSocket, &QWebSocket::textMessageReceived, this, &WebSocketClient::onTextMessageReceived);
    connect(m_webSocket, &QWebSocket::errorOccurred, this, &WebSocketClient::onError);

    QUrl url(m_wsUrl);
    QNetworkRequest request(url);
    QString token = UserSession::instance()->token();
    if (!token.isEmpty()) {
        request.setRawHeader("Authorization", QString("Bearer %1").arg(token).toUtf8());
    }

    m_webSocket->open(request);
}

void WebSocketClient::disconnectWebSocket()
{
    if (m_webSocket) {
        m_webSocket->close();
    }
}

bool WebSocketClient::isConnected() const
{
    if (m_webSocket) {
        return m_webSocket->state() == QAbstractSocket::ConnectedState;
    }
    return false;
}

void WebSocketClient::sendMessage(const QString& type, const QJsonObject& data)
{
    if (!m_webSocket || m_webSocket->state() != QAbstractSocket::ConnectedState) {
        emit errorOccurred("WebSocket not connected");
        return;
    }

    QJsonObject message = data;
    message["type"] = type;
    message["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    QJsonDocument doc(message);
    m_webSocket->sendTextMessage(doc.toJson(QJsonDocument::Compact));
}

void WebSocketClient::onConnected()
{
    qDebug() << "WebSocket connected";
    emit connected();
}

void WebSocketClient::onDisconnected()
{
    qDebug() << "WebSocket disconnected";
    emit disconnected();
}

void WebSocketClient::onTextMessageReceived(const QString& message)
{
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    QJsonObject obj = doc.object();

    QString type = obj["type"].toString();

    auto it = m_responseHandlers.find(type);
    if (it != m_responseHandlers.end()) {
        if (type.startsWith("new_")) {
            obj["success"] = true;
        }
        bool success = obj["success"].toBool();
        if (!success) {
            QString errorMessage = obj["content"].toString();
            emit errorOccurred(errorMessage);
            return;
        }
        it.value()(obj);
    }
}

void WebSocketClient::onError(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error);
    qDebug() << "WebSocket error:" << m_webSocket->errorString();
    emit errorOccurred(m_webSocket->errorString());
}

void WebSocketClient::initResponseHandlers()
{
    m_responseHandlers["send_friend_request_response"] = [this](const QJsonObject& obj) {
        QJsonObject result = QJsonDocument::fromJson(obj["content"].toString().toUtf8()).object();
        emit friendRequestSent(result);
    };

    m_responseHandlers["accept_friend_request_response"] = [this](const QJsonObject& obj) {
        emit friendRequestAccepted(obj["success"].toBool());
    };

    m_responseHandlers["reject_friend_request_response"] = [this](const QJsonObject& obj) {
        emit friendRequestRejected(obj["success"].toBool());
    };

    m_responseHandlers["cancel_friend_request_response"] = [this](const QJsonObject& obj) {
        emit friendRequestCancelled(obj["success"].toBool());
    };

    m_responseHandlers["delete_friend_response"] = [this](const QJsonObject& obj) {
        emit friendDeleted(obj["success"].toBool());
    };

    m_responseHandlers["create_group_response"] = [this](const QJsonObject& obj) {
        QJsonObject group = QJsonDocument::fromJson(obj["content"].toString().toUtf8()).object();
        emit groupCreated(group);
    };

    m_responseHandlers["update_group_response"] = [this](const QJsonObject& obj) {
        emit groupUpdated(obj["success"].toBool());
    };

    m_responseHandlers["dissolve_group_response"] = [this](const QJsonObject& obj) {
        emit groupDissolved(obj["success"].toBool());
    };

    m_responseHandlers["add_group_members_response"] = [this](const QJsonObject& obj) {
        emit groupMembersAdded(obj["success"].toBool());
    };

    m_responseHandlers["remove_group_member_response"] = [this](const QJsonObject& obj) {
        emit groupMemberRemoved(obj["success"].toBool());
    };

    m_responseHandlers["set_member_role_response"] = [this](const QJsonObject& obj) {
        emit memberRoleSet(obj["success"].toBool());
    };

    m_responseHandlers["join_group_response"] = [this](const QJsonObject& obj) {
        emit groupJoined(obj["success"].toBool());
    };

    m_responseHandlers["leave_group_response"] = [this](const QJsonObject& obj) {
        emit groupLeft(obj["success"].toBool());
    };

    m_responseHandlers["send_private_message_response"] = [this](const QJsonObject& obj) {
        QJsonObject result = QJsonDocument::fromJson(obj["content"].toString().toUtf8()).object();
        emit privateMessageSent(result);
    };

    m_responseHandlers["send_group_message_response"] = [this](const QJsonObject& obj) {
        QJsonObject result = QJsonDocument::fromJson(obj["content"].toString().toUtf8()).object();
        emit groupMessageSent(result);
    };

    m_responseHandlers["new_private_message"] = [this](const QJsonObject& obj) {
        QJsonObject message = QJsonDocument::fromJson(obj["content"].toString().toUtf8()).object();
        emit newPrivateMessageReceived(message);
    };

    m_responseHandlers["new_group_message"] = [this](const QJsonObject& obj) {
        QJsonObject message = QJsonDocument::fromJson(obj["content"].toString().toUtf8()).object();
        emit newGroupMessageReceived(message);
    };

    m_responseHandlers["recall_message_response"] = [this](const QJsonObject& obj) {
        emit messageRecalled(obj["success"].toBool());
    };
}

void WebSocketClient::sendFriendRequest(const QString& toUserUuid, const QString& message)
{
    QJsonObject data;
    data["toUserUuid"] = toUserUuid;
    if (!message.isEmpty()) data["message"] = message;
    sendMessage("send_friend_request", data);
}

void WebSocketClient::acceptFriendRequest(const QString& requuid)
{
    QJsonObject data;
    data["requuid"] = requuid;
    sendMessage("accept_friend_request", data);
}

void WebSocketClient::rejectFriendRequest(const QString& requuid)
{
    QJsonObject data;
    data["requuid"] = requuid;
    sendMessage("reject_friend_request", data);
}

void WebSocketClient::cancelFriendRequest(const QString& requuid)
{
    QJsonObject data;
    data["requuid"] = requuid;
    sendMessage("cancel_friend_request", data);
}

void WebSocketClient::deleteFriend(const QString& frienduuid)
{
    QJsonObject data;
    data["frienduuid"] = frienduuid;
    sendMessage("delete_friend", data);
}

void WebSocketClient::createGroup(const QString& name, const QString& description,
    const QString& avatarurl, const QJsonArray& memberuuids, int maxmembers, bool ispublic)
{
    QJsonObject data;
    data["name"] = name;
    if (!description.isEmpty()) data["description"] = description;
    if (!avatarurl.isEmpty()) data["avatarurl"] = avatarurl;
    if (!memberuuids.isEmpty()) data["memberuuids"] = memberuuids;
    if (maxmembers > 0) data["maxmembers"] = maxmembers;
    data["ispublic"] = ispublic;
    sendMessage("create_group", data);
}

void WebSocketClient::updateGroup(const QString& groupuuid, const QString& name,
    const QString& description, const QString& avatarurl, int maxmembers, bool ispublic)
{
    QJsonObject data;
    data["groupuuid"] = groupuuid;
    if (!name.isEmpty()) data["name"] = name;
    if (!description.isEmpty()) data["description"] = description;
    if (!avatarurl.isEmpty()) data["avatarurl"] = avatarurl;
    if (maxmembers > 0) data["maxmembers"] = maxmembers;
    data["ispublic"] = ispublic;
    sendMessage("update_group", data);
}

void WebSocketClient::dissolveGroup(const QString& groupuuid)
{
    QJsonObject data;
    data["groupuuid"] = groupuuid;
    sendMessage("dissolve_group", data);
}

void WebSocketClient::addGroupMembers(const QString& groupuuid, const QJsonArray& useruuids)
{
    QJsonObject data;
    data["groupuuid"] = groupuuid;
    data["useruuids"] = useruuids;
    sendMessage("add_group_members", data);
}

void WebSocketClient::removeGroupMember(const QString& groupuuid, const QString& useruuid)
{
    QJsonObject data;
    data["groupuuid"] = groupuuid;
    data["useruuid"] = useruuid;
    sendMessage("remove_group_member", data);
}

void WebSocketClient::setMemberRole(const QString& groupuuid, const QString& targetuseruuid, const QString& role)
{
    QJsonObject data;
    data["groupuuid"] = groupuuid;
    data["targetuseruuid"] = targetuseruuid;
    data["role"] = role;
    sendMessage("set_member_role", data);
}

void WebSocketClient::joinGroup(const QString& groupuuid)
{
    QJsonObject data;
    data["groupuuid"] = groupuuid;
    sendMessage("join_group", data);
}

void WebSocketClient::leaveGroup(const QString& groupuuid)
{
    QJsonObject data;
    data["groupuuid"] = groupuuid;
    sendMessage("leave_group", data);
}

void WebSocketClient::sendPrivateMessage(const QString& touseruuid, const QString& content, const QString& messagetype)
{
    QJsonObject data;
    data["touseruuid"] = touseruuid;
    data["content"] = content;
    data["messagetype"] = messagetype;
    sendMessage("send_private_message", data);
}

void WebSocketClient::recallMessage(const QString& messageuuid)
{
    QJsonObject data;
    data["messageuuid"] = messageuuid;
    sendMessage("recall_message", data);
}

void WebSocketClient::sendGroupMessage(const QString& groupuuid, const QString& content, const QString& messagetype)
{
    QJsonObject data;
    data["groupuuid"] = groupuuid;
    data["content"] = content;
    data["messagetype"] = messagetype;
    sendMessage("send_group_message", data);
}
