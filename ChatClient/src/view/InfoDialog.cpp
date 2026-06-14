#include "InfoDialog.h"
#include "../model/HttpApiClient.h"
#include "../model/WebSocketClient.h"
#include "../model/AsyncAvatarLoader.h"
#include "CustomDialog.h"
#include "AddGroupMembersDialog.h"
#include <QPainter>
#include <functional>

static QLabel* makeAvatar(QWidget* parent, const QString& url, const QString& name) {
    auto* label = new QLabel(parent);
    label->setAlignment(Qt::AlignCenter);
    label->setFixedSize(100, 100);
    QPixmap placeholder(100, 100);
    placeholder.fill(QColor("#A8D8B9"));
    QPainter painter(&placeholder);
    painter.setPen(QColor("#333"));
    QFont f = painter.font();
    f.setPixelSize(36);
    painter.setFont(f);
    if (!name.isEmpty())
        painter.drawText(placeholder.rect(), Qt::AlignCenter, name.left(1).toUpper());
    painter.end();
    label->setPixmap(placeholder);
    if (!url.isEmpty()) {
        auto* loader = new AsyncAvatarLoader(label);
        QObject::connect(loader, &AsyncAvatarLoader::avatarReady, label,
            [label](const QString&, QPixmap pix) {
                if (!pix.isNull()) label->setPixmap(pix.scaled(100, 100, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            });
        loader->load(url, name, QSize(100, 100));
    }
    return label;
}

static QPushButton* makeBtn(const QString& text, bool primary = true) {
    auto* btn = new QPushButton(text);
    if (primary)
        btn->setStyleSheet("QPushButton { background: #A8D8B9; color: #333; border: none; border-radius: 4px; padding: 8px 16px; font-weight: bold; } QPushButton:hover { background: #7CB894; color: white; }");
    else
        btn->setStyleSheet("QPushButton { border: 1px solid #A8D8B9; border-radius: 4px; padding: 8px 16px; color: #333; background: white; } QPushButton:hover { background: #7CB894; color: white; }");
    return btn;
}

static QLabel* makeCenterLabel(const QString& text) {
    auto* label = new QLabel(text);
    label->setAlignment(Qt::AlignCenter);
    return label;
}

InfoDialog* InfoDialog::showUserInfo(QWidget* parent, const QJsonObject& user, bool isSelf)
{
    InfoDialog* dlg = new InfoDialog(parent);
    dlg->setWindowTitle(isSelf ? QString::fromUtf8("个人资料") : QString::fromUtf8("用户详情"));
    dlg->setFixedSize(350, isSelf ? 380 : 280);
    dlg->setWindowIcon(QIcon(":/chat.svg"));
    dlg->setStyleSheet("QDialog { background: #FFFFFF; } QLabel { color: #333; font-size: 13px; }");

    auto* layout = new QVBoxLayout(dlg);
    layout->setSpacing(8);
    layout->setAlignment(Qt::AlignCenter);

    QString name = user["username"].toString();
    QString email = user["email"].toString();
    QString avatarUrl = user["avatarurl"].toString();
    QString status = user["status"].toString();
    QString lastSeen = user["lastseen"].toString();

    layout->addWidget(makeAvatar(dlg, avatarUrl, name), 0, Qt::AlignCenter);

    if (!email.isEmpty())
        layout->addWidget(makeCenterLabel(QString("邮箱: %1").arg(email)));
    layout->addWidget(makeCenterLabel(QString::fromUtf8("状态: %1").arg(status.isEmpty() ? "offline" : status)));
    if (!lastSeen.isEmpty())
        layout->addWidget(makeCenterLabel(QString::fromUtf8("最后在线: %1").arg(lastSeen)));

    if (isSelf) {
        QLabel* nameLabel = makeCenterLabel(QString::fromUtf8("用户名: %1").arg(name));
        layout->addWidget(nameLabel, 0, Qt::AlignCenter);
        layout->addStretch();
        auto* editBtn = makeBtn(QString::fromUtf8("修改资料"));
        editBtn->setFixedWidth(120);
        layout->addWidget(editBtn, 0, Qt::AlignCenter);
        QObject::connect(editBtn, &QPushButton::clicked, [dlg, user, nameLabel]() {
            QDialog* editDlg = new QDialog(dlg);
            editDlg->setWindowTitle(QString::fromUtf8("修改个人资料"));
            editDlg->setFixedSize(320, 200);
            editDlg->setStyleSheet("QDialog { background: #FFFFFF; } QLabel { color: #333; font-size: 13px; }");

            auto* editLayout = new QVBoxLayout(editDlg);
            editLayout->setSpacing(12);

            auto* label = new QLabel(QString::fromUtf8("用户名:"));
            editLayout->addWidget(label);

            auto* nameEdit = new QLineEdit(editDlg);
            nameEdit->setText(user["username"].toString());
            nameEdit->setStyleSheet("QLineEdit { border: 1px solid #A8D8B9; border-radius: 4px; padding: 6px; }");
            editLayout->addWidget(nameEdit);

            auto* btnLayout = new QHBoxLayout();
            auto* saveBtn = makeBtn(QString::fromUtf8("保存"));
            saveBtn->setFixedWidth(100);
            btnLayout->addWidget(saveBtn);
            auto* cancelBtn = makeBtn(QString::fromUtf8("取消"), false);
            cancelBtn->setFixedWidth(100);
            btnLayout->addWidget(cancelBtn);
            editLayout->addLayout(btnLayout);

            QObject::connect(saveBtn, &QPushButton::clicked, [editDlg, dlg, nameEdit, nameLabel]() {
                QString newUsername = nameEdit->text().trimmed();
                if (newUsername.isEmpty()) {
                    QMessageBox::warning(editDlg, QString::fromUtf8("错误"), QString::fromUtf8("用户名不能为空"));
                    return;
                }
                HttpApiClient::instance()->updateProfile(newUsername, "",
                    [editDlg, dlg, newUsername, nameLabel](const QJsonDocument& doc) {
                        QJsonObject updatedUser = doc.isArray() ? doc.array().first().toObject() : doc.object();
                        if (updatedUser.isEmpty() && doc.object().contains("content")) {
                            updatedUser = QJsonDocument::fromJson(doc.object()["content"].toString().toUtf8()).object();
                        }
                        QMessageBox::information(editDlg, QString::fromUtf8("成功"), QString::fromUtf8("资料更新成功"));
                        nameLabel->setText(QString::fromUtf8("用户名: %1").arg(newUsername));
                        emit dlg->userProfileUpdated(updatedUser);
                        editDlg->accept();
                    },
                    [editDlg](const QString& error, int code) {
                        QMessageBox::warning(editDlg, QString::fromUtf8("错误"), error);
                    });
            });
            QObject::connect(cancelBtn, &QPushButton::clicked, editDlg, &QDialog::reject);

            editDlg->exec();
        });

        auto* uploadBtn = makeBtn(QString::fromUtf8("上传头像"));
        uploadBtn->setFixedWidth(120);
        layout->addWidget(uploadBtn, 0, Qt::AlignCenter);
        QObject::connect(uploadBtn, &QPushButton::clicked, [dlg]() {
            QString path = QFileDialog::getOpenFileName(dlg, QString::fromUtf8("选择图片"), "",
                "Images (*.png *.jpg *.jpeg *.gif *.bmp)");
            if (path.isEmpty()) return;
            QFile file(path);
            if (!file.open(QIODevice::ReadOnly)) {
                QMessageBox::warning(dlg, QString::fromUtf8("错误"), QString::fromUtf8("无法打开文件"));
                return;
            }
            QByteArray data = file.readAll();
            file.close();
            QFileInfo fi(path);
            QMimeDatabase mimeDb;
            QMimeType mime = mimeDb.mimeTypeForFile(fi);
            HttpApiClient::instance()->uploadUserAvatarFile(fi.fileName(), mime.name(), fi.size(), data,
                [dlg](const QJsonDocument& doc) {
                    QJsonObject fileInfo = doc.object();
                    if (fileInfo.contains("content")) {
                        fileInfo = QJsonDocument::fromJson(fileInfo["content"].toString().toUtf8()).object();
                    }
                    QMessageBox::information(dlg, QString::fromUtf8("成功"), QString::fromUtf8("头像上传成功"));
                },
                [dlg](const QString& error, int code) {
                    QMessageBox::warning(dlg, QString::fromUtf8("错误"), error);
                });
            dlg->accept();
        });
    } else {
        layout->addWidget(makeCenterLabel(QString::fromUtf8("用户名: %1").arg(name)));
    }

    auto* closeBtn = makeBtn(QString::fromUtf8("关闭"), false);
    closeBtn->setFixedWidth(120);
    layout->addWidget(closeBtn, 0, Qt::AlignCenter);
    QObject::connect(closeBtn, &QPushButton::clicked, dlg, &QDialog::accept);

    dlg->show();
    return dlg;
}

InfoDialog* InfoDialog::showFriendInfo(QWidget* parent, const QJsonObject& user, bool isMuted)
{
    InfoDialog* dlg = new InfoDialog(parent);
    dlg->setWindowTitle(QString::fromUtf8("好友信息"));
    dlg->setFixedSize(350, 520);
    dlg->setWindowIcon(QIcon(":/chat.svg"));
    dlg->setStyleSheet("QDialog { background: #FFFFFF; } QLabel { color: #333; font-size: 13px; }");

    auto* layout = new QVBoxLayout(dlg);
    layout->setSpacing(8);
    layout->setAlignment(Qt::AlignCenter);

    QString name = user["username"].toString();
    QString uuid = user["useruuid"].toString();
    QString email = user["email"].toString();
    QString avatarUrl = user["avatarurl"].toString();
    QString status = user["status"].toString();
    QString lastSeen = user["lastseen"].toString();
    QString friendGroup = user["friendgroup"].toString();
    QString remark = user["remark"].toString();
    QString displayName = remark.isEmpty() ? name : QString("%1(%2)").arg(remark).arg(name);

    layout->addWidget(makeAvatar(dlg, avatarUrl, remark.isEmpty() ? name : remark), 0, Qt::AlignCenter);
    layout->addWidget(makeCenterLabel(QString::fromUtf8("用户名: %1").arg(displayName)));
    if (!email.isEmpty())
        layout->addWidget(makeCenterLabel(QString("邮箱: %1").arg(email)));
    if (!friendGroup.isEmpty())
        layout->addWidget(makeCenterLabel(QString::fromUtf8("分组: %1").arg(friendGroup)));
    layout->addWidget(makeCenterLabel(QString::fromUtf8("状态: %1").arg(status.isEmpty() ? "offline" : status)));
    if (!lastSeen.isEmpty())
        layout->addWidget(makeCenterLabel(QString::fromUtf8("最后在线: %1").arg(lastSeen)));

    auto* remarkRow = new QHBoxLayout();
    remarkRow->setAlignment(Qt::AlignCenter);
    remarkRow->addWidget(new QLabel(QString::fromUtf8("备注:")));
    auto* remarkEdit = new QLineEdit(dlg);
    remarkEdit->setText(remark);
    remarkEdit->setPlaceholderText(QString::fromUtf8("设置备注..."));
    remarkEdit->setStyleSheet("border: 1px solid #A8D8B9; border-radius: 4px; padding: 6px; background: #F5FAF7;");
    remarkRow->addWidget(remarkEdit);
    auto* remarkBtn = new QPushButton(QString::fromUtf8("保存"));
    remarkBtn->setStyleSheet("QPushButton { background: #A8D8B9; color: #333; border: none; border-radius: 4px; padding: 6px 12px; } QPushButton:hover { background: #7CB894; color: white; }");
    QObject::connect(remarkBtn, &QPushButton::clicked, [uuid, remarkEdit, dlg]() {
        QString remark = remarkEdit->text().trimmed();
        HttpApiClient::instance()->updateFriendRemark(uuid, remark,
            [dlg](const QJsonDocument& doc) {
                QMessageBox::information(dlg, QString::fromUtf8("提示"), QString::fromUtf8("备注已更新"));
            },
            [dlg](const QString& error, int code) {
                QMessageBox::warning(dlg, QString::fromUtf8("错误"), error);
            });
    });
    remarkRow->addWidget(remarkBtn);
    layout->addLayout(remarkRow);

    auto* muteBtn = new QPushButton(isMuted ? QString::fromUtf8("取消免打扰") : QString::fromUtf8("消息免打扰"));
    muteBtn->setStyleSheet("QPushButton { background: #A8D8B9; color: #333; border: none; border-radius: 4px; padding: 8px; } QPushButton:hover { background: #7CB894; color: white; }");
    muteBtn->setFixedWidth(140);
    bool muted = isMuted;
    QObject::connect(muteBtn, &QPushButton::clicked, [muteBtn, uuid, &muted, dlg]() {
        if (muted) {
            HttpApiClient::instance()->unmutePrivateConversation(uuid,
                [muteBtn, &muted](const QJsonDocument& doc) {
                    muteBtn->setText(QString::fromUtf8("消息免打扰"));
                    muted = false;
                },
                [dlg](const QString& error, int code) {
                    QMessageBox::warning(dlg, QString::fromUtf8("错误"), error);
                });
        } else {
            HttpApiClient::instance()->mutePrivateConversation(uuid,
                [muteBtn, &muted](const QJsonDocument& doc) {
                    muteBtn->setText(QString::fromUtf8("取消免打扰"));
                    muted = true;
                },
                [dlg](const QString& error, int code) {
                    QMessageBox::warning(dlg, QString::fromUtf8("错误"), error);
                });
        }
    });
    layout->addWidget(muteBtn, 0, Qt::AlignCenter);

    auto* blockBtn = makeBtn(QString::fromUtf8("拉黑用户"));
    blockBtn->setStyleSheet("QPushButton { background: #E88B8B; color: white; border: none; border-radius: 4px; padding: 8px 16px; font-weight: bold; } QPushButton:hover { background: #D07070; }");
    blockBtn->setFixedWidth(120);
    QObject::connect(blockBtn, &QPushButton::clicked, [dlg, uuid]() {
        if (CustomDialog::question(dlg, QString::fromUtf8("确认拉黑"), QString::fromUtf8("确定要拉黑此用户吗？"))) {
            HttpApiClient::instance()->blockUser(uuid,
                [dlg](const QJsonDocument& doc) {
                    dlg->accept();
                },
                [dlg](const QString& error, int code) {
                    QMessageBox::warning(dlg, QString::fromUtf8("错误"), error);
                });
        }
    });
    layout->addWidget(blockBtn, 0, Qt::AlignCenter);

    auto* closeBtn = makeBtn(QString::fromUtf8("关闭"), false);
    closeBtn->setFixedWidth(120);
    layout->addWidget(closeBtn, 0, Qt::AlignCenter);
    QObject::connect(closeBtn, &QPushButton::clicked, dlg, &QDialog::accept);

    dlg->show();
    return dlg;
}

InfoDialog* InfoDialog::showGroupInfo(QWidget* parent, const QJsonObject& group, const QJsonArray& members,
                                    const QString& currentUserUuid, const QJsonArray& friendsList,
                                    bool isMuted)
{
    InfoDialog* dlg = new InfoDialog(parent);
    dlg->setWindowTitle(QString::fromUtf8("群详情"));
    dlg->setFixedSize(420, 640);
    dlg->setWindowIcon(QIcon(":/chat.svg"));
    dlg->setStyleSheet("QDialog { background: #FFFFFF; } QLabel { color: #333; font-size: 13px; }");

    auto* layout = new QVBoxLayout(dlg);
    layout->setSpacing(8);
    layout->setAlignment(Qt::AlignCenter);

    QString name = group["name"].toString();
    QString uuid = group["uuid"].toString();
    QString avatarUrl = group["avatarurl"].toString();
    QString desc = group["description"].toString();
    int memberCount = group["membercount"].toInt();
    int maxMembers = group["maxmembers"].toInt(500);
    QString createdAt = group["createdat"].toString();

    QString currentUserRole = group["role"].toString();
    if (currentUserRole.isEmpty()) {
        for (const auto& m : members) {
            QJsonObject obj = m.toObject();
            if (obj["useruuid"].toString() == currentUserUuid) {
                currentUserRole = obj["role"].toString();
                break;
            }
        }
    }

    layout->addWidget(makeAvatar(dlg, avatarUrl, name), 0, Qt::AlignCenter);
    layout->addWidget(makeCenterLabel(QString::fromUtf8("群名称: %1").arg(name)));
    layout->addWidget(makeCenterLabel(QString::fromUtf8("成员数: %1/%2").arg(memberCount).arg(maxMembers)));
    if (!desc.isEmpty())
        layout->addWidget(makeCenterLabel(QString::fromUtf8("描述: %1").arg(desc)));
    if (!createdAt.isEmpty())
        layout->addWidget(makeCenterLabel(QString::fromUtf8("创建时间: %1").arg(createdAt)));

    auto* memberLabel = new QLabel(QString::fromUtf8("群成员:"));
    memberLabel->setStyleSheet("font-weight: bold; color: #333; margin-top: 8px;");
    memberLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(memberLabel);
    auto* memberList = new QListWidget(dlg);
    memberList->setFixedHeight(140);
    memberList->setStyleSheet("QListWidget { border: 1px solid #A8D8B9; border-radius: 4px; background: white; } QListWidget::item { padding: 6px; border-bottom: 1px solid #e0e8e3; }");
    layout->addWidget(memberList);

    for (const auto& m : members) {
        QJsonObject obj = m.toObject();
        QString username = obj["username"].toString();
        QString memberUuid = obj["useruuid"].toString();
        QString role = obj["role"].toString();

        QString displayText = username;
        if (role == "owner") {
            displayText = QString("%1 [群主]").arg(username);
        } else if (role == "admin") {
            displayText = QString("%1 [管理员]").arg(username);
        }

        auto* item = new QListWidgetItem(displayText);
        item->setData(Qt::UserRole, memberUuid);
        item->setData(Qt::UserRole + 1, username);
        item->setData(Qt::UserRole + 2, role);
        memberList->addItem(item);
    }

    bool isOwner = (currentUserRole == "owner");
    bool isAdmin = (currentUserRole == "admin");

    if (isOwner || isAdmin) {
        memberList->setContextMenuPolicy(Qt::CustomContextMenu);
        QObject::connect(memberList, &QListWidget::customContextMenuRequested, [memberList, dlged = dlg, uuid, parent, isOwner, isAdmin](const QPoint& pos) {
            auto* item = memberList->itemAt(pos);
            if (!item) return;

            QString memberUuid = item->data(Qt::UserRole).toString();
            QString memberName = item->data(Qt::UserRole + 1).toString();
            QString memberRole = item->data(Qt::UserRole + 2).toString();
            if (memberRole == "owner") return;

            QMenu menu(dlged);
            if (isOwner && memberRole == "member") {
                auto* promoteAct = menu.addAction(QString::fromUtf8("提权为管理员"));
                QObject::connect(promoteAct, &QAction::triggered, [dlged, uuid, memberUuid, memberName]() {
                    if (QMessageBox::question(dlged, QString::fromUtf8("确认"), QString::fromUtf8("确定要将 %1 提为管理员吗？").arg(memberName)) == QMessageBox::Yes) {
                        WebSocketClient::instance()->promoteGroupMember(uuid, memberUuid);
                        //QMessageBox::information(dlged, QString::fromUtf8("提示"), QString::fromUtf8("已发送提权请求"));
                    }
                });
            }
            if (isOwner && memberRole == "admin") {
                auto* demoteAct = menu.addAction(QString::fromUtf8("降权为普通成员"));
                QObject::connect(demoteAct, &QAction::triggered, [dlged, uuid, memberUuid, memberName]() {
                    if (QMessageBox::question(dlged, QString::fromUtf8("确认"), QString::fromUtf8("确定要将 %1 降为普通成员吗？").arg(memberName)) == QMessageBox::Yes) {
                        WebSocketClient::instance()->demoteGroupMember(uuid, memberUuid);
                        //QMessageBox::information(dlged, QString::fromUtf8("提示"), QString::fromUtf8("已发送降权请求"));
                    }
                });
            }
            auto* kickAct = menu.addAction(QString::fromUtf8("踢出群聊"));
            QObject::connect(kickAct, &QAction::triggered, [dlged, uuid, memberUuid, memberName]() {
                if (QMessageBox::question(dlged, QString::fromUtf8("确认"), QString::fromUtf8("确定要将 %1 踢出群聊吗？").arg(memberName)) == QMessageBox::Yes) {
                    WebSocketClient::instance()->removeGroupMember(uuid, memberUuid);
                    //QMessageBox::information(dlged, QString::fromUtf8("提示"), QString::fromUtf8("已发送踢出请求"));
                }
            });
            menu.exec(QCursor::pos());
        });

        memberList->setEditTriggers(QAbstractItemView::NoEditTriggers);
        QObject::connect(memberList, &QListWidget::itemDoubleClicked, [memberList, dlged = dlg](QListWidgetItem* item) {
            QString memberUuid = item->data(Qt::UserRole).toString();
            HttpApiClient::instance()->getUserInfo(memberUuid,
                [dlged](const QJsonDocument& doc) {
                    QJsonObject user = doc.object();
                    if (user.contains("content")) {
                        user = QJsonDocument::fromJson(user["content"].toString().toUtf8()).object();
                    }
                    InfoDialog::showUserInfo(dlged, user, false);
                },
                [dlged](const QString& error, int code) {
                    QMessageBox::warning(dlged, QString::fromUtf8("错误"), error);
                });
        });
    }

    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(10);

    auto* muteBtn = new QPushButton(isMuted ? QString::fromUtf8("取消免打扰") : QString::fromUtf8("消息免打扰"));
    muteBtn->setStyleSheet("QPushButton { background: #A8D8B9; color: #333; border: none; border-radius: 4px; padding: 8px; } QPushButton:hover { background: #7CB894; color: white; }");
    muteBtn->setFixedWidth(100);
    bool muted = isMuted;
    QObject::connect(muteBtn, &QPushButton::clicked, [muteBtn, uuid, &muted, dlg]() {
        if (muted) {
            HttpApiClient::instance()->unmuteGroupConversation(uuid,
                [muteBtn, &muted](const QJsonDocument& doc) {
                    muteBtn->setText(QString::fromUtf8("消息免打扰"));
                    muted = false;
                },
                [dlg](const QString& error, int code) {
                    QMessageBox::warning(dlg, QString::fromUtf8("错误"), error);
                });
        } else {
            HttpApiClient::instance()->muteGroupConversation(uuid,
                [muteBtn, &muted](const QJsonDocument& doc) {
                    muteBtn->setText(QString::fromUtf8("取消免打扰"));
                    muted = true;
                },
                [dlg](const QString& error, int code) {
                    QMessageBox::warning(dlg, QString::fromUtf8("错误"), error);
                });
        }
    });
    buttonLayout->addWidget(muteBtn);

    if (currentUserRole == "owner" || currentUserRole == "admin") {
        auto* addBtn = new QPushButton(QString::fromUtf8("添加成员"));
        addBtn->setStyleSheet("QPushButton { background: #A8D8B9; color: #333; border: none; border-radius: 4px; padding: 8px; } QPushButton:hover { background: #7CB894; color: white; }");
        addBtn->setFixedWidth(100);
        QObject::connect(addBtn, &QPushButton::clicked, [dlg, uuid, parent, friendsList]() {
            if (friendsList.isEmpty()) {
                QMessageBox::information(dlg, QString::fromUtf8("提示"), QString::fromUtf8("您还没有好友，无法添加成员"));
                return;
            }
            AddGroupMembersDialog addDlg(uuid, friendsList, dlg);
            addDlg.exec();
        });
        buttonLayout->addWidget(addBtn);
    }

    if (currentUserRole == "owner") {
        auto* editGroupBtn = new QPushButton(QString::fromUtf8("修改群信息"));
        editGroupBtn->setStyleSheet("QPushButton { background: #7CB894; color: white; border: none; border-radius: 4px; padding: 8px; } QPushButton:hover { background: #5A9E7A; }");
        editGroupBtn->setFixedWidth(100);
        QObject::connect(editGroupBtn, &QPushButton::clicked, [dlg, group, avatarUrl, members, currentUserUuid]() {
            QDialog* editDlg = new QDialog(dlg);
            editDlg->setWindowTitle(QString::fromUtf8("修改群信息"));
            editDlg->setFixedSize(350, 380);
            editDlg->setStyleSheet("QDialog { background: #FFFFFF; } QLabel { color: #333; font-size: 13px; }");
            
            auto* editLayout = new QVBoxLayout(editDlg);
            editLayout->setSpacing(12);
            
            // 头像上传区域
            auto* avatarLayout = new QHBoxLayout();
            avatarLayout->setSpacing(10);
            
            QLabel* avatarLabel = new QLabel(editDlg);
            avatarLabel->setFixedSize(60, 60);
            avatarLabel->setScaledContents(true);
            
            QString currentAvatarUrl = avatarUrl;
            QString localFilePath;
            
            // 加载当前头像
            auto loadAvatar = [avatarLabel](const QString& url) {
                QPixmap pix(url);
                if (pix.isNull()) {
                    pix = AsyncAvatarLoader::makePlaceholder("群头像", QSize(60, 60));
                } else {
                    pix = pix.scaled(60, 60, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                }
                avatarLabel->setPixmap(pix);
            };
            loadAvatar(currentAvatarUrl);
            
            // 只有群主才能上传群头像
            auto* uploadAvatarBtn = new QPushButton(QString::fromUtf8("上传群头像"), editDlg);
            uploadAvatarBtn->setStyleSheet("QPushButton { background: #A8D8B9; color: #333; border: none; border-radius: 4px; padding: 8px 16px; } QPushButton:hover { background: #7CB894; color: white; }");
            avatarLayout->addWidget(avatarLabel);
            avatarLayout->addWidget(uploadAvatarBtn);
            avatarLayout->addStretch();
            editLayout->addLayout(avatarLayout);
            
            // 获取当前用户角色，只有群主才能看到上传按钮
            QString editCurrentUserRole = group["role"].toString();
            if (editCurrentUserRole.isEmpty()) {
                for (const auto& m : members) {
                    QJsonObject obj = m.toObject();
                    if (obj["useruuid"].toString() == currentUserUuid) {
                        editCurrentUserRole = obj["role"].toString();
                        break;
                    }
                }
            }
            uploadAvatarBtn->setVisible(editCurrentUserRole == "owner");
            
            QObject::connect(uploadAvatarBtn, &QPushButton::clicked, [editDlg, avatarLabel, &currentAvatarUrl, &localFilePath, loadAvatar,group]() {
                QString fileName = QFileDialog::getOpenFileName(editDlg, QString::fromUtf8("选择头像图片"),
                    "", QString::fromUtf8("图片文件 (*.png *.jpg *.jpeg *.gif *.bmp)"));
                
                if (fileName.isEmpty()) return;
                
                QFile file(fileName);
                if (!file.open(QIODevice::ReadOnly)) {
                    QMessageBox::warning(editDlg, QString::fromUtf8("错误"), QString::fromUtf8("无法打开文件"));
                    return;
                }
                
                QByteArray fileData = file.readAll();
                qint64 fileSize = fileData.size();
                file.close();
                
                // 获取文件 MIME 类型
                QString mimeType = "image/png";
                if (fileName.endsWith(".jpg", Qt::CaseInsensitive) || fileName.endsWith(".jpeg", Qt::CaseInsensitive)) {
                    mimeType = "image/jpeg";
                } else if (fileName.endsWith(".gif", Qt::CaseInsensitive)) {
                    mimeType = "image/gif";
                } else if (fileName.endsWith(".bmp", Qt::CaseInsensitive)) {
                    mimeType = "image/bmp";
                }
                
                // 保存本地路径用于预览
                localFilePath = fileName;
                
                // 先上传文件
                HttpApiClient::instance()->uploadGroupAvatarFile(
                    group["uuid"].toString(),
                    QString("group_avatar.%1").arg(mimeType.section("/", 1)),
                    mimeType, fileSize, fileData,
                    [editDlg, avatarLabel, &currentAvatarUrl, loadAvatar, localFilePath](const QJsonDocument& doc) {
                        // 文件上传成功，获取URL
                        QJsonObject obj = doc.isArray() ? doc.array().first().toObject() : doc.object();
                        if (obj.isEmpty() && doc.object().contains("content")) {
                            obj = QJsonDocument::fromJson(doc.object()["content"].toString().toUtf8()).object();
                        }
                        currentAvatarUrl = obj["url"].toString();
                        
                        // 加载新头像预览
                        if (!localFilePath.isEmpty()) {
                            QPixmap pix(localFilePath);
                            if (!pix.isNull()) {
                                avatarLabel->setPixmap(pix.scaled(60, 60, Qt::KeepAspectRatio, Qt::SmoothTransformation));
                            }
                        } else {
                            loadAvatar(currentAvatarUrl);
                        }
                        
                        qDebug() << "Group avatar uploaded:" << currentAvatarUrl;
                    },
                    [editDlg](const QString& error, int code) {
                        QMessageBox::warning(editDlg, QString::fromUtf8("上传失败"), error);
                    });
            });
            
            auto* nameLabel = new QLabel(QString::fromUtf8("群名称:"));
            editLayout->addWidget(nameLabel);
            auto* nameEdit = new QLineEdit(editDlg);
            nameEdit->setText(group["name"].toString());
            nameEdit->setStyleSheet("QLineEdit { border: 1px solid #A8D8B9; border-radius: 4px; padding: 6px; }");
            editLayout->addWidget(nameEdit);
            
            auto* descLabel = new QLabel(QString::fromUtf8("群描述:"));
            editLayout->addWidget(descLabel);
            auto* descEdit = new QTextEdit(editDlg);
            descEdit->setText(group["description"].toString());
            descEdit->setStyleSheet("QTextEdit { border: 1px solid #A8D8B9; border-radius: 4px; padding: 6px; }");
            descEdit->setFixedHeight(60);
            editLayout->addWidget(descEdit);
            
            auto* maxLabel = new QLabel(QString::fromUtf8("最大成员数:"));
            editLayout->addWidget(maxLabel);
            auto* maxSpin = new QSpinBox(editDlg);
            maxSpin->setRange(10, 1000);
            maxSpin->setValue(group["maxmembers"].toInt(500));
            maxSpin->setStyleSheet("QSpinBox { border: 1px solid #A8D8B9; border-radius: 4px; padding: 6px; }");
            editLayout->addWidget(maxSpin);
            
            auto* btnLayout = new QHBoxLayout();
            auto* saveBtn = makeBtn(QString::fromUtf8("保存"));
            saveBtn->setFixedWidth(100);
            btnLayout->addWidget(saveBtn);
            auto* cancelBtn = makeBtn(QString::fromUtf8("取消"), false);
            cancelBtn->setFixedWidth(100);
            btnLayout->addWidget(cancelBtn);
            editLayout->addLayout(btnLayout);
            
            QObject::connect(saveBtn, &QPushButton::clicked, [editDlg, dlg, nameEdit, descEdit, maxSpin, group, currentAvatarUrl]() {
                QString newName = nameEdit->text().trimmed();
                if (newName.isEmpty()) {
                    QMessageBox::warning(editDlg, QString::fromUtf8("错误"), QString::fromUtf8("群名称不能为空"));
                    return;
                }
                HttpApiClient::instance()->updateGroupInfo(group["uuid"].toString(), newName, descEdit->toPlainText(),
                    currentAvatarUrl, maxSpin->value(), group["ispublic"].toBool(false),
                    [editDlg, dlg](const QJsonDocument& doc) {
                        QJsonObject updatedGroup = doc.isArray() ? doc.array().first().toObject() : doc.object();
                        if (updatedGroup.isEmpty() && doc.object().contains("content")) {
                            updatedGroup = QJsonDocument::fromJson(doc.object()["content"].toString().toUtf8()).object();
                        }
                        QMessageBox::information(editDlg, QString::fromUtf8("成功"), QString::fromUtf8("群信息更新成功"));
                        emit dlg->groupInfoUpdated(updatedGroup);
                        editDlg->accept();
                    },
                    [editDlg](const QString& error, int code) {
                        QMessageBox::warning(editDlg, QString::fromUtf8("错误"), error);
                    });
            });
            QObject::connect(cancelBtn, &QPushButton::clicked, editDlg, &QDialog::reject);
            
            editDlg->exec();
        });
        buttonLayout->addWidget(editGroupBtn);
        
        auto* dissolveBtn = new QPushButton(QString::fromUtf8("解散群聊"));
        dissolveBtn->setStyleSheet("QPushButton { background: #FF6B6B; color: white; border: none; border-radius: 4px; padding: 8px; } QPushButton:hover { background: #EE5A5A; }");
        dissolveBtn->setFixedWidth(100);
        QObject::connect(dissolveBtn, &QPushButton::clicked, [dlg, uuid, parent]() {
            if (QMessageBox::question(dlg, "确认", "确定要解散该群聊吗？") == QMessageBox::Yes) {
                WebSocketClient::instance()->dissolveGroup(uuid);
                QMessageBox::information(dlg, "提示", "已发送解散群聊请求");
                dlg->accept();
            }
        });
        buttonLayout->addWidget(dissolveBtn);
    } else if (!currentUserRole.isEmpty()) {
        auto* leaveBtn = new QPushButton(QString::fromUtf8("退出群聊"));
        leaveBtn->setStyleSheet("QPushButton { background: #FF6B6B; color: white; border: none; border-radius: 4px; padding: 8px; } QPushButton:hover { background: #EE5A5A; }");
        leaveBtn->setFixedWidth(100);
        QObject::connect(leaveBtn, &QPushButton::clicked, [dlg, uuid, parent]() {
            if (QMessageBox::question(dlg, "确认", "确定要退出该群聊吗？") == QMessageBox::Yes) {
                WebSocketClient::instance()->leaveGroup(uuid);
                QMessageBox::information(dlg, "提示", "已发送退出群聊请求");
                dlg->accept();
            }
        });
        buttonLayout->addWidget(leaveBtn);
    }

    auto* closeBtn = makeBtn(QString::fromUtf8("关闭"), false);
    closeBtn->setFixedWidth(100);
    buttonLayout->addWidget(closeBtn);

    layout->addLayout(buttonLayout);

    QObject::connect(closeBtn, &QPushButton::clicked, dlg, &QDialog::accept);

    dlg->show();
    return dlg;
}

InfoDialog* InfoDialog::showUnjoinedGroupInfo(QWidget* parent, const QJsonObject& group)
{
    InfoDialog* dlg = new InfoDialog(parent);
    dlg->setWindowTitle(QString::fromUtf8("群信息"));
    dlg->setFixedSize(420, 400);
    dlg->setWindowIcon(QIcon(":/chat.svg"));
    dlg->setStyleSheet("QDialog { background: #FFFFFF; } QLabel { color: #333; font-size: 13px; }");

    auto* layout = new QVBoxLayout(dlg);
    layout->setSpacing(8);
    layout->setAlignment(Qt::AlignCenter);

    QString name = group["name"].toString();
    QString avatarUrl = group["avatarurl"].toString();
    QString desc = group["description"].toString();
    int memberCount = group["membercount"].toInt();
    int maxMembers = group["maxmembers"].toInt(500);
    QString createdAt = group["createdat"].toString();

    layout->addWidget(makeAvatar(dlg, avatarUrl, name), 0, Qt::AlignCenter);
    layout->addWidget(makeCenterLabel(QString::fromUtf8("群名称: %1").arg(name)));
    layout->addWidget(makeCenterLabel(QString::fromUtf8("成员数: %1/%2").arg(memberCount).arg(maxMembers)));
    if (!desc.isEmpty())
        layout->addWidget(makeCenterLabel(QString::fromUtf8("描述: %1").arg(desc)));
    if (!createdAt.isEmpty())
        layout->addWidget(makeCenterLabel(QString::fromUtf8("创建时间: %1").arg(createdAt)));

    auto* infoLabel = new QLabel(QString::fromUtf8("（您尚未加入该群聊）"));
    infoLabel->setStyleSheet("color: #888; font-size: 12px; margin-top: 20px;");
    infoLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(infoLabel);

    auto* closeBtn = makeBtn(QString::fromUtf8("关闭"), false);
    closeBtn->setFixedWidth(120);
    layout->addWidget(closeBtn, 0, Qt::AlignCenter);
    QObject::connect(closeBtn, &QPushButton::clicked, dlg, &QDialog::accept);

    dlg->show();
    return dlg;
}
