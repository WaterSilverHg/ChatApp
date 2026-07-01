#pragma once

#include "global.h"
#include "server/dto/MessageDto.hpp"
#include "server/vo/MessageVo.hpp"
#include "server/postgresql/AppPostgresql.hpp"
#include "jwt/Appjwt.h"
#include "tool/UuidIdCache.hpp"

class MessageService {
private:
    std::shared_ptr<AppPostgresql> m_appClient;
    std::shared_ptr<UuidIdCache> m_idCache;
    using Status = oatpp::web::protocol::http::Status;
public:
    MessageService(const std::shared_ptr<AppPostgresql>& appClient, const std::shared_ptr<UuidIdCache>& idCache) 
        : m_appClient(appClient), m_idCache(idCache) {}

    //oatpp::Vector<oatpp::Object<PrivateMessageVO>> getPrivateMessages(const oatpp::String& currentUserIdHeader, const oatpp::String& targetUserUuid) {
    //    ASYNC_THROW_IF(currentUserIdHeader && !currentUserIdHeader->empty(), "User ID cannot be empty");
    //    ASYNC_THROW_IF(targetUserUuid && !targetUserUuid->empty(), "Target user ID cannot be empty");
    //    auto currentUserId = m_idCache->getUserId(currentUserIdHeader);
    //    ASYNC_THROW_IF(currentUserId > 0, "User does not exist or has been deactivated");

    //    auto targetUserId = m_idCache->getUserId(targetUserUuid);
    //    ASYNC_THROW_IF(targetUserId > 0, "Target user does not exist");

    //    auto result = m_appClient->getPrivateMessages(currentUserId, targetUserId, 50, 0);
    //    #ifdef SQLCHECK
    //    if (!result->isSuccess()) {
    //        OATPP_LOGD("MessageService", "Error: %s", result->getErrorMessage());
    //        throw std::runtime_error("Failed to get messages");
    //    }
    //    #else
    //    ASYNC_THROW_IF(result->isSuccess(), "Failed to get messages");
    //    #endif
    //    auto messages = result->fetch<oatpp::Vector<oatpp::Object<PrivateMessageVO>>>();
    //    processDeletedMessages(messages);
    //    return messages;
    //}

    //oatpp::Vector<oatpp::Object<PrivateMessageVO>> getPrivateMessagesPage(const oatpp::String& currentUserIdHeader, const oatpp::String& targetUserUuid, oatpp::Int32 page, oatpp::Int32 size) {
    //    ASYNC_THROW_IF(currentUserIdHeader && !currentUserIdHeader->empty(), "User ID cannot be empty");
    //    ASYNC_THROW_IF(targetUserUuid && !targetUserUuid->empty(), "Target user ID cannot be empty");
    //    ASYNC_THROW_IF(page > 0, "Page number must be greater than 0");
    //    ASYNC_THROW_IF(size > 0 && size <= 100, "Page size must be between 1 and 100");
    //    auto currentUserId = m_idCache->getUserId(currentUserIdHeader);
    //    ASYNC_THROW_IF(currentUserId > 0, "User does not exist or has been deactivated");

    //    auto targetUserId = m_idCache->getUserId(targetUserUuid);
    //    ASYNC_THROW_IF(targetUserId > 0, "Target user does not exist");

    //    int offset = (page - 1) * size;
    //    auto result = m_appClient->getPrivateMessages(currentUserId, targetUserId, size, offset);
    //    #ifdef SQLCHECK
    //    if (!result->isSuccess()) {
    //        OATPP_LOGD("MessageService", "Error: %s", result->getErrorMessage());
    //        throw std::runtime_error("Failed to get messages");
    //    }
    //    #else
    //    ASYNC_THROW_IF(result->isSuccess(), "Failed to get messages");
    //    #endif
    //    auto messages = result->fetch<oatpp::Vector<oatpp::Object<PrivateMessageVO>>>();
    //    processDeletedMessages(messages);
    //    return messages;
    //}

