#pragma once

#include "global.h"
#include "../vo/NotificationVo.hpp"
#include "../postgresql/AppPostgresql.hpp"
#include "../../redis/AppRedis.hpp"
#include "../../tool/UuidIdCache.hpp"

class NotificationService {
private:
    std::shared_ptr<AppPostgresql> m_appPostgresql;
    std::shared_ptr<AppRedis> m_redis;
    std::shared_ptr<UuidIdCache> m_idCache;
    using Status = oatpp::web::protocol::http::Status;
public:
    NotificationService(const std::shared_ptr<AppPostgresql>& appClient, 
                        const std::shared_ptr<AppRedis>& redis,
                        const std::shared_ptr<UuidIdCache>& idCache)
        : m_appPostgresql(appClient), m_redis(redis), m_idCache(idCache) {}

    oatpp::Vector<oatpp::Object<NotificationVO>> getNotifications(const oatpp::String& currentUserIdHeader) {
        auto id = m_idCache->getUserId(currentUserIdHeader);
        OATPP_ASSERT_HTTP(id > 0, Status::CODE_401, "用户不存在或已失效");

        auto result = m_appPostgresql->getNotifications(id);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, result->getErrorMessage());
        #else
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, "获取通知失败");
        #endif
        return result->fetch<oatpp::Vector<oatpp::Object<NotificationVO>>>();
    }

    oatpp::Object<UnreadCountVO> getUnreadCount(const oatpp::String& currentUserIdHeader) {
        auto id = m_idCache->getUserId(currentUserIdHeader);
        OATPP_ASSERT_HTTP(id > 0, Status::CODE_401, "用户不存在或已失效");

        auto result = m_appPostgresql->getUnreadCount(id);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, result->getErrorMessage());
        #else
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, "获取未读数量失败");
        #endif

        auto count = result->fetch<oatpp::Int32>();
        auto response = UnreadCountVO::createShared();
        response->count = count;
        return response;
    }

    oatpp::Boolean markAsRead(const oatpp::String& currentUserIdHeader, const oatpp::String& notificationIdStr) {
        int64_t notificationId;
        try {
            notificationId = std::stoll(notificationIdStr->c_str());
        } catch (...) {
            OATPP_ASSERT_HTTP(false, Status::CODE_400, "无效的通知ID");
        }

        auto userIdInt = m_idCache->getUserId(currentUserIdHeader);
        OATPP_ASSERT_HTTP(userIdInt > 0, Status::CODE_401, "用户不存在或已失效");

        auto result = m_appPostgresql->markNotificationRead(notificationId, userIdInt);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, result->getErrorMessage());
        #else
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_400, "标记已读失败");
        #endif
        return true;
    }

    oatpp::Boolean markAllAsRead(const oatpp::String& currentUserIdHeader) {
        auto id = m_idCache->getUserId(currentUserIdHeader);
        OATPP_ASSERT_HTTP(id > 0, Status::CODE_401, "用户不存在或已失效");

        auto result = m_appPostgresql->markAllNotificationsRead(id);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, result->getErrorMessage());
        #else
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_400, "标记全部已读失败");
        #endif
        return true;
    }

    oatpp::Boolean deleteNotification(const oatpp::String& currentUserIdHeader, const oatpp::String& notificationIdStr) {
        int64_t notificationId;
        try {
            notificationId = std::stoll(notificationIdStr->c_str());
        } catch (...) {
            OATPP_ASSERT_HTTP(false, Status::CODE_400, "无效的通知ID");
        }

        auto userIdInt = m_idCache->getUserId(currentUserIdHeader);
        OATPP_ASSERT_HTTP(userIdInt > 0, Status::CODE_401, "用户不存在或已失效");

        auto result = m_appPostgresql->deleteNotification(notificationId, userIdInt);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, result->getErrorMessage());
        #else
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_400, "删除通知失败");
        #endif
        return true;
    }

    oatpp::Boolean batchDeleteNotifications(const oatpp::String& currentUserIdHeader, const oatpp::Object<BatchDeleteNotificationsRequest>& request) {
        auto userIdInt = m_idCache->getUserId(currentUserIdHeader);
        OATPP_ASSERT_HTTP(userIdInt > 0, Status::CODE_401, "用户不存在或已失效");

        for (const auto& notificationId : *request->notificationIds) {
            auto result = m_appPostgresql->deleteNotification(notificationId, userIdInt);
            #ifdef SQLCHECK
            OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, result->getErrorMessage());
            #else
            OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_400, "删除通知失败");
            #endif
        }
        return true;
    }

    oatpp::Boolean clearNotifications(const oatpp::String& currentUserIdHeader) {
        auto id = m_idCache->getUserId(currentUserIdHeader);
        OATPP_ASSERT_HTTP(id > 0, Status::CODE_401, "用户不存在或已失效");

        auto result = m_appPostgresql->clearNotifications(id);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, result->getErrorMessage());
        #else
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_400, "清空通知失败");
        #endif
        return true;
    }
};
