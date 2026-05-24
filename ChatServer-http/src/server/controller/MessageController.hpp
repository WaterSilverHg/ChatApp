#pragma once

#include "global.h"
#include "../dto/MessageDto.hpp"
#include "../vo/MessageVO.hpp"
#include "../service/MessageService.hpp"
#include "../handler/AppAuthHandler.h"

#include OATPP_CODEGEN_BEGIN(ApiController) 

class MessageController : public oatpp::web::server::api::ApiController {
private:
    std::shared_ptr<MessageService> m_messageService;

public:
    MessageController(OATPP_COMPONENT(std::shared_ptr<ObjectMapper>, objectMapper),
                     OATPP_COMPONENT(std::shared_ptr<AppPostgresql>, appPostgresql),
                     OATPP_COMPONENT(std::shared_ptr<AppRedis>, redis),
                     OATPP_COMPONENT(std::shared_ptr<UuidIdCache>, uuidIdCache))
        : oatpp::web::server::api::ApiController(objectMapper),
          m_messageService(std::make_shared<MessageService>(appPostgresql, redis, uuidIdCache)) {
        setDefaultAuthorizationHandler(std::make_shared<AppAuthHandler>());
    }

    static std::shared_ptr<MessageController> createShared(
        OATPP_COMPONENT(std::shared_ptr<ObjectMapper>, objectMapper),
        OATPP_COMPONENT(std::shared_ptr<AppPostgresql>, appPostgresql),
        OATPP_COMPONENT(std::shared_ptr<AppRedis>, redis),
        OATPP_COMPONENT(std::shared_ptr<UuidIdCache>, uuidIdCache)
    ) {
        return std::make_shared<MessageController>(objectMapper, appPostgresql, redis, uuidIdCache);
    }

    //ENDPOINT_INFO(getPrivateMessages) {
    //    info->summary = "获取与某用户的聊天记录";
    //    info->description = "获取当前用户与指定用户的聊天记录，默认返回最近50条消息";
    //    info->addResponse<oatpp::Vector<Object<PrivateMessageVO>>>(Status::CODE_200, "application/json", "获取成功");
    //    info->addResponse<Object<ErrorStatusDto>>(Status::CODE_401, "application/json", "用户不存在或已失效");
    //    info->addResponse<Object<ErrorStatusDto>>(Status::CODE_404, "application/json", "目标用户不存在");
    //    info->addResponse<Object<ErrorStatusDto>>(Status::CODE_500, "application/json", "获取消息失败");
    //    info->addResponse<Object<ErrorStatusDto>>(Status::CODE_400, "application/json", "请求参数错误");
    //    info->addSecurityRequirement("BearerAuth");
    //}
    //ENDPOINT("GET", "/api/messages/private/{userUuid}", getPrivateMessages,
    //    PATH(String, userUuid, "userUuid"),
    //    AUTHORIZATION(std::shared_ptr<Appjwt::Payload>, authObject)) {
    //    return createDtoResponse(Status::CODE_200, m_messageService->getPrivateMessages(authObject->userUuid, userUuid));
    //}