    oatpp::Object<PrivateMessageVO> sendPrivateMessage(const oatpp::String& currentUserUuidHeader, const oatpp::Object<SendPrivateMessageRequestDTO>& request) {
        ASYNC_THROW_IF(currentUserUuidHeader && !currentUserUuidHeader->empty(), "User ID cannot be empty");
        ASYNC_THROW_IF(request, "Request parameters cannot be empty");
        ASYNC_THROW_IF(request->toUserUuid && !request->toUserUuid->empty(), "Target user ID cannot be empty");
        ASYNC_THROW_IF(request->messageType && !request->messageType->empty(), "Message type cannot be empty");
        ASYNC_THROW_IF(request->content && !request->content->empty(), "Message content cannot be empty");


        auto currentUserId = m_idCache->getUserId(currentUserUuidHeader);
        ASYNC_THROW_IF(currentUserId > 0, "User does not exist or has been deactivate");

        //auto targetUserResult = m_appClient->getGroupIdByUuid(request->toUserUuid);
        //#ifdef SQLCHECK
        //OATPP_ASSERT_HTTP(targetUserResult->isSuccess(), Status::CODE_500, targetUserResult->getErrorMessage());
        //OATPP_ASSERT_HTTP(targetUserResult->hasMoreToFetch(), Status::CODE_404, "群组不存在或已失效");
        //OATPP_ASSERT_HTTP(targetUserResult->isSuccess() && targetUserResult->hasMoreToFetch(), Status::CODE_404, "群组不存在或已失效");
        //#endif
        //auto userId = targetUserResult->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;


        auto targetUserId = m_idCache->getUserId(request->toUserUuid);
        ASYNC_THROW_IF(targetUserId > 0, "Target user does not exist or has been deactivated");

        // 判断关系 - 只检查发送方→接收方的关系
        // block = 我拉黑了对方
        // blocked = 对方拉黑了我
        // accepted = 正常好友
        auto statusResult = m_appClient->checkFriendshipStatus(currentUserId, targetUserId);
        #ifdef SQLCHECK
        if (!statusResult->isSuccess()) {
            OATPP_LOGD("MessageService", "Error: %s", statusResult->getErrorMessage()->c_str());
            throw std::runtime_error("Friendship does not exist");
        }
        #endif
        if (!statusResult->hasMoreToFetch()) {
            throw oatpp::web::protocol::http::HttpError(Status::CODE_401, "Friendship does not exist");
        }

        auto status = statusResult->fetch<oatpp::Vector<oatpp::Object<StatusDTO>>>()[0]->status;
        if (status == "block") {
            throw oatpp::web::protocol::http::HttpError(Status::CODE_401, "You have blocked this user, cannot send message");
        }
        if (status == "blocked") {
            throw oatpp::web::protocol::http::HttpError(Status::CODE_401, "This user has blocked you, cannot send message");
        }
        if (status != "accepted") {
            throw oatpp::web::protocol::http::HttpError(Status::CODE_401, "You are not friends with this user, cannot send message");
        }

        // 通过检查，发送消息

        auto result = m_appClient->sendPrivateMessage(currentUserId, targetUserId, request->messageType, request->content, request->fileUrl, request->fileSize, request->fileName, request->mimeType);
        #ifdef SQLCHECK
        if (!result->isSuccess()) {
            OATPP_LOGD("MessageService", "Error: %s", result->getErrorMessage()->c_str());
            throw std::runtime_error("Failed to send message");
        }
        if (!result->hasMoreToFetch()) {
            OATPP_LOGD("MessageService", "Error: %s", "Failed to send message");
            throw std::runtime_error("Failed to send message");
        }
        #else
        ASYNC_THROW_IF(result->isSuccess() && result->hasMoreToFetch(), "Failed to send message");
        #endif

        auto message = result->fetch<oatpp::Vector<oatpp::Object<PrivateMessageVO>>>()[0];
        message->fromUserUuid = currentUserUuidHeader;
        message->toUserUuid = request->toUserUuid;
        message->content = request->content;
        message->messageType = request->messageType;
        return message;
    }

    // 已删除 message_reads 表，标记已读现由 conversations.unread_count 管理
    // 发送时已自动处理：在线送达 → unread=0，离线 → unread+1
    // 用户主动打开会话时，调用方应直接使用 upsertConversationDelivered* 系列查询

    oatpp::Object<MessageInfoVO> getMessageInfo(const oatpp::String& messageUuid) {
        ASYNC_THROW_IF(messageUuid && !messageUuid->empty(), "Message ID cannot be empty");
        auto messageId = m_idCache->getMessageId(messageUuid);
        ASYNC_THROW_IF(messageId > 0, "Message does not exist");
        auto result = m_appClient->getMessageInfo(messageId);
        ASYNC_THROW_IF(result->isSuccess(), "Failed to get message info");
        return result->fetch<oatpp::Vector<oatpp::Object<MessageInfoVO>>>()[0];
    }

