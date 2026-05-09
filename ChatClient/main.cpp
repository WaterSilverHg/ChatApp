#include "LoginPage.h"
#include "RegisterPage.h"
#include "ChatMainPage.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    LoginPage loginPage;
    RegisterPage registerPage;
    ChatMainPage* chatMainPage = nullptr;

    auto showChatMainPage = [&](const QString& username, const QString& token, const QJsonObject& userInfo) {
        // 清理旧的聊天页面
        if (chatMainPage) {
            delete chatMainPage;
            chatMainPage = nullptr;
        }
        
        chatMainPage = new ChatMainPage(username, token, userInfo);
        chatMainPage->show();
        
        // 连接销毁信号
        QObject::connect(chatMainPage, &ChatMainPage::destroyed, [&]() {
            chatMainPage = nullptr;
            loginPage.show();
        });
        
        // 连接退出信号
        QObject::connect(chatMainPage, &ChatMainPage::logoutToLogin, [&]() {
            if (chatMainPage) {
                chatMainPage->deleteLater();
                chatMainPage = nullptr;
            }
            loginPage.show();
        });
        
        QObject::connect(chatMainPage, &ChatMainPage::logoutToRegister, [&]() {
            if (chatMainPage) {
                chatMainPage->deleteLater();
                chatMainPage = nullptr;
            }
            registerPage.show();
        });
        
        QObject::connect(chatMainPage, &ChatMainPage::logoutToExit, [&]() {
            if (chatMainPage) {
                chatMainPage->deleteLater();
                chatMainPage = nullptr;
            }
            QApplication::quit();
        });
    };

    QObject::connect(&loginPage, &LoginPage::loginSuccess, [&](const QString& username, const QString& token, const QJsonObject& userInfo) {
        loginPage.hide();
        showChatMainPage(username, token, userInfo);
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
        showChatMainPage(username, token, userInfo);
    });

    loginPage.show();

    return a.exec();
}
