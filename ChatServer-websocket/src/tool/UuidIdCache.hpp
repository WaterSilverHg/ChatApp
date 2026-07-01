#pragma once

#include"global.h"
#include "redis/AppRedis.hpp"
#include "server/postgresql/AppPostgresql.hpp"
#include "server/dto/GeneralDto.hpp"

// ============================================================
// UuidIdCache — Redis 缓存 UUID→BIGINT ID 映射，防缓存穿透
//
//   - 命中的键值缓存 1 小时
//   - 未命中（null）的键缓存 60 秒空字符串，防恶意请求穿透 DB
// ============================================================
class UuidIdCache {
public:
    UuidIdCache(std::shared_ptr<AppRedis> redis,
        std::shared_ptr<AppPostgresql> appClient)
        : m_redis(redis)
        , m_appClient(appClient)
    {}

    // ----------------------------------------------------------
    // User UUID → BIGINT id
    // ----------------------------------------------------------
    oatpp::Int64 getUserId(const oatpp::String& userUuid) {
        return resolve("user", userUuid, [this](const oatpp::String& uuid) {
            auto r = m_appClient->getUserIdByUuid(uuid);
#ifdef SQLCHECK
            if (!r->isSuccess()) {
                OATPP_LOGD("UuidIdCache", "getUserId failed: %s", r->getErrorMessage()->c_str());
                return oatpp::Int64(-1);
            }
            if (!r->hasMoreToFetch()) {
                OATPP_LOGD("UuidIdCache", "getUserId uuid=%s not found, return -1", uuid->c_str());
                return oatpp::Int64(-1);
            }
#else
            if (!r->isSuccess() || !r->hasMoreToFetch()) return oatpp::Int64(-1);
#endif
            return r->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;
            });
    }

    // ----------------------------------------------------------
    // Group UUID → BIGINT id
    // ----------------------------------------------------------
    oatpp::Int64 getGroupId(const oatpp::String& groupUuid) {
        return resolve("group", groupUuid, [this](const oatpp::String& uuid) {
            auto r = m_appClient->getGroupIdByUuid(uuid);
#ifdef SQLCHECK
            if (!r->isSuccess()) {
                OATPP_LOGD("UuidIdCache", "getGroupId failed: %s", r->getErrorMessage()->c_str());
                return oatpp::Int64(-1);
            }
            if (!r->hasMoreToFetch()) {
                OATPP_LOGD("UuidIdCache", "getGroupId uuid=%s not found, return -1", uuid->c_str());
                return oatpp::Int64(-1);
            }
#else
            if (!r->isSuccess() || !r->hasMoreToFetch()) return oatpp::Int64(-1);
#endif
            return r->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;
            });
    }

    // ----------------------------------------------------------
    // Message UUID → BIGINT id
    // ----------------------------------------------------------
    oatpp::Int64 getMessageId(const oatpp::String& messageUuid) {
        return resolve("msg", messageUuid, [this](const oatpp::String& uuid) {
            auto r = m_appClient->getMessageIdByUuid(uuid);
#ifdef SQLCHECK
            if (!r->isSuccess()) {
                OATPP_LOGD("UuidIdCache", "getMessageId failed: %s", r->getErrorMessage()->c_str());
                return oatpp::Int64(-1);
            }
            if (!r->hasMoreToFetch()) {
                OATPP_LOGD("UuidIdCache", "getMessageId uuid=%s not found, return -1", uuid->c_str());
                return oatpp::Int64(-1);
            }
#else
            if (!r->isSuccess() || !r->hasMoreToFetch()) return oatpp::Int64(-1);
#endif
            return r->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;
            });
    }

    // ----------------------------------------------------------
    // File UUID → BIGINT id
    // ----------------------------------------------------------
    oatpp::Int64 getFileId(const oatpp::String& fileUuid) {
        return resolve("file", fileUuid, [this](const oatpp::String& uuid) {
            auto r = m_appClient->getFileIdByUuid(uuid);
#ifdef SQLCHECK
            if (!r->isSuccess()) {
                OATPP_LOGD("UuidIdCache", "getFileId failed: %s", r->getErrorMessage()->c_str());
                return oatpp::Int64(-1);
            }
            if (!r->hasMoreToFetch()) {
                OATPP_LOGD("UuidIdCache", "getFileId uuid=%s not found, return -1", uuid->c_str());
                return oatpp::Int64(-1);
            }
#else
            if (!r->isSuccess() || !r->hasMoreToFetch()) return oatpp::Int64(-1);
#endif
            return r->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;
            });
    }

