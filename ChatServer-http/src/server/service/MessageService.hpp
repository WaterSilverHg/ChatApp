#pragma once

#include "global.h"
#include "../dto/MessageDto.hpp"
#include "../vo/MessageVo.hpp"
#include "../postgresql/AppPostgresql.hpp"
#include "../../jwt/Appjwt.h"
#include "../../redis/AppRedis.hpp"
#include "../../tool/HashUtils.hpp"
#include "../../tool/UuidIdCache.hpp"

class MessageService {
private:
    std::shared_ptr<AppPostgresql> m_appPostgresql;
    std::shared_ptr<AppRedis> m_redis;
    std::shared_ptr<UuidIdCache> m_idCache;
    using Status = oatpp::web::protocol::http::Status;

public:
    MessageService(const std::shared_ptr<AppPostgresql>& appClient, 
                   const std::shared_ptr<AppRedis>& redis,
                   const std::shared_ptr<UuidIdCache>& idCache)
        : m_appPostgresql(appClient), m_redis(redis), m_idCache(idCache) {}

    oatpp::Vector<oatpp::Object<PrivateMessageVO>> getPrivateMessages(const oatpp::String& currentUserIdHeader, const oatpp::String& targetUserUuid) {
        OATPP_ASSERT_HTTP(currentUserIdHeader && !currentUserIdHeader->empty(), Status::CODE_400, "用户ID不能为空");
        OATPP_ASSERT_HTTP(targetUserUuid && !targetUserUuid->empty(), Status::CODE_400, "目标用户ID不能为空");
        auto currentUserId = m_idCache->getUserId(currentUserIdHeader);
        OATPP_ASSERT_HTTP(currentUserId > 0, Status::CODE_401, "用户不存在或已失效");

        auto targetUserId = m_idCache->getUserId(targetUserUuid);
        OATPP_ASSERT_HTTP(targetUserId > 0, Status::CODE_404, "目标用户不存在");

        auto result = m_appPostgresql->getPrivateMessages(currentUserId, targetUserId, 50, 0);
        #ifdef SQLCHECK
        if(!result->isSuccess()) {
            OATPP_LOGE("SQL_ERROR", "%s", result->getErrorMessage()->c_str());
        }
        #endif
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, "获取消息失败");
        return result->fetch<oatpp::Vector<oatpp::Object<PrivateMessageVO>>>();
    }

    oatpp::Vector<oatpp::Object<PrivateMessageVO>> getPrivateMessagesPage(const oatpp::String& currentUserIdHeader, const oatpp::String& targetUserUuid, oatpp::Int32 page, oatpp::Int32 size) {
        OATPP_ASSERT_HTTP(currentUserIdHeader && !currentUserIdHeader->empty(), Status::CODE_400, "用户ID不能为空");
        OATPP_ASSERT_HTTP(targetUserUuid && !targetUserUuid->empty(), Status::CODE_400, "目标用户ID不能为空");
        OATPP_ASSERT_HTTP(page > 0, Status::CODE_400, "页码必须大于0");
        OATPP_ASSERT_HTTP(size > 0 && size <= 100, Status::CODE_400, "每页大小必须在1-100之间");
        auto currentUserId = m_idCache->getUserId(currentUserIdHeader);
        OATPP_ASSERT_HTTP(currentUserId > 0, Status::CODE_401, "用户不存在或已失效");

        auto targetUserId = m_idCache->getUserId(targetUserUuid);
        OATPP_ASSERT_HTTP(targetUserId > 0, Status::CODE_404, "目标用户不存在");

        int offset = (page - 1) * size;
        auto result = m_appPostgresql->getPrivateMessages(currentUserId, targetUserId, size, offset);
        #ifdef SQLCHECK
        if(!result->isSuccess()) {
            OATPP_LOGE("SQL_ERROR", "%s", result->getErrorMessage()->c_str());
        }
        #endif
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, "获取消息失败");
        return result->fetch<oatpp::Vector<oatpp::Object<PrivateMessageVO>>>();
    }

    oatpp::Vector<oatpp::Object<PrivateMessageVO>> getPrivateMessagesBefore(const oatpp::String& currentUserIdHeader, const oatpp::String& targetUserUuid, const oatpp::String& msgUuid, oatpp::Int32 size) {
        OATPP_ASSERT_HTTP(currentUserIdHeader && !currentUserIdHeader->empty(), Status::CODE_400, "用户ID不能为空");
        OATPP_ASSERT_HTTP(targetUserUuid && !targetUserUuid->empty(), Status::CODE_400, "目标用户ID不能为空");
        OATPP_ASSERT_HTTP(msgUuid && !msgUuid->empty(), Status::CODE_400, "消息ID不能为空");
        OATPP_ASSERT_HTTP(size > 0 && size <= 100, Status::CODE_400, "每页大小必须在1-100之间");
        
        auto currentUserId = m_idCache->getUserId(currentUserIdHeader);
        OATPP_ASSERT_HTTP(currentUserId > 0, Status::CODE_401, "用户不存在或已失效");

        auto targetUserId = m_idCache->getUserId(targetUserUuid);
        OATPP_ASSERT_HTTP(targetUserId > 0, Status::CODE_404, "目标用户不存在");

        auto msgId = m_idCache->getMessageId(msgUuid);
        OATPP_ASSERT_HTTP(msgId > 0, Status::CODE_404, "消息不存在");

        auto result = m_appPostgresql->getPrivateMessagesBefore(currentUserId, targetUserId, msgId, size);
        #ifdef SQLCHECK
        if(!result->isSuccess()) {
            OATPP_LOGE("SQL_ERROR", "%s", result->getErrorMessage()->c_str());
        }
        #endif
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, "获取消息失败");
        return result->fetch<oatpp::Vector<oatpp::Object<PrivateMessageVO>>>();
    }

    //oatpp::Object<PrivateMessageVO> sendPrivateMessage(const oatpp::String& currentUserIdHeader, const oatpp::Object<SendPrivateMessageRequestDTO>& request) {
    //    OATPP_ASSERT_HTTP(currentUserIdHeader && !currentUserIdHeader->empty(), Status::CODE_400, "用户ID不能为空");
    //    OATPP_ASSERT_HTTP(request, Status::CODE_400, "请求参数不能为空");
    //    OATPP_ASSERT_HTTP(request->toUserUuid && !request->toUserUuid->empty(), Status::CODE_400, "目标用户ID不能为空");
    //    OATPP_ASSERT_HTTP(request->messageType && !request->messageType->empty(), Status::CODE_400, "消息类型不能为空");
    //    OATPP_ASSERT_HTTP(request->content && !request->content->empty(), Status::CODE_400, "消息内容不能为空");
    //    auto userCheck = m_appPostgresql->getUserIdByUuid(currentUserIdHeader);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(userCheck->isSuccess(), Status::CODE_500, userCheck->getErrorMessage());
    //    OATPP_ASSERT_HTTP(userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
    //    #else
    //    OATPP_ASSERT_HTTP(userCheck->isSuccess() && userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
    //    #endif
    //    auto currentUserId = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

    //    //auto targetUserResult = m_appPostgresql->getGroupIdByUuid(request->toUserUuid);
    //    //#ifdef SQLCHECK
    //    //OATPP_ASSERT_HTTP(targetUserResult->isSuccess(), Status::CODE_500, targetUserResult->getErrorMessage());
    //    //OATPP_ASSERT_HTTP(targetUserResult->hasMoreToFetch(), Status::CODE_404, "群组不存在或已失效");
    //    //#else
    //    //OATPP_ASSERT_HTTP(targetUserResult->isSuccess() && targetUserResult->hasMoreToFetch(), Status::CODE_404, "群组不存在或已失效");
    //    //#endif
    //    //auto userId = targetUserResult->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

    //    auto touserCheck = m_appPostgresql->getUserIdByUuid(request->toUserUuid);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(touserCheck->isSuccess(), Status::CODE_500, touserCheck->getErrorMessage());
    //    OATPP_ASSERT_HTTP(touserCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
    //    #else
    //    OATPP_ASSERT_HTTP(touserCheck->isSuccess() && touserCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
    //    #endif
    //    auto targetUserId = touserCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

    //    // 判断关系
    //    auto status1Result = m_appPostgresql->checkFriendshipStatus(currentUserId, targetUserId);
    //    auto status2Result = m_appPostgresql->checkFriendshipStatus(targetUserId, currentUserId);
    //    #ifdef SQLCHECK
    //    //判断一个就好了。对称的
    //    OATPP_ASSERT_HTTP(status1Result->isSuccess(), Status::CODE_500, status1Result->getErrorMessage());
    //    #endif
    //    if (!status1Result->hasMoreToFetch()) {
    //        throw oatpp::web::protocol::http::HttpError(Status::CODE_401, "好友关系不存在");
    //    }

    //    auto status1 = status1Result->fetch<oatpp::Vector<oatpp::Object<StatusDTO>>>()[0]->status;
    //    if (status1 == "blocked") {
    //        throw oatpp::web::protocol::http::HttpError(Status::CODE_401, "已拉黑，无法发送消息");
    //    }
    //    auto status2 = status2Result->fetch<oatpp::Vector<oatpp::Object<StatusDTO>>>()[0]->status;
    //    if (status2 == "blocked") {
    //        throw oatpp::web::protocol::http::HttpError(Status::CODE_401, "已被拉黑，无法发送消息");
    //    }
    //    if (status1 != "accepted") {
    //        throw oatpp::web::protocol::http::HttpError(Status::CODE_401, "你们不是好友，无法发送消息");
    //    }

    //    // 通过检查，发送消息

    //    auto result = m_appPostgresql->sendPrivateMessage(currentUserId, targetUserId, request->messageType, request->content, request->fileUrl, request->fileSize, request->fileName, request->mimeType);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_400, result->getErrorMessage());
    //    OATPP_ASSERT_HTTP(result->hasMoreToFetch(), Status::CODE_400, "发送消息失败");
    //    #else
    //    OATPP_ASSERT_HTTP(result->isSuccess() && result->hasMoreToFetch(), Status::CODE_400, "发送消息失败");
    //    #endif

    //    auto message = result->fetch<oatpp::Vector<oatpp::Object<PrivateMessageVO>>>()[0];
    //    message->fromUserUuid = currentUserIdHeader;
    //    message->toUserUuid = request->toUserUuid;
    //    message->content = request->content;
    //    message->messageType = request->messageType;
    //    return message;
    //}

    oatpp::Boolean markMessageRead(const oatpp::String& currentUserIdHeader, const oatpp::String& messageUuid) {
        OATPP_ASSERT_HTTP(currentUserIdHeader && !currentUserIdHeader->empty(), Status::CODE_400, "用户ID不能为空");
        OATPP_ASSERT_HTTP(messageUuid && !messageUuid->empty(), Status::CODE_400, "消息ID不能为空");
        auto currentUserId = m_idCache->getUserId(currentUserIdHeader);
        OATPP_ASSERT_HTTP(currentUserId > 0, Status::CODE_401, "用户不存在或已失效");

        auto messageId = m_idCache->getMessageId(messageUuid);
        OATPP_ASSERT_HTTP(messageId > 0, Status::CODE_404, "消息不存在");

        auto result = m_appPostgresql->markMessageRead(messageId, currentUserId);
        #ifdef SQLCHECK
        if(!result->isSuccess()) {
            OATPP_LOGE("SQL_ERROR", "%s", result->getErrorMessage()->c_str());
        }
        #endif
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_400, "标记已读失败");
        
        return true;
    }

    oatpp::Boolean recallMessage(const oatpp::String& currentUserIdHeader, const oatpp::String& messageUuid) {
        OATPP_ASSERT_HTTP(currentUserIdHeader && !currentUserIdHeader->empty(), Status::CODE_400, "用户ID不能为空");
        OATPP_ASSERT_HTTP(messageUuid && !messageUuid->empty(), Status::CODE_400, "消息ID不能为空");
        auto currentUserId = m_idCache->getUserId(currentUserIdHeader);
        OATPP_ASSERT_HTTP(currentUserId > 0, Status::CODE_401, "用户不存在或已失效");

        auto messageId = m_idCache->getMessageId(messageUuid);
        OATPP_ASSERT_HTTP(messageId > 0, Status::CODE_404, "消息不存在");

        auto result = m_appPostgresql->recallMessage(messageId, currentUserId);
        #ifdef SQLCHECK
        if(!result->isSuccess()) {
            OATPP_LOGE("SQL_ERROR", "%s", result->getErrorMessage()->c_str());
        }
        #endif
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_400, "撤回消息失败");
        return true;
    }

    oatpp::Vector<oatpp::Object<GroupMessageVO>> getGroupMessages(const oatpp::String& currentUserIdHeader, const oatpp::String& groupUuid) {
        OATPP_ASSERT_HTTP(currentUserIdHeader && !currentUserIdHeader->empty(), Status::CODE_400, "用户ID不能为空");
        OATPP_ASSERT_HTTP(groupUuid && !groupUuid->empty(), Status::CODE_400, "群组ID不能为空");
        auto currentUserId = m_idCache->getUserId(currentUserIdHeader);
        OATPP_ASSERT_HTTP(currentUserId > 0, Status::CODE_401, "用户不存在或已失效");

        auto groupId = m_idCache->getGroupId(groupUuid);
        OATPP_ASSERT_HTTP(groupId > 0, Status::CODE_404, "群组不存在或已失效");

        auto result = m_appPostgresql->getGroupMessages(groupId, 50, 0);
        #ifdef SQLCHECK
        if(!result->isSuccess()) {
            OATPP_LOGE("SQL_ERROR", "%s", result->getErrorMessage()->c_str());
        }
        #endif
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, "获取群消息失败");
        return result->fetch<oatpp::Vector<oatpp::Object<GroupMessageVO>>>();
    }

    oatpp::Vector<oatpp::Object<GroupMessageVO>> getGroupMessagesPage(const oatpp::String& currentUserIdHeader, const oatpp::String& groupUuid, oatpp::Int32 page, oatpp::Int32 size) {
        OATPP_ASSERT_HTTP(currentUserIdHeader && !currentUserIdHeader->empty(), Status::CODE_400, "用户ID不能为空");
        OATPP_ASSERT_HTTP(groupUuid && !groupUuid->empty(), Status::CODE_400, "群组ID不能为空");
        OATPP_ASSERT_HTTP(page > 0, Status::CODE_400, "页码必须大于0");
        OATPP_ASSERT_HTTP(size > 0 && size <= 100, Status::CODE_400, "每页大小必须在1-100之间");
        auto currentUserId = m_idCache->getUserId(currentUserIdHeader);
        OATPP_ASSERT_HTTP(currentUserId > 0, Status::CODE_401, "用户不存在或已失效");

        auto groupId = m_idCache->getGroupId(groupUuid);
        OATPP_ASSERT_HTTP(groupId > 0, Status::CODE_404, "群组不存在或已失效");

        int offset = (page - 1) * size;
        auto result = m_appPostgresql->getGroupMessages(groupId, size, offset);
        #ifdef SQLCHECK
        if(!result->isSuccess()) {
            OATPP_LOGE("SQL_ERROR", "%s", result->getErrorMessage()->c_str());
        }
        #endif
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, "获取群消息失败");
        return result->fetch<oatpp::Vector<oatpp::Object<GroupMessageVO>>>();
    }

    oatpp::Vector<oatpp::Object<GroupMessageVO>> getGroupMessagesBefore(const oatpp::String& currentUserIdHeader, const oatpp::String& groupUuid, const oatpp::String& msgUuid, oatpp::Int32 size) {
        OATPP_ASSERT_HTTP(currentUserIdHeader && !currentUserIdHeader->empty(), Status::CODE_400, "用户ID不能为空");
        OATPP_ASSERT_HTTP(groupUuid && !groupUuid->empty(), Status::CODE_400, "群组ID不能为空");
        OATPP_ASSERT_HTTP(msgUuid && !msgUuid->empty(), Status::CODE_400, "消息ID不能为空");
        OATPP_ASSERT_HTTP(size > 0 && size <= 100, Status::CODE_400, "每页大小必须在1-100之间");
        
        auto currentUserId = m_idCache->getUserId(currentUserIdHeader);
        OATPP_ASSERT_HTTP(currentUserId > 0, Status::CODE_401, "用户不存在或已失效");

        auto groupId = m_idCache->getGroupId(groupUuid);
        OATPP_ASSERT_HTTP(groupId > 0, Status::CODE_404, "群组不存在或已失效");

        auto msgId = m_idCache->getMessageId(msgUuid);
        OATPP_ASSERT_HTTP(msgId > 0, Status::CODE_404, "消息不存在");

        auto result = m_appPostgresql->getGroupMessagesBefore(groupId, msgId, size);
        #ifdef SQLCHECK
        if(!result->isSuccess()) {
            OATPP_LOGE("SQL_ERROR", "%s", result->getErrorMessage()->c_str());
        }
        #endif
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, "获取群消息失败");
        return result->fetch<oatpp::Vector<oatpp::Object<GroupMessageVO>>>();
    }

    //oatpp::Object<GroupMessageVO> sendGroupMessage(const oatpp::String& currentUserIdHeader, const oatpp::Object<SendGroupMessageRequestDTO>& request) {
    //    OATPP_ASSERT_HTTP(currentUserIdHeader && !currentUserIdHeader->empty(), Status::CODE_400, "用户ID不能为空");
    //    OATPP_ASSERT_HTTP(request, Status::CODE_400, "请求参数不能为空");
    //    OATPP_ASSERT_HTTP(request->groupUuid && !request->groupUuid->empty(), Status::CODE_400, "群组ID不能为空");
    //    OATPP_ASSERT_HTTP(request->messageType && !request->messageType->empty(), Status::CODE_400, "消息类型不能为空");
    //    OATPP_ASSERT_HTTP(request->content && !request->content->empty(), Status::CODE_400, "消息内容不能为空");
    //    auto userCheck = m_appPostgresql->getUserIdByUuid(currentUserIdHeader);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(userCheck->isSuccess(), Status::CODE_500, userCheck->getErrorMessage());
    //    OATPP_ASSERT_HTTP(userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
    //    #else
    //    OATPP_ASSERT_HTTP(userCheck->isSuccess() && userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
    //    #endif
    //    auto currentUserId = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

    //    auto groupIdResult = m_appPostgresql->getGroupIdByUuid(request->groupUuid);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(groupIdResult->isSuccess(), Status::CODE_500, groupIdResult->getErrorMessage());
    //    OATPP_ASSERT_HTTP(groupIdResult->hasMoreToFetch(), Status::CODE_404, "群组不存在或已失效");
    //    #else
    //    OATPP_ASSERT_HTTP(groupIdResult->isSuccess() && groupIdResult->hasMoreToFetch(), Status::CODE_404, "群组不存在或已失效");
    //    #endif
    //    auto groupId = groupIdResult->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

    //    auto result = m_appPostgresql->sendGroupMessage(currentUserId, groupId, request->messageType, request->content, request->fileUrl, request->fileSize, request->fileName, request->mimeType);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_400, result->getErrorMessage());
    //    OATPP_ASSERT_HTTP(result->hasMoreToFetch(), Status::CODE_400, "发送群消息失败");
    //    #else
    //    OATPP_ASSERT_HTTP(result->isSuccess() && result->hasMoreToFetch(), Status::CODE_400, "发送群消息失败");
    //    #endif

    //    auto message = result->fetch<oatpp::Vector<oatpp::Object<GroupMessageVO>>>()[0];
    //    message->fromUserUuid = currentUserIdHeader;
    //    message->groupUuid = request->groupUuid;
    //    message->content = request->content;
    //    message->messageType = request->messageType;
    //    return message;
    //}

    //oatpp::Boolean markGroupMessageRead(const oatpp::String& currentUserIdHeader, const oatpp::String& messageUuid) {
    //    OATPP_ASSERT_HTTP(currentUserIdHeader && !currentUserIdHeader->empty(), Status::CODE_400, "用户ID不能为空");
    //    OATPP_ASSERT_HTTP(messageUuid && !messageUuid->empty(), Status::CODE_400, "消息ID不能为空");
    //    auto userCheck = m_appPostgresql->getUserIdByUuid(currentUserIdHeader);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(userCheck->isSuccess(), Status::CODE_500, userCheck->getErrorMessage());
    //    OATPP_ASSERT_HTTP(userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
    //    #else
    //    OATPP_ASSERT_HTTP(userCheck->isSuccess() && userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
    //    #endif
    //    auto currentUserId = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

    //    auto uuidResult = m_appPostgresql->getMessageIdByUuid(messageUuid);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(uuidResult->isSuccess(), Status::CODE_500, uuidResult->getErrorMessage());
    //    OATPP_ASSERT_HTTP(uuidResult->hasMoreToFetch(), Status::CODE_404, "消息不存在");
    //    #else
    //    OATPP_ASSERT_HTTP(uuidResult->isSuccess() && uuidResult->hasMoreToFetch(), Status::CODE_404, "消息不存在");
    //    #endif
    //    auto messageId = uuidResult->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

    //    auto result = m_appPostgresql->markGroupMessageRead(messageId, currentUserId);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_400, result->getErrorMessage());
    //    #else
    //    OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_400, "标记已读失败");
    //    #endif
    //    return true;
    //}
};
