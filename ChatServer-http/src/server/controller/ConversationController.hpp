#pragma once

#include "global.h"
#include "server/dto/ConversationDto.hpp"
#include "server/vo/ConversationVO.hpp"
#include "server/service/ConversationService.hpp"
#include "server/handler/AppAuthHandler.h"

#include OATPP_CODEGEN_BEGIN(ApiController) 

class ConversationController : public oatpp::web::server::api::ApiController {
private:
    std::shared_ptr<ConversationService> m_conversationService;

public:
    ConversationController(OATPP_COMPONENT(std::shared_ptr<ObjectMapper>, objectMapper),
                           OATPP_COMPONENT(std::shared_ptr<AppPostgresql>, appPostgresql),
                           OATPP_COMPONENT(std::shared_ptr<AppRedis>, redis),
                           OATPP_COMPONENT(std::shared_ptr<UuidIdCache>, uuidIdCache))
        : oatpp::web::server::api::ApiController(objectMapper),
          m_conversationService(std::make_shared<ConversationService>(appPostgresql, redis, uuidIdCache)) {
        setDefaultAuthorizationHandler(std::make_shared<AppAuthHandler>());
    }

    static std::shared_ptr<ConversationController> createShared(
        OATPP_COMPONENT(std::shared_ptr<ObjectMapper>, objectMapper),
        OATPP_COMPONENT(std::shared_ptr<AppPostgresql>, appPostgresql),
        OATPP_COMPONENT(std::shared_ptr<AppRedis>, redis),
        OATPP_COMPONENT(std::shared_ptr<UuidIdCache>, uuidIdCache)
    ) {
        return std::make_shared<ConversationController>(objectMapper, appPostgresql, redis, uuidIdCache);
    }

