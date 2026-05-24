#include "SearchDialog.h"
#include "../model/HttpApiClient.h"
#include "../model/WebSocketClient.h"

SearchDialog::SearchDialog(QWidget* parent)
    : QDialog(parent)
{
    ui.setupUi(this);
    setWindowIcon(QIcon(":/chat.svg"));

    connect(ui.searchBtn, &QPushButton::clicked, this, &SearchDialog::onSearchClicked);
    connect(ui.actionBtn, &QPushButton::clicked, this, &SearchDialog::onActionClicked);
    connect(ui.closeBtn, &QPushButton::clicked, this, &QDialog::reject);
    connect(ui.userResultList, &QListWidget::itemClicked, this, &SearchDialog::onUserResultSelected);
    connect(ui.groupResultList, &QListWidget::itemClicked, this, &SearchDialog::onGroupResultSelected);

    auto* http = HttpApiClient::instance();

    connect(http, &HttpApiClient::usersSearched, this, [this](const QJsonArray& users) {
        ui.userResultList->clear();
        for (const auto& v : users) {
            QJsonObject obj = v.toObject();
            QString name = obj["username"].toString();
            QString uuid = obj["useruuid"].toString();
            QString fs = obj["friendshipstatus"].toString();
            QString tag;
            if (fs == "accepted") tag = " [已是好友]";
            else if (fs == "pending") tag = " [已发送请求]";
            else if (fs == "blocked") tag = " [已拉黑]";
            QListWidgetItem* item = new QListWidgetItem(QString("[用户] %1%2").arg(name, tag));
            // 存储完整的搜索结果对象，以便发送请求时使用
            obj["_type"] = "user";
            item->setData(Qt::UserRole, QJsonDocument(obj).toJson(QJsonDocument::Compact));
            item->setData(Qt::UserRole + 1, uuid);
            ui.userResultList->addItem(item);
        }
        if (users.isEmpty()) {
            ui.userResultList->addItem(new QListWidgetItem("未找到用户"));
        }
    });

    connect(http, &HttpApiClient::groupsSearched, this, [this](const QJsonArray& groups) {
        ui.groupResultList->clear();
        for (const auto& v : groups) {
            QJsonObject obj = v.toObject();
            QString name = obj["name"].toString();
            QString uuid = obj["uuid"].toString();
            bool joined = obj["isjoined"].toBool();
            QString tag = joined ? " [已加入]" : "";
            QListWidgetItem* item = new QListWidgetItem(QString("[群聊] %1%2").arg(name, tag));
            obj["_type"] = "group";
            item->setData(Qt::UserRole, QJsonDocument(obj).toJson(QJsonDocument::Compact));
            item->setData(Qt::UserRole + 1, uuid);
            ui.groupResultList->addItem(item);
        }
        if (groups.isEmpty()) {
            ui.groupResultList->addItem(new QListWidgetItem("未找到群聊"));
        }
    });
}

void SearchDialog::onSearchClicked()
{
    QString keyword = ui.searchInput->text().trimmed();
    if (keyword.isEmpty()) {
        QMessageBox::warning(this, "提示", "请输入搜索关键词");
        return;
    }
    ui.userResultList->clear();
    ui.groupResultList->clear();
    ui.actionBtn->setEnabled(false);

    auto* http = HttpApiClient::instance();
    if (ui.tabWidget->currentIndex() == 0) {
        http->searchUsers(keyword);
    } else {
        http->searchGroups(keyword);
    }
}

void SearchDialog::onUserResultSelected()
{
    auto* item = ui.userResultList->currentItem();
    if (!item) return;
    QJsonObject obj = QJsonDocument::fromJson(item->data(Qt::UserRole).toByteArray()).object();
    m_selectedUser = obj;
    m_selectedGroup = QJsonObject();
    ui.actionBtn->setEnabled(true);
}

void SearchDialog::onGroupResultSelected()
{
    auto* item = ui.groupResultList->currentItem();
    if (!item) return;
    QJsonObject obj = QJsonDocument::fromJson(item->data(Qt::UserRole).toByteArray()).object();
    m_selectedGroup = obj;
    m_selectedUser = QJsonObject();
    ui.actionBtn->setEnabled(true);
}

