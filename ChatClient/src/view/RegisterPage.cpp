#include "RegisterPage.h"
#include "../model/UserSession.h"

RegisterPage::RegisterPage(QWidget *parent)
    : QWidget(parent)
    , m_httpClient(HttpApiClient::instance())
    , m_wsClient(WebSocketClient::instance())
    , m_codeCountdown(0)
{
    ui.setupUi(this);
    setWindowTitle("注册");
    setWindowIcon(QIcon(":/chat.svg"));
//    setStyleSheet(StyleConst::DIALOG_STYLE);

//    ui.registerButton->setStyleSheet(StyleConst::BUTTON_STYLE);
//    ui.sendCodeButton->setStyleSheet(StyleConst::BUTTON_STYLE);
//    ui.backToLoginLink->setStyleSheet("color: #1A4D1A; text-decoration: underline; background: none; border: none;");
//    ui.emailEdit->setStyleSheet(StyleConst::LINEEDIT_STYLE);
//    ui.usernameEdit->setStyleSheet(StyleConst::LINEEDIT_STYLE);
//    ui.passwordEdit->setStyleSheet(StyleConst::LINEEDIT_STYLE);
//    ui.confirmPasswordEdit->setStyleSheet(StyleConst::LINEEDIT_STYLE);
//    ui.verificationCodeEdit->setStyleSheet(StyleConst::LINEEDIT_STYLE);

    m_httpClient->setServerUrl(HTTP_BASE_URL);
    m_wsClient->setWsUrl(WEBSOCKET_URL);
}

void RegisterPage::connectSignals()
{
    connect(m_wsClient, &WebSocketClient::connected, this, &RegisterPage::onWebSocketConnected);
    connect(m_wsClient, &WebSocketClient::errorOccurred, this, &RegisterPage::onWebSocketError);
}

void RegisterPage::disconnectSignals()
{
    disconnect(m_wsClient, &WebSocketClient::connected, this, &RegisterPage::onWebSocketConnected);
    disconnect(m_wsClient, &WebSocketClient::errorOccurred, this, &RegisterPage::onWebSocketError);
}

void RegisterPage::showEvent(QShowEvent *event)
{
    ui.registerButton->setEnabled(true);
    ui.registerButton->setText("注册");
    ui.sendCodeButton->setEnabled(true);
    ui.sendCodeButton->setText("发送验证码");
    connectSignals();
    QWidget::showEvent(event);
}

void RegisterPage::hideEvent(QHideEvent *event)
{
    disconnectSignals();
    if (m_countdownTimer.isActive()) {
        m_countdownTimer.stop();
    }
    QWidget::hideEvent(event);
}

RegisterPage::~RegisterPage()
{}

void RegisterPage::on_showPasswordCheckBox_toggled(bool checked)
{
    if (checked) {
        ui.passwordEdit->setEchoMode(QLineEdit::Normal);
        ui.confirmPasswordEdit->setEchoMode(QLineEdit::Normal);
    } else {
        ui.passwordEdit->setEchoMode(QLineEdit::Password);
        ui.confirmPasswordEdit->setEchoMode(QLineEdit::Password);
    }
}

void RegisterPage::on_sendCodeButton_clicked()
{
    QString email = ui.emailEdit->text().trimmed();
    if (email.isEmpty()) {
        QMessageBox::warning(this, "提示", "请先输入邮箱地址！");
        return;
    }

    ui.sendCodeButton->setEnabled(false);
    m_httpClient->sendVerificationCode(email,
        [this](const QJsonDocument& doc) {
            QMessageBox::information(this, "提示", "验证码已发送到您的邮箱！");
            m_codeCountdown = 60;
            ui.sendCodeButton->setText(QString("重新发送 (%1s)").arg(m_codeCountdown));

            disconnect(m_countdownConnection);
            m_countdownConnection = connect(&m_countdownTimer, &QTimer::timeout, this, [this]() {
                m_codeCountdown--;
                if (m_codeCountdown <= 0) {
                    m_countdownTimer.stop();
                    ui.sendCodeButton->setText("发送验证码");
                    ui.sendCodeButton->setEnabled(true);
                } else {
                    ui.sendCodeButton->setText(QString("重新发送 (%1s)").arg(m_codeCountdown));
                }
            });
            m_countdownTimer.start(1000);
        },
        [this](const QString& errorMessage, int errorCode) {
            ui.sendCodeButton->setEnabled(true);
            QMessageBox::warning(this, "错误", errorMessage);
        });
}

void RegisterPage::on_registerButton_clicked()
{
    QString username = ui.usernameEdit->text().trimmed();
    QString email = ui.emailEdit->text().trimmed();
    QString password = ui.passwordEdit->text();
    QString confirmPwd = ui.confirmPasswordEdit->text();
    QString verificationCode = ui.verificationCodeEdit->text().trimmed();

    if (username.isEmpty() || email.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "提示", "请输入用户名、邮箱和密码!");
        return;
    }

    if (verificationCode.isEmpty()) {
        QMessageBox::warning(this, "提示", "请输入验证码!");
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

    m_httpClient->registerUser(username, email, password, verificationCode,
        [this](const QJsonDocument& doc) {
            QJsonObject data = doc.object();
            if (data.contains("content")) {
                data = QJsonDocument::fromJson(data["content"].toString().toUtf8()).object();
            }
            QString token = data["token"].toString();
            QJsonObject user = data["user"].toObject();
            QString usernameStr = user["username"].toString();

            m_pendingToken = token;
            m_pendingUser = user;

            UserSession::instance()->setToken(token);
            UserSession::instance()->setUserData(user);

            QMessageBox::information(this, "提示", "注册成功!正在连接...");
            m_wsClient->connectWebSocket();
        },
        [this](const QString& errorMessage, int errorCode) {
            ui.registerButton->setEnabled(true);
            ui.registerButton->setText("注册");
            ui.sendCodeButton->setEnabled(true);
            QMessageBox::warning(this, "注册失败", errorMessage);
        });
}

void RegisterPage::onWebSocketConnected()
{
    QString username = m_pendingUser["username"].toString();
    emit autoLogin(username, m_pendingToken, m_pendingUser);
}

void RegisterPage::onWebSocketError(const QString& error)
{
    ui.registerButton->setEnabled(true);
    ui.registerButton->setText("注册");
    QMessageBox::warning(this, "WebSocket连接失败", "连接服务器失败: " + error);
}

void RegisterPage::on_backToLoginLink_clicked()
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
            ui.verificationCodeEdit->setFocus();
            event->accept();
        } else if (currentWidget == ui.verificationCodeEdit) {
            ui.passwordEdit->setFocus();
            event->accept();
        } else if (currentWidget == ui.passwordEdit) {
            ui.confirmPasswordEdit->setFocus();
            event->accept();
        } else if (currentWidget == ui.confirmPasswordEdit) {
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