    ENDPOINT_INFO(getConversations) {
        info->summary = "获取会话列表";
        info->description = "获取当前用户的所有会话列表";
        info->addResponse<oatpp::Vector<Object<ConversationVO>>>(Status::CODE_200, "application/json", "获取成功");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_401, "application/json", "用户不存在或已失效");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_500, "application/json", "获取会话列表失败");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_400, "application/json", "请求参数错误");
        info->addSecurityRequirement("BearerAuth");
    }
    ENDPOINT("GET", "/api/conversations", getConversations,
        AUTHORIZATION(std::shared_ptr<Appjwt::Payload>, authObject)) {
        return createDtoResponse(Status::CODE_200, m_conversationService->getConversations(authObject->userUuid));
    }

    ENDPOINT_INFO(markPrivateConversationRead) {
        info->summary = "标记私聊会话已读";
        info->description = "将指定私聊会话标记为已读状态";
        info->addResponse<Boolean>(Status::CODE_200, "application/json", "标记成功");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_401, "application/json", "用户不存在或已失效");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_400, "application/json", "标记已读失败");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_404, "application/json", "目标用户不存在");
        info->addSecurityRequirement("BearerAuth");
    }
    ENDPOINT("PUT", "/api/conversations/users/{targetUserUuid}/read", markPrivateConversationRead,
        PATH(String, targetUserUuid, "targetUserUuid"),
        AUTHORIZATION(std::shared_ptr<Appjwt::Payload>, authObject)) {
        return createDtoResponse(Status::CODE_200, m_conversationService->markPrivateConversationRead(authObject->userUuid, targetUserUuid));
    }

    ENDPOINT_INFO(markGroupConversationRead) {
        info->summary = "标记群聊会话已读";
        info->description = "将指定群聊会话标记为已读状态";
        info->addResponse<Boolean>(Status::CODE_200, "application/json", "标记成功");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_401, "application/json", "用户不存在或已失效");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_400, "application/json", "标记已读失败");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_404, "application/json", "群组不存在");
        info->addSecurityRequirement("BearerAuth");
    }
    ENDPOINT("PUT", "/api/conversations/groups/{groupUuid}/read", markGroupConversationRead,
        PATH(String, groupUuid, "groupUuid"),
        AUTHORIZATION(std::shared_ptr<Appjwt::Payload>, authObject)) {
        return createDtoResponse(Status::CODE_200, m_conversationService->markGroupConversationRead(authObject->userUuid, groupUuid));
    }

    ENDPOINT_INFO(mutePrivateConversation) {
        info->summary = "设置私聊消息免打扰";
        info->description = "将指定私聊会话设置为免打扰模式";
        info->addResponse<Boolean>(Status::CODE_200, "application/json", "设置成功");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_401, "application/json", "用户不存在或已失效");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_400, "application/json", "请求参数错误");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_404, "application/json", "目标用户不存在");
        info->addSecurityRequirement("BearerAuth");
    }
    ENDPOINT("PUT", "/api/conversations/users/{targetUserUuid}/mute", mutePrivateConversation,
        PATH(String, targetUserUuid, "targetUserUuid"),
        AUTHORIZATION(std::shared_ptr<Appjwt::Payload>, authObject)) {
        return createDtoResponse(Status::CODE_200, m_conversationService->mutePrivateConversation(authObject->userUuid, targetUserUuid));
    }

    ENDPOINT_INFO(muteGroupConversation) {
        info->summary = "设置群聊消息免打扰";
        info->description = "将指定群聊会话设置为免打扰模式";
        info->addResponse<Boolean>(Status::CODE_200, "application/json", "设置成功");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_401, "application/json", "用户不存在或已失效");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_400, "application/json", "请求参数错误");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_404, "application/json", "群组不存在");
        info->addSecurityRequirement("BearerAuth");
    }
    ENDPOINT("PUT", "/api/conversations/groups/{groupUuid}/mute", muteGroupConversation,
        PATH(String, groupUuid, "groupUuid"),
        AUTHORIZATION(std::shared_ptr<Appjwt::Payload>, authObject)) {
        return createDtoResponse(Status::CODE_200, m_conversationService->muteGroupConversation(authObject->userUuid, groupUuid));
    }

    ENDPOINT_INFO(unmutePrivateConversation) {
        info->summary = "取消私聊消息免打扰";
        info->description = "取消指定私聊会话的免打扰模式";
        info->addResponse<Boolean>(Status::CODE_200, "application/json", "取消成功");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_401, "application/json", "用户不存在或已失效");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_400, "application/json", "请求参数错误");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_404, "application/json", "目标用户不存在");
        info->addSecurityRequirement("BearerAuth");
    }
    ENDPOINT("PUT", "/api/conversations/users/{targetUserUuid}/unmute", unmutePrivateConversation,
        PATH(String, targetUserUuid, "targetUserUuid"),
        AUTHORIZATION(std::shared_ptr<Appjwt::Payload>, authObject)) {
        return createDtoResponse(Status::CODE_200, m_conversationService->unmutePrivateConversation(authObject->userUuid, targetUserUuid));
    }

    ENDPOINT_INFO(unmuteGroupConversation) {
        info->summary = "取消群聊消息免打扰";
        info->description = "取消指定群聊会话的免打扰模式";
        info->addResponse<Boolean>(Status::CODE_200, "application/json", "取消成功");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_401, "application/json", "用户不存在或已失效");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_400, "application/json", "请求参数错误");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_404, "application/json", "群组不存在");
        info->addSecurityRequirement("BearerAuth");
    }
    ENDPOINT("PUT", "/api/conversations/groups/{groupUuid}/unmute", unmuteGroupConversation,
        PATH(String, groupUuid, "groupUuid"),
        AUTHORIZATION(std::shared_ptr<Appjwt::Payload>, authObject)) {
        return createDtoResponse(Status::CODE_200, m_conversationService->unmuteGroupConversation(authObject->userUuid, groupUuid));
    }

};

#include OATPP_CODEGEN_END(ApiController)