    // ----------------------------------------------------------
    // BIGINT id → User UUID
    // ----------------------------------------------------------
    oatpp::String getUserUuid(oatpp::Int64 userId) {
        if (userId <= 0) return nullptr;
        try {
            auto r = m_appClient->getUserUuidById(userId);
            if (r->isSuccess() && r->hasMoreToFetch()) {
                return r->fetch<oatpp::Vector<oatpp::Object<UuidDTO>>>()[0]->uuid;
            }
        } catch (const std::exception& e) {
            OATPP_LOGE("UuidIdCache", "getUserUuid failed: %s", e.what());
        }
        return nullptr;
    }

    // ----------------------------------------------------------
    // BIGINT id → Group UUID
    // ----------------------------------------------------------
    oatpp::String getGroupUuid(oatpp::Int64 groupId) {
        if (groupId <= 0) return nullptr;
        try {
            auto r = m_appClient->getGroupUuidById(groupId);
            if (r->isSuccess() && r->hasMoreToFetch()) {
                return r->fetch<oatpp::Vector<oatpp::Object<UuidDTO>>>()[0]->uuid;
            }
        } catch (const std::exception& e) {
            OATPP_LOGE("UuidIdCache", "getGroupUuid failed: %s", e.what());
        }
        return nullptr;
    }

    // ----------------------------------------------------------
    // 主动失效某条缓存（例如用户信息变更后）
    // ----------------------------------------------------------
    void invalidateUser(const oatpp::String& uuid) { delKey("user", uuid); }
    void invalidateGroup(const oatpp::String& uuid) { delKey("group", uuid); }
    void invalidateMessage(const oatpp::String& uuid) { delKey("msg", uuid); }
    void invalidateFile(const oatpp::String& uuid) { delKey("file", uuid); }

private:
    std::shared_ptr<AppRedis> m_redis;
    std::shared_ptr<AppPostgresql> m_appClient;

    static constexpr int POSITIVE_TTL = 3600;   // 正常值：1 小时
    static constexpr int NEGATIVE_TTL = 60;     // 空值：60 秒防穿透

    std::string buildKey(const std::string& type, const oatpp::String& uuid) {
        return "uuid:id:" + type + ":" + std::string(uuid->c_str());
    }

    void delKey(const std::string& type, const oatpp::String& uuid) {
        try {
            m_redis->getHandle()->del(buildKey(type, uuid));
        }
        catch (...) {}
    }

    // 通用解析流程：Redis 命中 → 返回 | Redis 未命中 → DB 查询 → 缓存 → 返回
    template<typename Fn>
    oatpp::Int64 resolve(const std::string& type, const oatpp::String& uuid, Fn dbQuery) {
        if (!uuid || uuid->empty()) return -1;

        std::string key = buildKey(type, uuid);

        // 1. 先查 Redis
        try {
            auto cached = m_redis->getHandle()->get(key);
            if (cached) {
                if (cached->empty()) {
                    return -1;                         // 缓存的空值（防穿透标记）
                }
                return oatpp::Int64(std::stoll(*cached));
            }
        }
        catch (const std::exception& e) {
            // Redis 不可用时退化到直接查 DB
        }

        // 2. Redis 未命中 → 查 DB
        oatpp::Int64 id = dbQuery(uuid);

        // 3. 回写 Redis
        try {
            if (id > 0) {
                m_redis->getHandle()->set(key, std::to_string(id.getValue(0)),
                    std::chrono::seconds(POSITIVE_TTL));
            }
            else {
                // 缓存空值，防穿透
                m_redis->getHandle()->set(key, "",
                    std::chrono::seconds(NEGATIVE_TTL));
            }
        }
        catch (...) {}

        return id;
    }
};
