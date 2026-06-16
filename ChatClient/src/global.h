// global.h
#ifndef GLOBAL_H
#define GLOBAL_H

// C++ 标准库头文件
#include <cmath>
#include <functional>

// QtCore 头文件
#include <QObject>
#include <QString>
#include <QMap>
#include <QList>
#include <QListWidgetItem>
#include <QDateTime>
#include <QKeyEvent>
#include <QCloseEvent>
#include <QSharedPointer>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QIODevice>
#include <QByteArray>
#include <QStandardPaths>
#include <QMimeDatabase>
#include <QMimeType>
#include <QDebug>
#include <QHash>
#include <QThread>
#include <QRandomGenerator>
#include <QPainter>
#include<QTimer>

// QtNetwork 头文件
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>
#include <QAbstractSocket>

// QtJSON 头文件
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

// QtWebSockets 头文件
#include <QWebSocket>

// QtWidgets 头文件
#include <QtWidgets/QWidget>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QFrame>
#include <QtWidgets/QApplication>
#include <QtWidgets/QDialog>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QTextEdit>
#include <QSpinBox>
#include <QScrollBar>
#include <QTextFrame>
#include <QMenu>
#include <QTextTable>
#include <QInputDialog>
#include <QStatusBar>

// QtGui 头文件
#include <QPixmap>

// 全局常量
// 连接到 Nginx 负载均衡器（开发环境）
static const QString HTTP_BASE_URL = "http://127.0.0.1:8080";
static const QString WEBSOCKET_URL = "ws://127.0.0.1:8080/ws";
// 如果使用 HTTPS，取消注释以下行
// static const QString HTTP_BASE_URL = "https://your-domain.com";
// static const QString WEBSOCKET_URL = "wss://your-domain.com/ws";
static const int MESSAGE_PAGE_SIZE = 50;

// UI 样式常量（浅绿-白主题）
namespace StyleConst {
    // 颜色
    static const QString COLOR_PRIMARY = "#A8D8B9";
    static const QString COLOR_PRIMARY_DARK = "#7CB894";
    static const QString COLOR_PRIMARY_DARKER = "#5EA07A";
    static const QString COLOR_BG_LIGHT = "#F5FAF7";
    static const QString COLOR_BG_WHITE = "#FFFFFF";
    static const QString COLOR_TEXT_DARK = "#333333";
    static const QString COLOR_TEXT_MEDIUM = "#555555";
    static const QString COLOR_TEXT_LIGHT = "#888888";
    static const QString COLOR_DANGER = "#E88B8B";
    static const QString COLOR_DANGER_HOVER = "#D07070";
    static const QString COLOR_BORDER = "#A8D8B9";
    static const QString COLOR_SELECTED = "#E8F5ED";
    static const QString COLOR_BADGE_RED = "#E57373";