    ENDPOINT_INFO(getPrivateMessagesPage) {
        info->summary = "分页获取历史消息";
        info->description = "分页获取当前用户与指定用户的历史聊天记录";
        info->addResponse<oatpp::Vector<Object<PrivateMessageVO>>>(Status::CODE_200, "application/json", "获取成功");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_401, "application/json", "用户不存在或已失效");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_404, "application/json", "目标用户不存在");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_500, "application/json", "获取消息失败");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_400, "application/json", "请求参数错误");
        info->addSecurityRequirement("BearerAuth");
    }
    ENDPOINT("GET", "/api/messages/private/{userUuid}/page", getPrivateMessagesPage, 
        PATH(String, userUuid, "userUuid"),
        QUERY(Int32, page, "page"),
        QUERY(Int32, size, "size"),
        AUTHORIZATION(std::shared_ptr<Appjwt::Payload>, authObject)) {
        return createDtoResponse(Status::CODE_200, m_messageService->getPrivateMessagesPage(authObject->userUuid, userUuid, page, size));
    }

    ENDPOINT_INFO(getPrivateMessagesBefore) {
        info->summary = "获取私聊消息（指定msgUuid之前的消息）";
        info->description = "获取当前用户与指定用户的历史聊天记录，返回msgUuid之前的size条消息";
        info->addResponse<oatpp::Vector<Object<PrivateMessageVO>>>(Status::CODE_200, "application/json", "获取成功");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_401, "application/json", "用户不存在或已失效");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_404, "application/json", "目标用户不存在或消息不存在");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_500, "application/json", "获取消息失败");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_400, "application/json", "请求参数错误");
        info->addSecurityRequirement("BearerAuth");
    }
    ENDPOINT("GET", "/api/messages/private/{userUuid}/before", getPrivateMessagesBefore, 
        PATH(String, userUuid, "userUuid"),
        QUERY(String, msgUuid, "msgUuid"),
        QUERY(Int32, size, "size"),
        AUTHORIZATION(std::shared_ptr<Appjwt::Payload>, authObject)) {
        return createDtoResponse(Status::CODE_200, m_messageService->getPrivateMessagesBefore(authObject->userUuid, userUuid, msgUuid, size));
    }

    //ENDPOINT_INFO(markMessageRead) {
    //    info->summary = "标记消息为已读";
    //    info->description = "将指定消息标记为已读状态";
    //    info->addResponse<Boolean>(Status::CODE_200, "application/json", "标记成功");
    //    info->addResponse<Object<ErrorStatusDto>>(Status::CODE_401, "application/json", "用户不存在或已失效");
    //    info->addResponse<Object<ErrorStatusDto>>(Status::CODE_404, "application/json", "消息不存在");
    //    info->addResponse<Object<ErrorStatusDto>>(Status::CODE_400, "application/json", "标记已读失败");
    //    info->addResponse<Object<ErrorStatusDto>>(Status::CODE_400, "application/json", "请求参数错误");
    //    info->addSecurityRequirement("BearerAuth");
    //}
    //ENDPOINT("PUT", "/api/messages/{messageUuid}/read", markMessageRead, 
    //    PATH(String, messageUuid, "messageUuid"),
    //    AUTHORIZATION(std::shared_ptr<Appjwt::Payload>, authObject)) {
    //    return createDtoResponse(Status::CODE_200, m_messageService->markMessageRead(authObject->userUuid, messageUuid));
    //}

    //ENDPOINT_INFO(getGroupMessages) {
    //    info->summary = "获取群聊消息";
    //    info->description = "获取指定群组的聊天记录";
    //    info->addResponse<oatpp::Vector<Object<GroupMessageVO>>>(Status::CODE_200, "application/json", "获取成功");
    //    info->addResponse<Object<ErrorStatusDto>>(Status::CODE_401, "application/json", "用户不存在或已失效");
    //    info->addResponse<Object<ErrorStatusDto>>(Status::CODE_400, "application/json", "群组ID不能为空");
    //    info->addResponse<Object<ErrorStatusDto>>(Status::CODE_404, "application/json", "群组不存在或已失效");
    //    info->addResponse<Object<ErrorStatusDto>>(Status::CODE_500, "application/json", "获取消息失败");
    //    info->addSecurityRequirement("BearerAuth");
    //}
    //ENDPOINT("GET", "/api/messages/group/{groupUuid}", getGroupMessages, 
    //    PATH(String, groupUuid, "groupUuid"),
    //    AUTHORIZATION(std::shared_ptr<Appjwt::Payload>, authObject)) {
    //    return createDtoResponse(Status::CODE_200, m_messageService->getGroupMessages(authObject->userUuid, groupUuid));
    //}

    ENDPOINT_INFO(getGroupMessagesPage) {
        info->summary = "分页获取群聊消息";
        info->description = "分页获取指定群组的历史聊天记录";
        info->addResponse<oatpp::Vector<Object<GroupMessageVO>>>(Status::CODE_200, "application/json", "获取成功");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_401, "application/json", "用户不存在或已失效");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_400, "application/json", "群组ID不能为空");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_404, "application/json", "群组不存在或已失效");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_500, "application/json", "获取消息失败");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_400, "application/json", "请求参数错误");
        info->addSecurityRequirement("BearerAuth");
    }
    ENDPOINT("GET", "/api/messages/group/{groupUuid}/page", getGroupMessagesPage, 
        PATH(String, groupUuid, "groupUuid"),
        QUERY(Int32, page, "page"),
        QUERY(Int32, size, "size"),
        AUTHORIZATION(std::shared_ptr<Appjwt::Payload>, authObject)) {
        return createDtoResponse(Status::CODE_200, m_messageService->getGroupMessagesPage(authObject->userUuid, groupUuid, page, size));
    }

    ENDPOINT_INFO(getGroupMessagesBefore) {
        info->summary = "获取群聊消息（指定msgUuid之前的消息）";
        info->description = "获取指定群组的历史聊天记录，返回msgUuid之前的size条消息";
        info->addResponse<oatpp::Vector<Object<GroupMessageVO>>>(Status::CODE_200, "application/json", "获取成功");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_401, "application/json", "用户不存在或已失效");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_400, "application/json", "群组ID不能为空");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_404, "application/json", "群组不存在或消息不存在");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_500, "application/json", "获取消息失败");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_400, "application/json", "请求参数错误");
        info->addSecurityRequirement("BearerAuth");
    }
    ENDPOINT("GET", "/api/messages/group/{groupUuid}/before", getGroupMessagesBefore, 
        PATH(String, groupUuid, "groupUuid"),
        QUERY(String, msgUuid, "msgUuid"),
        QUERY(Int32, size, "size"),
        AUTHORIZATION(std::shared_ptr<Appjwt::Payload>, authObject)) {
        return createDtoResponse(Status::CODE_200, m_messageService->getGroupMessagesBefore(authObject->userUuid, groupUuid, msgUuid, size));
    }
};

#include OATPP_CODEGEN_END(ApiController)
