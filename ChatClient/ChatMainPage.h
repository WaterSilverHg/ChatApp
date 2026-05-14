#pragma once

#include "global.h"
#include "ui_ChatMainPage.h"
#include "HttpApiClient.h"
#include "WebSocketClient.h"
#include <QSharedPointer>

class ChatMainPage : public QWidget
{
    Q_OBJECT

public:
    ChatMainPage(const QString& username, const QString& token, const QJsonObject& userInfo, QWidget *parent = nullptr);
    ~ChatMainPage();

signals:
    void logoutToLogin();
    void logoutToRegister();
    void logoutToExit();

private slots:
    void on_conversationListWidget_itemClicked(QListWidgetItem* item);
    void on_sendButton_clicked();
    void onConversationsReceived(const QJsonArray& conversations);
    void onPrivateMessageSent(const QJsonObject& message);
    void onGroupMessageSent(const QJsonObject& message);
    void onPrivateMessagesReceived(const QString& convUuid, const QJsonArray& messages);
    void onGroupMessagesReceived(const QString& convUuid, const QJsonArray& messages);
    void onError(const QString& errorMessage, int errorCode);

    void on_searchButton_clicked();
    void on_profileButton_clicked();
    void on_friendRequestsButton_clicked();
    void on_settingsButton_clicked();
    void on_showInfoButton_clicked();

    void onReceivedRequestsReceived(const QJsonArray& requests);
    void onReceivedGroupRequestsReceived(const QJsonArray& requests);
    void onUserDetailReceived(const QJsonObject& user);
    void onGroupDetailReceived(const QJsonObject& group);
    void onUsersSearched(const QJsonArray& users);
    void onGroupsSearched(const QJsonArray& groups);

    void onNewPrivateMessageReceived(const QJsonObject& message);
    void onNewGroupMessageReceived(const QJsonObject& message);
    void onFileUploaded(const QJsonObject& fileInfo);

protected:
    void closeEvent(QCloseEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void setupConversationList();
    void appendMessage(const QString& username, const QString& userUuid, const QString& message, bool isSelf, const QString& timeStr = "");
    QString getCurrentTime();
    void updateUserInfo();
    void showFriendRequestsDialog(const QJsonArray& requests);
    void showGroupRequestsDialog(const QJsonArray& requests);
    void showRequestsDialog();
    void showSearchDialog();
    void loadConversationMessages(const QString& convUuid, const QString& convType);
    void displayMessages(const QJsonArray& messages, const QString& convType);
    bool isConversationLoaded(const QString& convUuid) const;
    void markConversationLoaded(const QString& convUuid);
    void updateConversationUnreadCount(const QString& convUuid, int delta);
    void updateConversationItemDisplay(int row);
    void markCurrentConversationAsRead();
    QPixmap downloadAvatar(const QString& avatarUrl);
    void saveMessagesToCache(const QString& convUuid, const QJsonArray& messages);
    QJsonArray loadMessagesFromCache(const QString& convUuid);
    bool hasLocalCache(const QString& convUuid);
    QString getMessageCacheDir() const;
    QString getMessageCacheFilePath(const QString& convUuid) const;
    void onChatScrollRangeChanged(int min, int max);
    void onPrivateMessagesBeforeReceived(const QString& convUuid, const QJsonArray& messages);
    void onGroupMessagesBeforeReceived(const QString& convUuid, const QJsonArray& messages);
    void onPrivateMessagesPageReceived(const QString& convUuid, const QJsonArray& messages);
    void onGroupMessagesPageReceived(const QString& convUuid, const QJsonArray& messages);

private:
    Ui::ChatMainPageClass ui;
    QString m_currentUser;
    QString m_currentConvName;
    QString m_currentConvUuid;
    QString m_authToken;
    QJsonObject m_userInfo;
    QList<QJsonObject> m_conversationsList;
    QMap<QString, QJsonArray> m_messagesCache;
    QSet<QString> m_loadedConversations;
    QMap<QString, int> m_unreadCounts;
    HttpApiClient* m_httpClient;
    WebSocketClient* m_wsClient;
    
    QListWidget* m_searchUserResultList = nullptr;
    QListWidget* m_searchGroupResultList = nullptr;
    bool m_isLoadingOlderMessages = false;
    bool m_hasMoreMessages = true;
    QSharedPointer<QJsonArray> m_pendingFriendRequests;
    QSharedPointer<QJsonArray> m_pendingGroupRequests;
    QDialog* m_requestsDialog = nullptr;
};
