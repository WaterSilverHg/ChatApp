#include "view/LoginPage.h"
#include "view/RegisterPage.h"
#include "view/chatmainpage.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    LoginPage loginPage;
    RegisterPage registerPage;
    ChatMainPage* chatMainPage = nullptr;

    QObject::connect(&loginPage, &LoginPage::loginSuccess, [&](const QString& username, const QString& token, const QJsonObject& userInfo) {
        loginPage.hide();
        if (!chatMainPage) {
            chatMainPage = new ChatMainPage(username, token, userInfo);
            QObject::connect(chatMainPage, &ChatMainPage::logoutToLogin, [&]() {
                chatMainPage->hide();
                loginPage.show();
            });
            QObject::connect(chatMainPage, &ChatMainPage::logoutToExit, [&]() {
                QApplication::quit();
            });
        } else {
            chatMainPage->reinitialize(username, token, userInfo);
        }
        chatMainPage->show();
    });

    QObject::connect(&loginPage, &LoginPage::switchToRegister, [&]() {
        loginPage.hide();
        registerPage.show();
    });

    QObject::connect(&registerPage, &RegisterPage::switchToLogin, [&]() {
        registerPage.hide();
        loginPage.show();
    });

    QObject::connect(&registerPage, &RegisterPage::autoLogin, [&](const QString& username, const QString& token, const QJsonObject& userInfo) {
        registerPage.hide();
        if (!chatMainPage) {
            chatMainPage = new ChatMainPage(username, token, userInfo);
            QObject::connect(chatMainPage, &ChatMainPage::logoutToLogin, [&]() {
                chatMainPage->hide();
                loginPage.show();
            });
            QObject::connect(chatMainPage, &ChatMainPage::logoutToExit, [&]() {
                QApplication::quit();
            });
        } else {
            chatMainPage->reinitialize(username, token, userInfo);
        }
        chatMainPage->show();
    });

    loginPage.show();

    return a.exec();
}