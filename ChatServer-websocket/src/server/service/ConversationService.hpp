#pragma once

#include "global.h"
#include"../dto/GeneralDto.hpp"
#include "../dto/ConversationDto.hpp"
#include "../vo/ConversationVo.hpp"
#include "../postgresql/AppPostgresql.hpp"
#include "../../tool/UuidIdCache.hpp"

class ConversationService {
private:
    std::shared_ptr<AppPostgresql> m_appClient;
    std::shared_ptr<UuidIdCache> m_idCache;
    using Status = oatpp::web::protocol::http::Status;
public:
    ConversationService(const std::shared_ptr<AppPostgresql>& appClient,const std::shared_ptr<UuidIdCache>& idCache) 
        : m_appClient(appClient), m_idCache(idCache) {}

    oatpp::Vector<oatpp::Object<ConversationVO>> getConversations(const oatpp::String& currentUserIdHeader) {
        ASYNC_THROW_IF(currentUserIdHeader && !currentUserIdHeader->empty(), "User ID cannot be empty");
        auto id = m_idCache->getUserId(currentUserIdHeader);
        ASYNC_THROW_IF(id > 0, "User does not exist or has been deactivated");

        auto result = m_appClient->getConversations(id);
        #ifdef SQLCHECK
        if (!result->isSuccess()) {
            OATPP_LOGD("ConversationService", "Error: %s", result->getErrorMessage()->c_str());
            throw std::runtime_error("Failed to get conversation list");
        }
        #else
        ASYNC_THROW_IF(result->isSuccess(), "Failed to get conversation list");
        #endif
        return result->fetch<oatpp::Vector<oatpp::Object<ConversationVO>>>();
    }

    oatpp::Boolean markPrivateConversationRead(const oatpp::String& currentUserIdHeader, const oatpp::String& targetUserUuid) {
        ASYNC_THROW_IF(currentUserIdHeader && !currentUserIdHeader->empty(), "User ID cannot be empty");
        ASYNC_THROW_IF(targetUserUuid && !targetUserUuid->empty(), "Target user ID cannot be empty");

        auto userId = m_idCache->getUserId(currentUserIdHeader);
        ASYNC_THROW_IF(userId > 0, "User does not exist or has been deactivated");

        auto targetUserId = m_idCache->getUserId(targetUserUuid);
        ASYNC_THROW_IF(targetUserId > 0, "Target user does not exist");

        auto result = m_appClient->markPrivateConversationRead(userId, targetUserId);
        #ifdef SQLCHECK
        if (!result->isSuccess()) {
            OATPP_LOGE("SQL_ERROR", "%s", result->getErrorMessage()->c_str());
        }
        #endif
        ASYNC_THROW_IF(result->isSuccess(), "Failed to mark as read");
        return true;
    }

    oatpp::Boolean markGroupConversationRead(const oatpp::String& currentUserIdHeader, const oatpp::String& groupUuid) {
        ASYNC_THROW_IF(currentUserIdHeader && !currentUserIdHeader->empty(), "User ID cannot be empty");
        ASYNC_THROW_IF(groupUuid && !groupUuid->empty(), "Group ID cannot be empty");

        auto userId = m_idCache->getUserId(currentUserIdHeader);
        ASYNC_THROW_IF(userId > 0, "User does not exist or has been deactivated");

        auto groupId = m_idCache->getGroupId(groupUuid);
        ASYNC_THROW_IF(groupId > 0, "Group does not exist");

        auto result = m_appClient->markGroupConversationRead(userId, groupId);
        #ifdef SQLCHECK
        if (!result->isSuccess()) {
            OATPP_LOGE("SQL_ERROR", "%s", result->getErrorMessage()->c_str());
        }
        #endif
        ASYNC_THROW_IF(result->isSuccess(), "Failed to mark as read");
        return true;
    }