    // ==================== 通用按钮 ====================
    static const QString BUTTON_PRIMARY = R"(
        QPushButton {
            background-color: #A8D8B9; color: #333; border: none;
            border-radius: 5px; padding: 8px 16px; font-size: 13px; font-weight: bold;
        }
        QPushButton:hover { background-color: #7CB894; color: white; }
        QPushButton:pressed { background-color: #5EA07A; color: white; }
        QPushButton:disabled { background-color: #ddd; color: #999; }
    )";

    // ==================== 危险按钮（红色） ====================
    static const QString BUTTON_DANGER = R"(
        QPushButton {
            background-color: #E88B8B; color: white; border: none;
            border-radius: 5px; padding: 8px 16px; font-size: 13px; font-weight: bold;
        }
        QPushButton:hover { background-color: #D07070; }
    )";

    // ==================== 次要按钮（边框） ====================
    static const QString BUTTON_SECONDARY = R"(
        QPushButton {
            border: 1px solid #A8D8B9; border-radius: 5px;
            padding: 8px 16px; font-size: 13px; color: #333; background: white;
        }
        QPushButton:hover { background-color: #7CB894; color: white; }
    )";

    // ==================== 输入框 ====================
    static const QString LINEEDIT = R"(
        QLineEdit {
            background: #F5FAF7; border: 1px solid #A8D8B9; border-radius: 5px;
            padding: 8px 12px; font-size: 13px; color: #333;
        }
        QLineEdit:focus { border: 2px solid #7CB894; }
        QLineEdit:disabled { background: #eee; color: #aaa; }
    )";

    // ==================== 多行文本编辑 ====================
    static const QString TEXTEDIT = R"(
        QTextEdit {
            background: #F5FAF7; border: 1px solid #A8D8B9; border-radius: 5px;
            padding: 6px 10px; font-size: 13px; color: #333;
        }
        QTextEdit:focus { border: 2px solid #7CB894; }
    )";

    // ==================== 列表控件 ====================
    static const QString LISTWIDGET = R"(
        QListWidget {
            background: white; border: 1px solid #A8D8B9; border-radius: 5px; outline: none;
        }
        QListWidget::item { padding: 8px; border-bottom: 1px solid #e0e8e3; }
        QListWidget::item:selected { background-color: #E8F5ED; color: #333; }
        QListWidget::item:hover:!selected { background-color: #F5FAF7; }
    )";

    // ==================== 分组框 ====================
    static const QString GROUPBOX = R"(
        QGroupBox {
            border: 1px solid #A8D8B9; border-radius: 5px; margin-top: 10px;
            padding-top: 16px; font-weight: bold; color: #333;
        }
        QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; }
    )";

    // ==================== 对话框 ====================
    static const QString DIALOG = R"(
        QDialog { background-color: #FFFFFF; }
        QLabel { color: #333; font-size: 13px; }
    )";

    // ==================== TabWidget ====================
    static const QString TABWIDGET = R"(
        QTabWidget::pane { border: 1px solid #A8D8B9; border-radius: 4px; background: white; }
        QTabBar::tab { background: #A8D8B9; color: #333; padding: 6px 14px;
                       border-top-left-radius: 4px; border-top-right-radius: 4px; margin-right: 2px; }
        QTabBar::tab:selected { background: #7CB894; color: white; }
        QTabBar::tab:hover:!selected { background: #8CC8A5; }
    )";

    // ==================== 兼容旧名称（_STYLE 后缀，独立常量避免静态初始化顺序问题） ====================
    static const QString BUTTON_STYLE = QStringLiteral(
        "QPushButton { background-color: #A8D8B9; color: #333; border: none; border-radius: 5px; padding: 8px 16px; font-size: 13px; font-weight: bold; } "
        "QPushButton:hover { background-color: #7CB894; color: white; } "
        "QPushButton:pressed { background-color: #5EA07A; color: white; } "
        "QPushButton:disabled { background-color: #ddd; color: #999; }");
    static const QString BUTTON_SELECTED_STYLE = QStringLiteral(
        "QPushButton { background-color: #7CB894; color: white; border: none; border-radius: 5px; padding: 8px 16px; font-size: 13px; font-weight: bold; } "
        "QPushButton:hover { background-color: #5EA07A; }");
    static const QString LINEEDIT_STYLE = QStringLiteral(
        "QLineEdit { background: #F5FAF7; border: 1px solid #A8D8B9; border-radius: 5px; padding: 8px 12px; font-size: 13px; color: #333; } "
        "QLineEdit:focus { border: 2px solid #7CB894; } "
        "QLineEdit:disabled { background: #eee; color: #aaa; }");
    static const QString LIST_STYLE = QStringLiteral(
        "QListWidget { background: white; border: 1px solid #A8D8B9; border-radius: 5px; outline: none; } "
        "QListWidget::item { padding: 8px; border-bottom: 1px solid #e0e8e3; } "
        "QListWidget::item:selected { background-color: #E8F5ED; color: #333; } "
        "QListWidget::item:hover:!selected { background-color: #F5FAF7; }");
    static const QString DIALOG_STYLE = QStringLiteral(
        "QDialog { background-color: #FFFFFF; } QLabel { color: #333; font-size: 13px; }");
}

#endif // GLOBAL_H
