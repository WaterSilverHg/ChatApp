#include "ChatMainPage.h"
#include "UserSession.h"
#include <QMessageBox>
#include <QDateTime>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QListWidget>
#include <QGroupBox>
#include <QKeyEvent>

ChatMainPage::ChatMainPage(const QString& username, const QString& token, const QJsonObject& userInfo, QWidget *parent)
    : QWidget(parent)
    , m_currentUser(username)
    , m_currentConvName("")
    , m_currentConvUuid("")
    , m_authToken(token)
    , m_userInfo(userInfo)
    , m_httpClient(HttpApiClient::instance())
    , m_wsClient(WebSocketClient::instance())
{
    ui.setupUi(this);
    setWindowTitle("聊天系统 - " + username);

    ui.chatDisplayWidget->setOpenLinks(false);

    m_httpClient->setServerUrl(HTTP_BASE_URL);
    m_httpClient->setAuthToken(token);
    m_wsClient->setWsUrl(WEBSOCKET_URL);

    connect(m_httpClient, &HttpApiClient::conversationsReceived, this, &ChatMainPage::onConversationsReceived, Qt::UniqueConnection);
    connect(m_httpClient, &HttpApiClient::privateMessagesReceived, this, &ChatMainPage::onPrivateMessagesReceived, Qt::UniqueConnection);
    connect(m_httpClient, &HttpApiClient::groupMessagesReceived, this, &ChatMainPage::onGroupMessagesReceived, Qt::UniqueConnection);
    connect(m_httpClient, &HttpApiClient::errorOccurred, this, &ChatMainPage::onError, Qt::UniqueConnection);
    connect(m_httpClient, &HttpApiClient::receivedRequestsReceived, this, &ChatMainPage::onReceivedRequestsReceived, Qt::UniqueConnection);
    connect(m_httpClient, &HttpApiClient::userDetailReceived, this, &ChatMainPage::onUserDetailReceived, Qt::UniqueConnection);
    connect(m_httpClient, &HttpApiClient::usersSearched, this, &ChatMainPage::onUsersSearched, Qt::UniqueConnection);
    connect(m_httpClient, &HttpApiClient::groupsSearched, this, &ChatMainPage::onGroupsSearched, Qt::UniqueConnection);
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
    connect(m_wsClient, &WebSocketClient::friendRequestAccepted, this, [this](bool success) {
        if (!success) QMessageBox::warning(this, "失败", "同意好友请求失败");
    });
    connect(m_wsClient, &WebSocketClient::friendRequestRejected, this, [this](bool success) {
        if (!success) QMessageBox::warning(this, "失败", "拒绝好友请求失败");
    });
    connect(m_wsClient, &WebSocketClient::friendRequestSent, this, [this](const QJsonObject&) {
        QMessageBox::information(this, "成功", "好友请求已发送");
    });

    updateUserInfo();
    setupConversationList();

    ui.messageEdit->installEventFilter(this);
}

ChatMainPage::~ChatMainPage()
{}

void ChatMainPage::updateUserInfo()
{
}

void ChatMainPage::setupConversationList()
{
    m_httpClient->getConversations();
}

void ChatMainPage::onConversationsReceived(const QJsonArray& conversations)
{
    ui.conversationListWidget->clear();
    m_conversationsList.clear();

    for (const QJsonValue& convValue : conversations) {
        QJsonObject convObj = convValue.toObject();
        m_conversationsList.append(convObj);

        QString targetName = convObj["targetname"].toString();
        QString convType = convObj["type"].toString();
        QString lastMessage = convObj["lastmessage"].toString();
        int unreadCount = convObj["unreadcount"].toInt();

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

        QListWidgetItem* item = new QListWidgetItem(displayText);
        ui.conversationListWidget->addItem(item);
    }
}

