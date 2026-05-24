#include "chatmainpage.h"
#include "../model/UserSession.h"
#include "LoginPage.h"
#include "CustomDialog.h"
#include "SearchDialog.h"
#include "RequestDialog.h"
#include "InfoDialog.h"

ChatMainPage::ChatMainPage(const QString& username, const QString& token, const QJsonObject& userInfo, QWidget *parent)
    : QWidget(parent)
    , m_currentUser(username)
    , m_currentConvName("")
    , m_currentConvUuid("")
    , m_authToken(token)
    , m_userInfo(userInfo)
    , m_httpClient(HttpApiClient::instance())
    , m_wsClient(WebSocketClient::instance())
    , m_avatarLoader(new AsyncAvatarLoader(this))
    , m_cacheManager(new MessageCacheManager(this))
{
    ui.setupUi(this);
    setWindowTitle("聊天系统 - " + username);
    setWindowIcon(QIcon(":/chat.svg"));
//    setStyleSheet(StyleConst::DIALOG_STYLE);

//    ui.chatDisplayWidget->setOpenLinks(false);

//    ui.conversationListWidget->setStyleSheet(StyleConst::LIST_STYLE);
//    ui.sendButton->setStyleSheet(StyleConst::BUTTON_STYLE);
//    ui.searchButton->setStyleSheet(StyleConst::BUTTON_STYLE);
//    ui.profileButton->setStyleSheet(StyleConst::BUTTON_STYLE);
//    ui.friendRequestsButton->setStyleSheet(StyleConst::BUTTON_STYLE);
//    ui.settingsButton->setStyleSheet(StyleConst::BUTTON_STYLE);
//    ui.showInfoButton->setStyleSheet(StyleConst::BUTTON_STYLE);
//    ui.messageEdit->setStyleSheet(StyleConst::LINEEDIT_STYLE);

    m_httpClient->setServerUrl(HTTP_BASE_URL);
    m_httpClient->setAuthToken(token);
    m_wsClient->setWsUrl(WEBSOCKET_URL);

    // ---- 异步头像加载 ----
    connect(m_avatarLoader, &AsyncAvatarLoader::avatarReady,
            this, &ChatMainPage::onAvatarLoaded);

    // ---- 后台消息缓存加载 ----
    connect(m_cacheManager, &MessageCacheManager::loadFinished,
            this, &ChatMainPage::onCacheLoadFinished);

    connect(m_httpClient, &HttpApiClient::conversationsReceived, this, &ChatMainPage::onConversationsReceived, Qt::UniqueConnection);
    connect(m_httpClient, &HttpApiClient::privateMessagesReceived, this, &ChatMainPage::onPrivateMessagesReceived, Qt::UniqueConnection);
    connect(m_httpClient, &HttpApiClient::privateMessagesBeforeReceived, this, &ChatMainPage::onPrivateMessagesBeforeReceived, Qt::UniqueConnection);
    connect(m_httpClient, &HttpApiClient::privateMessagesPageReceived, this, &ChatMainPage::onPrivateMessagesPageReceived, Qt::UniqueConnection);
    connect(m_httpClient, &HttpApiClient::groupMessagesReceived, this, &ChatMainPage::onGroupMessagesReceived, Qt::UniqueConnection);
    connect(m_httpClient, &HttpApiClient::groupMessagesBeforeReceived, this, &ChatMainPage::onGroupMessagesBeforeReceived, Qt::UniqueConnection);
    connect(m_httpClient, &HttpApiClient::groupMessagesPageReceived, this, &ChatMainPage::onGroupMessagesPageReceived, Qt::UniqueConnection);
    connect(m_httpClient, &HttpApiClient::errorOccurred, this, &ChatMainPage::onError, Qt::UniqueConnection);
    connect(m_httpClient, &HttpApiClient::receivedRequestsReceived, this, &ChatMainPage::onReceivedRequestsReceived, Qt::UniqueConnection);
    connect(m_httpClient, &HttpApiClient::receivedGroupRequestsReceived, this, &ChatMainPage::onReceivedGroupRequestsReceived, Qt::UniqueConnection);
    connect(m_httpClient, &HttpApiClient::friendDetailReceived, this, &ChatMainPage::onFriendDetailReceived, Qt::UniqueConnection);
    connect(m_httpClient, &HttpApiClient::userInfoReceived, this, &ChatMainPage::onUserDetailReceived, Qt::UniqueConnection);
    connect(m_httpClient, &HttpApiClient::groupDetailReceived, this, &ChatMainPage::onGroupDetailReceived, Qt::UniqueConnection);
    connect(m_httpClient, &HttpApiClient::usersSearched, this, &ChatMainPage::onUsersSearched, Qt::UniqueConnection);
    connect(m_httpClient, &HttpApiClient::groupsSearched, this, &ChatMainPage::onGroupsSearched, Qt::UniqueConnection);
    connect(m_httpClient, &HttpApiClient::fileUploaded, this, &ChatMainPage::onFileUploaded, Qt::UniqueConnection);
    connect(m_httpClient, &HttpApiClient::friendsListReceived, this, &ChatMainPage::onFriendsListReceived, Qt::UniqueConnection);
    connect(m_httpClient, &HttpApiClient::myGroupsReceived, this, &ChatMainPage::onMyGroupsListReceived, Qt::UniqueConnection);
    connect(m_httpClient, &HttpApiClient::blockedUsersReceived, this, &ChatMainPage::onBlockedUsersReceived, Qt::UniqueConnection);
    connect(ui.chatDisplayWidget->verticalScrollBar(), &QScrollBar::rangeChanged, this, &ChatMainPage::onChatScrollRangeChanged);
    connect(ui.chatDisplayWidget, &QTextBrowser::anchorClicked, this, [](const QUrl& url) {
        if (url.scheme() == "user") {
            QString userUuid = url.authority();
            if (!userUuid.isEmpty()) {
                HttpApiClient::instance()->getUserInfo(userUuid);
            }
        }
    });
    connect(m_wsClient, &WebSocketClient::privateMessageSent, this, &ChatMainPage::onPrivateMessageSent, Qt::UniqueConnection);
    connect(m_wsClient, &WebSocketClient::groupMessageSent, this, &ChatMainPage::onGroupMessageSent, Qt::UniqueConnection);
    connect(m_wsClient, &WebSocketClient::newPrivateMessageReceived, this, &ChatMainPage::onNewPrivateMessageReceived, Qt::UniqueConnection);
    connect(m_wsClient, &WebSocketClient::newGroupMessageReceived, this, &ChatMainPage::onNewGroupMessageReceived, Qt::UniqueConnection);
    connect(m_wsClient, &WebSocketClient::friendRequestHandled, this, [this](bool success) {
        if (success) {
            CustomDialog::information(this, "成功", "好友请求已同意");
            m_httpClient->getConversations();  // 刷新会话列表（新好友的会话）
            m_httpClient->getFriends();        // 刷新好友列表
        } else {
            CustomDialog::warning(this, "失败", "处理好友请求失败");
        }
    });
    connect(m_wsClient, &WebSocketClient::errorOccurred, this, [this](const QString& msg) {
        CustomDialog::warning(this, "错误", msg);
    });
    connect(m_wsClient, &WebSocketClient::friendRequestSent, this, [this](const QJsonObject& result) {
        QString status = result["status"].toString();
        if (status == "already_pending") {
            CustomDialog::information(this, "提示", "好友请求已发送过，请等待对方处理");
        } else {
            CustomDialog::information(this, "成功", "好友请求已发送");
        }
    });
    connect(m_wsClient, &WebSocketClient::groupRequestHandled, this, [this](bool success) {
        if (success) {
            CustomDialog::information(this, "成功", "群聊请求已处理");
        } else {
            CustomDialog::warning(this, "失败", "处理群聊请求失败");
        }
    });
    connect(m_wsClient, &WebSocketClient::kickedByServer, this, [this]() {
        CustomDialog::warning(this, "下线通知", "您的账号已在另一个设备登录，您已被踢出。");
        UserSession::instance()->clear();
        LoginPage* loginPage = new LoginPage();  // Qt 父对象：由事件循环管理，无需手动 delete
        loginPage->setAttribute(Qt::WA_DeleteOnClose);
        loginPage->show();
        this->close();
    });

    updateUserInfo();
    setupConversationList();

    ui.messageEdit->installEventFilter(this);
}

ChatMainPage::~ChatMainPage()
{}

void ChatMainPage::reinitialize(const QString& username, const QString& token, const QJsonObject& userInfo)
{
    m_currentUser = username;
    m_authToken = token;
    m_userInfo = userInfo;
    m_currentConvName = "";
    m_currentConvUuid = "";

    m_conversationsList.clear();
    m_messagesCache.clear();
    m_loadedConversations.clear();
    m_unreadCounts.clear();
    m_friendsData = QJsonArray();
    m_groupsData = QJsonArray();
    m_blockedData = QJsonArray();
    m_sidebarTab = TabChats;

    m_httpClient->setAuthToken(token);
    m_wsClient->setWsUrl(WEBSOCKET_URL);

    ui.conversationListWidget->clear();
    ui.chatDisplayWidget->clear();
    ui.messageEdit->clear();

    setWindowTitle("聊天系统 - " + username);

    setupConversationList();
}

