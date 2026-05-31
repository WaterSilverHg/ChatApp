#pragma once

#include "../global.h"
#include "ui_SearchDialog.h"

class SearchDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SearchDialog(QWidget* parent = nullptr);
    explicit SearchDialog(const QJsonArray& friendsList, QWidget* parent = nullptr);

signals:
    void userClicked(const QString& userUuid);
    void groupClicked(const QString& groupUuid);
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
    QJsonArray m_friendsList;
    bool m_friendsOnlyMode = false;
};
