#pragma once

#include "global.h"
#include "ui_LoginPage.h"
#include "HttpApiClient.h"
#include "WebSocketClient.h"

class LoginPage : public QWidget
{
    Q_OBJECT

public:
    LoginPage(QWidget *parent = nullptr);
    ~LoginPage();

signals:
    void loginSuccess(const QString& username, const QString& token, const QJsonObject& userInfo);
    void switchToRegister();

private slots:
    void on_loginButton_clicked();
    void on_registerLink_clicked();
    void on_showPasswordCheckBox_toggled(bool checked);
    void onLoginSuccess(const QJsonObject& data);
    void onError(const QString& errorMessage, int errorCode);
    void onWebSocketConnected();
    void onWebSocketError(const QString& error);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

private:
    void connectSignals();
    void disconnectSignals();
    void loadSavedCredentials();
    void saveCredentials(const QString& email, const QString& password);
    QString getCredentialFilePath() const;

private:
    Ui::LoginPageClass ui;
    HttpApiClient* m_httpClient;
    WebSocketClient* m_wsClient;
    QString m_authToken;
    QJsonObject m_userInfo;
};