void ChatMainPage::updateUserInfo()
{
}

void ChatMainPage::setupConversationList()
{
    setupSidebar();
    m_httpClient->getConversations();
    m_httpClient->getFriends();
    m_httpClient->getMyGroups();
}

// ============================================================
// 左侧边栏：聊天 / 好友 / 群聊 / 拉黑
// ============================================================

void ChatMainPage::setupSidebar()
{
    connect(ui.tabChatsButton,   &QPushButton::clicked, [this]() { switchSidebarTab(TabChats); });
    connect(ui.tabFriendsButton, &QPushButton::clicked, [this]() { switchSidebarTab(TabFriends); });
    connect(ui.tabGroupsButton,  &QPushButton::clicked, [this]() { switchSidebarTab(TabGroups); });
    connect(ui.tabBlockedButton, &QPushButton::clicked, [this]() {
        m_httpClient->getBlockedUsers();
        switchSidebarTab(TabBlocked);
    });
}

void ChatMainPage::switchSidebarTab(SidebarTab tab)
{
    m_sidebarTab = tab;
    ui.tabChatsButton->setChecked(tab == TabChats);
    ui.tabFriendsButton->setChecked(tab == TabFriends);
    ui.tabGroupsButton->setChecked(tab == TabGroups);
    ui.tabBlockedButton->setChecked(tab == TabBlocked);

    switch (tab) {
    case TabChats:    populateConversationsTab(); break;
    case TabFriends:  populateFriendsTab(); break;
    case TabGroups:   populateGroupsTab(); break;
    case TabBlocked:  populateBlockedTab(); break;
    }
}

void ChatMainPage::populateConversationsTab()
{
    ui.conversationListWidget->clear();
    for (int i = 0; i < m_conversationsList.size(); ++i) {
        QJsonObject conv = m_conversationsList.at(i);
        QString targetName = conv["targetname"].toString();
        QString lastMsg = conv["lastmessage"].toString();
        QString convUuid = conv["targetuuid"].toString();
        int unread = m_unreadCounts.value(convUuid, 0);

        QListWidgetItem* item = new QListWidgetItem();
        item->setData(Qt::UserRole, convUuid);
        ui.conversationListWidget->addItem(item);

        QWidget* w = new QWidget();
        QVBoxLayout* mainLayout = new QVBoxLayout(w);
        mainLayout->setContentsMargins(8, 6, 8, 6);
        mainLayout->setSpacing(2);

        QHBoxLayout* topRow = new QHBoxLayout();
        topRow->setSpacing(6);

        QLabel* nameLabel = new QLabel(targetName);
        nameLabel->setStyleSheet("font-weight: bold; color: #333; font-size: 13px;");
        topRow->addWidget(nameLabel);
        topRow->addStretch();

        if (unread > 0) {
            QLabel* badge = new QLabel(QString::number(unread));
            badge->setAlignment(Qt::AlignCenter);
            badge->setFixedSize(20, 20);
            badge->setStyleSheet(
                "background:#E57373; color:white; border-radius:10px; "
                "min-width:20px; min-height:20px; font-size:11px; font-weight:bold;");
            topRow->addWidget(badge);
        }

        mainLayout->addLayout(topRow);

        QLabel* msgLabel = new QLabel(lastMsg.isEmpty() ? "" : lastMsg);
        msgLabel->setStyleSheet("color: #888; font-size: 12px;");
        msgLabel->setTextFormat(Qt::PlainText);
        QFontMetrics fm(msgLabel->font());
        msgLabel->setText(fm.elidedText(lastMsg, Qt::ElideRight, 200));
        mainLayout->addWidget(msgLabel);

        item->setSizeHint(w->sizeHint());
        ui.conversationListWidget->setItemWidget(item, w);
    }
}

void ChatMainPage::populateFriendsTab()
{
    ui.conversationListWidget->clear();

    // 管理分组按钮
    QListWidgetItem* mgmtItem = new QListWidgetItem();
    QWidget* mgmtWidget = new QWidget();
    QHBoxLayout* mgmtLayout = new QHBoxLayout(mgmtWidget);
    mgmtLayout->setContentsMargins(8, 4, 8, 4);
    QPushButton* addGroupBtn = new QPushButton("+ 管理分组");
    addGroupBtn->setStyleSheet("QPushButton { background:#A8D8B9; color:#333; border:none; border-radius:4px; padding:6px 12px; font-size:12px; } QPushButton:hover { background:#7CB894; color:white; }");
    connect(addGroupBtn, &QPushButton::clicked, [this]() { showGroupManagementDialog(); });
    mgmtLayout->addWidget(addGroupBtn);
    mgmtLayout->addStretch();
    mgmtItem->setSizeHint(mgmtWidget->sizeHint());
    ui.conversationListWidget->addItem(mgmtItem);
    ui.conversationListWidget->setItemWidget(mgmtItem, mgmtWidget);

    // 按 groupName 分组
    QMap<QString, QJsonArray> groups;
    for (const auto& f : m_friendsData) {
        QJsonObject obj = f.toObject();
        QString g = obj["groupname"].toString();
        if (g.isEmpty()) g = "默认分组";
        groups[g].append(obj);
    }
    for (auto it = groups.begin(); it != groups.end(); ++it) {
        // 分组标题
        QListWidgetItem* header = new QListWidgetItem(it.key());
        header->setFlags(Qt::NoItemFlags);
        header->setForeground(QColor("#5EA07A"));
        QFont f = header->font(); f.setBold(true); header->setFont(f);
        ui.conversationListWidget->addItem(header);
        // 好友项
        for (const auto& fv : it.value()) {
            QJsonObject fobj = fv.toObject();
            QString name = fobj["username"].toString();
            QString uuid = fobj["frienduuid"].toString();
            QString status = fobj["status"].toString();
            QString groupName = it.key();
            QString remark = fobj["remark"].toString();
            m_remarkCache[uuid] = remark;
            QString displayName = remark.isEmpty() ? name : QString("%1(%2)").arg(remark).arg(name);
            QString text = displayName + (status == "online" ? " [在线]" : "");
            QListWidgetItem* item = new QListWidgetItem(text);
            item->setData(Qt::UserRole, uuid);
            item->setData(Qt::UserRole + 1, displayName);
            item->setData(Qt::UserRole + 2, "private");
            item->setData(Qt::UserRole + 3, groupName);
            ui.conversationListWidget->addItem(item);
        }
    }
    if (m_friendsData.isEmpty()) {
        ui.conversationListWidget->addItem(new QListWidgetItem("暂无好友"));
    }
}

void ChatMainPage::populateGroupsTab()
{
    ui.conversationListWidget->clear();
    for (const auto& g : m_groupsData) {
        QJsonObject obj = g.toObject();
        QString name = obj["name"].toString();
        QString uuid = obj["uuid"].toString();
        QListWidgetItem* item = new QListWidgetItem("[群] " + name);
        item->setData(Qt::UserRole, uuid);
        item->setData(Qt::UserRole + 1, name);
        item->setData(Qt::UserRole + 2, "group");
        ui.conversationListWidget->addItem(item);
    }
    if (m_groupsData.isEmpty()) {
        ui.conversationListWidget->addItem(new QListWidgetItem("暂无群聊"));
    }
}

void ChatMainPage::populateBlockedTab()
{
    ui.conversationListWidget->clear();
    for (const auto& b : m_blockedData) {
        QJsonObject obj = b.toObject();
        QString name = obj["username"].toString();
        QString uuid = obj["frienduuid"].toString();
        QListWidgetItem* item = new QListWidgetItem("[拉黑] " + name);
        item->setData(Qt::UserRole, uuid);
        item->setData(Qt::UserRole + 1, name);
        // 右键或按钮解除拉黑：添加一个小按钮
        QWidget* w = new QWidget();
        QHBoxLayout* l = new QHBoxLayout(w);
        l->setContentsMargins(0, 0, 0, 0);
        l->addWidget(new QLabel("[拉黑] " + name));
        QPushButton* unblockBtn = new QPushButton("解除");
        unblockBtn->setFixedSize(48, 22);
        unblockBtn->setStyleSheet("background:#E88B8B;color:white;border:none;border-radius:3px;");
        connect(unblockBtn, &QPushButton::clicked, [this, uuid]() {
            m_httpClient->unblockUser(uuid);
            CustomDialog::information(this, "提示", "已解除拉黑");
            m_httpClient->getBlockedUsers(); // 刷新
        });
        l->addWidget(unblockBtn);
        l->addStretch();
        item->setSizeHint(w->sizeHint());
        ui.conversationListWidget->addItem(item);
        ui.conversationListWidget->setItemWidget(item, w);
    }
    if (m_blockedData.isEmpty()) {
        ui.conversationListWidget->addItem(new QListWidgetItem("暂无拉黑用户"));
    }
}

