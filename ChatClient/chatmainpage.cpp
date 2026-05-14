#include "ChatMainPage.h"
#include "UserSession.h"
#include "LoginPage.h"
#include <QMessageBox>
#include <QDateTime>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QListWidget>
#include <QTabWidget>
#include <QGroupBox>
#include <QKeyEvent>
#include <QFileDialog>
#include <QFile>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QDebug>
#include <QPixmap>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QCryptographicHash>
#include <QDir>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonArray>
#include <QScrollBar>

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

    ui.conversationListWidget->setStyleSheet("QListWidget::item:selected { color: black; }");

    m_httpClient->setServerUrl(HTTP_BASE_URL);
    m_httpClient->setAuthToken(token);
    m_wsClient->setWsUrl(WEBSOCKET_URL);

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
    connect(m_httpClient, &HttpApiClient::userDetailReceived, this, &ChatMainPage::onUserDetailReceived, Qt::UniqueConnection);
    connect(m_httpClient, &HttpApiClient::groupDetailReceived, this, &ChatMainPage::onGroupDetailReceived, Qt::UniqueConnection);
    connect(m_httpClient, &HttpApiClient::usersSearched, this, &ChatMainPage::onUsersSearched, Qt::UniqueConnection);
    connect(m_httpClient, &HttpApiClient::groupsSearched, this, &ChatMainPage::onGroupsSearched, Qt::UniqueConnection);
    connect(m_httpClient, &HttpApiClient::fileUploaded, this, &ChatMainPage::onFileUploaded, Qt::UniqueConnection);
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
            QMessageBox::information(this, "成功", "好友请求已处理");
        } else {
            QMessageBox::warning(this, "失败", "处理好友请求失败");
        }
    });
    connect(m_wsClient, &WebSocketClient::friendRequestSent, this, [this](const QJsonObject&) {
        QMessageBox::information(this, "成功", "好友请求已发送");
    });
    connect(m_wsClient, &WebSocketClient::groupRequestHandled, this, [this](bool success) {
        if (success) {
            QMessageBox::information(this, "成功", "群聊请求已处理");
        } else {
            QMessageBox::warning(this, "失败", "处理群聊请求失败");
        }
    });
    connect(m_wsClient, &WebSocketClient::kickedByServer, this, [this]() {
        QMessageBox::warning(this, "下线通知", "您的账号已在另一个设备登录，您已被踢出。");
        UserSession::instance()->clear();
        LoginPage* loginPage = new LoginPage();
        loginPage->show();
        this->close();
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
    m_unreadCounts.clear();

    for (const QJsonValue& convValue : conversations) {
        QJsonObject convObj = convValue.toObject();
        m_conversationsList.append(convObj);

        QString targetName = convObj["targetname"].toString();
        QString convType = convObj["type"].toString();
        QString lastMessage = convObj["lastmessage"].toString();
        QString convUuid = convObj["targetuuid"].toString();
        int unreadCount = convObj["unreadcount"].toInt();

        m_unreadCounts.insert(convUuid, unreadCount);

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
    if (hasLocalCache(convUuid)) {
        QJsonArray cachedMessages = loadMessagesFromCache(convUuid);
        m_messagesCache.insert(convUuid, cachedMessages);
        displayMessages(cachedMessages, convType);

        int unreadCount = m_unreadCounts.value(convUuid, 0);
        if (unreadCount > 0) {
            int fetchSize = qMin(MESSAGE_PAGE_SIZE, unreadCount);
            if (convType == "group") {
                m_httpClient->getGroupMessagesPage(convUuid, 1, fetchSize);
            } else {
                m_httpClient->getPrivateMessagesPage(convUuid, 1, fetchSize);
            }
        }
    } else {
        if (convType == "group") {
            m_httpClient->getGroupMessagesPage(convUuid, 1, MESSAGE_PAGE_SIZE);
        } else {
            m_httpClient->getPrivateMessagesPage(convUuid, 1, MESSAGE_PAGE_SIZE);
        }
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
        QMessageBox::information(this, "成功", QString("头像上传成功!\n文件UUID: %1").arg(fileUuid));
    } else {
        QMessageBox::warning(this, "错误", "头像上传失败");
    }
}

void ChatMainPage::on_friendRequestsButton_clicked()
{
    m_pendingFriendRequests.reset(new QJsonArray());
    m_pendingGroupRequests.reset(new QJsonArray());
    m_httpClient->getReceivedRequests();
    m_httpClient->getReceivedGroupRequests();
    showRequestsDialog();
}

void ChatMainPage::onReceivedRequestsReceived(const QJsonArray& requests)
{
    m_pendingFriendRequests.reset(new QJsonArray(requests));
}

void ChatMainPage::onReceivedGroupRequestsReceived(const QJsonArray& requests)
{
    m_pendingGroupRequests.reset(new QJsonArray(requests));
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

        QString displayText = QString("请求来自: %1").arg(fromUserName.isEmpty() ? fromUserUuid : fromUserName);
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
            m_wsClient->handleFriendRequest(requestUuid, "accepted");
            dialog->close();
            QMessageBox::information(this, "成功", "好友请求已同意");
        });

        connect(rejectBtn, &QPushButton::clicked, [this, requestUuid, dialog]() {
            m_wsClient->handleFriendRequest(requestUuid, "rejected");
            dialog->close();
            QMessageBox::information(this, "成功", "好友请求已拒绝");
        });

        reqLayout->addWidget(acceptBtn);
        reqLayout->addWidget(rejectBtn);

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

        acceptBtn->setStyleSheet("background-color: #4CAF50; color: white; border: none; padding: 4px 12px; border-radius: 4px;");
        rejectBtn->setStyleSheet("background-color: #f44336; color: white; border: none; padding: 4px 12px; border-radius: 4px;");

        connect(acceptBtn, &QPushButton::clicked, [this, requestUuid, dialog]() {
            m_wsClient->handleGroupRequest(requestUuid, "accept");
            dialog->close();
            QMessageBox::information(this, "成功", "入群请求已同意");
        });

        connect(rejectBtn, &QPushButton::clicked, [this, requestUuid, dialog]() {
            m_wsClient->handleGroupRequest(requestUuid, "rejected");
            dialog->close();
            QMessageBox::information(this, "成功", "入群请求已拒绝");
        });

        reqLayout->addWidget(acceptBtn);
        reqLayout->addWidget(rejectBtn);

        groupLayout->addWidget(reqWidget);
    }

    dialog->exec();
}

