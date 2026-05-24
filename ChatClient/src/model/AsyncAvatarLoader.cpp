#include "AsyncAvatarLoader.h"

AsyncAvatarLoader::AsyncAvatarLoader(QObject* parent)
    : QObject(parent)
{
    m_nam = new QNetworkAccessManager(this);
    connect(m_nam, &QNetworkAccessManager::finished, this, &AsyncAvatarLoader::onReplyFinished);
}

AsyncAvatarLoader::~AsyncAvatarLoader() = default;

// ---- public API ----

void AsyncAvatarLoader::load(const QString& url, const QString& cacheKey,
                             const QSize& targetSize)
{
    if (url.isEmpty()) {
        emit avatarReady(cacheKey, QPixmap());
        return;
    }
    QString path = cachePath(cacheKey);
    if (QFile::exists(path)) {
        QPixmap pix(path);
        if (!pix.isNull()) {
            emit avatarReady(cacheKey,
                pix.scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            return;
        }
    }
    QNetworkRequest req(url);
    QNetworkReply* reply = m_nam->get(req);
    m_pendingKeys.insert(reply, cacheKey);
    m_pendingSizes.insert(reply, targetSize);
}

QPixmap AsyncAvatarLoader::getCached(const QString& cacheKey, const QSize& targetSize)
{
    QString path = cachePath(cacheKey);
    if (QFile::exists(path)) {
        QPixmap pix(path);
        if (!pix.isNull())
            return pix.scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    return QPixmap();
}

// ---- static helpers ----

QPixmap AsyncAvatarLoader::makePlaceholder(const QString& name, const QSize& size)
{
    QPixmap px(size);
    px.fill(Qt::transparent);

    // 根据名字哈希生成稳定的背景色
    uint hash = qHash(name);
    int hue = hash % 360;
    QColor bg = QColor::fromHsl(hue, 180, 160);

    QPainter p(&px);
    p.setRenderHint(QPainter::Antialiasing);
    p.setBrush(bg);
    p.setPen(Qt::NoPen);
    int r = std::min(size.width(), size.height());
    p.drawEllipse(0, 0, r, r);

    // 取名字首字符
    QString initial;
    if (!name.isEmpty()) {
        QChar c = name.at(0);
        if (c.isLetterOrNumber())
            initial = c.toUpper();
    }
    if (!initial.isEmpty()) {
        QFont font;
        font.setPixelSize(r * 0.45);
        font.setBold(true);
        p.setFont(font);
        p.setPen(Qt::white);
        p.drawText(QRect(0, 0, r, r), Qt::AlignCenter, initial);
    }
    p.end();
    return px;
}

void AsyncAvatarLoader::clearDiskCache()
{
    QDir dir(cacheDir());
    dir.removeRecursively();
}

// ---- private ----

QString AsyncAvatarLoader::cacheDir()
{
    return QCoreApplication::applicationDirPath() + "/avatar_cache";
}

QString AsyncAvatarLoader::cachePath(const QString& cacheKey)
{
    QDir dir(cacheDir());
    if (!dir.exists()) dir.mkpath(cacheDir());
    QByteArray h = QCryptographicHash::hash(cacheKey.toUtf8(), QCryptographicHash::Md5);
    return cacheDir() + "/" + QString(h.toHex()) + ".png";
}

void AsyncAvatarLoader::onReplyFinished(QNetworkReply* reply)
{
    if (!reply) return;

    QString cacheKey = m_pendingKeys.take(reply);
    QSize targetSize = m_pendingSizes.take(reply);

    if (reply->error() != QNetworkReply::NoError || cacheKey.isEmpty()) {
        reply->deleteLater();
        emit avatarReady(cacheKey, QPixmap());
        return;
    }

    QByteArray data = reply->readAll();
    reply->deleteLater();

    QPixmap pix;
    if (!pix.loadFromData(data)) {
        emit avatarReady(cacheKey, QPixmap());
        return;
    }

    // 存磁盘缓存
    QString path = cachePath(cacheKey);
    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        pix.save(&file, "PNG");
        file.close();
    }

    emit avatarReady(cacheKey,
        pix.scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
}