    oatpp::Boolean recallMessage(const oatpp::String& currentUserIdHeader, const oatpp::String& messageUuid) {
        ASYNC_THROW_IF(currentUserIdHeader && !currentUserIdHeader->empty(), "User ID cannot be empty");
        ASYNC_THROW_IF(messageUuid && !messageUuid->empty(), "Message ID cannot be empty");
        
        auto currentUserId = m_idCache->getUserId(currentUserIdHeader);
        ASYNC_THROW_IF(currentUserId > 0, "User does not exist or has been deactivated");

        auto messageId = m_idCache->getMessageId(messageUuid);
        ASYNC_THROW_IF(messageId > 0, "Message does not exist");

        auto result = m_appClient->recallMessage(messageId, currentUserId);
        #ifdef SQLCHECK
        if (!result->isSuccess()) {
            OATPP_LOGD("MessageService", "Error: %s", result->getErrorMessage()->c_str());
            throw std::runtime_error("Failed to recall message");
        }
        #else
        ASYNC_THROW_IF(result->isSuccess(), "Failed to recall message");
        #endif
        return true;
    }

    //oatpp::Vector<oatpp::Object<GroupMessageVO>> getGroupMessages(const oatpp::String& currentUserIdHeader, const oatpp::String& groupUuid) {
    //    ASYNC_THROW_IF(currentUserIdHeader && !currentUserIdHeader->empty(), "User ID cannot be empty");
    //    ASYNC_THROW_IF(groupUuid && !groupUuid->empty(), "Group ID cannot be empty");
    //    
    //    auto currentUserId = m_idCache->getUserId(currentUserIdHeader);
    //    ASYNC_THROW_IF(currentUserId > 0, "User does not exist or has been deactivated");

    //    auto groupId = m_idCache->getGroupId(groupUuid);
    //    ASYNC_THROW_IF(groupId > 0, "Group does not exist or has been deactivated");

    //    auto result = m_appClient->getGroupMessages(currentUserId, groupId, 50, 0);
    //    #ifdef SQLCHECK
    //    if (!result->isSuccess()) {
    //        OATPP_LOGD("MessageService", "Error: %s", result->getErrorMessage());
    //        throw std::runtime_error("Failed to get group messages");
    //    }
    //    #else
    //    ASYNC_THROW_IF(result->isSuccess(), "Failed to get group messages");
    //    #endif
    //    auto messages = result->fetch<oatpp::Vector<oatpp::Object<GroupMessageVO>>>();
    //    processDeletedMessages(messages);
    //    return messages;
    //}

    //oatpp::Vector<oatpp::Object<GroupMessageVO>> getGroupMessagesPage(const oatpp::String& currentUserIdHeader, const oatpp::String& groupUuid, oatpp::Int32 page, oatpp::Int32 size) {
    //    ASYNC_THROW_IF(currentUserIdHeader && !currentUserIdHeader->empty(), "User ID cannot be empty");
    //    ASYNC_THROW_IF(groupUuid && !groupUuid->empty(), "Group ID cannot be empty");
    //    ASYNC_THROW_IF(page > 0, "Page number must be greater than 0");
    //    ASYNC_THROW_IF(size > 0 && size <= 100, "Page size must be between 1 and 100");
    //    
    //    auto currentUserId = m_idCache->getUserId(currentUserIdHeader);
    //    ASYNC_THROW_IF(currentUserId > 0, "User does not exist or has been deactivated");

    //    auto groupId = m_idCache->getGroupId(groupUuid);
    //    ASYNC_THROW_IF(groupId > 0, "Group does not exist or has been deactivated");

    //    int offset = (page - 1) * size;
    //    auto result = m_appClient->getGroupMessages(currentUserId, groupId, size, offset);
    //    #ifdef SQLCHECK
    //    if (!result->isSuccess()) {
    //        OATPP_LOGD("MessageService", "Error: %s", result->getErrorMessage());
    //        throw std::runtime_error("Failed to get group messages");
    //    }
    //    #else
    //    ASYNC_THROW_IF(result->isSuccess(), "Failed to get group messages");
    //    #endif
    //    auto messages = result->fetch<oatpp::Vector<oatpp::Object<GroupMessageVO>>>();
    //    processDeletedMessages(messages);
    //    return messages;
    //}

