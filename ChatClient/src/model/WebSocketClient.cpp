#include "WebSocketClient.h"
#include "UserSession.h"
#include <QMetaObject>

WebSocketClient* WebSocketClient::s_instance = nullptr;

WebSocketClient::WebSocketClient(QObject* parent)
    : QObject(parent), m_webSocket(nullptr), m_wsThread(new QThread()),
      m_reconnectTimer(nullptr), m_reconnectDelay(5000), m_manualDisconnect(false)
{
    initResponseHandlers();
    moveToThread(m_wsThread);
    m_wsThread->start();
}

WebSocketClient::~WebSocketClient()
{
    if (m_wsThread) {
        m_wsThread->quit();
        m_wsThread->wait();
        delete m_wsThread;
    }
    stopReconnectTimer();
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
    QMetaObject::invokeMethod(this, [this]() {
        m_manualDisconnect = false;
        
        if (m_webSocket) {
            disconnect(m_webSocket, nullptr, this, nullptr);
            m_webSocket->close();
            m_webSocket->deleteLater();
        }

        m_webSocket = new QWebSocket();
        m_webSocket->moveToThread(this->thread());

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
    }, Qt::QueuedConnection);
}

void WebSocketClient::disconnectWebSocket()
{
    QMetaObject::invokeMethod(this, [this]() {
        m_manualDisconnect = true;
        stopReconnectTimer();
        
        if (m_webSocket) {
            m_webSocket->close();
        }
        
        emit connectionStatusChanged(false, "已断开连接");
    }, Qt::QueuedConnection);
}

bool WebSocketClient::isConnected() const
{
    if (thread() != QThread::currentThread()) {
        bool result = false;
        QMetaObject::invokeMethod(const_cast<WebSocketClient*>(this), [this, &result]() {
            if (m_webSocket) {
                result = m_webSocket->state() == QAbstractSocket::ConnectedState;
            }
        }, Qt::BlockingQueuedConnection);
        return result;
    }
    if (m_webSocket) {
        return m_webSocket->state() == QAbstractSocket::ConnectedState;
    }
    return false;
}