void ChatMainPage::startChatWith(const QString& targetUuid, const QString& displayName, const QString& convType)
{
    // 检查是否已有该会话
    for (int i = 0; i < m_conversationsList.size(); ++i) {
        if (m_conversationsList.at(i)["targetuuid"].toString() == targetUuid) {
            // 已有会话 → 切换到聊天 tab 并选中
            switchSidebarTab(TabChats);
            ui.conversationListWidget->setCurrentRow(i);
            on_conversationListWidget_itemClicked(nullptr);
            return;
        }
    }
    // 没有会话 → 创建虚拟条目、切换到聊天 tab
    QJsonObject newConv;
    newConv["targetuuid"] = targetUuid;
    newConv["targetname"] = displayName;
    newConv["type"] = convType;
    newConv["lastmessage"] = "";
    m_conversationsList.prepend(newConv);
    m_unreadCounts.insert(targetUuid, 0);
    switchSidebarTab(TabChats);
    ui.conversationListWidget->clear();
    populateConversationsTab();
    ui.conversationListWidget->setCurrentRow(0);
    m_currentConvUuid = targetUuid;
    m_currentConvName = displayName;
    ui.chatTitleLabel->setText(displayName);
    ui.chatDisplayWidget->clear();
    loadConversationMessages(targetUuid, convType);
}

// ---- 数据回调 ----

void ChatMainPage::onFriendsListReceived(const QJsonArray& friends)
{
    m_friendsData = friends;
    if (m_sidebarTab == TabFriends) populateFriendsTab();
}

void ChatMainPage::onMyGroupsListReceived(const QJsonArray& groups)
{
    m_groupsData = groups;
    if (m_sidebarTab == TabGroups) populateGroupsTab();
}

void ChatMainPage::onBlockedUsersReceived(const QJsonArray& users)
{
    m_blockedData = users;
    if (m_sidebarTab == TabBlocked) populateBlockedTab();
}

// ============================================================

void ChatMainPage::onConversationsReceived(const QJsonArray& conversations)
{
    m_conversationsList.clear();
    m_unreadCounts.clear();
    m_mutedConversations.clear();

    for (const QJsonValue& convValue : conversations) {
        QJsonObject convObj = convValue.toObject();
        m_conversationsList.append(convObj);

        QString targetName = convObj["targetname"].toString();
        QString convUuid = convObj["targetuuid"].toString();
        int unreadCount = convObj["unreadcount"].toInt();
        bool isMuted = convObj["ismuted"].toBool();
        m_unreadCounts.insert(convUuid, unreadCount);
        if (isMuted) {
            m_mutedConversations.insert(convUuid);
        }

        if (!convUuid.isEmpty() && !targetName.isEmpty()) {
            m_userNameCache[convUuid] = {targetName, QDateTime::currentMSecsSinceEpoch()};
        }
    }

    if (m_sidebarTab == TabChats) {
        populateConversationsTab();
    }
}

void ChatMainPage::on_conversationListWidget_itemClicked(QListWidgetItem* item)
{
    // 好友/群聊/拉黑 tab → 开始聊天
    if (m_sidebarTab != TabChats) {
        QString targetUuid = item->data(Qt::UserRole).toString();
        QString displayName = item->data(Qt::UserRole + 1).toString();
        QString convType = item->data(Qt::UserRole + 2).toString();
        if (!targetUuid.isEmpty()) {
            startChatWith(targetUuid, displayName, convType);
        }
        return;
    }

    // 聊天 tab → 原有逻辑
    int row = ui.conversationListWidget->currentRow();
    if (row < 0 || row >= m_conversationsList.size()) return;

    QJsonObject convObj = m_conversationsList.at(row);
    QString convUuid = convObj["targetuuid"].toString();
    QString convType = convObj["type"].toString();

    if (m_currentConvUuid == convUuid) return;

    m_currentConvUuid = convUuid;
    m_currentConvName = convObj["targetname"].toString();

    ui.chatTitleLabel->setText(m_currentConvName);
    ui.chatDisplayWidget->clear();
    ui.messageEdit->setFocus();
    markCurrentConversationAsRead();

    if (isConversationLoaded(convUuid)) {
        displayMessages(m_messagesCache.value(convUuid), convType);
    } else {
        loadConversationMessages(convUuid, convType);
    }
}

bool ChatMainPage::isConversationLoaded(const QString& convUuid) const
{
    return m_loadedConversations.contains(convUuid);
}

void ChatMainPage::markConversationLoaded(const QString& convUuid)
{
    m_loadedConversations.insert(convUuid);
}

void ChatMainPage::loadConversationMessages(const QString& convUuid, const QString& convType)
{
    // 磁盘缓存：先快速展示本地数据，再异步拉服务端保证最新
    if (m_cacheManager->exists(convUuid)) {
        m_cacheManager->loadAsync(convUuid);
        // 结果异步到达 onCacheLoadFinished 后继续拉服务端
        return;
    }

    // 无本地缓存，直接拉服务端
    if (convType == "group") {
        m_httpClient->getGroupMessagesPage(convUuid, 1, MESSAGE_PAGE_SIZE);
    } else {
        m_httpClient->getPrivateMessagesPage(convUuid, 1, MESSAGE_PAGE_SIZE);
    }
}

void ChatMainPage::displayMessages(const QJsonArray& messages, const QString& convType)
{
    for (const QJsonValue& messageValue : messages) {
        QJsonObject messageObj = messageValue.toObject();
        QString fromUserUuid = messageObj["fromuseruuid"].toString();
        QString username = resolveUsername(fromUserUuid);
        QString content = messageObj["content"].toString();
        QString createdAt = messageObj["createdat"].toString();
        bool isSelf = (fromUserUuid == m_userInfo["useruuid"].toString());

        appendMessage(username, fromUserUuid, content, isSelf, createdAt);
    }
}

void ChatMainPage::on_searchButton_clicked()
{
    showSearchDialog();
}

void ChatMainPage::showSearchDialog()
{
    SearchDialog dlg(this);
    dlg.exec();
}

void ChatMainPage::on_profileButton_clicked()
{
    QString selfUuid = m_userInfo["useruuid"].toString();
    m_httpClient->getUserInfo(selfUuid);
}

void ChatMainPage::onFileUploaded(const QJsonObject& fileInfo)
{
    QString fileUrl = fileInfo["fileurl"].toString();
    QString fileUuid = fileInfo["uuid"].toString();

    if (!fileUrl.isEmpty()) {
        m_userInfo["avatarurl"] = fileUrl;
        UserSession::instance()->setAvatarUrl(fileUrl);
        CustomDialog::information(this, "成功", QString("头像上传成功!\n文件UUID: %1").arg(fileUuid));
    } else {
        CustomDialog::warning(this, "错误", "头像上传失败");
    }
}

void ChatMainPage::on_friendRequestsButton_clicked()
{
    m_pendingFriendRequests.reset(new QJsonArray());
    m_pendingGroupRequests.reset(new QJsonArray());
    m_pendingRequestResponses = 0;
    m_httpClient->getReceivedRequests();
    m_httpClient->getReceivedGroupRequests();
    // 对话框等两个 HTTP 响应都到达后再显示（见 onReceivedRequestsReceived / onReceivedGroupRequestsReceived）
}

void ChatMainPage::onReceivedRequestsReceived(const QJsonArray& requests)
{
    m_pendingFriendRequests.reset(new QJsonArray(requests));
    m_pendingRequestResponses++;
    if (m_pendingRequestResponses >= 2) {
        showRequestsDialog();
    }
}

void ChatMainPage::onReceivedGroupRequestsReceived(const QJsonArray& requests)
{
    m_pendingGroupRequests.reset(new QJsonArray(requests));
    m_pendingRequestResponses++;
    if (m_pendingRequestResponses >= 2) {
        showRequestsDialog();
    }
}