    oatpp::Boolean mutePrivateConversation(const oatpp::String& currentUserIdHeader, const oatpp::String& targetUserUuid) {
        ASYNC_THROW_IF(currentUserIdHeader && !currentUserIdHeader->empty(), "User ID cannot be empty");
        ASYNC_THROW_IF(targetUserUuid && !targetUserUuid->empty(), "Target user ID cannot be empty");

        auto userId = m_idCache->getUserId(currentUserIdHeader);
        ASYNC_THROW_IF(userId > 0, "User does not exist or has been deactivated");

        auto targetUserId = m_idCache->getUserId(targetUserUuid);
        ASYNC_THROW_IF(targetUserId > 0, "Target user does not exist");

        auto result = m_appClient->mutePrivateConversation(userId, targetUserId);
        #ifdef SQLCHECK
        if (!result->isSuccess()) {
            OATPP_LOGE("SQL_ERROR", "%s", result->getErrorMessage()->c_str());
        }
        #endif
        ASYNC_THROW_IF(result->isSuccess(), "Failed to mute conversation");
        return true;
    }

    oatpp::Boolean muteGroupConversation(const oatpp::String& currentUserIdHeader, const oatpp::String& groupUuid) {
        ASYNC_THROW_IF(currentUserIdHeader && !currentUserIdHeader->empty(), "User ID cannot be empty");
        ASYNC_THROW_IF(groupUuid && !groupUuid->empty(), "Group ID cannot be empty");

        auto userId = m_idCache->getUserId(currentUserIdHeader);
        ASYNC_THROW_IF(userId > 0, "User does not exist or has been deactivated");

        auto groupId = m_idCache->getGroupId(groupUuid);
        ASYNC_THROW_IF(groupId > 0, "Group does not exist");

        auto result = m_appClient->muteGroupConversation(userId, groupId);
        #ifdef SQLCHECK
        if (!result->isSuccess()) {
            OATPP_LOGE("SQL_ERROR", "%s", result->getErrorMessage()->c_str());
        }
        #endif
        ASYNC_THROW_IF(result->isSuccess(), "Failed to mute conversation");
        return true;
    }

    oatpp::Boolean unmutePrivateConversation(const oatpp::String& currentUserIdHeader, const oatpp::String& targetUserUuid) {
        ASYNC_THROW_IF(currentUserIdHeader && !currentUserIdHeader->empty(), "User ID cannot be empty");
        ASYNC_THROW_IF(targetUserUuid && !targetUserUuid->empty(), "Target user ID cannot be empty");

        auto userId = m_idCache->getUserId(currentUserIdHeader);
        ASYNC_THROW_IF(userId > 0, "User does not exist or has been deactivated");

        auto targetUserId = m_idCache->getUserId(targetUserUuid);
        ASYNC_THROW_IF(targetUserId > 0, "Target user does not exist");

        auto result = m_appClient->unmutePrivateConversation(userId, targetUserId);
        #ifdef SQLCHECK
        if (!result->isSuccess()) {
            OATPP_LOGE("SQL_ERROR", "%s", result->getErrorMessage()->c_str());
        }
        #endif
        ASYNC_THROW_IF(result->isSuccess(), "Failed to unmute conversation");
        return true;
    }

    oatpp::Boolean unmuteGroupConversation(const oatpp::String& currentUserIdHeader, const oatpp::String& groupUuid) {
        ASYNC_THROW_IF(currentUserIdHeader && !currentUserIdHeader->empty(), "User ID cannot be empty");
        ASYNC_THROW_IF(groupUuid && !groupUuid->empty(), "Group ID cannot be empty");

        auto userId = m_idCache->getUserId(currentUserIdHeader);
        ASYNC_THROW_IF(userId > 0, "User does not exist or has been deactivated");

        auto groupId = m_idCache->getGroupId(groupUuid);
        ASYNC_THROW_IF(groupId > 0, "Group does not exist");

        auto result = m_appClient->unmuteGroupConversation(userId, groupId);
        #ifdef SQLCHECK
        if (!result->isSuccess()) {
            OATPP_LOGE("SQL_ERROR", "%s", result->getErrorMessage()->c_str());
        }
        #endif
        ASYNC_THROW_IF(result->isSuccess(), "Failed to unmute conversation");
        return true;
    }

