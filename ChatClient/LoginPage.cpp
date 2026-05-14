#include "LoginPage.h"
#include "UserSession.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

LoginPage::LoginPage(QWidget *parent)
    : QWidget(parent)
    , m_httpClient(HttpApiClient::instance())
    , m_wsClient(WebSocketClient::instance())
{
    ui.setupUi(this);
    setWindowTitle("登录");

    m_httpClient->setServerUrl(HTTP_BASE_URL);
    m_wsClient->setWsUrl(WEBSOCKET_URL);

    loadSavedCredentials();
}

void LoginPage::connectSignals()
{
    connect(m_httpClient, &HttpApiClient::loginSuccess, this, &LoginPage::onLoginSuccess);
    connect(m_httpClient, &HttpApiClient::errorOccurred, this, &LoginPage::onError);
    connect(m_wsClient, &WebSocketClient::connected, this, &LoginPage::onWebSocketConnected);
    connect(m_wsClient, &WebSocketClient::errorOccurred, this, &LoginPage::onWebSocketError);
}

void LoginPage::disconnectSignals()
{
    disconnect(m_httpClient, &HttpApiClient::loginSuccess, this, &LoginPage::onLoginSuccess);
    disconnect(m_httpClient, &HttpApiClient::errorOccurred, this, &LoginPage::onError);
    disconnect(m_wsClient, &WebSocketClient::connected, this, &LoginPage::onWebSocketConnected);
    disconnect(m_wsClient, &WebSocketClient::errorOccurred, this, &LoginPage::onWebSocketError);
}

void LoginPage::showEvent(QShowEvent *event)
{
    // 重置按钮状态
    ui.loginButton->setEnabled(true);
    ui.loginButton->setText("登录");
    connectSignals();
    QWidget::showEvent(event);
}

void LoginPage::hideEvent(QHideEvent *event)
{
    disconnectSignals();
    QWidget::hideEvent(event);
}

LoginPage::~LoginPage()
{}

void LoginPage::on_showPasswordCheckBox_toggled(bool checked)
{
    if (checked) {
        ui.passwordEdit->setEchoMode(QLineEdit::Normal);
    } else {
        ui.passwordEdit->setEchoMode(QLineEdit::Password);
    }
}

void LoginPage::on_loginButton_clicked()
{
    QString email = ui.emailEdit->text().trimmed();
    QString password = ui.passwordEdit->text();

    if (email.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "提示", "请输入邮箱和密码!");
        return;
    }

    ui.loginButton->setEnabled(false);
    ui.loginButton->setText("登录中...");

    m_httpClient->login(email, password);
}

void LoginPage::onLoginSuccess(const QJsonObject& data)
{
    QString token = data["token"].toString();
    QJsonObject user = data["user"].toObject();

    m_authToken = token;
    m_userInfo = user;

    UserSession::instance()->setToken(token);
    UserSession::instance()->setUserData(user);
    //记录邮箱和密码，方便下次直接登录
    saveCredentials(ui.emailEdit->text().trimmed(), ui.passwordEdit->text());

    m_wsClient->connectWebSocket();
}

void LoginPage::onWebSocketConnected()
{
    QString username = m_userInfo["username"].toString();
    emit loginSuccess(username, m_authToken, m_userInfo);
}

void LoginPage::onError(const QString& errorMessage, int errorCode)
{
    ui.loginButton->setEnabled(true);
    ui.loginButton->setText("登录");
    QMessageBox::warning(this, "登录失败", errorMessage);
}

void LoginPage::onWebSocketError(const QString& error)
{
    ui.loginButton->setEnabled(true);
    ui.loginButton->setText("登录");
    QMessageBox::warning(this, "WebSocket连接失败", "连接服务器失败: " + error);
}

void LoginPage::on_registerLink_clicked()
{
    emit switchToRegister();
}

void LoginPage::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        on_loginButton_clicked();
    } else if (event->key() == Qt::Key_Tab) {
        QWidget* currentWidget = focusWidget();
        
        if (currentWidget == ui.emailEdit) {
            ui.passwordEdit->setFocus();
            event->accept();
        } else if (currentWidget == ui.passwordEdit) {
            ui.loginButton->setFocus();
            event->accept();
        } else if (currentWidget == ui.loginButton) {
            ui.emailEdit->setFocus();
            event->accept();
        }
    } else {
        QWidget::keyPressEvent(event);
    }
}

QString LoginPage::getCredentialFilePath() const
{
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir appDir(appDataPath);
    if (!appDir.exists()) {
        appDir.mkpath(".");
    }
    return appDir.filePath("credentials.json");
}

void LoginPage::loadSavedCredentials()
{
    QString filePath = getCredentialFilePath();
    QFile file(filePath);
    if (file.exists() && file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (!doc.isNull()) {
            QJsonObject obj = doc.object();
            QString email = obj["email"].toString();
            QString password = obj["password"].toString();
            if (!email.isEmpty()) {
                ui.emailEdit->setText(email);
            }
            if (!password.isEmpty()) {
                ui.passwordEdit->setText(password);
            }
        }
        file.close();
    }
}

void LoginPage::saveCredentials(const QString& email, const QString& password)
{
    QString filePath = getCredentialFilePath();
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        QJsonObject obj;
        obj["email"] = email;
        obj["password"] = password;
        QJsonDocument doc(obj);
        file.write(doc.toJson());
        file.close();
    }
}
