#include "AddGroupMembersDialog.h"
#include "ui_AddGroupMembersDialog.h"
#include "../model/WebSocketClient.h"
#include <QMessageBox>
#include <QInputDialog>

AddGroupMembersDialog::AddGroupMembersDialog(const QString& groupUuid, const QJsonArray& friendsList, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::AddGroupMembersDialog)
    , m_groupUuid(groupUuid)
    , m_friendsList(friendsList)
{
    ui->setupUi(this);
    setWindowTitle(QString::fromUtf8("添加群成员"));
    setFixedSize(500, 500);

    ui->friendsListWidget->setStyleSheet("QListWidget { border: 1px solid #A8D8B9; border-radius: 4px; background: white; } QListWidget::item { padding: 8px; border-bottom: 1px solid #e0e8e3; } QListWidget::item:selected { background: #E8F5E9; }");
    ui->searchInput->setStyleSheet("QLineEdit { border: 1px solid #A8D8B9; border-radius: 4px; padding: 6px; }");
    ui->addButton->setStyleSheet("QPushButton { background: #A8D8B9; color: #333; border: none; border-radius: 4px; padding: 8px; } QPushButton:hover { background: #7CB894; color: white; }");
    ui->cancelButton->setStyleSheet("QPushButton { background: #E0E0E0; color: #333; border: none; border-radius: 4px; padding: 8px; } QPushButton:hover { background: #BDBDBD; }");

    for (const auto& friendData : friendsList) {
        QJsonObject friendObj = friendData.toObject();
        QString username = friendObj["username"].toString();
        QString friendUuid = friendObj["frienduuid"].toString();

        auto* item = new QListWidgetItem(username);
        item->setData(Qt::UserRole, friendUuid);
        item->setData(Qt::UserRole + 1, username);
        ui->friendsListWidget->addItem(item);
    }

    connect(ui->searchInput, &QLineEdit::textChanged, this, &AddGroupMembersDialog::onSearchUsers);
    connect(ui->friendsListWidget, &QListWidget::itemSelectionChanged, this, &AddGroupMembersDialog::updateSelectedCount);
    connect(ui->addButton, &QPushButton::clicked, this, &AddGroupMembersDialog::onAddClicked);
    connect(ui->cancelButton, &QPushButton::clicked, this, &QDialog::reject);
}

AddGroupMembersDialog::~AddGroupMembersDialog()
{
    delete ui;
}

void AddGroupMembersDialog::onSearchUsers()
{
    QString searchText = ui->searchInput->text().trimmed().toLower();
    for (int i = 0; i < ui->friendsListWidget->count(); ++i) {
        QListWidgetItem* item = ui->friendsListWidget->item(i);
        QString username = item->text().toLower();
        item->setHidden(!username.contains(searchText));
    }
}

void AddGroupMembersDialog::onUserSelected(QListWidgetItem* item)
{
    if (!item) return;

    QString friendUuid = item->data(Qt::UserRole).toString();
    QString username = item->data(Qt::UserRole + 1).toString();

    bool alreadySelected = false;
    for (const auto& m : m_selectedMembers) {
        if (m.toObject()["frienduuid"].toString() == friendUuid) {
            alreadySelected = true;
            break;
        }
    }

    if (!alreadySelected) {
        QJsonObject member;
        member["frienduuid"] = friendUuid;
        member["username"] = username;
        m_selectedMembers.append(member);
    }

    updateSelectedCount();
}

void AddGroupMembersDialog::onAddClicked()
{
    QList<QListWidgetItem*> selectedItems = ui->friendsListWidget->selectedItems();
    if (selectedItems.isEmpty()) {
        QMessageBox::warning(this, QString::fromUtf8("提示"), QString::fromUtf8("请先选择要添加的好友"));
        return;
    }

    m_selectedMembers = QJsonArray();
    for (auto* item : selectedItems) {
        QJsonObject member;
        member["frienduuid"] = item->data(Qt::UserRole).toString();
        member["username"] = item->data(Qt::UserRole + 1).toString();
        m_selectedMembers.append(member);
    }

    QJsonArray memberUuids;
    for (const auto& m : m_selectedMembers) {
        memberUuids.append(m.toObject()["frienduuid"].toString());
    }

    WebSocketClient::instance()->addGroupMembers(m_groupUuid, memberUuids);
    QMessageBox::information(this, QString::fromUtf8("提示"), QString::fromUtf8("已发送添加成员请求"));
    accept();
}

void AddGroupMembersDialog::updateSelectedCount()
{
    int count = ui->friendsListWidget->selectedItems().size();
    ui->selectedCountLabel->setText(QString::fromUtf8("已选择: %1 人").arg(count));
}