    //oatpp::Object<ConversationDetailVO> getConversationDetail(const oatpp::String& userUuid,const oatpp::String& convUuid) {
    //    OATPP_ASSERT_HTTP(convUuid && !convUuid->empty(), Status::CODE_400, "会话ID不能为空");
    //    auto convCheck = m_appClient->getConversationId(userUuid,convUuid);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(convCheck->isSuccess(), Status::CODE_500, convCheck->getErrorMessage());
    //    OATPP_ASSERT_HTTP(convCheck->hasMoreToFetch(), Status::CODE_404, "会话不存在");
    //    #else
    //    OATPP_ASSERT_HTTP(convCheck->isSuccess() && convCheck->hasMoreToFetch(), Status::CODE_404, "会话不存在");
    //    #endif
    //    auto conv_id = convCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

    //    auto result = m_appClient->getConversationDetail(conv_id);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_404, result->getErrorMessage());
    //    OATPP_ASSERT_HTTP(result->hasMoreToFetch(), Status::CODE_404, "会话不存在");
    //    #else
    //    OATPP_ASSERT_HTTP(result->isSuccess() && result->hasMoreToFetch(), Status::CODE_404, "会话不存在");
    //    #endif
    //    return result->fetch<oatpp::Vector<oatpp::Object<ConversationDetailVO>>>()[0];
    //}

    //oatpp::Object<ConversationVO> createConversation(const oatpp::String& currentUserIdHeader, const oatpp::Object<CreateConversationRequestDTO>& request) {
    //    OATPP_ASSERT_HTTP(currentUserIdHeader && !currentUserIdHeader->empty(), Status::CODE_400, "用户ID不能为空");
    //    OATPP_ASSERT_HTTP(request, Status::CODE_400, "请求参数不能为空");
    //    OATPP_ASSERT_HTTP(request->type && !request->type->empty(), Status::CODE_400, "会话类型不能为空");
    //    auto result = m_appClient->getUserIdByUuid(currentUserIdHeader);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, result->getErrorMessage());
    //    OATPP_ASSERT_HTTP(result->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
    //    #else
    //    OATPP_ASSERT_HTTP(result->isSuccess() && result->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
    //    #endif
    //    auto currentUserId = result->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

    //    if (request->type == "private") {
    //        OATPP_ASSERT_HTTP(request->targetUserId && !request->targetUserId->empty(), Status::CODE_400, "私聊会话缺少目标用户ID");

    //        auto userCheck = m_appClient->getUserIdByUuid(request->targetUserId);
    //        #ifdef SQLCHECK
    //        OATPP_ASSERT_HTTP(userCheck->isSuccess() && userCheck->hasMoreToFetch(), Status::CODE_404, "目标用户不存在");
    //        #else
    //        OATPP_ASSERT_HTTP(userCheck->isSuccess() && userCheck->hasMoreToFetch(), Status::CODE_404, "目标用户不存在");
    //        #endif
    //        auto target_id = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

    //        OATPP_ASSERT_HTTP(currentUserId != target_id, Status::CODE_400, "不能与自己创建会话");

    //        auto createResult = m_appClient->createPrivateConversation(currentUserId, target_id);
    //        #ifdef SQLCHECK
    //        OATPP_ASSERT_HTTP(createResult->isSuccess(), Status::CODE_500, "创建会话失败");
    //        OATPP_ASSERT_HTTP(createResult->hasMoreToFetch(), Status::CODE_500, "创建会话失败");
    //        #else
    //        OATPP_ASSERT_HTTP(createResult->isSuccess() && createResult->hasMoreToFetch(), Status::CODE_500, "创建会话失败");
    //        #endif

    //        return createResult->fetch<oatpp::Object<ConversationVO>>();
    //    } else if (request->type == "group") {
    //        OATPP_ASSERT_HTTP(request->groupId && !request->groupId->empty(), Status::CODE_400, "群聊会话缺少群组ID");

    //        auto groupCheck = m_appClient->getGroupIdByUuid(request->groupId);
    //        #ifdef SQLCHECK
    //        OATPP_ASSERT_HTTP(groupCheck->isSuccess() && groupCheck->hasMoreToFetch(), Status::CODE_404, "群组不存在");
    //        #else
    //        OATPP_ASSERT_HTTP(groupCheck->isSuccess() && groupCheck->hasMoreToFetch(), Status::CODE_404, "群组不存在");
    //        #endif
    //        auto group_id = groupCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

