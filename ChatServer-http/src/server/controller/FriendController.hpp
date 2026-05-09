#pragma once

#include "global.h"
#include "../dto/FriendDto.hpp"
#include "../vo/FriendVO.hpp"
#include "../service/FriendService.hpp"
#include "../handler/AppAuthHandler.h"

#include OATPP_CODEGEN_BEGIN(ApiController) 

class FriendController : public oatpp::web::server::api::ApiController {
private:
    std::shared_ptr<FriendService> m_friendService;

public:
    FriendController(OATPP_COMPONENT(std::shared_ptr<ObjectMapper>, objectMapper),
                     OATPP_COMPONENT(std::shared_ptr<AppClient>, appClient))
        : oatpp::web::server::api::ApiController(objectMapper),
          m_friendService(std::make_shared<FriendService>(appClient)) {
        setDefaultAuthorizationHandler(std::make_shared<AppAuthHandler>());
    }

    static std::shared_ptr<FriendController> createShared(
        OATPP_COMPONENT(std::shared_ptr<ObjectMapper>, objectMapper),
        OATPP_COMPONENT(std::shared_ptr<AppClient>, appClient)
    ) {
        return std::make_shared<FriendController>(objectMapper, appClient);
    }

    ENDPOINT_INFO(getSentRequests) {
        info->summary = "获取已发送的请求";
        info->description = "获取当前用户已发送的好友请求列表";
        info->addResponse<oatpp::Vector<oatpp::Object<FriendResponseVO>>>(Status::CODE_200, "application/json", "获取成功");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_401, "application/json", "用户不存在或已失效");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_500, "application/json", "获取请求失败");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_400, "application/json", "请求参数错误");
        info->addSecurityRequirement("BearerAuth");
    }
    ENDPOINT("GET", "/api/friends/requests/sent", getSentRequests,
        AUTHORIZATION(std::shared_ptr<Appjwt::Payload>, authObject)) {
        return createDtoResponse(Status::CODE_200, m_friendService->getSentRequests(authObject->userUuid));
    }

    ENDPOINT_INFO(getReceivedRequests) {
        info->summary = "获取收到的请求";
        info->description = "获取当前用户收到的好友请求列表";
        info->addResponse<oatpp::Vector<oatpp::Object<FriendResponseVO>>>(Status::CODE_200, "application/json", "获取成功");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_401, "application/json", "用户不存在或已失效");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_500, "application/json", "获取请求失败");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_400, "application/json", "请求参数错误");
        info->addSecurityRequirement("BearerAuth");
    }
    ENDPOINT("GET", "/api/friends/requests/received", getReceivedRequests,
        AUTHORIZATION(std::shared_ptr<Appjwt::Payload>, authObject)) {
        return createDtoResponse(Status::CODE_200, m_friendService->getReceivedRequests(authObject->userUuid));
    }

    ENDPOINT_INFO(getFriends) {
        info->summary = "获取好友列表";
        info->description = "获取当前用户的好友列表";
        info->addResponse<oatpp::Vector<oatpp::Object<FriendInfoVO>>>(Status::CODE_200, "application/json", "获取成功");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_401, "application/json", "用户不存在或已失效");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_500, "application/json", "获取好友列表失败");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_400, "application/json", "请求参数错误");
        info->addSecurityRequirement("BearerAuth");
    }
    ENDPOINT("GET", "/api/friends", getFriends,
        AUTHORIZATION(std::shared_ptr<Appjwt::Payload>, authObject)) {
        return createDtoResponse(Status::CODE_200, m_friendService->getFriends(authObject->userUuid));
    }

    ENDPOINT_INFO(updateFriendRemark) {
        info->summary = "修改好友备注";
        info->description = "修改指定好友的备注信息";
        info->addResponse<Boolean>(Status::CODE_200, "application/json", "修改成功");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_401, "application/json", "用户不存在或已失效");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_400, "application/json", "修改备注失败");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_400, "application/json", "请求参数错误");
        info->addConsumes<Object<UpdateFriendRemarkDTO>>("application/json");
        info->addSecurityRequirement("BearerAuth");
    }
    ENDPOINT("PUT", "/api/friends/{friendUuid}/remark", updateFriendRemark,
        PATH(String, friendUuid, "friendUuid"),
        BODY_DTO(Object<UpdateFriendRemarkDTO>, request),
        AUTHORIZATION(std::shared_ptr<Appjwt::Payload>, authObject)) {
        return createDtoResponse(Status::CODE_200, m_friendService->updateFriendRemark(authObject->userUuid, friendUuid, request));
    }

    ENDPOINT_INFO(updateFriendGroup) {
        info->summary = "移动好友到分组";
        info->description = "将指定好友移动到指定分组";
        info->addResponse<Boolean>(Status::CODE_200, "application/json", "移动成功");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_401, "application/json", "用户不存在或已失效");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_400, "application/json", "移动分组失败");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_400, "application/json", "请求参数错误");
        info->addConsumes<Object<UpdateFriendGroupDTO>>("application/json");
        info->addSecurityRequirement("BearerAuth");
    }
    ENDPOINT("PUT", "/api/friends/{friendUuid}/group", updateFriendGroup,
        PATH(String, friendUuid, "friendUuid"),
        BODY_DTO(Object<UpdateFriendGroupDTO>, request),
        AUTHORIZATION(std::shared_ptr<Appjwt::Payload>, authObject)) {
        return createDtoResponse(Status::CODE_200, m_friendService->updateFriendGroup(authObject->userUuid, friendUuid, request));
    }

    ENDPOINT_INFO(blockUser) {
        info->summary = "拉黑用户";
        info->description = "将指定用户加入黑名单";
        info->addResponse<Boolean>(Status::CODE_200, "application/json", "拉黑成功");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_401, "application/json", "用户不存在或已失效");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_404, "application/json", "目标用户不存在");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_400, "application/json", "拉黑用户失败");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_400, "application/json", "请求参数错误");
        info->addSecurityRequirement("BearerAuth");
    }
    ENDPOINT("POST", "/api/friends/block/{targetUserUuid}", blockUser,
        PATH(String, targetUserUuid, "targetUserUuid"),
        AUTHORIZATION(std::shared_ptr<Appjwt::Payload>, authObject)) {
        return createDtoResponse(Status::CODE_200, m_friendService->blockUser(authObject->userUuid, targetUserUuid));
    }

    ENDPOINT_INFO(unblockUser) {
        info->summary = "取消拉黑用户";
        info->description = "将指定用户从黑名单中移除";
        info->addResponse<Boolean>(Status::CODE_200, "application/json", "取消拉黑成功");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_401, "application/json", "用户不存在或已失效");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_404, "application/json", "目标用户不存在");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_400, "application/json", "取消拉黑用户失败");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_400, "application/json", "请求参数错误");
        info->addSecurityRequirement("BearerAuth");
    }
    ENDPOINT("POST", "/api/friends/unblock/{targetUserUuid}", unblockUser,
        PATH(String, targetUserUuid, "targetUserUuid"),
        AUTHORIZATION(std::shared_ptr<Appjwt::Payload>, authObject)) {
        return createDtoResponse(Status::CODE_200, m_friendService->unblockUser(authObject->userUuid, targetUserUuid));
    }
};

#include OATPP_CODEGEN_END(ApiController)