void ChatMainPage::showRequestsDialog()
{
    if (m_requestsDialog) {
        return;
    }

    m_requestsDialog = new QDialog(this);
    m_requestsDialog->setWindowTitle("请求列表");
    m_requestsDialog->setFixedSize(500, 400);

    QVBoxLayout* mainLayout = new QVBoxLayout(m_requestsDialog);

    QTabWidget* tabWidget = new QTabWidget(m_requestsDialog);

    QWidget* friendTab = new QWidget(tabWidget);
    QVBoxLayout* friendLayout = new QVBoxLayout(friendTab);

    QListWidget* friendListWidget = new QListWidget(friendTab);
    friendLayout->addWidget(friendListWidget);

    for (const QJsonValue& reqValue : *m_pendingFriendRequests) {
        QJsonObject req = reqValue.toObject();
        QString requestUuid = req["uuid"].toString();
        QString message = req["message"].toString();
        QString createdAt = req["createdat"].toString();
        QJsonObject requester = req["requester"].toObject();
        QString fromUserUuid = requester["uuid"].toString();
        QString fromUserName = requester["username"].toString();

        QString displayText = QString("请求来自: %1").arg(fromUserName.isEmpty() ? fromUserUuid : fromUserName);
        if (!message.isEmpty()) {
            displayText += QString(" - %1").arg(message);
        }
        if (!createdAt.isEmpty()) {
            displayText += QString(" (%1)").arg(createdAt);
        }

        QListWidgetItem* item = new QListWidgetItem(displayText, friendListWidget);
        item->setData(Qt::UserRole, requestUuid);

        QWidget* itemWidget = new QWidget();
        QHBoxLayout* itemLayout = new QHBoxLayout(itemWidget);

        QLabel* nameLabel = new QLabel(displayText, itemWidget);
        itemLayout->addWidget(nameLabel);

        QPushButton* acceptBtn = new QPushButton("同意", itemWidget);
        QPushButton* rejectBtn = new QPushButton("拒绝", itemWidget);

        acceptBtn->setStyleSheet("background-color: #4CAF50; color: white; border: none; padding: 4px 12px; border-radius: 4px;");
        rejectBtn->setStyleSheet("background-color: #f44336; color: white; border: none; padding: 4px 12px; border-radius: 4px;");

        connect(acceptBtn, &QPushButton::clicked, [this, requestUuid, friendListWidget]() {
            m_wsClient->handleFriendRequest(requestUuid, "accepted");
            QMessageBox::information(this, "成功", "好友请求已同意");
            friendListWidget->takeItem(friendListWidget->currentRow());
        });

        connect(rejectBtn, &QPushButton::clicked, [this, requestUuid, friendListWidget]() {
            m_wsClient->handleFriendRequest(requestUuid, "rejected");
            QMessageBox::information(this, "成功", "好友请求已拒绝");
            friendListWidget->takeItem(friendListWidget->currentRow());
        });

        itemLayout->addWidget(acceptBtn);
        itemLayout->addWidget(rejectBtn);
        itemWidget->setLayout(itemLayout);

        item->setSizeHint(itemWidget->sizeHint());
        friendListWidget->setItemWidget(item, itemWidget);
    }

    if (!m_pendingFriendRequests || m_pendingFriendRequests->isEmpty()) {
        QLabel* emptyLabel = new QLabel("暂无好友请求", friendTab);
        emptyLabel->setAlignment(Qt::AlignCenter);
        friendLayout->addWidget(emptyLabel);
    }

    tabWidget->addTab(friendTab, "好友请求");

    QWidget* groupTab = new QWidget(tabWidget);
    QVBoxLayout* groupLayout = new QVBoxLayout(groupTab);

    QListWidget* groupListWidget = new QListWidget(groupTab);
    groupLayout->addWidget(groupListWidget);

    for (const QJsonValue& reqValue : *m_pendingGroupRequests) {
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

        QListWidgetItem* item = new QListWidgetItem(displayText, groupListWidget);
        item->setData(Qt::UserRole, requestUuid);

        QWidget* itemWidget = new QWidget();
        QHBoxLayout* itemLayout = new QHBoxLayout(itemWidget);

        QLabel* nameLabel = new QLabel(displayText, itemWidget);
        itemLayout->addWidget(nameLabel);

        QPushButton* acceptBtn = new QPushButton("同意", itemWidget);
        QPushButton* rejectBtn = new QPushButton("拒绝", itemWidget);

        acceptBtn->setStyleSheet("background-color: #4CAF50; color: white; border: none; padding: 4px 12px; border-radius: 4px;");
        rejectBtn->setStyleSheet("background-color: #f44336; color: white; border: none; padding: 4px 12px; border-radius: 4px;");

        connect(acceptBtn, &QPushButton::clicked, [this, requestUuid, groupListWidget]() {
            m_wsClient->handleGroupRequest(requestUuid, "accept");
            QMessageBox::information(this, "成功", "入群请求已同意");
            groupListWidget->takeItem(groupListWidget->currentRow());
        });

        connect(rejectBtn, &QPushButton::clicked, [this, requestUuid, groupListWidget]() {
            m_wsClient->handleGroupRequest(requestUuid, "rejected");
            QMessageBox::information(this, "成功", "入群请求已拒绝");
            groupListWidget->takeItem(groupListWidget->currentRow());
        });

        itemLayout->addWidget(acceptBtn);
        itemLayout->addWidget(rejectBtn);
        itemWidget->setLayout(itemLayout);

        item->setSizeHint(itemWidget->sizeHint());
        groupListWidget->setItemWidget(item, itemWidget);
    }

    if (!m_pendingGroupRequests || m_pendingGroupRequests->isEmpty()) {
        QLabel* emptyLabel = new QLabel("暂无群聊请求", groupTab);
        emptyLabel->setAlignment(Qt::AlignCenter);
        groupLayout->addWidget(emptyLabel);
    }

    tabWidget->addTab(groupTab, "群聊请求");

    mainLayout->addWidget(tabWidget);

    connect(m_requestsDialog, &QDialog::finished, this, [this]() {
        m_requestsDialog = nullptr;
    });

    m_requestsDialog->exec();
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
        m_httpClient->getGroupDetail(targetId);
    } else {
        m_httpClient->getUserInfo(targetId);
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
    QString username = message["username"].toString();
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
    
    if (convUuid == m_currentConvUuid) {
        displayMessages(messages, "private");
    }
}