void ChatMainPage::on_conversationListWidget_itemClicked(QListWidgetItem* item)
{
    Q_UNUSED(item);

    int row = ui.conversationListWidget->currentRow();
    if (row < 0 || row >= m_conversationsList.size()) {
        return;
    }

    QJsonObject convObj = m_conversationsList.at(row);
    QString convUuid = convObj["targetuuid"].toString();
    QString convType = convObj["type"].toString();

    // 如果点击的是已经选中的会话，则不进行操作
    if (m_currentConvUuid == convUuid) {
        return;
    }

    m_currentConvUuid = convUuid;
    m_currentConvName = convObj["targetname"].toString();

    ui.chatTitleLabel->setText(m_currentConvName);
    ui.chatDisplayWidget->clear();
    ui.messageEdit->setFocus();

    markCurrentConversationAsRead();

    if (isConversationLoaded(convUuid)) {
        QJsonArray cachedMessages = m_messagesCache.value(convUuid);
        displayMessages(cachedMessages, convType);
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
    if (convType == "group") {
        m_httpClient->getGroupMessages(convUuid);
    } else {
        m_httpClient->getPrivateMessages(convUuid);
    }
}

void ChatMainPage::displayMessages(const QJsonArray& messages, const QString& convType)
{
    for (const QJsonValue& messageValue : messages) {
        QJsonObject messageObj = messageValue.toObject();
        QString username = messageObj["username"].toString();
        QString fromUserUuid = messageObj["fromuseruuid"].toString();
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
    QDialog* searchDialog = new QDialog(this);
    searchDialog->setWindowTitle("搜索用户或群聊");
    searchDialog->setFixedSize(500, 400);

    QVBoxLayout* layout = new QVBoxLayout(searchDialog);

    QLineEdit* searchEdit = new QLineEdit(searchDialog);
    searchEdit->setPlaceholderText("输入用户名或群聊名称");
    layout->addWidget(searchEdit);

    QPushButton* searchBtn = new QPushButton("搜索", searchDialog);
    layout->addWidget(searchBtn);

    QTabWidget* tabWidget = new QTabWidget(searchDialog);
    
    m_searchUserResultList = new QListWidget(searchDialog);
    tabWidget->addTab(m_searchUserResultList, "用户");
    
    m_searchGroupResultList = new QListWidget(searchDialog);
    tabWidget->addTab(m_searchGroupResultList, "群聊");
    
    layout->addWidget(tabWidget);

    QPushButton* actionBtn = new QPushButton("发送请求", searchDialog);
    actionBtn->setEnabled(false);
    layout->addWidget(actionBtn);

    connect(searchBtn, &QPushButton::clicked, [this, searchEdit]() {
        QString keyword = searchEdit->text().trimmed();
        if (keyword.isEmpty()) {
            QMessageBox::warning(nullptr, "提示", "请输入搜索关键词");
            return;
        }

        m_searchUserResultList->clear();
        m_searchGroupResultList->clear();

        m_httpClient->searchUsers(keyword);
        m_httpClient->searchGroups(keyword);
    });

    connect(m_searchUserResultList, &QListWidget::itemClicked, [actionBtn](QListWidgetItem*) {
        actionBtn->setEnabled(true);
    });

    connect(m_searchGroupResultList, &QListWidget::itemClicked, [actionBtn, tabWidget](QListWidgetItem*) {
        actionBtn->setEnabled(true);
        tabWidget->setCurrentIndex(1);
    });

    connect(actionBtn, &QPushButton::clicked, [this, tabWidget, searchDialog]() {
        QListWidget* resultList = tabWidget->currentIndex() == 0 ? m_searchUserResultList : m_searchGroupResultList;
        QListWidgetItem* selected = resultList->currentItem();
        if (!selected) return;

        QJsonObject obj = selected->data(Qt::UserRole).value<QJsonObject>();
        QString type = obj["type"].toString();
        QString uuid = obj["uuid"].toString();
        QString name = type == "user" ? obj["username"].toString() : obj["name"].toString();
        bool isJoined = obj["isJoined"].toBool();

        if (type == "group" && isJoined) {
            QMessageBox::information(nullptr, "提示", "您已加入该群聊");
            return;
        }

        QString requestType = type == "user" ? "好友" : "群聊";
        QString title = QString("发送%1请求").arg(requestType);
        QString msg = QString("确定要向 %1 发送%2请求吗？").arg(name).arg(requestType);

        if (QMessageBox::question(nullptr, title, msg) == QMessageBox::Yes) {
            if (type == "user") {
                m_wsClient->sendFriendRequest(uuid, "");
            } else {
                m_wsClient->joinGroup(uuid);
            }
            QMessageBox::information(nullptr, "成功", QString("%1请求已发送").arg(requestType));
            searchDialog->close();
        }
    });

    searchDialog->exec();
}

void ChatMainPage::on_profileButton_clicked()
{
    QString info = QString("个人资料\n\n用户名: %1\n邮箱: %2\nUUID: %3\n状态: %4")
        .arg(m_currentUser)
        .arg(m_userInfo["email"].toString())
        .arg(m_userInfo["useruuid"].toString())
        .arg(m_userInfo["status"].toString().isEmpty() ? "offline" : m_userInfo["status"].toString());

    QMessageBox::information(this, "个人资料", info);
}

void ChatMainPage::on_friendRequestsButton_clicked()
{
    m_httpClient->getReceivedRequests();
}

void ChatMainPage::onReceivedRequestsReceived(const QJsonArray& requests)
{
    showFriendRequestsDialog(requests);
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
        QString fromUserUuid = req["fromuseruuid"].toString();
        QString requestUuid = req["uuid"].toString();
        QString message = req["message"].toString();
        QString createdAt = req["createdat"].toString();

        QString displayText = QString("请求来自: %1").arg(fromUserUuid);
        if (!message.isEmpty()) {
            displayText += QString(" - %1").arg(message);
        }
        if (!createdAt.isEmpty()) {
            displayText += QString(" (%1)").arg(createdAt);
        }

        QWidget* reqWidget = new QWidget(groupBox);
        QHBoxLayout* reqLayout = new QHBoxLayout(reqWidget);

        QLabel* nameLabel = new QLabel(displayText, reqWidget);
        reqLayout->addWidget(nameLabel);

        QPushButton* acceptBtn = new QPushButton("同意", reqWidget);
        QPushButton* rejectBtn = new QPushButton("拒绝", reqWidget);

        acceptBtn->setStyleSheet("background-color: #4CAF50; color: white; border: none; padding: 4px 12px; border-radius: 4px;");
        rejectBtn->setStyleSheet("background-color: #f44336; color: white; border: none; padding: 4px 12px; border-radius: 4px;");

        connect(acceptBtn, &QPushButton::clicked, [this, requestUuid, dialog]() {
            m_wsClient->acceptFriendRequest(requestUuid);
            dialog->close();
            QMessageBox::information(this, "成功", "好友请求已同意");
        });

        connect(rejectBtn, &QPushButton::clicked, [this, requestUuid, dialog]() {
            m_wsClient->rejectFriendRequest(requestUuid);
            dialog->close();
            QMessageBox::information(this, "成功", "好友请求已拒绝");
        });

        reqLayout->addWidget(acceptBtn);
        reqLayout->addWidget(rejectBtn);

        groupLayout->addWidget(reqWidget);
    }

    dialog->exec();
}

void ChatMainPage::on_settingsButton_clicked()
{
    QMessageBox::information(this, "设置", "设置功能正在开发中...");
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

    if (convType == "group") {
        QMessageBox::information(this, "群聊信息",
            QString("群名称: %1\n群ID: %2").arg(targetName).arg(targetId));
    } else {
        QMessageBox::information(this, "好友信息",
            QString("好友名称: %1\n好友ID: %2").arg(targetName).arg(targetId));
    }
}

void ChatMainPage::on_sendButton_clicked()
{
    int row = ui.conversationListWidget->currentRow();
    if (row < 0 || row >= m_conversationsList.size()) {
        QMessageBox::warning(this, "提示", "请先选择一个会话!");
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
    QString username = message["username"].toString();
    QString fromUserUuid = message["fromuseruuid"].toString();
    appendMessage(username, fromUserUuid, content, true, createdAt);

    if (!m_currentConvUuid.isEmpty() && m_messagesCache.contains(m_currentConvUuid)) {
        QJsonArray cachedMessages = m_messagesCache.value(m_currentConvUuid);
        cachedMessages.append(message);
        m_messagesCache.insert(m_currentConvUuid, cachedMessages);
    }
}

void ChatMainPage::onGroupMessageSent(const QJsonObject& message)
{
    QString content = message["content"].toString();
    QString createdAt = message["createdat"].toString();
    QString username = message["username"].toString();
    QString fromUserUuid = message["fromuseruuid"].toString();
    appendMessage(username, fromUserUuid, content, true, createdAt);

    if (!m_currentConvUuid.isEmpty() && m_messagesCache.contains(m_currentConvUuid)) {
        QJsonArray cachedMessages = m_messagesCache.value(m_currentConvUuid);
        cachedMessages.append(message);
        m_messagesCache.insert(m_currentConvUuid, cachedMessages);
    }
}

void ChatMainPage::onPrivateMessagesReceived(const QString& convUuid, const QJsonArray& messages)
{
    m_messagesCache.insert(convUuid, messages);
    markConversationLoaded(convUuid);
    
    // 只有当这个响应是当前会话的消息时才显示
    if (convUuid == m_currentConvUuid) {
        displayMessages(messages, "private");
    }
}

void ChatMainPage::onGroupMessagesReceived(const QString& convUuid, const QJsonArray& messages)
{
    m_messagesCache.insert(convUuid, messages);
    markConversationLoaded(convUuid);
    
    // 只有当这个响应是当前会话的消息时才显示
    if (convUuid == m_currentConvUuid) {
        displayMessages(messages, "group");
    }
}

void ChatMainPage::onNewPrivateMessageReceived(const QJsonObject& message)
{
    QString fromUserUuid = message["fromuseruuid"].toString();
    QString touseruuid = message["touseruuid"].toString();
    QString currentUserUuid = m_userInfo["useruuid"].toString();

    QString convUuid;
    if (fromUserUuid == currentUserUuid) {
        convUuid = touseruuid;
    } else {
        convUuid = fromUserUuid;
    }

    if (m_messagesCache.contains(convUuid)) {
        QJsonArray cachedMessages = m_messagesCache.value(convUuid);
        cachedMessages.append(message);
        m_messagesCache.insert(convUuid, cachedMessages);
    }

    if (m_currentConvUuid == convUuid) {
        QString username = message["username"].toString();
        QString content = message["content"].toString();
        QString createdAt = message["createdat"].toString();
        appendMessage(username, fromUserUuid, content, false, createdAt);
    } else {
        updateConversationUnreadCount(convUuid, 1);
    }
}

void ChatMainPage::onNewGroupMessageReceived(const QJsonObject& message)
{
    QString groupUuid = message["groupuuid"].toString();
    QString fromUserUuid = message["fromuseruuid"].toString();
    QString username = message["username"].toString();
    QString content = message["content"].toString();
    QString createdAt = message["createdat"].toString();

    if (m_messagesCache.contains(groupUuid)) {
        QJsonArray cachedMessages = m_messagesCache.value(groupUuid);
        cachedMessages.append(message);
        m_messagesCache.insert(groupUuid, cachedMessages);
    }

    if (m_currentConvUuid == groupUuid) {
        bool isSelf = (fromUserUuid == m_userInfo["useruuid"].toString());
        appendMessage(username, fromUserUuid, content, isSelf, createdAt);
    } else {
        updateConversationUnreadCount(groupUuid, 1);
    }
}

void ChatMainPage::onError(const QString& errorMessage, int errorCode)
{
    QMessageBox::warning(this, "错误", errorMessage);
}

void ChatMainPage::closeEvent(QCloseEvent *event)
{
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("确认退出");
    msgBox.setText("请选择退出方式:");
    msgBox.setEscapeButton(QMessageBox::Cancel);

    QPushButton *loginBtn = msgBox.addButton("返回登录", QMessageBox::ActionRole);
    QPushButton *registerBtn = msgBox.addButton("返回注册", QMessageBox::ActionRole);
    QPushButton *exitBtn = msgBox.addButton("直接退出", QMessageBox::RejectRole);

    msgBox.exec();

    if (msgBox.clickedButton() == loginBtn) {
        m_wsClient->disconnectWebSocket();
        m_httpClient->logout();
        UserSession::instance()->clear();
        emit logoutToLogin();
        event->accept();
    } else if (msgBox.clickedButton() == registerBtn) {
        m_wsClient->disconnectWebSocket();
        m_httpClient->logout();
        UserSession::instance()->clear();
        emit logoutToRegister();
        event->accept();
    } else if (msgBox.clickedButton() == exitBtn) {
        m_wsClient->disconnectWebSocket();
        m_httpClient->logout();
        UserSession::instance()->clear();
        emit logoutToExit();
        event->accept();
    } else {
        event->ignore();
    }
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
    QString bubbleBg = isSelf ? "#DCF8C6" : "#FFFFFF";

    QString floatDirection = isSelf ? "right" : "left";
    QString safeMessage = message.toHtmlEscaped();

    QString msgHtml;
    if (isSelf) {
        msgHtml = QString(
                      "<table width='100%%' cellpadding='0' cellspacing='0' style='margin:5px 0;'>"
                      "<tr>"
                      "<td width='*'></td>"
                      "<td max-width: 70% bgcolor='%1' style='padding:8px 12px; border-radius:15px;'>"
                      "  <div style='font-size:12px; color:#666; margin-bottom:4px;'>"
                      "    <a href='user://%2' style='color:#1a0dab; text-decoration:none;'>%3</a>"
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
                      "<td max-width: 70% bgcolor='%1' style='padding:8px 12px; border-radius:15px;'>"
                      "  <div style='font-size:12px; color:#666; margin-bottom:4px;'>"
                      "    <a href='user://%2' style='color:#1a0dab; text-decoration:none;'>%3</a>"
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

void ChatMainPage::onUserDetailReceived(const QJsonObject& user)
{
    QString username = user["username"].toString();
    QString email = user["email"].toString();
    QString userUuid = user["useruuid"].toString();
    QString avatarUrl = user["avatarurl"].toString();
    QString status = user["status"].toString();
    QString lastSeen = user["lastseen"].toString();

    QString info = QString("用户名: %1\n邮箱: %2\nUUID: %3\n状态: %4\n最后在线: %5")
        .arg(username)
        .arg(email)
        .arg(userUuid)
        .arg(status.isEmpty() ? "offline" : status)
        .arg(lastSeen.isEmpty() ? "未知" : lastSeen);

    QMessageBox::information(this, "用户详情", info);
}

void ChatMainPage::onUsersSearched(const QJsonArray& users)
{
    if (!m_searchUserResultList) return;

    for (const QJsonValue& val : users) {
        QJsonObject obj = val.toObject();
        QString username = obj["username"].toString();
        QString userUuid = obj["useruuid"].toString();
        QString email = obj["email"].toString();

        QJsonObject resultObj;
        resultObj["type"] = "user";
        resultObj["username"] = username;
        resultObj["uuid"] = userUuid;
        resultObj["email"] = email;

        QString display = QString("[用户] %1").arg(username);
        QListWidgetItem* item = new QListWidgetItem(display);
        item->setData(Qt::UserRole, QVariant::fromValue(resultObj));
        m_searchUserResultList->addItem(item);
    }
}

void ChatMainPage::onGroupsSearched(const QJsonArray& groups)
{
    if (!m_searchGroupResultList) return;

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
    if (row < 0 || row >= m_conversationsList.size()) {
        return;
    }

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
    if (item) {
        item->setText(displayText);
    }
}

void ChatMainPage::markCurrentConversationAsRead()
{
    if (m_currentConvUuid.isEmpty()) {
        return;
    }

    m_unreadCounts.insert(m_currentConvUuid, 0);
    m_httpClient->markConversationRead(m_currentConvUuid);

    for (int i = 0; i < m_conversationsList.size(); ++i) {
        QJsonObject convObj = m_conversationsList.at(i);
        if (convObj["targetuuid"].toString() == m_currentConvUuid) {
            updateConversationItemDisplay(i);
            break;
        }
    }
}
