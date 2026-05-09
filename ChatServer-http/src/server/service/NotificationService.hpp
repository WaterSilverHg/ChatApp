#pragma once

#include "global.h"
#include "../vo/NotificationVo.hpp"
#include "../postgresql/AppClient.hpp"

class NotificationService {
private:
    std::shared_ptr<AppClient> m_appClient;
    using Status = oatpp::web::protocol::http::Status;
public:
    NotificationService(const std::shared_ptr<AppClient>& appClient) : m_appClient(appClient) {}

    oatpp::Vector<oatpp::Object<NotificationVO>> getNotifications(const oatpp::String& currentUserIdHeader) {
        auto userCheck = m_appClient->getUserIdByUuid(currentUserIdHeader);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(userCheck->isSuccess(), Status::CODE_500, userCheck->getErrorMessage());
        OATPP_ASSERT_HTTP(userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
        #else
        OATPP_ASSERT_HTTP(userCheck->isSuccess() && userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
        #endif
        auto id = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        auto result = m_appClient->getNotifications(id);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, result->getErrorMessage());
        #else
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, "获取通知失败");
        #endif
        return result->fetch<oatpp::Vector<oatpp::Object<NotificationVO>>>();
    }

    oatpp::Object<UnreadCountVO> getUnreadCount(const oatpp::String& currentUserIdHeader) {
        auto userCheck = m_appClient->getUserIdByUuid(currentUserIdHeader);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(userCheck->isSuccess(), Status::CODE_500, userCheck->getErrorMessage());
        OATPP_ASSERT_HTTP(userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
        #else
        OATPP_ASSERT_HTTP(userCheck->isSuccess() && userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
        #endif
        auto id = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        auto result = m_appClient->getUnreadCount(id);
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

        auto userCheck = m_appClient->getUserIdByUuid(currentUserIdHeader);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(userCheck->isSuccess(), Status::CODE_500, userCheck->getErrorMessage());
        OATPP_ASSERT_HTTP(userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
        #else
        OATPP_ASSERT_HTTP(userCheck->isSuccess() && userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
        #endif
        auto userIdInt = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        auto result = m_appClient->markNotificationRead(notificationId, userIdInt);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, result->getErrorMessage());
        #else
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_400, "标记已读失败");
        #endif
        return true;
    }

    oatpp::Boolean markAllAsRead(const oatpp::String& currentUserIdHeader) {
        auto userCheck = m_appClient->getUserIdByUuid(currentUserIdHeader);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(userCheck->isSuccess(), Status::CODE_500, userCheck->getErrorMessage());
        OATPP_ASSERT_HTTP(userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
        #else
        OATPP_ASSERT_HTTP(userCheck->isSuccess() && userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
        #endif
        auto id = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        auto result = m_appClient->markAllNotificationsRead(id);
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

        auto userCheck = m_appClient->getUserIdByUuid(currentUserIdHeader);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(userCheck->isSuccess(), Status::CODE_500, userCheck->getErrorMessage());
        OATPP_ASSERT_HTTP(userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
        #else
        OATPP_ASSERT_HTTP(userCheck->isSuccess() && userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
        #endif
        auto userIdInt = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        auto result = m_appClient->deleteNotification(notificationId, userIdInt);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, result->getErrorMessage());
        #else
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_400, "删除通知失败");
        #endif
        return true;
    }

    oatpp::Boolean batchDeleteNotifications(const oatpp::String& currentUserIdHeader, const oatpp::Object<BatchDeleteNotificationsRequest>& request) {
        auto userCheck = m_appClient->getUserIdByUuid(currentUserIdHeader);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(userCheck->isSuccess(), Status::CODE_500, userCheck->getErrorMessage());
        OATPP_ASSERT_HTTP(userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
        #else
        OATPP_ASSERT_HTTP(userCheck->isSuccess() && userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
        #endif
        auto userIdInt = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        for (const auto& notificationId : *request->notificationIds) {
            auto result = m_appClient->deleteNotification(notificationId, userIdInt);
            #ifdef SQLCHECK
            OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, result->getErrorMessage());
            #else
            OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_400, "删除通知失败");
            #endif
        }
        return true;
    }

    oatpp::Boolean clearNotifications(const oatpp::String& currentUserIdHeader) {
        auto userCheck = m_appClient->getUserIdByUuid(currentUserIdHeader);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(userCheck->isSuccess(), Status::CODE_500, userCheck->getErrorMessage());
        OATPP_ASSERT_HTTP(userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
        #else
        OATPP_ASSERT_HTTP(userCheck->isSuccess() && userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
        #endif
        auto id = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        auto result = m_appClient->clearNotifications(id);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, result->getErrorMessage());
        #else
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_400, "清空通知失败");
        #endif
        return true;
    }
};
