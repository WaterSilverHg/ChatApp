#ifndef WEBSOCKETCLIENT_H
#define WEBSOCKETCLIENT_H

#include "global.h"
#include <functional>

class WebSocketClient : public QObject
{
    Q_OBJECT

public:
    static WebSocketClient* instance();

    void setWsUrl(const QString& url);
    QString wsUrl() const { return m_wsUrl; }

    void connectWebSocket();
    void disconnectWebSocket();
    bool isConnected() const;

    void sendFriendRequest(const QString& toUserUuid, const QString& message = "");
    void acceptFriendRequest(const QString& requuid);
    void rejectFriendRequest(const QString& requuid);
    void cancelFriendRequest(const QString& requuid);
    void deleteFriend(const QString& frienduuid);

    void createGroup(const QString& name, const QString& description = "",
        const QString& avatarurl = "", const QJsonArray& memberuuids = QJsonArray(),
        int maxmembers = 500, bool ispublic = true);
    void updateGroup(const QString& groupuuid, const QString& name = "",
        const QString& description = "", const QString& avatarurl = "",
        int maxmembers = 0, bool ispublic = true);
    void dissolveGroup(const QString& groupuuid);
    void addGroupMembers(const QString& groupuuid, const QJsonArray& useruuids);
    void removeGroupMember(const QString& groupuuid, const QString& useruuid);
    void setMemberRole(const QString& groupuuid, const QString& targetuseruuid, const QString& role);
    void joinGroup(const QString& groupuuid);
    void leaveGroup(const QString& groupuuid);

    void sendPrivateMessage(const QString& touseruuid, const QString& content, const QString& messagetype = "text");
    void recallMessage(const QString& messageuuid);
    void sendGroupMessage(const QString& groupuuid, const QString& content, const QString& messagetype = "text");

signals:
    void connected();
    void disconnected();
    void errorOccurred(const QString& errorMessage);

    void friendRequestSent(const QJsonObject& result);
    void friendRequestAccepted(bool success);
    void friendRequestRejected(bool success);
    void friendRequestCancelled(bool success);
    void friendDeleted(bool success);

    void groupCreated(const QJsonObject& group);
    void groupUpdated(bool success);
    void groupDissolved(bool success);
    void groupMembersAdded(bool success);
    void groupMemberRemoved(bool success);
    void memberRoleSet(bool success);
    void groupJoined(bool success);
    void groupLeft(bool success);

    void privateMessageSent(const QJsonObject& result);
    void groupMessageSent(const QJsonObject& result);
    void newPrivateMessageReceived(const QJsonObject& message);
    void newGroupMessageReceived(const QJsonObject& message);
    void messageRecalled(bool success);

private slots:
    void onConnected();
    void onDisconnected();
    void onTextMessageReceived(const QString& message);
    void onError(QAbstractSocket::SocketError error);

private:
    explicit WebSocketClient(QObject* parent = nullptr);
    ~WebSocketClient() = default;
    WebSocketClient(const WebSocketClient&) = delete;
    WebSocketClient& operator=(const WebSocketClient&) = delete;

    void sendMessage(const QString& type, const QJsonObject& data);
    void initResponseHandlers();

    using ResponseHandler = std::function<void(const QJsonObject&)>;
    QHash<QString, ResponseHandler> m_responseHandlers;

    static WebSocketClient* s_instance;

    QWebSocket* m_webSocket;
    QString m_wsUrl;
};

#endif // WEBSOCKETCLIENT_H
