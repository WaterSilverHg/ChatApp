#pragma once

#include "global.h"
#include "server/dto/GroupDto.hpp"
#include "server/vo/GroupVo.hpp"
#include "server/service/GroupService.hpp"
#include "server/handler/AppAuthHandler.h"

#include OATPP_CODEGEN_BEGIN(ApiController) 

class GroupController : public oatpp::web::server::api::ApiController {
private:
    std::shared_ptr<GroupService> m_groupService;

public:
    GroupController(OATPP_COMPONENT(std::shared_ptr<ObjectMapper>, objectMapper),
                    OATPP_COMPONENT(std::shared_ptr<AppPostgresql>, appPostgresql),
                    OATPP_COMPONENT(std::shared_ptr<AppRedis>, redis),
                    OATPP_COMPONENT(std::shared_ptr<UuidIdCache>, uuidIdCache))
        : oatpp::web::server::api::ApiController(objectMapper),
          m_groupService(std::make_shared<GroupService>(appPostgresql, redis, uuidIdCache)) {
        setDefaultAuthorizationHandler(std::make_shared<AppAuthHandler>());
    }

    static std::shared_ptr<GroupController> createShared(
        OATPP_COMPONENT(std::shared_ptr<ObjectMapper>, objectMapper),
        OATPP_COMPONENT(std::shared_ptr<AppPostgresql>, appPostgresql),
        OATPP_COMPONENT(std::shared_ptr<AppRedis>, redis),
        OATPP_COMPONENT(std::shared_ptr<UuidIdCache>, uuidIdCache)
    ) {
        return std::make_shared<GroupController>(objectMapper, appPostgresql, redis, uuidIdCache);
    }