void ChatMainPage::showFriendRequestsDialog(const QJsonArray& requests)
{
    QDialog* dialog = new QDialog(this);
    dialog->setWindowTitle("好友请求");
    dialog->setFixedSize(450, 350);

    QVBoxLayout* layout = new QVBoxLayout(dialog);

    if (requests.isEmpty()) {
        QLabel* emptyLabel = new QLabel("暂无好友请求", dialog);
        emptyLabel->setAlignment(Qt::AlignCenter);
        layout->addWidget(emptyLabel);
        dialog->exec();
        return;
    }

    QGroupBox* groupBox = new QGroupBox("待处理请求", dialog);
    layout->addWidget(groupBox);

    QVBoxLayout* groupLayout = new QVBoxLayout(groupBox);

    for (const QJsonValue& reqValue : requests) {
        QJsonObject req = reqValue.toObject();
        QString requestUuid = req["uuid"].toString();
        QString message = req["message"].toString();
        QString createdAt = req["createdat"].toString();
        QJsonObject requester = req["requester"].toObject();
        QString fromUserUuid = requester["uuid"].toString();
        QString fromUserName = requester["username"].toString();

        QString titleText = QString("请求来自：%1    %2")
            .arg(fromUserName.isEmpty() ? fromUserUuid : fromUserName)
            .arg(createdAt.isEmpty() ? "" : createdAt);
        QString descText = message.isEmpty() ? "" : QString("描述：%1").arg(message);

        QWidget* reqWidget = new QWidget(groupBox);
        QVBoxLayout* reqLayout = new QVBoxLayout(reqWidget);
        reqLayout->setSpacing(4);

        QLabel* titleLabel = new QLabel(titleText, reqWidget);
        titleLabel->setStyleSheet("font-weight: bold; color: #333;");
        reqLayout->addWidget(titleLabel);

        if (!descText.isEmpty()) {
            QLabel* descLabel = new QLabel(descText, reqWidget);
            descLabel->setStyleSheet("color: #666; font-size: 12px;");
            descLabel->setWordWrap(true);
            reqLayout->addWidget(descLabel);
        }

        QHBoxLayout* btnLayout = new QHBoxLayout();
        btnLayout->addStretch();

        QPushButton* acceptBtn = new QPushButton("同意", reqWidget);
        QPushButton* rejectBtn = new QPushButton("拒绝", reqWidget);
        acceptBtn->setFixedSize(60, 28);
        rejectBtn->setFixedSize(60, 28);
        acceptBtn->setStyleSheet("QPushButton { background-color: #A8D8B9; color: white; border: none; padding: 4px 8px; border-radius: 4px; } QPushButton:hover { background-color: #7CB894; }");
        rejectBtn->setStyleSheet("QPushButton { background-color: #E88B8B; color: white; border: none; padding: 4px 8px; border-radius: 4px; } QPushButton:hover { background-color: #D07070; }");

        btnLayout->addWidget(acceptBtn);
        btnLayout->addWidget(rejectBtn);
        reqLayout->addLayout(btnLayout);

        connect(acceptBtn, &QPushButton::clicked, [this, requestUuid, dialog]() {
            m_wsClient->handleFriendRequest(requestUuid, "accepted");
            dialog->close();
        });

        connect(rejectBtn, &QPushButton::clicked, [this, requestUuid, dialog]() {
            m_wsClient->handleFriendRequest(requestUuid, "rejected");
            dialog->close();
        });

        groupLayout->addWidget(reqWidget);
    }

    dialog->exec();
}

void ChatMainPage::showGroupRequestsDialog(const QJsonArray& requests)
{
    QDialog* dialog = new QDialog(this);
    dialog->setWindowTitle("群聊请求");
    dialog->setFixedSize(450, 350);

    QVBoxLayout* layout = new QVBoxLayout(dialog);

    if (requests.isEmpty()) {
        QLabel* emptyLabel = new QLabel("暂无群聊请求", dialog);
        emptyLabel->setAlignment(Qt::AlignCenter);
        layout->addWidget(emptyLabel);
        dialog->exec();
        return;
    }

    QGroupBox* groupBox = new QGroupBox("待处理入群请求", dialog);
    layout->addWidget(groupBox);

    QVBoxLayout* groupLayout = new QVBoxLayout(groupBox);

    for (const QJsonValue& reqValue : requests) {
        QJsonObject req = reqValue.toObject();
        QString requestUuid = req["uuid"].toString();
        QString message = req["message"].toString();
        QString createdAt = req["createdat"].toString();
        QString status = req["status"].toString();
        QString groupName = req["groupname"].toString();
        QString groupUuid = req["groupuuid"].toString();
        QJsonObject requester = req["requester"].toObject();
        QString requesterName = requester["username"].toString();
        QString requesterUuid = requester["uuid"].toString();

        QString displayText = QString("群聊: %1 - 请求者: %2").arg(groupName.isEmpty() ? groupUuid : groupName, 
                                                                     requesterName.isEmpty() ? requesterUuid : requesterName);
        if (!message.isEmpty()) {
            displayText += QString(" - %1").arg(message);
        }
        if (!createdAt.isEmpty()) {
            displayText += QString(" (%1)").arg(createdAt);
        }
        if (!status.isEmpty()) {
            displayText += QString(" [状态: %1]").arg(status);
        }

        QWidget* reqWidget = new QWidget(groupBox);
        QHBoxLayout* reqLayout = new QHBoxLayout(reqWidget);

        QLabel* nameLabel = new QLabel(displayText, reqWidget);
        reqLayout->addWidget(nameLabel);

        QPushButton* acceptBtn = new QPushButton("同意", reqWidget);
        QPushButton* rejectBtn = new QPushButton("拒绝", reqWidget);

        acceptBtn->setStyleSheet("background-color: #A8D8B9; color: white; border: none; padding: 4px 12px; border-radius: 4px;");
        rejectBtn->setStyleSheet("background-color: #E88B8B; color: white; border: none; padding: 4px 12px; border-radius: 4px;");

        connect(acceptBtn, &QPushButton::clicked, [this, requestUuid, dialog]() {
            m_wsClient->handleGroupRequest(requestUuid, "accept");
            dialog->close();
            CustomDialog::information(this, "成功", "入群请求已同意");
        });

        connect(rejectBtn, &QPushButton::clicked, [this, requestUuid, dialog]() {
            m_wsClient->handleGroupRequest(requestUuid, "rejected");
            dialog->close();
            CustomDialog::information(this, "成功", "入群请求已拒绝");
        });

        reqLayout->addWidget(acceptBtn);
        reqLayout->addWidget(rejectBtn);

        groupLayout->addWidget(reqWidget);
    }

    dialog->exec();
}

void ChatMainPage::showRequestsDialog()
{
    if (m_requestsDialog) return;

    RequestDialog* dlg = new RequestDialog(this);
    m_requestsDialog = dlg;

    if (m_pendingFriendRequests)
        dlg->setFriendRequests(*m_pendingFriendRequests);
    if (m_pendingGroupRequests)
        dlg->setGroupRequests(*m_pendingGroupRequests);

    connect(dlg, &QDialog::finished, this, [this]() {
        m_requestsDialog = nullptr;
    });

    dlg->exec();
}

void ChatMainPage::on_settingsButton_clicked()
{
    CustomDialog::information(this, "设置", "设置功能正在开发中...");
}

void ChatMainPage::on_showInfoButton_clicked()
{
    int row = ui.conversationListWidget->currentRow();
    if (row < 0 || row >= m_conversationsList.size()) {
        return;
    }

    QJsonObject convObj = m_conversationsList.at(row);
    QString convType = convObj["type"].toString();
    QString targetName = convObj["targetname"].toString();
    QString targetId = convObj["targetuuid"].toString();
    m_pendingInfoIsMuted = convObj["ismuted"].toBool();

    if (convType == "group") {
        m_httpClient->getGroupDetail(targetId);
    } else {
        bool isFriend = false;
        for (const auto& f : m_friendsData) {
            if (f.toObject()["frienduuid"].toString() == targetId) {
                isFriend = true;
                break;
            }
        }
        m_pendingInfoIsFriend = isFriend;
        m_pendingInfoTargetId = targetId;
        m_httpClient->getFriendDetail(targetId);
    }
}

void ChatMainPage::on_sendButton_clicked()
{
    int row = ui.conversationListWidget->currentRow();
    if (row < 0 || row >= m_conversationsList.size()) {
        CustomDialog::warning(this, "提示", "请先选择一个会话!");
        return;
    }

    QString message = ui.messageEdit->toPlainText().trimmed();
    if (message.isEmpty()) {
        return;
    }

    QJsonObject convObj = m_conversationsList.at(row);
    QString convType = convObj["type"].toString();
    QString targetId = convObj["targetuuid"].toString();

    if (convType == "group") {
        m_wsClient->sendGroupMessage(targetId, message);
    } else {
        m_wsClient->sendPrivateMessage(targetId, message);
    }

    ui.messageEdit->clear();
}

void ChatMainPage::onPrivateMessageSent(const QJsonObject& message)
{
    QString content = message["content"].toString();
    QString createdAt = message["createdat"].toString();
    QString username = m_userInfo["username"].toString();
    QString fromUserUuid = message["fromuseruuid"].toString();
    appendMessage(username, fromUserUuid, content, true, createdAt);

    if (!m_currentConvUuid.isEmpty()) {
        QJsonArray cachedMessages = m_messagesCache.value(m_currentConvUuid);
        cachedMessages.append(message);
        m_messagesCache.insert(m_currentConvUuid, cachedMessages);
        saveMessagesToCache(m_currentConvUuid, cachedMessages);
    }
}

void ChatMainPage::onGroupMessageSent(const QJsonObject& message)
{
    QString content = message["content"].toString();
    QString createdAt = message["createdat"].toString();
    QString username = m_userInfo["username"].toString();
    QString fromUserUuid = message["fromuseruuid"].toString();
    appendMessage(username, fromUserUuid, content, true, createdAt);

    if (!m_currentConvUuid.isEmpty()) {
        QJsonArray cachedMessages = m_messagesCache.value(m_currentConvUuid);
        cachedMessages.append(message);
        m_messagesCache.insert(m_currentConvUuid, cachedMessages);
        saveMessagesToCache(m_currentConvUuid, cachedMessages);
    }
}

