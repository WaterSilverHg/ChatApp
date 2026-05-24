#pragma once

#include "../global.h"
#include "ui_SearchDialog.h"

class SearchDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SearchDialog(QWidget* parent = nullptr);

signals:
    void sendFriendRequest(const QString& userUuid, const QString& message);
    void sendGroupRequest(const QString& groupUuid, const QString& message);

private slots:
    void onSearchClicked();
    void onActionClicked();
    void onUserResultSelected();
    void onGroupResultSelected();

private:
    Ui::SearchDialogClass ui;
    QJsonObject m_selectedUser;
    QJsonObject m_selectedGroup;
};
