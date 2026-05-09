#include "RegisterPage.h"
#include "UserSession.h"

RegisterPage::RegisterPage(QWidget *parent)
    : QWidget(parent)
    , m_httpClient(HttpApiClient::instance())
    , m_wsClient(WebSocketClient::instance())
{
    ui.setupUi(this);
    setWindowTitle("注册");

    m_httpClient->setServerUrl(HTTP_BASE_URL);
    m_wsClient->setWsUrl(WEBSOCKET_URL);
}

void RegisterPage::connectSignals()
{
    connect(m_httpClient, &HttpApiClient::registerSuccess, this, &RegisterPage::onRegisterSuccess);
    connect(m_httpClient, &HttpApiClient::loginSuccess, this, &RegisterPage::onLoginSuccess);
    connect(m_httpClient, &HttpApiClient::errorOccurred, this, &RegisterPage::onError);
    connect(m_wsClient, &WebSocketClient::connected, this, &RegisterPage::onWebSocketConnected);
    connect(m_wsClient, &WebSocketClient::errorOccurred, this, &RegisterPage::onWebSocketError);
}

void RegisterPage::disconnectSignals()
{
    disconnect(m_httpClient, &HttpApiClient::registerSuccess, this, &RegisterPage::onRegisterSuccess);
    disconnect(m_httpClient, &HttpApiClient::loginSuccess, this, &RegisterPage::onLoginSuccess);
    disconnect(m_httpClient, &HttpApiClient::errorOccurred, this, &RegisterPage::onError);
    disconnect(m_wsClient, &WebSocketClient::connected, this, &RegisterPage::onWebSocketConnected);
    disconnect(m_wsClient, &WebSocketClient::errorOccurred, this, &RegisterPage::onWebSocketError);
}

void RegisterPage::showEvent(QShowEvent *event)
{
    // 重置按钮状态
    ui.registerButton->setEnabled(true);
    ui.registerButton->setText("注册");
    connectSignals();
    QWidget::showEvent(event);
}

void RegisterPage::hideEvent(QHideEvent *event)
{
    disconnectSignals();
    QWidget::hideEvent(event);
}

RegisterPage::~RegisterPage()
{}

void RegisterPage::on_showPasswordCheckBox_toggled(bool checked)
{
    if (checked) {
        ui.passwordEdit->setEchoMode(QLineEdit::Normal);
        ui.confirmPwdEdit->setEchoMode(QLineEdit::Normal);
    } else {
        ui.passwordEdit->setEchoMode(QLineEdit::Password);
        ui.confirmPwdEdit->setEchoMode(QLineEdit::Password);
    }
}

void RegisterPage::on_registerButton_clicked()
{
    QString username = ui.usernameEdit->text().trimmed();
    QString email = ui.emailEdit->text().trimmed();
    QString password = ui.passwordEdit->text();
    QString confirmPwd = ui.confirmPwdEdit->text();

    if (username.isEmpty() || email.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "提示", "请输入用户名、邮箱和密码!");
        return;
    }

    if (password.length() < 6) {
        QMessageBox::warning(this, "提示", "密码长度至少6位!");
        return;
    }

    if (password != confirmPwd) {
        QMessageBox::warning(this, "提示", "两次输入的密码不一致!");
        return;
    }

    ui.registerButton->setEnabled(false);
    ui.registerButton->setText("注册中...");

    m_httpClient->registerUser(username, email, password);
}

void RegisterPage::onRegisterSuccess(const QJsonObject& data)
{
    QString token = data["token"].toString();
    QJsonObject user = data["user"].toObject();
    QString username = user["username"].toString();

    m_pendingToken = token;
    m_pendingUser = user;

    QMessageBox::information(this, "提示", "注册成功!正在登录...");
    m_httpClient->login(user["email"].toString(), "");
}

void RegisterPage::onLoginSuccess(const QJsonObject& data)
{
    QString token = data["token"].toString();
    QJsonObject user = data["user"].toObject();
    QString username = user["username"].toString();

    m_pendingToken = token;
    m_pendingUser = user;

    UserSession::instance()->setToken(token);
    UserSession::instance()->setUserData(user);

    m_wsClient->connectWebSocket();
}

void RegisterPage::onWebSocketConnected()
{
    QString username = m_pendingUser["username"].toString();
    emit autoLogin(username, m_pendingToken, m_pendingUser);
}

void RegisterPage::onError(const QString& errorMessage, int errorCode)
{
    ui.registerButton->setEnabled(true);
    ui.registerButton->setText("注册");
    QMessageBox::warning(this, "注册失败", errorMessage);
}

void RegisterPage::onWebSocketError(const QString& error)
{
    ui.registerButton->setEnabled(true);
    ui.registerButton->setText("注册");
    QMessageBox::warning(this, "WebSocket连接失败", "连接服务器失败: " + error);
}

void RegisterPage::on_backLink_clicked()
{
    emit switchToLogin();
}

void RegisterPage::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        on_registerButton_clicked();
    } else if (event->key() == Qt::Key_Tab) {
        QWidget* currentWidget = focusWidget();
        
        if (currentWidget == ui.usernameEdit) {
            ui.emailEdit->setFocus();
            event->accept();
        } else if (currentWidget == ui.emailEdit) {
            ui.passwordEdit->setFocus();
            event->accept();
        } else if (currentWidget == ui.passwordEdit) {
            ui.confirmPwdEdit->setFocus();
            event->accept();
        } else if (currentWidget == ui.confirmPwdEdit) {
            ui.registerButton->setFocus();
            event->accept();
        } else if (currentWidget == ui.registerButton) {
            ui.usernameEdit->setFocus();
            event->accept();
        }
    } else {
        QWidget::keyPressEvent(event);
    }
}
