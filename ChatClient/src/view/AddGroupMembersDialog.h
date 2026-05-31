#pragma once

#include "../global.h"
#include <QDialog>
#include <QJsonArray>

namespace Ui {
class AddGroupMembersDialog;
}

class AddGroupMembersDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddGroupMembersDialog(const QString& groupUuid, const QJsonArray& friendsList, QWidget* parent = nullptr);
    ~AddGroupMembersDialog();

private slots:
    void onSearchUsers();
    void onUserSelected(QListWidgetItem* item);
    void onAddClicked();

private:
    void updateSelectedCount();

    Ui::AddGroupMembersDialog* ui;
    QString m_groupUuid;
    QJsonArray m_selectedMembers;
    QJsonArray m_friendsList;
};