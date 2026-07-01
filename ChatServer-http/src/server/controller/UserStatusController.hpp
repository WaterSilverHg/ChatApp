#pragma once

#include "global.h"
#include "server/dto/UserStatusDto.hpp"
#include "server/vo/UserStatusVO.hpp"
#include "server/service/UserStatusService.hpp"
#include "server/handler/AppAuthHandler.h"

#include OATPP_CODEGEN_BEGIN(ApiController) 

class UserStatusController : public oatpp::web::server::api::ApiController {
private:
    std::shared_ptr<StatusService> m_statusService;

public:
    UserStatusController(OATPP_COMPONENT(std::shared_ptr<ObjectMapper>, objectMapper),
                     OATPP_COMPONENT(std::shared_ptr<AppPostgresql>, appPostgresql),
                     OATPP_COMPONENT(std::shared_ptr<AppRedis>, redis),
                     OATPP_COMPONENT(std::shared_ptr<UuidIdCache>, uuidIdCache))
        : oatpp::web::server::api::ApiController(objectMapper),
          m_statusService(std::make_shared<StatusService>(appPostgresql, redis, uuidIdCache)) {
        setDefaultAuthorizationHandler(std::make_shared<AppAuthHandler>());
    }

    static std::shared_ptr<UserStatusController> createShared(
        OATPP_COMPONENT(std::shared_ptr<ObjectMapper>, objectMapper),
        OATPP_COMPONENT(std::shared_ptr<AppPostgresql>, appPostgresql),
        OATPP_COMPONENT(std::shared_ptr<AppRedis>, redis),
        OATPP_COMPONENT(std::shared_ptr<UuidIdCache>, uuidIdCache)
    ) {
        return std::make_shared<UserStatusController>(objectMapper, appPostgresql, redis, uuidIdCache);
    }

    ENDPOINT_INFO(updateStatus) {
        info->summary = "更新用户状态";
        info->description = "更新当前用户的状态，支持online, offline, away, busy四种状态";
        info->addResponse<Object<UserStatusVO>>(Status::CODE_200, "application/json", "更新成功");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_401, "application/json", "用户不存在或已失效");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_401, "application/json", "未提供用户认证信息");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_400, "application/json", "状态参数不能为空");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_400, "application/json", "无效的状态值，必须是online, offline, away, busy之一");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_500, "application/json", "更新状态失败");
        info->addConsumes<Object<UpdateStatusRequestDTO>>("application/json");
        info->addSecurityRequirement("BearerAuth");
    }
    ENDPOINT("PUT", "/api/status", updateStatus,
        BODY_DTO(Object<UpdateStatusRequestDTO>, request),
        AUTHORIZATION(std::shared_ptr<Appjwt::Payload>, authObject)) {
        return createDtoResponse(Status::CODE_200, m_statusService->updateStatus(authObject->userUuid, request));
    }

    ENDPOINT_INFO(getMultipleStatuses) {
        info->summary = "获取多个用户状态";
        info->description = "批量获取多个用户的状态信息";
        info->addResponse<oatpp::Vector<Object<UserStatusVO>>>(Status::CODE_200, "application/json", "获取成功");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_401, "application/json", "用户不存在或已失效");
        info->addConsumes<Object<BatchStatusRequestDTO>>("application/json");
        info->addSecurityRequirement("BearerAuth");
    }
    ENDPOINT("POST", "/api/status/batch", getMultipleStatuses, 
        BODY_DTO(Object<BatchStatusRequestDTO>, request),
        AUTHORIZATION(std::shared_ptr<Appjwt::Payload>, authObject)) {
        return createDtoResponse(Status::CODE_200, m_statusService->getMultipleStatuses(authObject->userUuid, request));
    }

    //ENDPOINT_INFO(getCurrentUser) {
    //    info->summary = "获取当前用户信息";
    //    info->description = "根据用户ID获取当前用户的详细信息";
    //    info->addResponse<Object<UserInfoVO>>(Status::CODE_200, "application/json", "获取成功");
    //    info->addResponse<Object<ErrorStatusDto>>(Status::CODE_401, "application/json", "用户不存在或已失效");
    //    info->addResponse<Object<ErrorStatusDto>>(Status::CODE_404, "application/json", "用户不存在");
    //    info->addResponse<Object<ErrorStatusDto>>(Status::CODE_400, "application/json", "请求参数错误");
    //    info->addSecurityRequirement("BearerAuth");
    //}
    //ENDPOINT("GET", "/api/user/me", getCurrentUser, 
    //    AUTHORIZATION(std::shared_ptr<Appjwt::Payload>, authObject)) {
    //    return createDtoResponse(Status::CODE_200, m_statusService->getCurrentUser(authObject->userUuid));
    //}

    ENDPOINT_INFO(searchUsers) {
        info->summary = "搜索用户";
        info->description = "根据用户名搜索用户，返回用户信息及好友关系状态";
        info->addResponse<oatpp::Vector<Object<SearchUserVO>>>(Status::CODE_200, "application/json", "搜索成功");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_401, "application/json", "用户不存在或已失效");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_400, "application/json", "搜索关键词不能为空");
        info->addSecurityRequirement("BearerAuth");
    }
    ENDPOINT("GET", "/api/users/search", searchUsers,
        QUERY(String, keyword, "keyword"),
        AUTHORIZATION(std::shared_ptr<Appjwt::Payload>, authObject)) {
        return createDtoResponse(Status::CODE_200, m_statusService->searchUsers(oatpp::encoding::Url::decode(keyword),authObject->userUuid));
    }

    ENDPOINT_INFO(getUserInfoByUuid) {
        info->summary = "根据UUID获取用户信息";
        info->description = "根据用户UUID获取用户的详细信息";
        info->addResponse<Object<UserInfoVO>>(Status::CODE_200, "application/json", "获取成功");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_401, "application/json", "用户不存在或已失效");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_404, "application/json", "用户不存在");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_400, "application/json", "请求参数错误");
        info->addSecurityRequirement("BearerAuth");
    }
    ENDPOINT("GET", "/api/users/info/{userUuid}", getUserInfoByUuid,
        PATH(String, userUuid, "userUuid"),
        AUTHORIZATION(std::shared_ptr<Appjwt::Payload>, authObject)) {
        return createDtoResponse(Status::CODE_200, m_statusService->getUserInfoByUuid(userUuid));
    }

    ENDPOINT_INFO(updateProfile) {
        info->summary = "更新个人信息";
        info->description = "更新当前用户的用户名和头像";
        info->addResponse<Object<UserInfoVO>>(Status::CODE_200, "application/json", "更新成功");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_401, "application/json", "用户不存在或已失效");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_500, "application/json", "更新失败");
        info->addConsumes<Object<UpdateProfileRequestDTO>>("application/json");
        info->addSecurityRequirement("BearerAuth");
    }
    ENDPOINT("PUT", "/api/user/profile", updateProfile,
        BODY_DTO(Object<UpdateProfileRequestDTO>, request),
        AUTHORIZATION(std::shared_ptr<Appjwt::Payload>, authObject)) {
        return createDtoResponse(Status::CODE_200, m_statusService->updateUserInfo(request, authObject->userUuid));
    }
};

#include OATPP_CODEGEN_END(ApiController)
