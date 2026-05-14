// global.h
#ifndef GLOBAL_H
#define GLOBAL_H

// Qt核心头文件
#include <QObject>
#include <QString>
#include <QMap>
#include <QList>
#include <QListWidgetItem>
#include <QDateTime>
#include <QKeyEvent>
#include <QCloseEvent>


// Qt网络相关头文件
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include <QUrl>

// Qt JSON相关头文件
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

// WebSocket相关
#include <QWebSocket>
#include <QAbstractSocket>

// QtWidgets相关头文件
#include <QtWidgets/QWidget>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QFrame>
#include <QScrollBar>

// 全局常量
static const QString HTTP_BASE_URL = "http://127.0.0.1:8080";
static const QString WEBSOCKET_URL = "ws://127.0.0.1:4567/ws";
static const int MESSAGE_PAGE_SIZE = 50;

#endif // GLOBAL_H
