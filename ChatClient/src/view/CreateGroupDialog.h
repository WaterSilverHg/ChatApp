#pragma once

#include "../global.h"
#include <QDialog>
#include <QJsonArray>

namespace Ui {
class CreateGroupDialog;
}

class CreateGroupDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CreateGroupDialog(const QJsonArray& friendsList, QWidget* parent = nullptr);
    ~CreateGroupDialog();

signals:
    void groupCreated(const QJsonObject& group);

private slots:
    void onSearchUsers();
    void onUserSelected(QListWidgetItem* item);
    void onCreateClicked();
    void onSelectedItemDoubleClicked(QListWidgetItem* item);

private:
    Ui::CreateGroupDialog* ui;
    QJsonArray m_selectedMembers;
    QJsonArray m_friendsList;
};