#pragma once

#include "../global.h"
#include <functional>

class InfoDialog : public QDialog
{
    Q_OBJECT

public:
    InfoDialog(QWidget* parent = nullptr) : QDialog(parent) {}
    
    static InfoDialog* showUserInfo(QWidget* parent, const QJsonObject& user, bool isSelf);
    static InfoDialog* showFriendInfo(QWidget* parent, const QJsonObject& user, bool isMuted = false);
    static InfoDialog* showGroupInfo(QWidget* parent, const QJsonObject& group, const QJsonArray& members,
                              const QString& currentUserUuid, const QJsonArray& friendsList = QJsonArray(),
                              bool isMuted = false);
    static InfoDialog* showUnjoinedGroupInfo(QWidget* parent, const QJsonObject& group);

signals:
    void userProfileUpdated(const QJsonObject& user);
    void groupInfoUpdated(const QJsonObject& group);
};