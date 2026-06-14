#pragma once

#include "../global.h"
#include "ui_LoginPage.h"
#include "../model/HttpApiClient.h"
#include "../model/WebSocketClient.h"

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
    void onWebSocketConnected();
    void onWebSocketError(const QString& error);
    void onHistoryAccountSelected(const QString& email);
    void onClearHistoryClicked();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

private:
    void connectSignals();
    void disconnectSignals();
    void loadSavedCredentials();
    void saveCredentials(const QString& email, const QString& password);
    void saveAllCredentials();
    void updateHistoryComboBox();
    QString getCredentialFilePath() const;

private:
    Ui::LoginPageClass ui;
    HttpApiClient* m_httpClient;
    WebSocketClient* m_wsClient;
    QString m_authToken;
    QJsonObject m_userInfo;
    QMap<QString, QString> m_loginHistory;
};