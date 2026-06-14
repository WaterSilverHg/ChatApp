#include "MessageCacheManager.h"

// ============================================================
// Worker
// ============================================================

MessageCacheWorker::MessageCacheWorker(QObject* parent) : QObject(parent) {}

void MessageCacheWorker::ensureDir() {
    QDir dir(cacheDirPath());
    if (!dir.exists()) dir.mkpath(cacheDirPath());
}

QString MessageCacheWorker::cacheDirPath() const {
    if (!m_currentUserUuid.isEmpty()) {
        return QCoreApplication::applicationDirPath() + "/message_cache/" + m_currentUserUuid;
    }
    return QCoreApplication::applicationDirPath() + "/message_cache";
}

QString MessageCacheWorker::filePath(const QString& convUuid) const {
    return cacheDirPath() + "/" + convUuid + ".json";
}

void MessageCacheWorker::doLoad(const QString& convUuid) {
    ensureDir();
    QFile file(filePath(convUuid));
    if (!file.open(QIODevice::ReadOnly)) {
        emit loadFinished(convUuid, QJsonArray());
        return;
    }
    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonArray arr = (doc.isArray()) ? doc.array() : QJsonArray();
    emit loadFinished(convUuid, arr);
}

void MessageCacheWorker::doSave(const QString& convUuid, const QJsonArray& messages) {
    ensureDir();
    QFile file(filePath(convUuid));
    if (file.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(messages);
        file.write(doc.toJson());
        file.close();
    }
}

void MessageCacheWorker::doAppend(const QString& convUuid, const QJsonObject& message) {
    ensureDir();
    QJsonArray arr;
    QFile file(filePath(convUuid));
    if (file.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
        if (doc.isArray()) arr = doc.array();
        file.close();
    }
    arr.append(message);
    if (file.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(arr);
        file.write(doc.toJson());
        file.close();
    }
}

// ============================================================
// Manager
// ============================================================

MessageCacheManager::MessageCacheManager(QObject* parent)
    : QObject(parent)
{
    m_workerThread = new QThread(this);
    m_worker = new MessageCacheWorker();  // 无 parent，由 moveToThread 管理
    m_worker->moveToThread(m_workerThread);

    // 主线程 → worker 线程 的请求
    connect(this, &MessageCacheManager::requestLoad,  m_worker, &MessageCacheWorker::doLoad);
    connect(this, &MessageCacheManager::requestSave,  m_worker, &MessageCacheWorker::doSave);
    connect(this, &MessageCacheManager::requestAppend,m_worker, &MessageCacheWorker::doAppend);
    connect(this, &MessageCacheManager::requestSetUserUuid, m_worker, &MessageCacheWorker::setCurrentUserUuid);

    // worker 线程 → 主线程 的结果
    connect(m_worker, &MessageCacheWorker::loadFinished,
            this, &MessageCacheManager::loadFinished);

    m_workerThread->start();
}

MessageCacheManager::~MessageCacheManager() {
    m_workerThread->quit();
    m_workerThread->wait();
    delete m_worker;  // worker thread 退出后安全删除
}

void MessageCacheManager::loadAsync(const QString& convUuid) {
    emit requestLoad(convUuid);
}

void MessageCacheManager::saveAsync(const QString& convUuid, const QJsonArray& messages) {
    emit requestSave(convUuid, messages);
}

void MessageCacheManager::appendAsync(const QString& convUuid, const QJsonObject& message) {
    emit requestAppend(convUuid, message);
}

void MessageCacheManager::setHasMore(const QString& convUuid, bool has) {
    m_hasMore[convUuid] = has;
}

bool MessageCacheManager::hasMore(const QString& convUuid) const {
    return m_hasMore.value(convUuid, true);  // 默认还有更多
}

void MessageCacheManager::setCurrentUserUuid(const QString& uuid) {
    m_currentUserUuid_main = uuid;
    emit requestSetUserUuid(uuid);
}

bool MessageCacheManager::exists(const QString& convUuid) const {
    // exists() 在 setCurrentUserUuid 之前可能被调用（构造期间），此时使用旧路径兜底
    QString baseDir = QCoreApplication::applicationDirPath() + "/message_cache";
    // 由于 worker 线程可能还没设置 uuid，这里需要在主线程侧维护一份 uuid 副本
    // 简化处理：直接检查两个可能的路径
    if (!m_currentUserUuid_main.isEmpty()) {
        baseDir = baseDir + "/" + m_currentUserUuid_main;
    }
    QString path = baseDir + "/" + convUuid + ".json";
    return QFile::exists(path);
}