void ChatMainPage::onGroupMessagesReceived(const QString& convUuid, const QJsonArray& messages)
{
    m_messagesCache.insert(convUuid, messages);
    saveMessagesToCache(convUuid, messages);
    markConversationLoaded(convUuid);
    m_hasMoreMessages = !messages.isEmpty();
    
    if (convUuid == m_currentConvUuid) {
        displayMessages(messages, "group");
    }
}

void ChatMainPage::onPrivateMessagesBeforeReceived(const QString& convUuid, const QJsonArray& messages)
{
    m_isLoadingOlderMessages = false;
    
    if (messages.isEmpty()) {
        m_hasMoreMessages = false;
        return;
    }
    
    m_hasMoreMessages = true;
    
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
            QString username = messageObj["username"].toString();
            QString fromUserUuid = messageObj["fromuseruuid"].toString();
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
        m_hasMoreMessages = false;
        return;
    }
    
    m_hasMoreMessages = true;
    
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
            QString username = messageObj["username"].toString();
            QString fromUserUuid = messageObj["fromuseruuid"].toString();
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
        m_hasMoreMessages = false;
        return;
    }
    
    m_hasMoreMessages = true;
    m_messagesCache.insert(convUuid, messages);
    saveMessagesToCache(convUuid, messages);
    markConversationLoaded(convUuid);
    
    if (convUuid == m_currentConvUuid) {
        displayMessages(messages, "private");
    }
}