    oatpp::Object<GroupMessageVO> sendGroupMessage(const oatpp::String& currentUserIdHeader, const oatpp::Object<SendGroupMessageRequestDTO>& request) {
        ASYNC_THROW_IF(currentUserIdHeader && !currentUserIdHeader->empty(), "User ID cannot be empty");
        ASYNC_THROW_IF(request, "Request parameters cannot be empty");
        ASYNC_THROW_IF(request->groupUuid && !request->groupUuid->empty(), "Group ID cannot be empty");
        ASYNC_THROW_IF(request->messageType && !request->messageType->empty(), "Message type cannot be empty");
        ASYNC_THROW_IF(request->content && !request->content->empty(), "Message content cannot be empty");
        
        auto currentUserId = m_idCache->getUserId(currentUserIdHeader);
        ASYNC_THROW_IF(currentUserId > 0, "User does not exist or has been deactivated");

        auto groupId = m_idCache->getGroupId(request->groupUuid);
        ASYNC_THROW_IF(groupId > 0, "Group does not exist or has been deactivated");

        // 检查用户是否是群成员
        auto roleResult = m_appClient->getUserRoleInGroup(groupId, currentUserId);
        #ifdef SQLCHECK
        if (!roleResult->isSuccess()) {
            OATPP_LOGD("MessageService", "Error: %s", roleResult->getErrorMessage()->c_str());
            throw std::runtime_error("You are not a member of this group, cannot send message");
        }
        if (!roleResult->hasMoreToFetch()) {
            OATPP_LOGD("MessageService", "Error: User is not a member of this group");
            throw std::runtime_error("You are not a member of this group, cannot send message");
        }
        #else
        if (!roleResult->isSuccess() || !roleResult->hasMoreToFetch()) {
            throw std::runtime_error("You are not a member of this group, cannot send message");
        }
        #endif

        auto result = m_appClient->sendGroupMessage(currentUserId, groupId, request->messageType, request->content, request->fileUrl, request->fileSize, request->fileName, request->mimeType);
        #ifdef SQLCHECK
        if (!result->isSuccess()) {
            OATPP_LOGD("MessageService", "Error: %s", result->getErrorMessage()->c_str());
            throw std::runtime_error("Failed to send group message");
        }
        if (!result->hasMoreToFetch()) {
            OATPP_LOGD("MessageService", "Error: %s", "Failed to send group message");
            throw std::runtime_error("Failed to send group message");
        }
        #else
        ASYNC_THROW_IF(result->isSuccess() && result->hasMoreToFetch(), "Failed to send group message");
        #endif

        auto message = result->fetch<oatpp::Vector<oatpp::Object<GroupMessageVO>>>()[0];
        message->fromUserUuid = currentUserIdHeader;
        message->groupUuid = request->groupUuid;
        message->content = request->content;
        message->messageType = request->messageType;
        return message;
    }

    //oatpp::Boolean markGroupMessageRead(const oatpp::String& currentUserIdHeader, const oatpp::String& messageUuid) {
    //    OATPP_ASSERT_HTTP(currentUserIdHeader && !currentUserIdHeader->empty(), Status::CODE_400, "用户ID不能为空");
    //    OATPP_ASSERT_HTTP(messageUuid && !messageUuid->empty(), Status::CODE_400, "消息ID不能为空");
    //    auto userCheck = m_appClient->getUserIdByUuid(currentUserIdHeader);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(userCheck->isSuccess(), Status::CODE_500, userCheck->getErrorMessage());
    //    OATPP_ASSERT_HTTP(userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
    //    #else
    //    OATPP_ASSERT_HTTP(userCheck->isSuccess() && userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
    //    #endif
    //    auto currentUserId = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

    //    auto uuidResult = m_appClient->getMessageIdByUuid(messageUuid);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(uuidResult->isSuccess(), Status::CODE_500, uuidResult->getErrorMessage());
    //    OATPP_ASSERT_HTTP(uuidResult->hasMoreToFetch(), Status::CODE_404, "消息不存在");
    //    #else
    //    OATPP_ASSERT_HTTP(uuidResult->isSuccess() && uuidResult->hasMoreToFetch(), Status::CODE_404, "消息不存在");
    //    #endif
    //    auto messageId = uuidResult->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

    //    auto result = m_appClient->markGroupMessageRead(messageId, currentUserId);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_400, result->getErrorMessage());
    //    #else
    //    OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_400, "标记已读失败");
    //    #endif
    //    return true;
    //}

private:
    // 处理已删除的私聊消息
    //void processDeletedMessages(oatpp::Vector<oatpp::Object<PrivateMessageVO>>& messages) {
    //    for (auto& msg : messages) {
    //        if (msg->deletedAt && !msg->deletedAt->empty()) {
    //            msg->messageType = "recalled";
    //            msg->content = "";
    //            msg->fileUrl = "";
    //            msg->fileName = "";
    //            msg->fileSize = 0;
    //            msg->mimeType = "";
    //        }
    //    }
    //}

    //// 处理已删除的群消息
    //void processDeletedMessages(oatpp::Vector<oatpp::Object<GroupMessageVO>>& messages) {
    //    for (auto& msg : messages) {
    //        if (msg->deletedAt && !msg->deletedAt->empty()) {
    //            msg->messageType = "recalled";
    //            msg->content = "";
    //            msg->fileUrl = "";
    //            msg->fileName = "";
    //            msg->fileSize = 0;
    //            msg->mimeType = "";
    //        }
    //    }
    //}
};
