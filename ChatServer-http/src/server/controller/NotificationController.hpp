#pragma once

#include "global.h"
#include "server/vo/NotificationVO.hpp"
#include "server/service/NotificationService.hpp"

#include OATPP_CODEGEN_BEGIN(ApiController) 

class NotificationController : public oatpp::web::server::api::ApiController {
private:
    std::shared_ptr<NotificationService> m_notificationService;

public:
    NotificationController(OATPP_COMPONENT(std::shared_ptr<ObjectMapper>, objectMapper),
                           OATPP_COMPONENT(std::shared_ptr<AppPostgresql>, appPostgresql),
                           OATPP_COMPONENT(std::shared_ptr<AppRedis>, redis),
                           OATPP_COMPONENT(std::shared_ptr<UuidIdCache>, uuidIdCache))
        : oatpp::web::server::api::ApiController(objectMapper),
          m_notificationService(std::make_shared<NotificationService>(appPostgresql, redis, uuidIdCache)) {}

    static std::shared_ptr<NotificationController> createShared(
        OATPP_COMPONENT(std::shared_ptr<ObjectMapper>, objectMapper),
        OATPP_COMPONENT(std::shared_ptr<AppPostgresql>, appPostgresql),
        OATPP_COMPONENT(std::shared_ptr<AppRedis>, redis),
        OATPP_COMPONENT(std::shared_ptr<UuidIdCache>, uuidIdCache)
    ) {
        return std::make_shared<NotificationController>(objectMapper, appPostgresql, redis, uuidIdCache);
    }

    ENDPOINT_INFO(getNotifications) {
        info->summary = "获取通知列表";
        info->description = "获取当前用户的通知列表";
        info->addResponse<oatpp::Vector<Object<NotificationVO>>>(Status::CODE_200, "application/json", "获取成功");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_401, "application/json", "用户不存在或已失效");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_500, "application/json", "获取通知列表失败");
    }
    ENDPOINT("GET", "/api/notifications", getNotifications,
        HEADER(String, userUuid, "userUuid")) {
        return createDtoResponse(Status::CODE_200, m_notificationService->getNotifications(userUuid));
    }

    ENDPOINT_INFO(getUnreadCount) {
        info->summary = "获取未读通知数量";
        info->description = "获取当前用户的未读通知数量";
        info->addResponse<Object<UnreadCountVO>>(Status::CODE_200, "application/json", "获取成功");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_401, "application/json", "用户不存在或已失效");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_500, "application/json", "获取未读通知数量失败");
    }
    ENDPOINT("GET", "/api/notifications/unread-count", getUnreadCount,
        HEADER(String, userUuid, "userUuid")) {
        return createDtoResponse(Status::CODE_200, m_notificationService->getUnreadCount(userUuid));
    }

    ENDPOINT_INFO(markAsRead) {
        info->summary = "标记通知为已读";
        info->description = "将指定通知标记为已读状态";
        info->addResponse<Boolean>(Status::CODE_200, "application/json", "标记成功");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_401, "application/json", "用户不存在或已失效");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_404, "application/json", "通知不存在");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_400, "application/json", "标记已读失败");
    }
    ENDPOINT("PUT", "/api/notifications/{notificationUuid}/read", markAsRead, 
        PATH(String, notificationUuid, "notificationUuid"),
        HEADER(String, userUuid, "userUuid")) {
        return createDtoResponse(Status::CODE_200, m_notificationService->markAsRead(userUuid, notificationUuid));
    }

    ENDPOINT_INFO(markAllAsRead) {
        info->summary = "标记所有通知为已读";
        info->description = "将当前用户的所有通知标记为已读状态";
        info->addResponse<Boolean>(Status::CODE_200, "application/json", "标记成功");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_401, "application/json", "用户不存在或已失效");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_400, "application/json", "标记全部已读失败");
    }
    ENDPOINT("PUT", "/api/notifications/read-all", markAllAsRead,
        HEADER(String, userUuid, "userUuid")) {
        return createDtoResponse(Status::CODE_200, m_notificationService->markAllAsRead(userUuid));
    }

    ENDPOINT_INFO(deleteNotification) {
        info->summary = "删除通知";
        info->description = "删除指定的通知";
        info->addResponse<Boolean>(Status::CODE_200, "application/json", "删除成功");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_401, "application/json", "用户不存在或已失效");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_400, "application/json", "删除通知失败");
    }
    ENDPOINT("DELETE", "/api/notifications/{notificationUuid}", deleteNotification, 
        PATH(String, notificationUuid, "notificationUuid"),
        HEADER(String, userUuid, "userUuid")) {
        return createDtoResponse(Status::CODE_200, m_notificationService->deleteNotification(userUuid, notificationUuid));
    }

    ENDPOINT_INFO(batchDeleteNotifications) {
        info->summary = "批量删除通知";
        info->description = "批量删除多个通知";
        info->addResponse<Boolean>(Status::CODE_200, "application/json", "删除成功");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_401, "application/json", "用户不存在或已失效");
        info->addConsumes<Object<BatchDeleteNotificationsRequest>>("application/json");
    }
    ENDPOINT("POST", "/api/notifications/batch-delete", batchDeleteNotifications, 
        BODY_DTO(Object<BatchDeleteNotificationsRequest>, request),
        HEADER(String, userUuid, "userUuid")) {
        return createDtoResponse(Status::CODE_200, m_notificationService->batchDeleteNotifications(userUuid, request));
    }

    ENDPOINT_INFO(clearNotifications) {
        info->summary = "清空通知";
        info->description = "清空当前用户的所有通知";
        info->addResponse<Boolean>(Status::CODE_200, "application/json", "清空成功");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_401, "application/json", "用户不存在或已失效");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_400, "application/json", "清空通知失败");
    }
    ENDPOINT("DELETE", "/api/notifications", clearNotifications,
        HEADER(String, userUuid, "userUuid")) {
        return createDtoResponse(Status::CODE_200, m_notificationService->clearNotifications(userUuid));
    }
};

#include OATPP_CODEGEN_END(ApiController)
