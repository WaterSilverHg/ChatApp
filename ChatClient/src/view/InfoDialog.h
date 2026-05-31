#pragma once

#include "../global.h"
#include <functional>

class InfoDialog : public QDialog
{
    Q_OBJECT

public:
    static void showUserInfo(QWidget* parent, const QJsonObject& user, bool isSelf);
    static void showFriendInfo(QWidget* parent, const QJsonObject& user, bool isMuted = false);
    static void showGroupInfo(QWidget* parent, const QJsonObject& group, const QJsonArray& members,
                              const QString& currentUserUuid, const QJsonArray& friendsList = QJsonArray(),
                              bool isMuted = false);
    static void showUnjoinedGroupInfo(QWidget* parent, const QJsonObject& group);
};