    //        auto membersResult = m_appClient->getGroupMembers(group_id);
    //        #ifdef SQLCHECK
    //        OATPP_ASSERT_HTTP(membersResult->isSuccess(), Status::CODE_500, "获取群组成员失败");
    //        #else
    //        OATPP_ASSERT_HTTP(membersResult->isSuccess(), Status::CODE_500, "获取群组成员失败");
    //        #endif

    //        auto memberIds = membersResult->fetch<oatpp::Vector<oatpp::Int64>>();
    //        OATPP_ASSERT_HTTP(memberIds && !memberIds->empty(), Status::CODE_404, "群组没有成员");

    //        oatpp::Vector<oatpp::Object<ConversationVO>> createdConversations = oatpp::Vector<oatpp::Object<ConversationVO>>::createShared();
    //        for (const auto& memberId : *memberIds) {
    //            auto convResult = m_appClient->createGroupConversation(memberId, group_id);
    //            if (convResult->isSuccess()) {
    //                auto conversation = convResult->fetch<oatpp::Object<ConversationVO>>();
    //                if (conversation) {
    //                    createdConversations->push_back(conversation);
    //                }
    //            }
    //        }

    //        OATPP_ASSERT_HTTP(createdConversations && !createdConversations->empty(), Status::CODE_500, "创建群聊会话失败");
    //        return createdConversations->front();
    //    } else {
    //        OATPP_ASSERT_HTTP(false, Status::CODE_400, "无效的会话类型，使用'private'或'group'");
    //        return nullptr;
    //    }
    //}

    //oatpp::Object<ConversationVO> updateConversation(const oatpp::String& convUuid, const oatpp::Object<UpdateConversationRequestDTO>& request) {
    //    OATPP_ASSERT_HTTP(convUuid && !convUuid->empty(), Status::CODE_400, "会话ID不能为空");
    //    OATPP_ASSERT_HTTP(request, Status::CODE_400, "请求参数不能为空");
    //    auto groupCheck = m_appClient->getGroupIdByUuid(convUuid);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(groupCheck->isSuccess(), Status::CODE_500, groupCheck->getErrorMessage());
    //    OATPP_ASSERT_HTTP(groupCheck->hasMoreToFetch(), Status::CODE_404, "会话不存在");
    //    #else
    //    OATPP_ASSERT_HTTP(groupCheck->isSuccess(), Status::CODE_500, groupCheck->getErrorMessage());
    //    #endif
    //    auto group_id = groupCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

    //    auto result = m_appClient->updateConversation(group_id, request->name, request->avatarUrl);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_400, "更新会话失败");
    //    OATPP_ASSERT_HTTP(result->hasMoreToFetch(), Status::CODE_404, "会话不存在");
    //    #else
    //    OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_400, "更新会话失败");
    //    #endif

    //    auto updatedConversation = m_appClient->getConversationDetail(group_id);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(updatedConversation->isSuccess(), Status::CODE_404, "会话不存在");
    //    OATPP_ASSERT_HTTP(updatedConversation->hasMoreToFetch(), Status::CODE_404, "会话不存在");
    //    #else
    //    OATPP_ASSERT_HTTP(updatedConversation->isSuccess(), Status::CODE_404, "会话不存在");
    //    #endif

    //    return updatedConversation->fetch<oatpp::Object<ConversationVO>>();
    //}

    //oatpp::Boolean deleteConversation(const oatpp::String& convUuid) {
    //    OATPP_ASSERT_HTTP(convUuid && !convUuid->empty(), Status::CODE_400, "会话ID不能为空");
    //    auto convCheck = m_appClient->getConversationIdByUuid(convUuid);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(convCheck->isSuccess(), Status::CODE_500, convCheck->getErrorMessage());
    //    OATPP_ASSERT_HTTP(convCheck->hasMoreToFetch(), Status::CODE_404, "会话不存在");
    //    #else
    //    OATPP_ASSERT_HTTP(convCheck->isSuccess() && convCheck->hasMoreToFetch(), Status::CODE_404, "会话不存在");
    //    #endif
    //    auto conv_id = convCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

    //    auto result = m_appClient->deleteConversation(conv_id);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_400, "删除会话失败");
    //    #else
    //    OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_400, "删除会话失败");
    //    #endif
    //    return true;
    //}