void WebSocketClient::sendMessage(const QString& type, const QJsonObject& data)
{
    QMetaObject::invokeMethod(this, [this, type, data]() {
        if (!m_webSocket || m_webSocket->state() != QAbstractSocket::ConnectedState) {
            emit errorOccurred("WebSocket not connected");
            return;
        }

        QJsonObject message = data;
        message["type"] = type;
        message["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);

        QJsonDocument doc(message);
        m_webSocket->sendTextMessage(doc.toJson(QJsonDocument::Compact));
    }, Qt::QueuedConnection);
}

void WebSocketClient::onConnected()
{
    qDebug() << "WebSocket connected";
    m_reconnectDelay = 5000;
    stopReconnectTimer();
    emit connected();
    emit connectionStatusChanged(true, "已连接");
}

void WebSocketClient::onDisconnected()
{
    qDebug() << "WebSocket disconnected";
    qDebug() << "Close code:" << m_webSocket->closeCode();
    qDebug() << "Close reason:" << m_webSocket->closeReason();

    if (m_webSocket->closeCode() == 1001 || m_webSocket->closeReason().contains("kick")) {
        emit kickedByServer();
        emit connectionStatusChanged(false, "已被服务器踢出");
        return;
    }
    
    emit disconnected();
    
    if (!m_manualDisconnect) {
        emit connectionStatusChanged(false, "连接断开，正在尝试重连...");
        startReconnectTimer();
    }
}

void WebSocketClient::onTextMessageReceived(const QString& message)
{
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    QJsonObject obj = doc.object();

    QString type = obj["type"].toString();

    if (type == "new_group_member") {
        QJsonObject event = QJsonDocument::fromJson(obj["content"].toString().toUtf8()).object();
        emit groupMemberAddedBroadcast(event);
        return;
    }
    
    if (type == "group_member_removed") {
        QJsonObject event = QJsonDocument::fromJson(obj["content"].toString().toUtf8()).object();
        emit groupMemberRemovedBroadcast(event);
        return;
    }
    
    if (type == "group_dissolved") {
        QJsonObject event = QJsonDocument::fromJson(obj["content"].toString().toUtf8()).object();
        emit groupDissolvedBroadcast(event);
        return;
    }
    
    if (type == "group_member_left") {
        QJsonObject event = QJsonDocument::fromJson(obj["content"].toString().toUtf8()).object();
        emit groupMemberLeftBroadcast(event);
        return;
    }
    
    if (type == "group_created") {
        QJsonObject group = QJsonDocument::fromJson(obj["content"].toString().toUtf8()).object();
        emit groupCreatedBroadcast(group);
        return;
    }

    if (type == "friend_request_accepted") {
        QJsonObject friendInfo = QJsonDocument::fromJson(obj["content"].toString().toUtf8()).object();
        emit friendRequestAcceptedBroadcast(friendInfo);
        return;
    }

    if (type == "group_request_accepted") {
        QJsonObject groupInfo = QJsonDocument::fromJson(obj["content"].toString().toUtf8()).object();
        emit groupRequestAcceptedBroadcast(groupInfo);
        return;
    }

    auto it = m_responseHandlers.find(type);
    if (it != m_responseHandlers.end()) {
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
    
    if (!m_manualDisconnect) {
        emit connectionStatusChanged(false, QString("连接错误: %1").arg(m_webSocket->errorString()));
        startReconnectTimer();
    }
}

void WebSocketClient::onReconnectTimer()
{
    if (!m_manualDisconnect) {
        qDebug() << "Attempting to reconnect...";
        connectWebSocket();
    }
}

void WebSocketClient::startReconnectTimer()
{
    if (!m_reconnectTimer) {
        m_reconnectTimer = new QTimer(this);
        connect(m_reconnectTimer, &QTimer::timeout, this, &WebSocketClient::onReconnectTimer);
    }
    
    m_reconnectTimer->stop();
    m_reconnectTimer->start(m_reconnectDelay);
    
    m_reconnectDelay = qMin(m_reconnectDelay * 2, 30000);
}

void WebSocketClient::stopReconnectTimer()
{
    if (m_reconnectTimer) {
        m_reconnectTimer->stop();
    }
    m_reconnectDelay = 5000;
}

void WebSocketClient::initResponseHandlers()
{
    m_responseHandlers["send_friend_request_response"] = [this](const QJsonObject& obj) {
        QJsonObject result = QJsonDocument::fromJson(obj["content"].toString().toUtf8()).object();
        emit friendRequestSent(result);
    };

    m_responseHandlers["handle_friend_request_response"] = [this](const QJsonObject& obj) {
        QString requestUuid = obj["requestuuid"].toString();
        emit friendRequestHandled(requestUuid, obj["success"].toBool());
    };

    m_responseHandlers["delete_friend_response"] = [this](const QJsonObject& obj) {
        emit friendDeleted(obj["success"].toBool());
    };

    m_responseHandlers["send_group_request_response"] = [this](const QJsonObject& obj) {
        QJsonObject result = QJsonDocument::fromJson(obj["content"].toString().toUtf8()).object();
        emit groupRequestSent(result);
    };

    m_responseHandlers["handle_group_request_response"] = [this](const QJsonObject& obj) {
        QString requestUuid = obj["uuid"].toString();
        emit groupRequestHandled(requestUuid, obj["success"].toBool());
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

    m_responseHandlers["message_recalled"] = [this](const QJsonObject& obj) {
        QString messageUuid = obj["content"].toString();
        emit messageRecalledByOther(messageUuid);
    };

    m_responseHandlers["error"] = [this](const QJsonObject& obj) {
        QString errorMsg = obj["content"].toString();
        emit errorOccurred(errorMsg);
    };
}

void WebSocketClient::sendFriendRequest(const QString& toUserUuid, const QString& message)
{
    QJsonObject data;
    data["touseruuid"] = toUserUuid;
    if (!message.isEmpty()) data["message"] = message;
    sendMessage("send_friend_request", data);
}

void WebSocketClient::handleFriendRequest(const QString& reqUuid, const QString& status)
{
    QJsonObject data;
    data["requuid"] = reqUuid;
    data["status"] = status;
    sendMessage("handle_friend_request", data);
}

void WebSocketClient::deleteFriend(const QString& frienduuid)
{
    QJsonObject data;
    data["frienduuid"] = frienduuid;
    sendMessage("delete_friend", data);
}

void WebSocketClient::sendGroupRequest(const QString& groupUuid, const QString& message)
{
    QJsonObject data;
    data["groupuuid"] = groupUuid;
    if (!message.isEmpty()) data["message"] = message;
    sendMessage("send_group_request", data);
}

void WebSocketClient::handleGroupRequest(const QString& grUuid, const QString& status)
{
    QJsonObject data;
    data["gruuid"] = grUuid;
    data["status"] = status;
    sendMessage("handle_group_request", data);
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

void WebSocketClient::promoteGroupMember(const QString& groupuuid, const QString& targetuseruuid)
{
    setMemberRole(groupuuid, targetuseruuid, "admin");
}

void WebSocketClient::demoteGroupMember(const QString& groupuuid, const QString& targetuseruuid)
{
    setMemberRole(groupuuid, targetuseruuid, "member");
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