void ChatMainPage::onPrivateMessagesReceived(const QString& convUuid, const QJsonArray& messages)
{
    m_messagesCache.insert(convUuid, messages);
    saveMessagesToCache(convUuid, messages);
    markConversationLoaded(convUuid);
    m_cacheManager->setHasMore(convUuid, !messages.isEmpty());
    
    if (convUuid == m_currentConvUuid) {
        displayMessages(messages, "private");
    }
}

void ChatMainPage::onGroupMessagesReceived(const QString& convUuid, const QJsonArray& messages)
{
    m_messagesCache.insert(convUuid, messages);
    saveMessagesToCache(convUuid, messages);
    markConversationLoaded(convUuid);
    m_cacheManager->setHasMore(convUuid, !messages.isEmpty());
    
    if (convUuid == m_currentConvUuid) {
        displayMessages(messages, "group");
    }
}

void ChatMainPage::onPrivateMessagesBeforeReceived(const QString& convUuid, const QJsonArray& messages)
{
    m_isLoadingOlderMessages = false;
    
    if (messages.isEmpty()) {
        m_cacheManager->setHasMore(convUuid, false);
        return;
    }

    m_cacheManager->setHasMore(convUuid, true);

    if (m_messagesCache.contains(convUuid)) {
        QJsonArray cachedMessages = m_messagesCache.value(convUuid);
        QJsonArray combined = messages;
        for (const QJsonValue& msg : cachedMessages) {
            combined.append(msg);
        }
        m_messagesCache.insert(convUuid, combined);
        saveMessagesToCache(convUuid, combined);
    }

    if (convUuid == m_currentConvUuid) {
        for (int i = 0; i < messages.size(); ++i) {
            QJsonObject messageObj = messages[i].toObject();
            QString fromUserUuid = messageObj["fromuseruuid"].toString();
            QString username = resolveUsername(fromUserUuid);
            QString content = messageObj["content"].toString();
            QString createdAt = messageObj["createdat"].toString();
            bool isSelf = (fromUserUuid == m_userInfo["useruuid"].toString());
            appendMessage(username, fromUserUuid, content, isSelf, createdAt);
        }
    }
}

void ChatMainPage::onGroupMessagesBeforeReceived(const QString& convUuid, const QJsonArray& messages)
{
    m_isLoadingOlderMessages = false;

    if (messages.isEmpty()) {
        m_cacheManager->setHasMore(convUuid, false);
        return;
    }

    m_cacheManager->setHasMore(convUuid, true);
    
    if (m_messagesCache.contains(convUuid)) {
        QJsonArray cachedMessages = m_messagesCache.value(convUuid);
        QJsonArray combined = messages;
        for (const QJsonValue& msg : cachedMessages) {
            combined.append(msg);
        }
        m_messagesCache.insert(convUuid, combined);
        saveMessagesToCache(convUuid, combined);
    }
    
    if (convUuid == m_currentConvUuid) {
        for (int i = 0; i < messages.size(); ++i) {
            QJsonObject messageObj = messages[i].toObject();
            QString fromUserUuid = messageObj["fromuseruuid"].toString();
            QString username = resolveUsername(fromUserUuid);
            QString content = messageObj["content"].toString();
            QString createdAt = messageObj["createdat"].toString();
            bool isSelf = (fromUserUuid == m_userInfo["useruuid"].toString());
            appendMessage(username, fromUserUuid, content, isSelf, createdAt);
        }
    }
}

void ChatMainPage::onPrivateMessagesPageReceived(const QString& convUuid, const QJsonArray& messages)
{
    if (messages.isEmpty()) {
        m_cacheManager->setHasMore(convUuid, false);
        return;
    }

    m_cacheManager->setHasMore(convUuid, true);
    
    // 合并缓存，保留已有的消息（包括WebSocket收到的新消息）
    QJsonArray mergedMessages;
    if (m_messagesCache.contains(convUuid)) {
        mergedMessages = m_messagesCache.value(convUuid);
    }
    
    // 将HTTP响应中的消息合并到已有缓存（去重）
    for (const auto& msg : messages) {
        QString msgUuid = msg.toObject()["msguuid"].toString();
        bool exists = false;
        for (const auto& cachedMsg : mergedMessages) {
            if (cachedMsg.toObject()["msguuid"].toString() == msgUuid) {
                exists = true;
                break;
            }
        }
        if (!exists) {
            mergedMessages.append(msg);
        }
    }
    
    m_messagesCache.insert(convUuid, mergedMessages);
    saveMessagesToCache(convUuid, mergedMessages);
    markConversationLoaded(convUuid);

    // 清除之前缓存加载的临时显示，用合并后的完整数据重新渲染
    if (convUuid == m_currentConvUuid) {
        ui.chatDisplayWidget->clear();
        displayMessages(mergedMessages, "private");
    }
}

void ChatMainPage::onGroupMessagesPageReceived(const QString& convUuid, const QJsonArray& messages)
{
    if (messages.isEmpty()) {
        m_cacheManager->setHasMore(convUuid, false);
        return;
    }

    m_cacheManager->setHasMore(convUuid, true);
    
    // 合并缓存，保留已有的消息（包括WebSocket收到的新消息）
    QJsonArray mergedMessages;
    if (m_messagesCache.contains(convUuid)) {
        mergedMessages = m_messagesCache.value(convUuid);
    }
    
    // 将HTTP响应中的消息合并到已有缓存（去重）
    for (const auto& msg : messages) {
        QString msgUuid = msg.toObject()["msguuid"].toString();
        bool exists = false;
        for (const auto& cachedMsg : mergedMessages) {
            if (cachedMsg.toObject()["msguuid"].toString() == msgUuid) {
                exists = true;
                break;
            }
        }
        if (!exists) {
            mergedMessages.append(msg);
        }
    }
    
    m_messagesCache.insert(convUuid, mergedMessages);
    saveMessagesToCache(convUuid, mergedMessages);
    markConversationLoaded(convUuid);

    // 清除之前缓存加载的临时显示，用合并后的完整数据重新渲染
    if (convUuid == m_currentConvUuid) {
        ui.chatDisplayWidget->clear();
        displayMessages(mergedMessages, "group");
    }
}

void ChatMainPage::showGroupManagementDialog()
{
    QDialog* dlg = new QDialog(this);
    dlg->setWindowTitle("管理好友分组");
    dlg->setFixedSize(400, 400);
    QVBoxLayout* mainLayout = new QVBoxLayout(dlg);

    // 现有分组
    QLabel* titleLabel = new QLabel("好友分组", dlg);
    titleLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #333;");
    mainLayout->addWidget(titleLabel);

    // 收集所有分组名
    QSet<QString> groupNames;
    for (const auto& f : m_friendsData) {
        QString g = f.toObject()["groupname"].toString();
        groupNames.insert(g.isEmpty() ? "默认分组" : g);
    }
    if (!groupNames.contains("默认分组")) groupNames.insert("默认分组");

    QListWidget* groupList = new QListWidget(dlg);
    for (const auto& g : groupNames) {
        QListWidgetItem* item = new QListWidgetItem(g);
        groupList->addItem(item);
    }
    groupList->setStyleSheet("QListWidget { border: 1px solid #A8D8B9; border-radius: 4px; } QListWidget::item { padding: 6px; }");
    mainLayout->addWidget(groupList);

    // 按钮
    QHBoxLayout* btnRow = new QHBoxLayout();
    QPushButton* addBtn = new QPushButton("添加分组", dlg);
    QPushButton* renameBtn = new QPushButton("重命名", dlg);
    QPushButton* deleteBtn = new QPushButton("删除分组", dlg);
    addBtn->setStyleSheet("QPushButton { background: #A8D8B9; color: #333; border: none; border-radius: 4px; padding: 6px 12px; } QPushButton:hover { background: #7CB894; color: white; }");
    renameBtn->setStyleSheet(addBtn->styleSheet());
    deleteBtn->setStyleSheet("QPushButton { background: #E88B8B; color: white; border: none; border-radius: 4px; padding: 6px 12px; } QPushButton:hover { background: #D07070; }");
    btnRow->addWidget(addBtn);
    btnRow->addWidget(renameBtn);
    btnRow->addWidget(deleteBtn);
    mainLayout->addLayout(btnRow);

    QPushButton* closeBtn = new QPushButton("关闭", dlg);
    closeBtn->setStyleSheet("QPushButton { border: 1px solid #A8D8B9; border-radius: 4px; padding: 6px 16px; color: #333; } QPushButton:hover { background: #7CB894; color: white; }");
    connect(closeBtn, &QPushButton::clicked, dlg, &QDialog::accept);
    mainLayout->addWidget(closeBtn);

    connect(addBtn, &QPushButton::clicked, [this, dlg]() {
        QString name = QInputDialog::getText(dlg, "添加分组", "分组名称:", QLineEdit::Normal, "");
        if (name.isEmpty() || name == "默认分组") return;
        m_httpClient->createFriendGroup(name);
        connect(m_httpClient, &HttpApiClient::friendGroupsReceived, dlg, [dlg, this](const QJsonArray&) {
            m_httpClient->getFriends();
            dlg->accept();
        });
    });

    connect(renameBtn, &QPushButton::clicked, [this, groupList, dlg]() {
        auto* item = groupList->currentItem();
        if (!item) { QMessageBox::warning(dlg, "提示", "请先选择分组"); return; }
        QString oldName = item->text();
        if (oldName == "默认分组") { QMessageBox::warning(dlg, "提示", "默认分组不能重命名"); return; }
        QString newName = QInputDialog::getText(dlg, "重命名分组", "新名称:", QLineEdit::Normal, oldName);
        if (newName.isEmpty() || newName == oldName) return;
        m_httpClient->renameFriendGroup(oldName, newName);
        connect(m_httpClient, &HttpApiClient::friendGroupRenamed, dlg, [dlg, this](bool) {
            m_httpClient->getFriends();
            dlg->accept();
        });
    });

    connect(deleteBtn, &QPushButton::clicked, [this, groupList, dlg]() {
        auto* item = groupList->currentItem();
        if (!item) { QMessageBox::warning(dlg, "提示", "请先选择分组"); return; }
        QString name = item->text();
        if (name == "默认分组") { QMessageBox::warning(dlg, "提示", "默认分组不能删除"); return; }
        auto reply = QMessageBox::question(dlg, "确认删除", QString("删除分组 \"%1\" 后，组内成员将移至默认分组。确定删除?").arg(name));
        if (reply != QMessageBox::Yes) return;
        m_httpClient->deleteFriendGroup(name);
        connect(m_httpClient, &HttpApiClient::friendGroupDeleted, dlg, [dlg, this](bool) {
            m_httpClient->getFriends();
            dlg->accept();
        });
    });

    dlg->exec();
}

