#pragma once

#include "../global.h"

// 异步头像加载器 —— 不阻塞UI线程
// 用法：
//    auto loader = new AsyncAvatarLoader(this);
//    connect(loader, &AsyncAvatarLoader::avatarReady, label, &QLabel::setPixmap);
//    loader->load("https://...", uuid, QSize(80,80));
class AsyncAvatarLoader : public QObject
{
    Q_OBJECT

public:
    explicit AsyncAvatarLoader(QObject* parent = nullptr);
    ~AsyncAvatarLoader();

    // 异步加载头像，完成时发射 avatarReady。加载期间可先设占位图。
    void load(const QString& url, const QString& cacheKey, const QSize& targetSize = QSize(80, 80));

    // 从磁盘缓存同步获取（已有的才返回，不走网络），用于快速首屏
    QPixmap getCached(const QString& cacheKey, const QSize& targetSize = QSize(80, 80));

    // 生成纯色占位图（用名字首字母），不联网
    static QPixmap makePlaceholder(const QString& name, const QSize& size = QSize(80, 80));

    // 清空磁盘缓存
    static void clearDiskCache();

signals:
    void avatarReady(const QString& cacheKey, QPixmap pixmap);

private slots:
    void onReplyFinished(QNetworkReply* reply);

private:
    static QString cacheDir();
    static QString cachePath(const QString& cacheKey);

    QNetworkAccessManager* m_nam;
    QHash<QNetworkReply*, QString> m_pendingKeys;   // reply → cacheKey
    QHash<QNetworkReply*, QSize>   m_pendingSizes;   // reply → targetSize
};