    //oatpp::Boolean markConversationRead(const oatpp::String& currentUserIdHeader, const oatpp::String& convUuid) {
    //    ASYNC_THROW_IF(currentUserIdHeader && !currentUserIdHeader->empty(), "User ID cannot be empty");
    //    ASYNC_THROW_IF(convUuid && !convUuid->empty(), "Conversation ID cannot be empty");
    //    auto userCheck = m_appClient->getUserIdByUuid(currentUserIdHeader);
    //    #ifdef SQLCHECK
    //    ASYNC_THROW_IF(userCheck->isSuccess(), userCheck->getErrorMessage());
    //    ASYNC_THROW_IF(userCheck->hasMoreToFetch(), "User does not exist or has been deactivated");
    //    #else
    //    ASYNC_THROW_IF(userCheck->isSuccess() && userCheck->hasMoreToFetch(), "User does not exist or has been deactivated");
    //    #endif
    //    auto userId = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

    //    // 2. 尝试作为目标用户 ID 查询
    //    auto targetUserResult = m_appClient->getUserIdByUuid(convUuid);
    //    if (targetUserResult->isSuccess() && targetUserResult->hasMoreToFetch()) {
    //        auto targetUserId = targetUserResult->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

    //        // 查询私聊会话
    //        auto convResult = m_appClient->getConversationId(userId, targetUserId);
    //        if (convResult->isSuccess() && convResult->hasMoreToFetch()) {
    //            #ifdef SQLCHECK
    //            auto result = m_appClient->markConversationRead(targetUserId, userId);
    //            ASYNC_THROW_IF(result->isSuccess(), "Failed to mark as read");
    //            #else
    //            auto result = m_appClient->markConversationRead(targetUserId, userId);
    //            ASYNC_THROW_IF(result->isSuccess(), "Failed to mark as read");
    //            #endif
    //            return true;
    //        }
    //    }

    //    // 3. 尝试作为群组 ID 查询
    //    auto groupResult = m_appClient->getGroupIdByUuid(convUuid);
    //    if (groupResult->isSuccess() && groupResult->hasMoreToFetch()) {
    //        auto groupId = groupResult->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

    //        // 查询群聊会话
    //        auto convResult = m_appClient->getConversationId(userId, groupId);
    //        if (convResult->isSuccess() && convResult->hasMoreToFetch()) {
    //            #ifdef SQLCHECK
    //            auto result = m_appClient->markConversationRead(groupId, userId);
    //            ASYNC_THROW_IF(result->isSuccess(), result->getErrorMessage());
    //            #else
    //            auto result = m_appClient->markConversationRead(groupId, userId);
    //            ASYNC_THROW_IF(result->isSuccess(), "Failed to mark as read");
    //            #endif
    //            return true;
    //        }
    //    }
    //    throw oatpp::web::protocol::http::HttpError(Status::CODE_400, "Failed to mark as read");
    //}

    //oatpp::Vector<oatpp::Object<ConversationMemberVO>> getConversationMembers(const oatpp::String& currentUserIdHeader, const oatpp::String& convUuid) {
    //    OATPP_ASSERT_HTTP(currentUserIdHeader && !currentUserIdHeader->empty(), Status::CODE_400, "用户ID不能为空");
    //    OATPP_ASSERT_HTTP(convUuid && !convUuid->empty(), Status::CODE_400, "会话ID不能为空");
    //    auto userCheck = m_appClient->getUserIdByUuid(currentUserIdHeader);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(userCheck->isSuccess(), Status::CODE_500, userCheck->getErrorMessage());
    //    OATPP_ASSERT_HTTP(userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
    //    #else
    //    OATPP_ASSERT_HTTP(userCheck->isSuccess(), Status::CODE_500, userCheck->getErrorMessage());
    //    #endif
    //    auto user_id = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

    //    auto convCheck = m_appClient->getConversationIdByUuid(convUuid);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(convCheck->isSuccess(), Status::CODE_500, convCheck->getErrorMessage());
    //    OATPP_ASSERT_HTTP(convCheck->hasMoreToFetch(), Status::CODE_404, "会话不存在");
    //    #else
    //    OATPP_ASSERT_HTTP(convCheck->isSuccess() && convCheck->hasMoreToFetch(), Status::CODE_404, "会话不存在");
    //    #endif
    //    auto conv_id = convCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

