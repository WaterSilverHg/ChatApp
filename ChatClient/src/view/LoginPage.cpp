#include "LoginPage.h"
#include "../model/UserSession.h"
#include <QMessageBox>

LoginPage::LoginPage(QWidget *parent)
    : QWidget(parent)
    , m_httpClient(HttpApiClient::instance())
    , m_wsClient(WebSocketClient::instance())
{
    ui.setupUi(this);
    setWindowTitle("登录");
    setWindowIcon(QIcon(":/chat.svg"));

    m_httpClient->setServerUrl(HTTP_BASE_URL);
    m_wsClient->setWsUrl(WEBSOCKET_URL);

    connect(ui.historyComboBox, &QComboBox::currentTextChanged, this, &LoginPage::onHistoryAccountSelected);
    connect(ui.clearHistoryBtn, &QPushButton::clicked, this, &LoginPage::onClearHistoryClicked);

    loadSavedCredentials();
    updateHistoryComboBox();
}

void LoginPage::connectSignals()
{
    connect(m_wsClient, &WebSocketClient::connected, this, &LoginPage::onWebSocketConnected);
    connect(m_wsClient, &WebSocketClient::errorOccurred, this, &LoginPage::onWebSocketError);
}

void LoginPage::disconnectSignals()
{
    disconnect(m_wsClient, &WebSocketClient::connected, this, &LoginPage::onWebSocketConnected);
    disconnect(m_wsClient, &WebSocketClient::errorOccurred, this, &LoginPage::onWebSocketError);
}

void LoginPage::showEvent(QShowEvent *event)
{
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

void LoginPage::onHistoryAccountSelected(const QString& email)
{
    if (email.isEmpty() || email == "-- 选择历史账号 --") {
        return;
    }

    if (m_loginHistory.contains(email)) {
        QString password = m_loginHistory[email];
        ui.emailEdit->setText(email);
        ui.passwordEdit->setText(password);
    }
}

void LoginPage::onClearHistoryClicked()
{
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "确认", "确定要清除所有登录历史记录吗？",
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        m_loginHistory.clear();
        saveAllCredentials();
        updateHistoryComboBox();
        ui.emailEdit->clear();
        ui.passwordEdit->clear();
    }
}

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

    m_httpClient->login(email, password,
        [this](const QJsonDocument& doc) {
            QJsonObject data = doc.object();
            if (data.contains("content")) {
                data = QJsonDocument::fromJson(data["content"].toString().toUtf8()).object();
            }
            QString token = data["token"].toString();
            QJsonObject user = data["user"].toObject();

            m_authToken = token;
            m_userInfo = user;

            UserSession::instance()->setToken(token);
            UserSession::instance()->setUserData(user);

            saveCredentials(ui.emailEdit->text().trimmed(), ui.passwordEdit->text());

            m_wsClient->connectWebSocket();
        },
        [this](const QString& errorMessage, int errorCode) {
            ui.loginButton->setEnabled(true);
            ui.loginButton->setText("登录");
            QMessageBox::warning(this, "登录失败", errorMessage);
        });
}

void LoginPage::onWebSocketConnected()
{
    QString username = m_userInfo["username"].toString();
    emit loginSuccess(username, m_authToken, m_userInfo);
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
            QJsonObject history = obj["history"].toObject();

            m_loginHistory.clear();
            for (auto it = history.begin(); it != history.end(); ++it) {
                QString email = it.key();
                QJsonObject accountInfo = it.value().toObject();
                QString password = accountInfo["password"].toString();
                if (!email.isEmpty() && !password.isEmpty()) {
                    m_loginHistory[email] = password;
                }
            }

            QString lastEmail = obj["lastEmail"].toString();
            QString lastPassword = obj["lastPassword"].toString();
            if (!lastEmail.isEmpty()) {
                ui.emailEdit->setText(lastEmail);
                if (!lastPassword.isEmpty()) {
                    ui.passwordEdit->setText(lastPassword);
                }
            } else if (!m_loginHistory.isEmpty()) {
                QString mostRecentEmail = m_loginHistory.firstKey();
                ui.emailEdit->setText(mostRecentEmail);
                ui.passwordEdit->setText(m_loginHistory[mostRecentEmail]);
            }
        }
        file.close();
    }
}

void LoginPage::saveCredentials(const QString& email, const QString& password)
{
    if (email.isEmpty() || password.isEmpty()) {
        return;
    }

    m_loginHistory.remove(email);
    m_loginHistory.insert(email, password);

    while (m_loginHistory.size() > 10) {
        m_loginHistory.remove(m_loginHistory.firstKey());
    }

    saveAllCredentials();
}

void LoginPage::saveAllCredentials()
{
    QString filePath = getCredentialFilePath();
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        QJsonObject obj;

        QJsonObject history;
        for (auto it = m_loginHistory.begin(); it != m_loginHistory.end(); ++it) {
            QJsonObject accountInfo;
            accountInfo["password"] = it.value();
            history[it.key()] = accountInfo;
        }
        obj["history"] = history;

        if (!m_loginHistory.isEmpty()) {
            QString mostRecentEmail = m_loginHistory.lastKey();
            obj["lastEmail"] = mostRecentEmail;
            obj["lastPassword"] = m_loginHistory[mostRecentEmail];
        }

        QJsonDocument doc(obj);
        file.write(doc.toJson());
        file.close();
    }
}

void LoginPage::updateHistoryComboBox()
{
    ui.historyComboBox->blockSignals(true);
    ui.historyComboBox->clear();

    ui.historyComboBox->addItem("-- 选择历史账号 --");

    QStringList emails = m_loginHistory.keys();
    for (const QString& email : emails) {
        ui.historyComboBox->addItem(email);
    }

    ui.historyComboBox->blockSignals(false);
}