void ChatMainPage::onChatScrollRangeChanged(int min, int max)
{
    QScrollBar* scrollBar = ui.chatDisplayWidget->verticalScrollBar();
    if (scrollBar->value() == min && min != max && !m_isLoadingOlderMessages
        && m_cacheManager->hasMore(m_currentConvUuid)) {
        QString convType;
        if (m_currentConvUuid.isEmpty()) {
            return;
        }
        
        for (const QJsonObject& conv : m_conversationsList) {
            if (conv["targetuuid"].toString() == m_currentConvUuid) {
                convType = conv["type"].toString();
                break;
            }
        }
        
        if (convType.isEmpty()) {
            return;
        }
        
        QJsonArray cachedMessages = m_messagesCache.value(m_currentConvUuid);
        if (cachedMessages.isEmpty()) {
            return;
        }
        
        QJsonObject topMessage = cachedMessages.first().toObject();
        QString topMsgUuid = topMessage["msguuid"].toString();
        
        if (topMsgUuid.isEmpty()) {
            return;
        }
        
        m_isLoadingOlderMessages = true;
        
        if (convType == "group") {
            m_httpClient->getGroupMessagesBefore(m_currentConvUuid, topMsgUuid, MESSAGE_PAGE_SIZE);
        } else {
            m_httpClient->getPrivateMessagesBefore(m_currentConvUuid, topMsgUuid, MESSAGE_PAGE_SIZE);
        }
    }
}

void ChatMainPage::onNewPrivateMessageReceived(const QJsonObject& message)
{
    QString fromUserUuid = message["fromuseruuid"].toString();
    QString currentUserUuid = m_userInfo["useruuid"].toString();

    QString convUuid = fromUserUuid;
    QString content = message["content"].toString();

    // ---- 关键修复：所有会话的消息都缓存（之前只缓存当前活跃会话）----
    if (m_messagesCache.contains(convUuid)) {
        QJsonArray cachedMessages = m_messagesCache.value(convUuid);
        cachedMessages.append(message);
        m_messagesCache.insert(convUuid, cachedMessages);
    }
    // 异步增量写入磁盘（不重写整个文件）
    m_cacheManager->appendAsync(convUuid, message);

    if (m_currentConvUuid == convUuid) {
        QString username = resolveUsername(fromUserUuid);
        QString createdAt = message["createdat"].toString();
        appendMessage(username, fromUserUuid, content, false, createdAt);
        markCurrentConversationAsRead();
    }

    for (int i = 0; i < m_conversationsList.size(); ++i) {
        QJsonObject convObj = m_conversationsList.at(i);
        if (convObj["targetuuid"].toString() == convUuid) {
            QJsonObject updatedConv = convObj;
            updatedConv["lastmessage"] = content;
            m_conversationsList.replace(i, updatedConv);
            updateConversationItemDisplay(i);
            break;
        }
    }

    if (m_currentConvUuid != convUuid) {
        updateConversationUnreadCount(convUuid, 1);
    }
}

void ChatMainPage::onNewGroupMessageReceived(const QJsonObject& message)
{
    QString groupUuid = message["groupuuid"].toString();
    QString fromUserUuid = message["fromuseruuid"].toString();
    QString username = resolveUsername(fromUserUuid);
    QString content = message["content"].toString();
    QString createdAt = message["createdat"].toString();

    // ---- 关键修复：所有群聊会话的消息都缓存 ----
    if (m_messagesCache.contains(groupUuid)) {
        QJsonArray cachedMessages = m_messagesCache.value(groupUuid);
        cachedMessages.append(message);
        m_messagesCache.insert(groupUuid, cachedMessages);
    }
    m_cacheManager->appendAsync(groupUuid, message);

    if (m_currentConvUuid == groupUuid) {
        bool isSelf = (fromUserUuid == m_userInfo["useruuid"].toString());
        appendMessage(username, fromUserUuid, content, isSelf, createdAt);
        markCurrentConversationAsRead();
    }

    for (int i = 0; i < m_conversationsList.size(); ++i) {
        QJsonObject convObj = m_conversationsList.at(i);
        if (convObj["targetuuid"].toString() == groupUuid) {
            QJsonObject updatedConv = convObj;
            updatedConv["lastmessage"] = content;
            m_conversationsList.replace(i, updatedConv);
            updateConversationItemDisplay(i);
            break;
        }
    }

    if (m_currentConvUuid != groupUuid) {
        updateConversationUnreadCount(groupUuid, 1);
    }
}

void ChatMainPage::onError(const QString& errorMessage, int errorCode)
{
    QMessageBox::warning(this, "错误", errorMessage);
}

