#pragma once
#include "../global.h"

// ============================================================
// MessageCacheWorker — 在后台线程中执行磁盘 I/O，UI 线程不阻塞
// ============================================================
class MessageCacheWorker : public QObject
{
    Q_OBJECT
public:
    explicit MessageCacheWorker(QObject* parent = nullptr);

public slots:
    void doLoad(const QString& convUuid);
    void doSave(const QString& convUuid, const QJsonArray& messages);
    void doAppend(const QString& convUuid, const QJsonObject& message);
    void setCurrentUserUuid(const QString& uuid) { m_currentUserUuid = uuid; }

signals:
    void loadFinished(const QString& convUuid, QJsonArray messages);

private:
    QString cacheDirPath() const;
    QString filePath(const QString& convUuid) const;
    void ensureDir();
    QString m_currentUserUuid;
};

// ============================================================
// MessageCacheManager — 主线程接口，内部通过 Worker 线程做 I/O
// ============================================================
class MessageCacheManager : public QObject
{
    Q_OBJECT
public:
    explicit MessageCacheManager(QObject* parent = nullptr);
    ~MessageCacheManager();

    // 异步从磁盘加载会话消息，完成后通过 loadFinished 信号返回
    void loadAsync(const QString& convUuid);

    // 异步保存会话消息到磁盘
    void saveAsync(const QString& convUuid, const QJsonArray& messages);

    // 异步追加单条消息到磁盘缓存（增量写入，不重写整个文件）
    void appendAsync(const QString& convUuid, const QJsonObject& message);

    // ---- 每会话分页状态（纯内存，不用线程）----
    void setHasMore(const QString& convUuid, bool has);
    bool hasMore(const QString& convUuid) const;

    // 检查磁盘缓存是否存在（同步，轻量操作）
    bool exists(const QString& convUuid) const;

    // 设置当前用户 UUID，用于缓存目录隔离（不同用户的缓存文件分开存储）
    void setCurrentUserUuid(const QString& uuid);

signals:
    void loadFinished(const QString& convUuid, QJsonArray messages);
    void requestLoad(const QString& convUuid);
    void requestSave(const QString& convUuid, QJsonArray messages);
    void requestAppend(const QString& convUuid, QJsonObject message);
    void requestSetUserUuid(const QString& uuid);

private:
    QThread* m_workerThread;
    MessageCacheWorker* m_worker;
    QHash<QString, bool> m_hasMore;       // 分会话分页状态
    QString m_currentUserUuid_main;        // 主线程侧 UUID 副本（exists() 使用）
};
