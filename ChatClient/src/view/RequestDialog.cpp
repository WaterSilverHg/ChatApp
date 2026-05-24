#include "RequestDialog.h"
#include "../model/HttpApiClient.h"
#include "../model/WebSocketClient.h"

RequestDialog::RequestDialog(QWidget* parent)
    : QDialog(parent)
{
    ui.setupUi(this);
    setWindowIcon(QIcon(":/chat.svg"));
    connect(ui.closeBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void RequestDialog::setFriendRequests(const QJsonArray& requests)
{
    ui.friendListWidget->clear();
    for (const auto& v : requests)
        buildFriendRequestItem(v.toObject(), ui.friendListWidget);
}

void RequestDialog::setGroupRequests(const QJsonArray& requests)
{
    ui.groupListWidget->clear();
    for (const auto& v : requests)
        buildGroupRequestItem(v.toObject(), ui.groupListWidget);
}

void RequestDialog::buildFriendRequestItem(const QJsonObject& req, QListWidget* list)
{
    QString requestUuid = req["uuid"].toString();
    QString message = req["message"].toString();
    QString createdAt = req["createdat"].toString();
    QJsonObject requester = req["requester"].toObject();
    QString fromUserName = requester["username"].toString();
    QString fromUserUuid = requester["uuid"].toString();

    QListWidgetItem* item = new QListWidgetItem();
    item->setData(Qt::UserRole, requestUuid);

    QWidget* w = new QWidget();
    auto* vlay = new QVBoxLayout(w);
    vlay->setSpacing(4);
    vlay->setContentsMargins(8, 6, 8, 6);

    QString titleText = QString::fromUtf8("请求来自: %1    %2")
        .arg(fromUserName.isEmpty() ? fromUserUuid : fromUserName,
             createdAt.isEmpty() ? "" : createdAt);
    auto* titleLabel = new QLabel(titleText);
    titleLabel->setStyleSheet("font-weight: bold; color: #333;");
    vlay->addWidget(titleLabel);

    if (!message.isEmpty()) {
        auto* descLabel = new QLabel(QString::fromUtf8("描述: %1").arg(message));
        descLabel->setStyleSheet("color: #666; font-size: 12px;");
        descLabel->setWordWrap(true);
        vlay->addWidget(descLabel);
    }

    auto* btnRow = new QHBoxLayout();
    btnRow->addStretch();

    auto* acceptBtn = new QPushButton(QString::fromUtf8("同意"));
    auto* rejectBtn = new QPushButton(QString::fromUtf8("拒绝"));
    acceptBtn->setFixedSize(60, 28);
    rejectBtn->setFixedSize(60, 28);
    acceptBtn->setStyleSheet("QPushButton { background: #A8D8B9; color: #333; border: none; padding: 4px 8px; border-radius: 4px; } QPushButton:hover { background: #7CB894; color: white; }");
    rejectBtn->setStyleSheet("QPushButton { background: #E88B8B; color: white; border: none; padding: 4px 8px; border-radius: 4px; } QPushButton:hover { background: #D07070; }");

    auto* ws = WebSocketClient::instance();
    connect(acceptBtn, &QPushButton::clicked, [this, requestUuid, list, item,ws]() {
        auto* conn = new QMetaObject::Connection;
        *conn = connect(WebSocketClient::instance(), &WebSocketClient::friendRequestHandled, this,
            [list, item, conn](bool success) {
                if (success && item) {
                    int row = list->row(item);
                    if (row >= 0) {
                        auto* taken = list->takeItem(row);
                        delete taken;
                    }
                }
                disconnect(*conn);
                delete conn;
            });
        ws->handleFriendRequest(requestUuid, "accepted");
    });

    connect(rejectBtn, &QPushButton::clicked, [this, requestUuid, list, item,ws]() {
        auto* conn = new QMetaObject::Connection;
        *conn = connect(WebSocketClient::instance(), &WebSocketClient::friendRequestHandled, this,
            [list, item, conn](bool success) {
                if (success && item) {
                    int row = list->row(item);
                    if (row >= 0) {
                        auto* taken = list->takeItem(row);
                        delete taken;
                    }
                }
                disconnect(*conn);
                delete conn;
            });
        ws->handleFriendRequest(requestUuid, "rejected");
    });

    btnRow->addWidget(acceptBtn);
    btnRow->addWidget(rejectBtn);
    vlay->addLayout(btnRow);

    item->setSizeHint(w->sizeHint());
    list->setItemWidget(item, w);
}

void RequestDialog::buildGroupRequestItem(const QJsonObject& req, QListWidget* list)
{
    QString requestUuid = req["uuid"].toString();
    QString message = req["message"].toString();
    QString createdAt = req["createdat"].toString();
    QString groupName = req["groupname"].toString();
    QString groupUuid = req["groupuuid"].toString();
    QJsonObject requester = req["requester"].toObject();
    QString requesterName = requester["username"].toString();

    QListWidgetItem* item = new QListWidgetItem();
    item->setData(Qt::UserRole, requestUuid);

    QWidget* w = new QWidget();
    auto* vlay = new QVBoxLayout(w);
    vlay->setSpacing(4);
    vlay->setContentsMargins(8, 6, 8, 6);

    QString titleText = QString::fromUtf8("入群申请 - %1    %2")
        .arg(groupName.isEmpty() ? groupUuid : groupName,
             createdAt.isEmpty() ? "" : createdAt);
    auto* titleLabel = new QLabel(titleText);
    titleLabel->setStyleSheet("font-weight: bold; color: #333;");
    vlay->addWidget(titleLabel);

    auto* nameLabel = new QLabel(QString::fromUtf8("申请人: %1").arg(requesterName));
    nameLabel->setStyleSheet("color: #555;");
    vlay->addWidget(nameLabel);

    if (!message.isEmpty()) {
        auto* descLabel = new QLabel(QString::fromUtf8("描述: %1").arg(message));
        descLabel->setStyleSheet("color: #666; font-size: 12px;");
        descLabel->setWordWrap(true);
        vlay->addWidget(descLabel);
    }

    auto* btnRow = new QHBoxLayout();
    btnRow->addStretch();

    auto* acceptBtn = new QPushButton(QString::fromUtf8("同意"));
    auto* rejectBtn = new QPushButton(QString::fromUtf8("拒绝"));
    acceptBtn->setFixedSize(60, 28);
    rejectBtn->setFixedSize(60, 28);
    acceptBtn->setStyleSheet("QPushButton { background: #A8D8B9; color: #333; border: none; padding: 4px 8px; border-radius: 4px; } QPushButton:hover { background: #7CB894; color: white; }");
    rejectBtn->setStyleSheet("QPushButton { background: #E88B8B; color: white; border: none; padding: 4px 8px; border-radius: 4px; } QPushButton:hover { background: #D07070; }");

    auto* ws = WebSocketClient::instance();
    connect(acceptBtn, &QPushButton::clicked, [this, requestUuid, list, item,ws]() {
        auto* conn = new QMetaObject::Connection;
        *conn = connect(WebSocketClient::instance(), &WebSocketClient::groupRequestHandled, this,
            [list, item, conn](bool success) {
                if (success && item) {
                    int row = list->row(item);
                    if (row >= 0) {
                        auto* taken = list->takeItem(row);
                        delete taken;
                    }
                }
                disconnect(*conn);
                delete conn;
            });
        ws->handleGroupRequest(requestUuid, "accept");
    });

    connect(rejectBtn, &QPushButton::clicked, [this, requestUuid, list, item,ws]() {
        auto* conn = new QMetaObject::Connection;
        *conn = connect(WebSocketClient::instance(), &WebSocketClient::groupRequestHandled, this,
            [list, item, conn](bool success) {
                if (success && item) {
                    int row = list->row(item);
                    if (row >= 0) {
                        auto* taken = list->takeItem(row);
                        delete taken;
                    }
                }
                disconnect(*conn);
                delete conn;
            });
        ws->handleGroupRequest(requestUuid, "rejected");
    });

    btnRow->addWidget(acceptBtn);
    btnRow->addWidget(rejectBtn);
    vlay->addLayout(btnRow);

    item->setSizeHint(w->sizeHint());
    list->setItemWidget(item, w);
}