void ChatMainPage::closeEvent(QCloseEvent *event)
{
    QDialog dlg(this);
    dlg.setWindowTitle(QString::fromUtf8("确认退出"));
    dlg.setFixedSize(280, 130);
    dlg.setWindowIcon(QIcon(":/chat.svg"));
    dlg.setStyleSheet("QDialog { background: #FFFFFF; } QLabel { color: #333; font-size: 14px; }");

    auto* layout = new QVBoxLayout(&dlg);
    layout->setSpacing(12);
    layout->setContentsMargins(20, 16, 20, 16);

    auto* label = new QLabel(QString::fromUtf8("请选择退出方式:"), &dlg);
    label->setAlignment(Qt::AlignCenter);
    layout->addWidget(label);

    auto* btnRow = new QHBoxLayout();
    btnRow->setSpacing(12);
    btnRow->addStretch();

    auto* loginBtn = new QPushButton(QString::fromUtf8("返回登录"), &dlg);
    auto* exitBtn = new QPushButton(QString::fromUtf8("直接退出"), &dlg);

    loginBtn->setStyleSheet("QPushButton { background: #A8D8B9; color: #333; border: none; border-radius: 4px; padding: 8px 16px; font-weight: bold; } QPushButton:hover { background: #7CB894; color: white; }");
    exitBtn->setStyleSheet("QPushButton { background: #E88B8B; color: white; border: none; border-radius: 4px; padding: 8px 16px; } QPushButton:hover { background: #D07070; }");

    btnRow->addWidget(loginBtn);
    btnRow->addWidget(exitBtn);
    btnRow->addStretch();
    layout->addLayout(btnRow);

    connect(loginBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
    connect(exitBtn, &QPushButton::clicked, &dlg, &QDialog::accept);

    QPushButton* clickedBtn = nullptr;
    connect(loginBtn, &QPushButton::clicked, [&clickedBtn, loginBtn]() { clickedBtn = loginBtn; });
    connect(exitBtn, &QPushButton::clicked, [&clickedBtn, exitBtn]() { clickedBtn = exitBtn; });

    if (dlg.exec() != QDialog::Accepted) {
        event->ignore();
        return;
    }

    if (m_wsClient) m_wsClient->disconnectWebSocket();
    if (m_httpClient) m_httpClient->abortAllRequests();
    UserSession::instance()->clear();
    event->accept();

    if (clickedBtn == exitBtn)
        emit logoutToExit();
    else
        emit logoutToLogin();
}

bool ChatMainPage::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == ui.messageEdit && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            if (keyEvent->modifiers() & Qt::ShiftModifier) {
                return false;
            } else {
                on_sendButton_clicked();
                return true;
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}

void ChatMainPage::appendMessage(const QString& username,
                                 const QString& userUuid,
                                 const QString& message,
                                 bool isSelf,
                                 const QString& timeStr)
{
    QString displayTime = timeStr.isEmpty()
                              ? QDateTime::currentDateTime().toString("hh:mm")
                              : timeStr;

    QString displayName = isSelf ? "我" : username;
    QString bubbleBg = isSelf ? "#E8F5ED" : "#FFFFFF";

    QString safeMessage = message.toHtmlEscaped();

    QString msgHtml;
    if (isSelf) {
        msgHtml = QString(
                      "<table width='100%%' cellpadding='0' cellspacing='0' style='margin:5px 0;'>"
                      "<tr>"
                      "<td width='*'></td>"
                      "<td style='max-width:70%%; background-color:%1; padding:8px 12px; border-radius:15px; text-align:center;'>"
                      "  <div style='font-size:12px; color:#666; margin-bottom:4px;'>"
                      "    <a href='user://%2' style='color:#5EA07A; text-decoration:none;'>%3</a>"
                      "    <span style='color:#999; font-size:10px; margin-left:8px;'>%4</span>"
                      "  </div>"
                      "  <div style='font-size:14px; color:#333; word-wrap:break-word;'>%5</div>"
                      "</td>"
                      "</tr>"
                      "</table>"
                      ).arg(bubbleBg, userUuid, displayName, displayTime, safeMessage);
    } else {
        msgHtml = QString(
                      "<table width='100%%' cellpadding='0' cellspacing='0' style='margin:5px 0;'>"
                      "<tr>"
                      "<td style='max-width:70%%; background-color:%1; padding:8px 12px; border-radius:15px; text-align:center;'>"
                      "  <div style='font-size:12px; color:#666; margin-bottom:4px;'>"
                      "    <a href='user://%2' style='color:#5EA07A; text-decoration:none;'>%3</a>"
                      "    <span style='color:#999; font-size:10px; margin-left:8px;'>%4</span>"
                      "  </div>"
                      "  <div style='font-size:14px; color:#333; word-wrap:break-word;'>%5</div>"
                      "</td>"
                      "<td width='*'></td>"
                      "</tr>"
                      "</table>"
                      ).arg(bubbleBg, userUuid, displayName, displayTime, safeMessage);
    }

    ui.chatDisplayWidget->append(msgHtml);
    auto *scrollBar = ui.chatDisplayWidget->verticalScrollBar();
    scrollBar->setValue(scrollBar->maximum());
}

void ChatMainPage::onFriendDetailReceived(const QJsonObject& friendDetail)
{
    QString username = friendDetail["username"].toString();
    QString userUuid = friendDetail["useruuid"].toString();
    bool isMuted = friendDetail["ismuted"].toBool();
    QString remark = friendDetail["remark"].toString();

    if (!userUuid.isEmpty() && !username.isEmpty()) {
        m_userNameCache[userUuid] = {username, QDateTime::currentMSecsSinceEpoch()};
        m_pendingNameLookups.remove(userUuid);
    }
    if (!userUuid.isEmpty()) {
        m_remarkCache[userUuid] = remark;
    }

    if (m_pendingInfoTargetId != userUuid) return;

    bool isFriend = m_pendingInfoIsFriend && (m_pendingInfoTargetId == userUuid);
    m_pendingInfoIsFriend = false;
    m_pendingInfoTargetId.clear();
    m_pendingInfoIsMuted = false;

    if (isFriend)
        InfoDialog::showFriendInfo(this, friendDetail, isMuted);
    else
        InfoDialog::showUserInfo(this, friendDetail, false);
}

void ChatMainPage::onUserDetailReceived(const QJsonObject& user)
{
    QString username = user["username"].toString();
    QString userUuid = user["useruuid"].toString();

    if (!userUuid.isEmpty() && !username.isEmpty()) {
        m_userNameCache[userUuid] = {username, QDateTime::currentMSecsSinceEpoch()};
        m_pendingNameLookups.remove(userUuid);
    }

    bool isCacheLookup = m_pendingNameLookups.contains(userUuid);
    if (isCacheLookup) return;

    bool isSelf = (userUuid == m_userInfo["useruuid"].toString());
    m_pendingInfoIsFriend = false;
    m_pendingInfoTargetId.clear();
    m_pendingInfoIsMuted = false;

    InfoDialog::showUserInfo(this, user, isSelf);
}

void ChatMainPage::onGroupDetailReceived(const QJsonObject& group)
{
    QString groupUuid = group["groupuuid"].toString();
    bool isMuted = false;
    for (const QJsonValue& convValue : m_conversationsList) {
        QJsonObject convObj = convValue.toObject();
        if (convObj["targetuuid"].toString() == groupUuid) {
            isMuted = convObj["ismuted"].toBool();
            break;
        }
    }
    InfoDialog::showGroupInfo(this, group, isMuted);
}

void ChatMainPage::onUsersSearched(const QJsonArray& users)
{
    if (!m_searchUserResultList) return;

    if (users.isEmpty()) {
        QListWidgetItem* emptyItem = new QListWidgetItem("搜索结果为空");
        emptyItem->setFlags(emptyItem->flags() & ~Qt::ItemIsSelectable);
        emptyItem->setForeground(Qt::gray);
        m_searchUserResultList->addItem(emptyItem);
        return;
    }

    for (const QJsonValue& val : users) {
        QJsonObject obj = val.toObject();
        QString username = obj["username"].toString();
        QString userUuid = obj["useruuid"].toString();
        if (userUuid.isEmpty()) userUuid = obj["frienduuid"].toString();
        QString email = obj["email"].toString();
        QString fs = obj["friendshipstatus"].toString();

        QJsonObject resultObj;
        resultObj["type"] = "user";
        resultObj["username"] = username;
        resultObj["uuid"] = userUuid;
        resultObj["email"] = email;
        resultObj["friendshipstatus"] = fs;

        QString tag;
        if (fs == "accepted") tag = " [已是好友]";
        else if (fs == "pending") tag = " [已发送请求]";
        else if (fs == "blocked") tag = " [已拉黑]";
        QString display = QString("[用户] %1%2").arg(username).arg(tag);
        QListWidgetItem* item = new QListWidgetItem(display);
        item->setData(Qt::UserRole, QVariant::fromValue(resultObj));
        item->setData(Qt::UserRole + 1, userUuid);   // 直接存 UUID 字符串，防 QVariant 反序列化失败
        m_searchUserResultList->addItem(item);
    }
}

void ChatMainPage::onGroupsSearched(const QJsonArray& groups)
{
    if (!m_searchGroupResultList) return;

    if (groups.isEmpty()) {
        QListWidgetItem* emptyItem = new QListWidgetItem("搜索结果为空");
        emptyItem->setFlags(emptyItem->flags() & ~Qt::ItemIsSelectable);
        emptyItem->setForeground(Qt::gray);
        m_searchGroupResultList->addItem(emptyItem);
        return;
    }

    for (const QJsonValue& val : groups) {
        QJsonObject obj = val.toObject();
        QString groupName = obj["name"].toString();
        QString groupUuid = obj["uuid"].toString();
        bool isJoined = obj["isJoined"].toBool();

        QJsonObject resultObj;
        resultObj["type"] = "group";
        resultObj["name"] = groupName;
        resultObj["uuid"] = groupUuid;
        resultObj["isJoined"] = isJoined;

        QString display = QString("[群聊] %1%2").arg(groupName).arg(isJoined ? " (已加入)" : "");
        QListWidgetItem* item = new QListWidgetItem(display);
        item->setData(Qt::UserRole, QVariant::fromValue(resultObj));
        item->setData(Qt::UserRole + 1, groupUuid);  // 直接存 UUID 字符串
        m_searchGroupResultList->addItem(item);
    }
}

QString ChatMainPage::getCurrentTime()
{
    return QDateTime::currentDateTime().toString("hh:mm:ss");
}

void ChatMainPage::updateConversationUnreadCount(const QString& convUuid, int delta)
{
    int currentCount = m_unreadCounts.value(convUuid, 0);
    int newCount = currentCount + delta;
    m_unreadCounts.insert(convUuid, newCount);

    for (int i = 0; i < m_conversationsList.size(); ++i) {
        QJsonObject convObj = m_conversationsList.at(i);
        if (convObj["targetuuid"].toString() == convUuid) {
            updateConversationItemDisplay(i);
            break;
        }
    }
}

void ChatMainPage::updateConversationItemDisplay(int row)
{
    if (m_sidebarTab != TabChats) return;
    if (row < 0 || row >= m_conversationsList.size()) return;

    QJsonObject convObj = m_conversationsList.at(row);
    QString targetName = convObj["targetname"].toString();
    QString convType = convObj["type"].toString();
    QString lastMessage = convObj["lastmessage"].toString();
    QString convUuid = convObj["targetuuid"].toString();

    int unreadCount = m_unreadCounts.value(convUuid, 0);

    QString displayText = targetName;
    if (!convType.isEmpty()) {
        displayText += " [" + convType + "]";
    }
    if (!lastMessage.isEmpty()) {
        displayText += " - " + lastMessage;
    }
    if (unreadCount > 0) {
        displayText += QString(" (%1)").arg(unreadCount);
    }

    QListWidgetItem* item = ui.conversationListWidget->item(row);
    if (!item) return;

    QWidget* w = ui.conversationListWidget->itemWidget(item);
    if (!w) return;

    // 布局: QVBoxLayout → [QHBoxLayout(name + badge), QLabel(msg)]
    QVBoxLayout* mainLayout = qobject_cast<QVBoxLayout*>(w->layout());
    if (!mainLayout || mainLayout->count() < 2) return;

    QHBoxLayout* topRow = qobject_cast<QHBoxLayout*>(mainLayout->itemAt(0)->layout());
    if (topRow && topRow->count() >= 1) {
        QLabel* nameLabel = qobject_cast<QLabel*>(topRow->itemAt(0)->widget());
        if (nameLabel) nameLabel->setText(targetName);
    }

    // 未读徽章
    QLabel* badge = nullptr;
    if (topRow && topRow->count() >= 2) {
        QWidgetItem* wi = dynamic_cast<QWidgetItem*>(topRow->itemAt(topRow->count() - 1));
        if (wi) badge = qobject_cast<QLabel*>(wi->widget());
    }
    if (unreadCount > 0) {
        if (badge) {
            badge->setText(QString::number(unreadCount));
            badge->show();
        } else if (topRow) {
            badge = new QLabel(QString::number(unreadCount));
            badge->setAlignment(Qt::AlignCenter);
            badge->setFixedSize(20, 20);
            badge->setStyleSheet(
                "background:#E57373; color:white; border-radius:10px; "
                "min-width:20px; min-height:20px; font-size:11px; font-weight:bold;");
            topRow->addWidget(badge);
        }
    } else {
        if (badge) badge->hide();
    }

    QLabel* msgLabel = qobject_cast<QLabel*>(mainLayout->itemAt(1)->widget());
    if (msgLabel) {
        QFontMetrics fm(msgLabel->font());
        msgLabel->setText(fm.elidedText(lastMessage.isEmpty() ? "" : lastMessage, Qt::ElideRight, 200));
    }
}

void ChatMainPage::markCurrentConversationAsRead()
{
    if (m_currentConvUuid.isEmpty()) {
        return;
    }

    m_unreadCounts.insert(m_currentConvUuid, 0);

    QString convType;
    for (const QJsonObject& convObj : m_conversationsList) {
        if (convObj["targetuuid"].toString() == m_currentConvUuid) {
            convType = convObj["type"].toString();
            break;
        }
    }

    if (convType == "group") {
        m_httpClient->markGroupConversationRead(m_currentConvUuid);
    } else {
        m_httpClient->markPrivateConversationRead(m_currentConvUuid);
    }

    for (int i = 0; i < m_conversationsList.size(); ++i) {
        QJsonObject convObj = m_conversationsList.at(i);
        if (convObj["targetuuid"].toString() == m_currentConvUuid) {
            updateConversationItemDisplay(i);
            break;
        }
    }
}

// ============================================================
// 用户名缓存 — UUID→用户名，3 分钟 TTL，过期自动重新拉取
// ============================================================

QString ChatMainPage::resolveUsername(const QString& userUuid)
{
    if (userUuid.isEmpty()) return QString();
    if (userUuid == m_userInfo["useruuid"].toString())
        return m_userInfo["username"].toString();

    qint64 now = QDateTime::currentMSecsSinceEpoch();
    static constexpr qint64 TTL_MS = 3 * 60 * 1000;

    auto it = m_userNameCache.find(userUuid);
    if (it != m_userNameCache.end() && (now - it->fetchedAt) < TTL_MS) {
        return it->name;                    // 缓存命中且未过期
    }

    // 未命中或已过期 → 异步拉取（防止并发重复请求）
    if (!m_pendingNameLookups.contains(userUuid)) {
        m_pendingNameLookups.insert(userUuid);
        m_httpClient->getUserInfo(userUuid);
    }

    // 返回过期缓存或 UUID 作为降级显示
    return (it != m_userNameCache.end()) ? it->name : userUuid;
}

// ============================================================
// 备注显示 — 优先显示备注，原名作为次级显示
// ============================================================

QString ChatMainPage::resolveDisplayName(const QString& userUuid, const QString& originalName)
{
    QString remark = m_remarkCache.value(userUuid);
    if (!remark.isEmpty()) {
        return QString("%1 (%2)").arg(remark).arg(originalName);
    }
    return originalName;
}

void ChatMainPage::fetchAndCacheUsername(const QString& uuid)
{
    if (!m_pendingNameLookups.contains(uuid)) {
        m_pendingNameLookups.insert(uuid);
        m_httpClient->getUserInfo(uuid);
    }
}

// ============================================================
// 消息免打扰设置
// ============================================================

bool ChatMainPage::isMuted(const QString& convUuid) const
{
    return m_mutedConversations.contains(convUuid);
}

void ChatMainPage::setMuted(const QString& convUuid, bool muted)
{
    if (muted) {
        m_mutedConversations.insert(convUuid);
    } else {
        m_mutedConversations.remove(convUuid);
    }
}

// ============================================================
// 异步头像加载 — 不再阻塞 UI 线程
// ============================================================

QPixmap ChatMainPage::getOrPlaceholderAvatar(const QString& avatarUrl, const QString& name, const QSize& size)
{
    if (avatarUrl.isEmpty()) {
        return AsyncAvatarLoader::makePlaceholder(name, size);
    }
    QPixmap cached = m_avatarLoader->getCached(avatarUrl, size);
    if (!cached.isNull()) {
        return cached;
    }
    return AsyncAvatarLoader::makePlaceholder(name, size);
}

void ChatMainPage::loadAvatarAsync(QLabel* label, const QString& url, const QString& name, const QSize& size)
{
    if (!label) return;

    // 立即设占位图（已缓存则直接用缓存）
    label->setPixmap(getOrPlaceholderAvatar(url, name, size));

    if (url.isEmpty()) return;

    // 异步下载，完成时更新 label（用 cacheKey 匹配确保正确 label）
    QString cacheKey = url;
    connect(m_avatarLoader, &AsyncAvatarLoader::avatarReady,
        [label, cacheKey](const QString& key, QPixmap pix) {
            if (key == cacheKey && !pix.isNull()) {
                label->setPixmap(pix);
            }
        });

    m_avatarLoader->load(url, cacheKey, size);
}

void ChatMainPage::onAvatarLoaded(const QString& cacheKey, QPixmap pixmap)
{
    Q_UNUSED(cacheKey);
    Q_UNUSED(pixmap);
    // 每个调用方通过自己的 lambda connect 到 avatarReady 处理
}

// ============================================================
// 消息缓存加载完成回调（Worker 线程 → UI 线程）
// ============================================================

void ChatMainPage::onCacheLoadFinished(const QString& convUuid, QJsonArray messages)
{
    QString convType = "private";
    for (const auto& conv : m_conversationsList) {
        if (conv["targetuuid"].toString() == convUuid) {
            convType = conv["type"].toString();
            break;
        }
    }

    m_messagesCache.insert(convUuid, messages);
    markConversationLoaded(convUuid);

    // 只在当前活跃会话中显示，防止缓存加载到错误的会话窗口
    if (convUuid == m_currentConvUuid) {
        displayMessages(messages, convType);
    }

    // 缓存加载后始终拉一次服务端，保证数据最新且修复可能的磁盘缓存污染
    if (convType == "group") {
        m_httpClient->getGroupMessagesPage(convUuid, 1, MESSAGE_PAGE_SIZE);
    } else {
        m_httpClient->getPrivateMessagesPage(convUuid, 1, MESSAGE_PAGE_SIZE);
    }
}

QString ChatMainPage::getMessageCacheDir() const
{
    QString cacheDir = QCoreApplication::applicationDirPath() + "/message_cache";
    QDir dir(cacheDir);
    if (!dir.exists()) {
        dir.mkpath(cacheDir);
    }
    return cacheDir;
}

QString ChatMainPage::getMessageCacheFilePath(const QString& convUuid) const
{
    return getMessageCacheDir() + "/" + convUuid + ".json";
}

void ChatMainPage::saveMessagesToCache(const QString& convUuid, const QJsonArray& messages)
{
    m_cacheManager->saveAsync(convUuid, messages);
}

QJsonArray ChatMainPage::loadMessagesFromCache(const QString& convUuid)
{
    // 同步读取，仅用于过渡；新代码走 onCacheLoadFinished
    QString filePath = getMessageCacheFilePath(convUuid);
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) return QJsonArray();
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    return (doc.isArray()) ? doc.array() : QJsonArray();
}

bool ChatMainPage::hasLocalCache(const QString& convUuid)
{
    return m_cacheManager->exists(convUuid);
}