    //    auto result = m_appClient->getConversationMembers(conv_id);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_404, "会话不存在");
    //    #else
    //    OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_404, "会话不存在");
    //    #endif
    //    return result->fetch<oatpp::Vector<oatpp::Object<ConversationMemberVO>>>();
    //}

    //oatpp::Boolean addConversationMembers(const oatpp::String& convUuid, const oatpp::Object<AddConversationMemberRequestDTO>& request) {
    //    OATPP_ASSERT_HTTP(convUuid && !convUuid->empty(), Status::CODE_400, "会话ID不能为空");
    //    OATPP_ASSERT_HTTP(request, Status::CODE_400, "请求参数不能为空");
    //    OATPP_ASSERT_HTTP(request->userIds && !request->userIds->empty(), Status::CODE_400, "用户ID列表不能为空");
    //    auto convCheck = m_appClient->getConversationIdByUuid(convUuid);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(convCheck->isSuccess(), Status::CODE_500, convCheck->getErrorMessage());
    //    OATPP_ASSERT_HTTP(convCheck->hasMoreToFetch(), Status::CODE_404, "会话不存在");
    //    #else
    //    OATPP_ASSERT_HTTP(convCheck->isSuccess() && convCheck->hasMoreToFetch(), Status::CODE_404, "会话不存在");
    //    #endif
    //    auto conv_id = convCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

    //    auto transaction = m_appClient->beginTransaction();
    //    for (auto userId : *request->userIds) {
    //        OATPP_ASSERT_HTTP(userId && !userId->empty(), Status::CODE_400, "用户ID不能为空");
    //        auto userCheck = m_appClient->getUserIdByUuid(userId);
    //        #ifdef SQLCHECK
    //        OATPP_ASSERT_HTTP(userCheck->isSuccess() && userCheck->hasMoreToFetch(), Status::CODE_404, "用户不存在");
    //        #else
    //        OATPP_ASSERT_HTTP(userCheck->isSuccess(), Status::CODE_500, userCheck->getErrorMessage());
    //        #endif
    //        auto user_id = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;
    //        auto result = m_appClient->addConversationMembers(conv_id, user_id);
    //        if (!result->isSuccess()) {
    //            transaction.rollback();
    //            OATPP_ASSERT_HTTP(false, Status::CODE_400, "添加成员失败");
    //        }
    //    }
    //    transaction.commit();
    //    return true;
    //}

    //oatpp::Boolean leaveConversation(const oatpp::String& currentUserIdHeader, const oatpp::String& convUuid) {
    //    OATPP_ASSERT_HTTP(currentUserIdHeader && !currentUserIdHeader->empty(), Status::CODE_400, "用户ID不能为空");
    //    OATPP_ASSERT_HTTP(convUuid && !convUuid->empty(), Status::CODE_400, "会话ID不能为空");
    //    auto userCheck = m_appClient->getUserIdByUuid(currentUserIdHeader);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(userCheck->isSuccess(), Status::CODE_500, userCheck->getErrorMessage());
    //    OATPP_ASSERT_HTTP(userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
    //    #else
    //    OATPP_ASSERT_HTTP(userCheck->isSuccess(), Status::CODE_500, userCheck->getErrorMessage());
    //    #endif
    //    auto user_id = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

    //    auto convCheck = m_appClient->getConversationIdByUuid(convUuid);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(convCheck->isSuccess(), Status::CODE_500, convCheck->getErrorMessage());
    //    OATPP_ASSERT_HTTP(convCheck->hasMoreToFetch(), Status::CODE_404, "会话不存在");
    //    #else
    //    OATPP_ASSERT_HTTP(convCheck->isSuccess() && convCheck->hasMoreToFetch(), Status::CODE_404, "会话不存在");
    //    #endif
    //    auto conv_id = convCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

    //    auto result = m_appClient->leaveConversation(conv_id, user_id);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_400, "退出会话失败");
    //    #else
    //    OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_400, "退出会话失败");
    //    #endif
    //    return true;
    //}
};
