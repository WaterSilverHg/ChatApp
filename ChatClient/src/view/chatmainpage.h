#pragma once

#include "../global.h"
#include "ui_ChatMainPage.h"
#include "../model/HttpApiClient.h"
#include "../model/WebSocketClient.h"
#include "../model/AsyncAvatarLoader.h"
#include "../model/MessageCacheManager.h"
#include "RequestDialog.h"
#include "CreateGroupDialog.h"

class ChatMainPage : public QWidget
{
    Q_OBJECT

public:
    ChatMainPage(const QString& username, const QString& token, const QJsonObject& userInfo, QWidget *parent = nullptr);
    ~ChatMainPage();
    void reinitialize(const QString& username, const QString& token, const QJsonObject& userInfo);

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
    void on_createGroupButton_clicked();
    void on_profileButton_clicked();
    void on_friendRequestsButton_clicked();
    void on_settingsButton_clicked();
    void on_showInfoButton_clicked();

    void onReceivedFriendRequestsReceived(const QJsonArray& requests);
    void onReceivedGroupRequestsReceived(const QJsonArray& requests);
    void onFriendDetailReceived(const QJsonObject& friendDetail, const QVariant& context = QVariant());
    void onUserDetailReceived(const QJsonObject& user, const QVariant& context = QVariant());
    void onGroupDetailReceived(const QJsonObject& group, const QVariant& context = QVariant());
    void onUsersSearched(const QJsonArray& users);
    void onGroupsSearched(const QJsonArray& groups);

    void onNewPrivateMessageReceived(const QJsonObject& message);
    void onNewGroupMessageReceived(const QJsonObject& message);
    void onFileUploaded(const QJsonObject& fileInfo);
    void onFriendsListReceived(const QJsonArray& friends);
    void onBlockedUsersReceived(const QJsonArray& users);
    void onMyGroupsListReceived(const QJsonArray& groups);
    void onAvatarLoaded(const QString& cacheKey, QPixmap pixmap);
    void onCacheLoadFinished(const QString& convUuid, QJsonArray messages);
    void onMessageRecalledByOther(const QString& messageUuid);
    void onChatDisplayContextMenuRequested(const QPoint& pos);

protected:
    void closeEvent(QCloseEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void setupConversationList();
    //void appendMessage(const QString& username, const QString& userUuid, const QString& message, bool isSelf, const QString& timeStr = "");
    void appendMessage(const QString& username,
                       const QString& userUuid,
                       const QString& msguuid,
                       const QString& message,
                       bool isSelf,
                       const QString& timeStr,
                       const QString& messageType);
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
    QPixmap getOrPlaceholderAvatar(const QString& avatarUrl, const QString& name, const QSize& size);
    void loadAvatarAsync(QLabel* label, const QString& url, const QString& name, const QSize& size);


    // ---- 用户名缓存（UUID → 用户名，3 分钟 TTL）----
    struct UserNameEntry {
        QString name;
        qint64 fetchedAt = 0;
    };
    QHash<QString, UserNameEntry> m_userNameCache;
    QSet<QString> m_pendingNameLookups;  // 防止并发重复请求
    QString resolveUsername(const QString& userUuid);

    // ---- 备注缓存（UUID → 备注名）----
    QHash<QString, QString> m_remarkCache;
    QString resolveDisplayName(const QString& userUuid, const QString& originalName);
    void fetchAndCacheUsername(const QString& uuid);

    // ---- 消息免打扰设置（UUID → 是否免打扰）----
    QSet<QString> m_mutedConversations;
    bool isMuted(const QString& convUuid) const;
    void setMuted(const QString& convUuid, bool muted);

    // ---- 消息缓存（已迁移到 MessageCacheManager，以下方法保留兼容）----
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
    QSet<QString> m_verifiedConversations;
    QMap<QString, int> m_unreadCounts;
    HttpApiClient* m_httpClient;
    WebSocketClient* m_wsClient;
    AsyncAvatarLoader* m_avatarLoader;
    MessageCacheManager* m_cacheManager;
    
    QListWidget* m_searchUserResultList = nullptr;
    QListWidget* m_searchGroupResultList = nullptr;
    bool m_isLoadingOlderMessages = false;
    QSharedPointer<QJsonArray> m_pendingFriendRequests;
    QSharedPointer<QJsonArray> m_pendingGroupRequests;
    RequestDialog* m_requestsDialog = nullptr;
    int m_pendingRequestResponses = 0;

    // ---- 查看信息请求上下文 ----
    struct InfoRequestContext {
        QString targetId;
        bool isFriend = false;
        bool isMuted = false;
        QString requestId;
    };
    QHash<QString, InfoRequestContext> m_infoRequestContexts;

    // ---- 左侧边栏：聊天 / 好友 / 群聊 / 拉黑 ----
    enum SidebarTab { TabChats, TabFriends, TabGroups, TabBlocked };
    SidebarTab m_sidebarTab = TabChats;
    QJsonArray m_friendsData;
    QJsonArray m_groupsData;
    QJsonArray m_blockedData;

    void setupSidebar();
    void switchSidebarTab(SidebarTab tab);
    void populateConversationsTab();
    void populateFriendsTab();
    void populateGroupsTab();
    void populateBlockedTab();
    void startChatWith(const QString& targetUuid, const QString& displayName, const QString& convType);
    void showGroupManagementDialog();
};
