#pragma once

#include "../global.h"

class InfoDialog : public QDialog
{
    Q_OBJECT

public:
    // 查看用户详情（非好友）
    static void showUserInfo(QWidget* parent, const QJsonObject& user, bool isSelf);
    // 查看好友信息（好友）
    static void showFriendInfo(QWidget* parent, const QJsonObject& user, bool isMuted = false);
    // 查看群组详情
    static void showGroupInfo(QWidget* parent, const QJsonObject& group, bool isMuted = false);
};
