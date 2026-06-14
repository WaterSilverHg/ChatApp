#pragma once

#include "../global.h"
#include "ui_RegisterPage.h"
#include "../model/HttpApiClient.h"
#include "../model/WebSocketClient.h"

class RegisterPage : public QWidget
{
    Q_OBJECT

public:
    RegisterPage(QWidget *parent = nullptr);
    ~RegisterPage();

signals:
    void registerSuccess(const QString& username);
    void switchToLogin();
    void autoLogin(const QString& username, const QString& token, const QJsonObject& userInfo);

private slots:
    void on_registerButton_clicked();
    void on_backToLoginLink_clicked();
    void on_showPasswordCheckBox_toggled(bool checked);
    void on_sendCodeButton_clicked();
    void onWebSocketConnected();
    void onWebSocketError(const QString& error);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

private:
    void connectSignals();
    void disconnectSignals();

private:
    Ui::RegisterPageClass ui;
    HttpApiClient* m_httpClient;
    WebSocketClient* m_wsClient;
    QString m_pendingToken;
    QJsonObject m_pendingUser;
    QTimer m_countdownTimer;
    QMetaObject::Connection m_countdownConnection;
    int m_codeCountdown;
};