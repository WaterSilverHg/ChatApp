#include "CreateGroupDialog.h"
#include "ui_CreateGroupDialog.h"
#include "../model/HttpApiClient.h"
#include "../model/WebSocketClient.h"
#include <QMessageBox>
#include <QJsonObject>
#include <QJsonDocument>

CreateGroupDialog::CreateGroupDialog(const QJsonArray& friendsList, QWidget* parent)
    : QDialog(parent),
      ui(new Ui::CreateGroupDialog),
      m_friendsList(friendsList)
{
    ui->setupUi(this);
    setWindowTitle(QString::fromUtf8("创建群聊"));
    setWindowIcon(QIcon(":/chat.svg"));
    setFixedSize(420, 520);

    connect(ui->searchBtn, &QPushButton::clicked, this, &CreateGroupDialog::onSearchUsers);
    connect(ui->searchResultList, &QListWidget::itemDoubleClicked, this, &CreateGroupDialog::onUserSelected);
    connect(ui->selectedList, &QListWidget::itemDoubleClicked, this, &CreateGroupDialog::onSelectedItemDoubleClicked);
    connect(ui->createBtn, &QPushButton::clicked, this, &CreateGroupDialog::onCreateClicked);
    connect(ui->cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    
    connect(ui->nameInput, &QLineEdit::textChanged, this, [this]() {
        ui->createBtn->setEnabled(!m_selectedMembers.isEmpty() && !ui->nameInput->text().trimmed().isEmpty());
    });

    connect(WebSocketClient::instance(), &WebSocketClient::groupCreated, this, [this](const QJsonObject& group) {
        QMessageBox::information(this, QString::fromUtf8("提示"), QString::fromUtf8("群聊创建成功!"));
        emit groupCreated(group);
        accept();
    });

    onSearchUsers();
}

CreateGroupDialog::~CreateGroupDialog()
{
    delete ui;
}

void CreateGroupDialog::onSearchUsers()
{
    QString keyword = ui->searchInput->text().trimmed().toLower();
    
    ui->searchResultList->clear();
    
    for (const auto& friendObj : m_friendsList) {
        QJsonObject obj = friendObj.toObject();
        QString name = obj["username"].toString();
        QString uuid = obj["frienduuid"].toString();
        QString remark = obj["remark"].toString();
        
        QString displayName = remark.isEmpty() ? name : remark;
        
        if (keyword.isEmpty() || displayName.toLower().contains(keyword) || name.toLower().contains(keyword)) {
            bool alreadySelected = false;
            for (const auto& m : m_selectedMembers) {
                if (m.toObject()["frienduuid"].toString() == uuid) {
                    alreadySelected = true;
                    break;
                }
            }
            if (alreadySelected) continue;
            
            QString displayText = displayName;
            if (!remark.isEmpty()) {
                displayText = QString("%1 (%2)").arg(remark, name);
            }
            
            auto* item = new QListWidgetItem(displayText);
            item->setData(Qt::UserRole, QJsonDocument(obj).toJson(QJsonDocument::Compact));
            ui->searchResultList->addItem(item);
        }
    }
    
    if (ui->searchResultList->count() == 0 && !keyword.isEmpty()) {
        ui->searchResultList->addItem(new QListWidgetItem(QString::fromUtf8("未找到好友或已全部添加")));
    }
}

void CreateGroupDialog::onUserSelected(QListWidgetItem* item)
{
    if (!item) return;
    QJsonObject obj = QJsonDocument::fromJson(item->data(Qt::UserRole).toByteArray()).object();
    QString uuid = obj["frienduuid"].toString();
    QString name = obj["username"].toString();

    m_selectedMembers.append(obj);

    auto* newItem = new QListWidgetItem(QString("%1 [双击移除]").arg(name));
    newItem->setData(Qt::UserRole, uuid);
    ui->selectedList->addItem(newItem);

    ui->createBtn->setEnabled(!m_selectedMembers.isEmpty() && !ui->nameInput->text().trimmed().isEmpty());
    item->setHidden(true);
}

void CreateGroupDialog::onSelectedItemDoubleClicked(QListWidgetItem* item)
{
    if (!item) return;
    QString uuid = item->data(Qt::UserRole).toString();

    for (int i = 0; i < m_selectedMembers.size(); ++i) {
        if (m_selectedMembers[i].toObject()["frienduuid"].toString() == uuid) {
            m_selectedMembers.removeAt(i);
            break;
        }
    }

    delete ui->selectedList->takeItem(ui->selectedList->row(item));
    ui->createBtn->setEnabled(!m_selectedMembers.isEmpty() && !ui->nameInput->text().trimmed().isEmpty());

    for (int i = 0; i < ui->searchResultList->count(); ++i) {
        auto* searchItem = ui->searchResultList->item(i);
        QJsonObject searchObj = QJsonDocument::fromJson(searchItem->data(Qt::UserRole).toByteArray()).object();
        if (searchObj["frienduuid"].toString() == uuid) {
            searchItem->setHidden(false);
            break;
        }
    }
}

void CreateGroupDialog::onCreateClicked()
{
    QString name = ui->nameInput->text().trimmed();
    if (name.isEmpty()) {
        QMessageBox::warning(this, QString::fromUtf8("提示"), QString::fromUtf8("请输入群名称"));
        return;
    }

    if (m_selectedMembers.isEmpty()) {
        QMessageBox::warning(this, QString::fromUtf8("提示"), QString::fromUtf8("请至少添加一个成员"));
        return;
    }

    QString description = ui->descInput->toPlainText().trimmed();
    QJsonArray memberUuids;
    for (const auto& m : m_selectedMembers) {
        memberUuids.append(m.toObject()["frienduuid"].toString());
    }

    WebSocketClient::instance()->createGroup(name, description, "", memberUuids, 500, true);
}