    ENDPOINT_INFO(searchGroups) {
        info->summary = "搜索群组";
        info->description = "根据群名搜索群组，同名结果按相关度+活跃度排序：优先展示用户已加入的群（用isJoined标记），其次按成员数降序";
        info->addResponse<oatpp::Vector<Object<GroupInfoVO>>>(Status::CODE_200, "application/json", "搜索成功");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_401, "application/json", "用户不存在或已失效");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_400, "application/json", "搜索关键词不能为空");
        info->addSecurityRequirement("BearerAuth");
    }
    ENDPOINT("GET", "/api/groups/search", searchGroups,
        QUERY(String, keyword, "keyword"),
        AUTHORIZATION(std::shared_ptr<Appjwt::Payload>, authObject)) {
        //auto decoded = oatpp::encoding::Url::decode(keyword);
        //OATPP_LOGD("Search", "Received keyword: %s", keyword->c_str());
        //OATPP_LOGD("Search", "Received decode keyword: %s", decoded->c_str());
        //OATPP_LOGD("Search", "Received keyword: %s", String("游戏")->c_str());
        return createDtoResponse(Status::CODE_200, m_groupService->searchGroups(oatpp::encoding::Url::decode(keyword), authObject->userUuid));
    }

    ENDPOINT_INFO(getMyGroups) {
        info->summary = "获取我的群组列表";
        info->description = "获取当前用户加入的所有群组，包含群组信息、角色、未读消息数等";
        info->addResponse<oatpp::Vector<Object<GroupInfoVO>>>(Status::CODE_200, "application/json", "获取成功");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_401, "application/json", "用户不存在或已失效");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_500, "application/json", "获取群组列表失败");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_400, "application/json", "请求参数错误");
        info->addSecurityRequirement("BearerAuth");
    }
    ENDPOINT("GET", "/api/groups/list", getMyGroups,
        AUTHORIZATION(std::shared_ptr<Appjwt::Payload>, authObject)) {
        return createDtoResponse(Status::CODE_200, m_groupService->getMyGroups(authObject->userUuid));
    }

    ENDPOINT_INFO(getGroupDetail) {
        info->summary = "获取群组详情";
        info->description = "根据群组UUID获取群组详细信息";
        info->addResponse<Object<GroupDetailInfoVO>>(Status::CODE_200, "application/json", "获取成功");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_401, "application/json", "用户不存在或已失效");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_403, "application/json", "您不是该群组的成员，无权查看");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_404, "application/json", "群组不存在");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_400, "application/json", "群组ID不能为空");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_400, "application/json", "请求参数错误");
        info->addSecurityRequirement("BearerAuth");
    }
    ENDPOINT("GET", "/api/groups/{groupUuid}", getGroupDetail,
        PATH(String, groupUuid, "groupUuid"),
        AUTHORIZATION(std::shared_ptr<Appjwt::Payload>, authObject)) {
        return createDtoResponse(Status::CODE_200, m_groupService->getGroupDetail(authObject->userUuid, groupUuid));
    }

    ENDPOINT_INFO(getGroupMembers) {
        info->summary = "获取群成员列表";
        info->description = "获取指定群组的成员列表";
        info->addResponse<oatpp::Vector<Object<GroupMemberVO>>>(Status::CODE_200, "application/json", "获取成功");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_401, "application/json", "群组不存在或已失效");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_404, "application/json", "群组不存在");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_400, "application/json", "群组ID不能为空");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_400, "application/json", "请求参数错误");
        info->addSecurityRequirement("BearerAuth");
    }
    ENDPOINT("GET", "/api/groups/{groupUuid}/members", getGroupMembers, 
        AUTHORIZATION(std::shared_ptr<Appjwt::Payload>, authObject),
        PATH(String, groupUuid, "groupUuid")) {
        return createDtoResponse(Status::CODE_200, m_groupService->getGroupMembers(groupUuid));
    }

    ENDPOINT_INFO(getReceivedGroupRequests) {
        info->summary = "获取收到的群聊请求";
        info->description = "获取当前用户作为管理员或群主收到的入群请求列表";
        info->addResponse<oatpp::Vector<Object<ReceivedGroupRequestVO>>>(Status::CODE_200, "application/json", "获取成功");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_401, "application/json", "用户不存在或已失效");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_500, "application/json", "获取群聊请求失败");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_400, "application/json", "请求参数错误");
        info->addSecurityRequirement("BearerAuth");
    }
    ENDPOINT("GET", "/api/groups/requests/received", getReceivedGroupRequests,
        AUTHORIZATION(std::shared_ptr<Appjwt::Payload>, authObject)) {
        return createDtoResponse(Status::CODE_200, m_groupService->getReceivedGroupRequests(authObject->userUuid));
    }

    ENDPOINT_INFO(getSentGroupRequests) {
        info->summary = "获取发送的群聊请求";
        info->description = "获取当前用户发送的入群请求列表";
        info->addResponse<oatpp::Vector<Object<SentGroupRequestVO>>>(Status::CODE_200, "application/json", "获取成功");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_401, "application/json", "用户不存在或已失效");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_500, "application/json", "获取群聊请求失败");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_400, "application/json", "请求参数错误");
        info->addSecurityRequirement("BearerAuth");
    }
    ENDPOINT("GET", "/api/groups/requests/sent", getSentGroupRequests,
        AUTHORIZATION(std::shared_ptr<Appjwt::Payload>, authObject)) {
        return createDtoResponse(Status::CODE_200, m_groupService->getSentGroupRequests(authObject->userUuid));
    }

    //ENDPOINT_INFO(sendGroupRequest) {
    //    info->summary = "发送群聊请求";
    //    info->description = "向指定群组发送入群请求";
    //    info->addResponse<Boolean>(Status::CODE_200, "application/json", "发送成功");
    //    info->addResponse<Object<ErrorStatusDto>>(Status::CODE_401, "application/json", "用户不存在或已失效");
    //    info->addResponse<Object<ErrorStatusDto>>(Status::CODE_404, "application/json", "群组不存在");
    //    info->addResponse<Object<ErrorStatusDto>>(Status::CODE_400, "application/json", "您已经是该群成员");
    //    info->addResponse<Object<ErrorStatusDto>>(Status::CODE_400, "application/json", "发送群聊请求失败");
    //    info->addResponse<Object<ErrorStatusDto>>(Status::CODE_400, "application/json", "请求参数错误");
    //    info->addConsumes<Object<SendGroupRequestDTO>>("application/json");
    //    info->addSecurityRequirement("BearerAuth");
    //}
    //ENDPOINT("POST", "/api/groups/{groupUuid}/requests", sendGroupRequest,
    //    PATH(String, groupUuid, "groupUuid"),
    //    BODY_DTO(Object<SendGroupRequestDTO>, request),
    //    AUTHORIZATION(std::shared_ptr<Appjwt::Payload>, authObject)) {
    //    return createDtoResponse(Status::CODE_200, m_groupService->sendGroupRequest(authObject->userUuid, groupUuid, request));
    //}

    //ENDPOINT_INFO(handleGroupRequest) {
    //    info->summary = "处理群聊请求";
    //    info->description = "通过或拒绝入群请求（仅限管理员/群主）";
    //    info->addResponse<Boolean>(Status::CODE_200, "application/json", "处理成功");
    //    info->addResponse<Object<ErrorStatusDto>>(Status::CODE_401, "application/json", "用户不存在或已失效");
    //    info->addResponse<Object<ErrorStatusDto>>(Status::CODE_400, "application/json", "处理群聊请求失败");
    //    info->addResponse<Object<ErrorStatusDto>>(Status::CODE_400, "application/json", "请求参数错误");
    //    info->addConsumes<Object<HandleGroupRequestDTO>>("application/json");
    //    info->addSecurityRequirement("BearerAuth");
    //}
    //ENDPOINT("PUT", "/api/groups/requests/{requestUuid}", handleGroupRequest,
    //    PATH(String, requestUuid, "requestUuid"),
    //    BODY_DTO(Object<HandleGroupRequestDTO>, request),
    //    AUTHORIZATION(std::shared_ptr<Appjwt::Payload>, authObject)) {
    //    return createDtoResponse(Status::CODE_200, m_groupService->handleGroupRequest(authObject->userUuid, requestUuid, request));
    //}

    ENDPOINT_INFO(updateGroupInfo) {
        info->summary = "更新群信息";
        info->description = "更新群名称、描述、头像等信息（仅限群主）";
        info->addResponse<Object<GroupDetailInfoVO>>(Status::CODE_200, "application/json", "更新成功");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_401, "application/json", "用户不存在或已失效");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_403, "application/json", "无权限操作（仅限群主）");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_404, "application/json", "群组不存在");
        info->addConsumes<Object<UpdateGroupRequestDTO>>("application/json");
        info->addSecurityRequirement("BearerAuth");
    }
    ENDPOINT("PUT", "/api/groups/{groupUuid}", updateGroupInfo,
        PATH(String, groupUuid, "groupUuid"),
        BODY_DTO(Object<UpdateGroupRequestDTO>, request),
        AUTHORIZATION(std::shared_ptr<Appjwt::Payload>, authObject)) {
        return createDtoResponse(Status::CODE_200, m_groupService->updateGroup(authObject->userUuid, groupUuid, request));
    }
};

#include OATPP_CODEGEN_END(ApiController)