void ChatMainPage::onGroupMessagesPageReceived(const QString& convUuid, const QJsonArray& messages)
{
    if (messages.isEmpty()) {
        m_hasMoreMessages = false;
        return;
    }
    
    m_hasMoreMessages = true;
    m_messagesCache.insert(convUuid, messages);
    saveMessagesToCache(convUuid, messages);
    markConversationLoaded(convUuid);
    
    if (convUuid == m_currentConvUuid) {
        displayMessages(messages, "group");
    }
}

void ChatMainPage::onChatScrollRangeChanged(int min, int max)
{
    QScrollBar* scrollBar = ui.chatDisplayWidget->verticalScrollBar();
    if (scrollBar->value() == min && min != max && !m_isLoadingOlderMessages && m_hasMoreMessages) {
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

    if (m_currentConvUuid == convUuid) {
        if (m_messagesCache.contains(convUuid)) {
            QJsonArray cachedMessages = m_messagesCache.value(convUuid);
            cachedMessages.append(message);
            m_messagesCache.insert(convUuid, cachedMessages);
            saveMessagesToCache(convUuid, cachedMessages);
        }

        QString username = message["username"].toString();
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
    QString username = message["username"].toString();
    QString content = message["content"].toString();
    QString createdAt = message["createdat"].toString();

    if (m_currentConvUuid == groupUuid) {
        if (m_messagesCache.contains(groupUuid)) {
            QJsonArray cachedMessages = m_messagesCache.value(groupUuid);
            cachedMessages.append(message);
            m_messagesCache.insert(groupUuid, cachedMessages);
            saveMessagesToCache(groupUuid, cachedMessages);
        }

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
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("确认退出");
    msgBox.setText("请选择退出方式:");
    msgBox.setEscapeButton(QMessageBox::Cancel);

    QPushButton *loginBtn = msgBox.addButton("返回登录", QMessageBox::ActionRole);
    QPushButton *exitBtn = msgBox.addButton("直接退出", QMessageBox::RejectRole);

    msgBox.exec();

    if (msgBox.clickedButton() == loginBtn) {
        if (m_wsClient) {
            m_wsClient->disconnectWebSocket();
        }
        if (m_httpClient) {
            m_httpClient->abortAllRequests();
        }
        UserSession::instance()->clear();
        event->accept();
        emit logoutToLogin();
    } else if (msgBox.clickedButton() == exitBtn) {
        if (m_wsClient) {
            m_wsClient->disconnectWebSocket();
        }
        if (m_httpClient) {
            m_httpClient->abortAllRequests();
        }
        UserSession::instance()->clear();
        event->accept();
        emit logoutToExit();
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
    QString friendGroup = user["friendgroup"].toString();

    bool isSelf = (userUuid == m_userInfo["useruuid"].toString());

    QDialog* dialog = new QDialog(this);
    dialog->setWindowTitle(isSelf ? "个人资料" : "用户详情");
    dialog->setFixedSize(isSelf ? 400 : 350, isSelf ? 380 : 320);

    QVBoxLayout* layout = new QVBoxLayout(dialog);

    if (isSelf) {
        QLabel* titleLabel = new QLabel("个人资料", dialog);
        titleLabel->setStyleSheet("font-size: 18px; font-weight: bold;");
        titleLabel->setAlignment(Qt::AlignCenter);
        layout->addWidget(titleLabel);
    }

    QLabel* avatarLabel = new QLabel(dialog);
    avatarLabel->setAlignment(Qt::AlignCenter);
    QPixmap avatarPixmap = downloadAvatar(avatarUrl);
    if (!avatarPixmap.isNull()) {
        avatarLabel->setPixmap(avatarPixmap.scaled(100, 100, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        avatarLabel->setText("暂无头像");
        avatarLabel->setStyleSheet("color: #999;");
    }
    layout->addWidget(avatarLabel);

    QLabel* usernameLabel = new QLabel(QString("用户名: %1").arg(username), dialog);
    layout->addWidget(usernameLabel);

    if (!email.isEmpty()) {
        QLabel* emailLabel = new QLabel(QString("邮箱: %1").arg(email), dialog);
        layout->addWidget(emailLabel);
    }

    QLabel* uuidLabel = new QLabel(QString("UUID: %1").arg(userUuid), dialog);
    layout->addWidget(uuidLabel);

    if (!friendGroup.isEmpty()) {
        QLabel* groupLabel = new QLabel(QString("分组: %1").arg(friendGroup), dialog);
        layout->addWidget(groupLabel);
    }

    QLabel* statusLabel = new QLabel(QString("状态: %1").arg(status.isEmpty() ? "offline" : status), dialog);
    layout->addWidget(statusLabel);

    if (!lastSeen.isEmpty()) {
        QLabel* lastSeenLabel = new QLabel(QString("最后在线: %1").arg(lastSeen), dialog);
        layout->addWidget(lastSeenLabel);
    }

    if (isSelf) {
        layout->addStretch();

        QPushButton* uploadBtn = new QPushButton("上传头像", dialog);
        uploadBtn->setStyleSheet("background-color: #4CAF50; color: white; border: none; padding: 8px 16px; border-radius: 4px;");
        layout->addWidget(uploadBtn);

        connect(uploadBtn, &QPushButton::clicked, this, [this, dialog]() {
            QString filePath = QFileDialog::getOpenFileName(this, "选择图片", "", "Images (*.png *.jpg *.jpeg *.gif *.bmp)");
            if (filePath.isEmpty()) {
                return;
            }

            QFile file(filePath);
            if (!file.open(QIODevice::ReadOnly)) {
                QMessageBox::warning(this, "错误", "无法打开文件");
                return;
            }

            QByteArray fileData = file.readAll();
            file.close();

            QFileInfo fileInfo(filePath);
            QString fileName = fileInfo.fileName();

            QMimeDatabase mimeDb;
            QMimeType mime = mimeDb.mimeTypeForFile(fileInfo);
            QString mimeType = mime.name();

            qint64 fileSize = fileData.size();

            m_httpClient->uploadAvatarFile(fileName, mimeType, fileSize, fileData);
            dialog->close();
        });
    }

    QPushButton* closeBtn = new QPushButton("关闭", dialog);
    closeBtn->setStyleSheet("background-color: #4CAF50; color: white; border: none; padding: 8px 16px; border-radius: 4px;");
    layout->addWidget(closeBtn);
    connect(closeBtn, &QPushButton::clicked, dialog, &QDialog::close);

    dialog->exec();
}

void ChatMainPage::onGroupDetailReceived(const QJsonObject& group)
{
    QString groupUuid = group["uuid"].toString();
    QString groupName = group["name"].toString();
    QString avatarUrl = group["avatarurl"].toString();
    QString description = group["description"].toString();
    QString memberCount = QString::number(group["membercount"].toInt());
    QString createdAt = group["createdat"].toString();

    QDialog* dialog = new QDialog(this);
    dialog->setWindowTitle("群详情");
    dialog->setFixedSize(450, 500);

    QVBoxLayout* layout = new QVBoxLayout(dialog);

    QLabel* avatarLabel = new QLabel(dialog);
    avatarLabel->setAlignment(Qt::AlignCenter);
    QPixmap avatarPixmap = downloadAvatar(avatarUrl);
    if (!avatarPixmap.isNull()) {
        avatarLabel->setPixmap(avatarPixmap.scaled(100, 100, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        avatarLabel->setText("暂无头像");
        avatarLabel->setStyleSheet("color: #999;");
    }
    layout->addWidget(avatarLabel);

    QLabel* nameLabel = new QLabel(QString("群名称: %1").arg(groupName), dialog);
    layout->addWidget(nameLabel);

    QLabel* uuidLabel = new QLabel(QString("群ID: %1").arg(groupUuid), dialog);
    layout->addWidget(uuidLabel);

    QLabel* memberCountLabel = new QLabel(QString("成员数: %1").arg(memberCount), dialog);
    layout->addWidget(memberCountLabel);

    if (!description.isEmpty()) {
        QLabel* descLabel = new QLabel(QString("群描述: %1").arg(description), dialog);
        layout->addWidget(descLabel);
    }

    if (!createdAt.isEmpty()) {
        QLabel* createdAtLabel = new QLabel(QString("创建时间: %1").arg(createdAt), dialog);
        layout->addWidget(createdAtLabel);
    }

    QLabel* memberListLabel = new QLabel("群成员:", dialog);
    memberListLabel->setStyleSheet("font-weight: bold; margin-top: 10px;");
    layout->addWidget(memberListLabel);

    QListWidget* memberListWidget = new QListWidget(dialog);
    memberListWidget->setFixedHeight(200);
    memberListWidget->setStyleSheet("QListWidget::item { padding: 5px; border-bottom: 1px solid #eee; }");
    layout->addWidget(memberListWidget);

    connect(m_httpClient, &HttpApiClient::groupMembersReceived, [this, memberListWidget](const QJsonArray& members) {
        memberListWidget->clear();
        for (const QJsonValue& memberValue : members) {
            QJsonObject member = memberValue.toObject();
            QString username = member["username"].toString();
            QString userUuid = member["useruuid"].toString();
            
            QListWidgetItem* item = new QListWidgetItem(username);
            item->setData(Qt::UserRole, userUuid);
            memberListWidget->addItem(item);
        }
    });

    connect(memberListWidget, &QListWidget::itemClicked, [this](QListWidgetItem* item) {
        QString userUuid = item->data(Qt::UserRole).toString();
        m_httpClient->getUserInfo(userUuid);
    });

    QPushButton* closeBtn = new QPushButton("关闭", dialog);
    closeBtn->setStyleSheet("background-color: #4CAF50; color: white; border: none; padding: 8px 16px; border-radius: 4px;");
    layout->addWidget(closeBtn);
    connect(closeBtn, &QPushButton::clicked, dialog, &QDialog::close);

    m_httpClient->getGroupMembers(groupUuid);

    dialog->exec();
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

QPixmap ChatMainPage::downloadAvatar(const QString& avatarUrl)
{
    if (avatarUrl.isEmpty()) {
        return QPixmap();
    }

    QByteArray hash = QCryptographicHash::hash(avatarUrl.toUtf8(), QCryptographicHash::Md5);
    QString localFileName = QString(hash.toHex()) + ".png";

    QString cacheDir = QCoreApplication::applicationDirPath() + "/avatar_cache";
    QDir dir(cacheDir);
    if (!dir.exists()) {
        dir.mkpath(cacheDir);
    }

    QString localPath = cacheDir + "/" + localFileName;

    if (QFile::exists(localPath)) {
        QPixmap pixmap(localPath);
        return pixmap.scaled(80, 80, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    QNetworkAccessManager manager;
    QNetworkRequest request(avatarUrl);

    QNetworkReply* reply = manager.get(request);
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "Failed to download avatar:" << reply->errorString();
        reply->deleteLater();
        return QPixmap();
    }

    QByteArray data = reply->readAll();
    reply->deleteLater();

    QPixmap pixmap;
    if (!pixmap.loadFromData(data)) {
        qDebug() << "Failed to load avatar from data";
        return QPixmap();
    }

    QFile file(localPath);
    if (file.open(QIODevice::WriteOnly)) {
        pixmap.save(&file, "PNG");
        file.close();
    }

    return pixmap.scaled(80, 80, Qt::KeepAspectRatio, Qt::SmoothTransformation);
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
    QString filePath = getMessageCacheFilePath(convUuid);
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(messages);
        file.write(doc.toJson());
        file.close();
    }
}

QJsonArray ChatMainPage::loadMessagesFromCache(const QString& convUuid)
{
    QString filePath = getMessageCacheFilePath(convUuid);
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return QJsonArray();
    }
    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isArray()) {
        return QJsonArray();
    }
    return doc.array();
}

bool ChatMainPage::hasLocalCache(const QString& convUuid)
{
    QString filePath = getMessageCacheFilePath(convUuid);
    return QFile::exists(filePath);
}
