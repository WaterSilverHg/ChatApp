#pragma once

#include "global.h"
#include "../dto/ConversationDto.hpp"
#include "../vo/ConversationVO.hpp"
#include "../service/ConversationService.hpp"
#include "../handler/AppAuthHandler.h"

#include OATPP_CODEGEN_BEGIN(ApiController) 

class ConversationController : public oatpp::web::server::api::ApiController {
private:
    std::shared_ptr<ConversationService> m_conversationService;

public:
    ConversationController(OATPP_COMPONENT(std::shared_ptr<ObjectMapper>, objectMapper),
                           OATPP_COMPONENT(std::shared_ptr<AppClient>, appClient))
        : oatpp::web::server::api::ApiController(objectMapper),
          m_conversationService(std::make_shared<ConversationService>(appClient)) {
        setDefaultAuthorizationHandler(std::make_shared<AppAuthHandler>());
    }

    static std::shared_ptr<ConversationController> createShared(
        OATPP_COMPONENT(std::shared_ptr<ObjectMapper>, objectMapper),
        OATPP_COMPONENT(std::shared_ptr<AppClient>, appClient)
    ) {
        return std::make_shared<ConversationController>(objectMapper, appClient);
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

    ENDPOINT_INFO(markConversationRead) {
        info->summary = "标记会话已读";
        info->description = "将指定会话标记为已读状态";
        info->addResponse<Boolean>(Status::CODE_200, "application/json", "标记成功");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_401, "application/json", "用户不存在或已失效");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_400, "application/json", "标记已读失败");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_400, "application/json", "请求参数错误");
        info->addSecurityRequirement("BearerAuth");
    }
    ENDPOINT("PUT", "/api/conversations/{conversationUuid}/read", markConversationRead, 
        PATH(String, conversationUuid, "conversationUuid"),
        AUTHORIZATION(std::shared_ptr<Appjwt::Payload>, authObject)) {
        return createDtoResponse(Status::CODE_200, m_conversationService->markConversationRead(authObject->userUuid, conversationUuid));
    }
};

#include OATPP_CODEGEN_END(ApiController)