void SearchDialog::onActionClicked()
{
    if (!m_selectedUser.isEmpty()) {
        QString uuid = m_selectedUser["useruuid"].toString();
        QString name = m_selectedUser["username"].toString();
        QString fs = m_selectedUser["friendshipstatus"].toString();

        if (fs == "accepted") {
            QMessageBox::information(this, "提示", "你们已经是好友了");
            return;
        }
        if (fs == "pending") {
            QMessageBox::information(this, "提示", "好友请求已发送，请等待对方处理");
            return;
        }

        // 发送好友请求子对话框
        QDialog* dlg = new QDialog(this);
        dlg->setWindowTitle("发送好友请求");
        dlg->setFixedSize(400, 230);
        dlg->setWindowIcon(QIcon(":/chat.svg"));
        dlg->setStyleSheet("QDialog { background: #FFFFFF; } QLabel { color: #333; }");
        auto* layout = new QVBoxLayout(dlg);

        layout->addWidget(new QLabel(QString("向 %1 发送好友请求").arg(name)));
        layout->addWidget(new QLabel("请求消息:"));
        auto* msgEdit = new QTextEdit(dlg);
        msgEdit->setMaximumHeight(100);
        msgEdit->setPlaceholderText("写一段好友请求消息...");
        msgEdit->setStyleSheet("QTextEdit { border: 1px solid #A8D8B9; border-radius: 4px; background: #F5FAF7; }");
        layout->addWidget(msgEdit);

        auto* btnRow = new QHBoxLayout();
        auto* cancelBtn = new QPushButton("取消", dlg);
        cancelBtn->setStyleSheet("QPushButton { border: 1px solid #A8D8B9; border-radius: 4px; padding: 6px 16px; color: #333; background: white; } QPushButton:hover { background: #7CB894; color: white; }");
        auto* sendBtn = new QPushButton("发送请求", dlg);
        sendBtn->setStyleSheet("QPushButton { background: #A8D8B9; color: #333; border: none; border-radius: 4px; padding: 6px 16px; font-weight: bold; } QPushButton:hover { background: #7CB894; color: white; }");
        btnRow->addStretch();
        btnRow->addWidget(cancelBtn);
        btnRow->addWidget(sendBtn);
        layout->addLayout(btnRow);

        connect(cancelBtn, &QPushButton::clicked, dlg, &QDialog::reject);
        connect(sendBtn, &QPushButton::clicked, [this, dlg, uuid, msgEdit]() {
            QString msg = msgEdit->toPlainText().trimmed();
            if (msg.isEmpty()) msg = QString::fromUtf8("你好，我想加你为好友。");
            WebSocketClient::instance()->sendFriendRequest(uuid, msg);
            dlg->accept();
            accept();
        });
        dlg->exec();
    }
    else if (!m_selectedGroup.isEmpty()) {
        QString uuid = m_selectedGroup["uuid"].toString();
        QString name = m_selectedGroup["name"].toString();
        bool joined = m_selectedGroup["isjoined"].toBool();

        if (joined) {
            QMessageBox::information(this, "提示", "你已经加入该群聊");
            return;
        }

        // 发送入群请求子对话框
        QDialog* dlg = new QDialog(this);
        dlg->setWindowTitle("发送入群请求");
        dlg->setFixedSize(400, 250);
        dlg->setWindowIcon(QIcon(":/chat.svg"));
        dlg->setStyleSheet("QDialog { background: #FFFFFF; } QLabel { color: #333; }");
        auto* layout = new QVBoxLayout(dlg);

        layout->addWidget(new QLabel(QString("群聊: %1").arg(name)));
        layout->addWidget(new QLabel("入群请求消息:"));
        auto* msgEdit = new QTextEdit(dlg);
        msgEdit->setMaximumHeight(100);
        msgEdit->setPlaceholderText("写一段入群请求消息...");
        msgEdit->setStyleSheet("QTextEdit { border: 1px solid #A8D8B9; border-radius: 4px; background: #F5FAF7; }");
        layout->addWidget(msgEdit);

        auto* btnRow = new QHBoxLayout();
        auto* cancelBtn = new QPushButton("取消", dlg);
        cancelBtn->setStyleSheet("QPushButton { border: 1px solid #A8D8B9; border-radius: 4px; padding: 6px 16px; color: #333; background: white; } QPushButton:hover { background: #7CB894; color: white; }");
        auto* sendBtn = new QPushButton("发送请求", dlg);
        sendBtn->setStyleSheet("QPushButton { background: #A8D8B9; color: #333; border: none; border-radius: 4px; padding: 6px 16px; font-weight: bold; } QPushButton:hover { background: #7CB894; color: white; }");
        btnRow->addStretch();
        btnRow->addWidget(cancelBtn);
        btnRow->addWidget(sendBtn);
        layout->addLayout(btnRow);

        connect(cancelBtn, &QPushButton::clicked, dlg, &QDialog::reject);
        connect(sendBtn, &QPushButton::clicked, [this, dlg, uuid, msgEdit]() {
            QString msg = msgEdit->toPlainText().trimmed();
            if (msg.isEmpty()) msg = QString::fromUtf8("我想加入这个群聊。");
            WebSocketClient::instance()->sendGroupRequest(uuid, msg);
            dlg->accept();
            accept();
        });
        dlg->exec();
    }
}
