#pragma once

#include "../global.h"
#include "ui_RequestDialog.h"

class RequestDialog : public QDialog
{
    Q_OBJECT

public:
    explicit RequestDialog(QWidget* parent = nullptr);
    void setFriendRequests(const QJsonArray& requests);
    void setGroupRequests(const QJsonArray& requests);

private:
    void buildFriendRequestItem(const QJsonObject& req, QListWidget* list);
    void buildGroupRequestItem(const QJsonObject& req, QListWidget* list);

    Ui::RequestDialogClass ui;
};
