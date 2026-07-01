#pragma once

#include "global.h"
#include "server/dto/ConversationDto.hpp"
#include "server/vo/ConversationVo.hpp"
#include "server/postgresql/AppPostgresql.hpp"
#include "redis/AppRedis.hpp"
#include "tool/UuidIdCache.hpp"

class ConversationService {
private:
    std::shared_ptr<AppPostgresql> m_appPostgresql;
    std::shared_ptr<AppRedis> m_redis;
    std::shared_ptr<UuidIdCache> m_idCache;
    using Status = oatpp::web::protocol::http::Status;
public:
    ConversationService(const std::shared_ptr<AppPostgresql>& appClient, 
                        const std::shared_ptr<AppRedis>& redis,
                        const std::shared_ptr<UuidIdCache>& idCache)
        : m_appPostgresql(appClient), m_redis(redis), m_idCache(idCache) {}

    oatpp::Vector<oatpp::Object<ConversationVO>> getConversations(const oatpp::String& currentUserIdHeader) {
        OATPP_ASSERT_HTTP(currentUserIdHeader && !currentUserIdHeader->empty(), Status::CODE_400, "用户ID不能为空");
        auto id = m_idCache->getUserId(currentUserIdHeader);
        OATPP_ASSERT_HTTP(id > 0, Status::CODE_401, "用户不存在或已失效");

        auto result = m_appPostgresql->getConversations(id);
        #ifdef SQLCHECK
        if(!result->isSuccess()) {
            OATPP_LOGE("SQL_ERROR", "%s", result->getErrorMessage()->c_str());
        }
        #endif
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, "获取会话列表失败");
        return result->fetch<oatpp::Vector<oatpp::Object<ConversationVO>>>();
    }

    //oatpp::Object<ConversationDetailVO> getConversationDetail(const oatpp::String& userUuid,const oatpp::String& convUuid) {
    //    OATPP_ASSERT_HTTP(convUuid && !convUuid->empty(), Status::CODE_400, "会话ID不能为空");
    //    auto convCheck = m_appPostgresql->getConversationId(userUuid,convUuid);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(convCheck->isSuccess(), Status::CODE_500, convCheck->getErrorMessage());
    //    OATPP_ASSERT_HTTP(convCheck->hasMoreToFetch(), Status::CODE_404, "会话不存在");
    //    #else
    //    OATPP_ASSERT_HTTP(convCheck->isSuccess() && convCheck->hasMoreToFetch(), Status::CODE_404, "会话不存在");
    //    #endif
    //    auto conv_id = convCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

    //    auto result = m_appPostgresql->getConversationDetail(conv_id);
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
    //    auto result = m_appPostgresql->getUserIdByUuid(currentUserIdHeader);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, result->getErrorMessage());
    //    OATPP_ASSERT_HTTP(result->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
    //    #else
    //    OATPP_ASSERT_HTTP(result->isSuccess() && result->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
    //    #endif
    //    auto currentUserId = result->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

    //    if (request->type == "private") {
    //        OATPP_ASSERT_HTTP(request->targetUserId && !request->targetUserId->empty(), Status::CODE_400, "私聊会话缺少目标用户ID");

    //        auto userCheck = m_appPostgresql->getUserIdByUuid(request->targetUserId);
    //        #ifdef SQLCHECK
    //        OATPP_ASSERT_HTTP(userCheck->isSuccess() && userCheck->hasMoreToFetch(), Status::CODE_404, "目标用户不存在");
    //        #else
    //        OATPP_ASSERT_HTTP(userCheck->isSuccess() && userCheck->hasMoreToFetch(), Status::CODE_404, "目标用户不存在");
    //        #endif
    //        auto target_id = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

    //        OATPP_ASSERT_HTTP(currentUserId != target_id, Status::CODE_400, "不能与自己创建会话");

    //        auto createResult = m_appPostgresql->createPrivateConversation(currentUserId, target_id);
    //        #ifdef SQLCHECK
    //        OATPP_ASSERT_HTTP(createResult->isSuccess(), Status::CODE_500, "创建会话失败");
    //        OATPP_ASSERT_HTTP(createResult->hasMoreToFetch(), Status::CODE_500, "创建会话失败");
    //        #else
    //        OATPP_ASSERT_HTTP(createResult->isSuccess() && createResult->hasMoreToFetch(), Status::CODE_500, "创建会话失败");
    //        #endif

    //        return createResult->fetch<oatpp::Object<ConversationVO>>();
    //    } else if (request->type == "group") {
    //        OATPP_ASSERT_HTTP(request->groupId && !request->groupId->empty(), Status::CODE_400, "群聊会话缺少群组ID");

    //        auto groupCheck = m_appPostgresql->getGroupIdByUuid(request->groupId);
    //        #ifdef SQLCHECK
    //        OATPP_ASSERT_HTTP(groupCheck->isSuccess() && groupCheck->hasMoreToFetch(), Status::CODE_404, "群组不存在");
    //        #else
    //        OATPP_ASSERT_HTTP(groupCheck->isSuccess() && groupCheck->hasMoreToFetch(), Status::CODE_404, "群组不存在");
    //        #endif
    //        auto group_id = groupCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

    //        auto membersResult = m_appPostgresql->getGroupMembers(group_id);
    //        #ifdef SQLCHECK
    //        OATPP_ASSERT_HTTP(membersResult->isSuccess(), Status::CODE_500, "获取群组成员失败");
    //        #else
    //        OATPP_ASSERT_HTTP(membersResult->isSuccess(), Status::CODE_500, "获取群组成员失败");
    //        #endif

    //        auto memberIds = membersResult->fetch<oatpp::Vector<oatpp::Int64>>();
    //        OATPP_ASSERT_HTTP(memberIds && !memberIds->empty(), Status::CODE_404, "群组没有成员");

    //        oatpp::Vector<oatpp::Object<ConversationVO>> createdConversations = oatpp::Vector<oatpp::Object<ConversationVO>>::createShared();
    //        for (const auto& memberId : *memberIds) {
    //            auto convResult = m_appPostgresql->createGroupConversation(memberId, group_id);
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
    //    auto groupCheck = m_appPostgresql->getGroupIdByUuid(convUuid);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(groupCheck->isSuccess(), Status::CODE_500, groupCheck->getErrorMessage());
    //    OATPP_ASSERT_HTTP(groupCheck->hasMoreToFetch(), Status::CODE_404, "会话不存在");
    //    #else
    //    OATPP_ASSERT_HTTP(groupCheck->isSuccess(), Status::CODE_500, groupCheck->getErrorMessage());
    //    #endif
    //    auto group_id = groupCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

    //    auto result = m_appPostgresql->updateConversation(group_id, request->name, request->avatarUrl);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_400, "更新会话失败");
    //    OATPP_ASSERT_HTTP(result->hasMoreToFetch(), Status::CODE_404, "会话不存在");
    //    #else
    //    OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_400, "更新会话失败");
    //    #endif

    //    auto updatedConversation = m_appPostgresql->getConversationDetail(group_id);
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
    //    auto convCheck = m_appPostgresql->getConversationIdByUuid(convUuid);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(convCheck->isSuccess(), Status::CODE_500, convCheck->getErrorMessage());
    //    OATPP_ASSERT_HTTP(convCheck->hasMoreToFetch(), Status::CODE_404, "会话不存在");
    //    #else
    //    OATPP_ASSERT_HTTP(convCheck->isSuccess() && convCheck->hasMoreToFetch(), Status::CODE_404, "会话不存在");
    //    #endif
    //    auto conv_id = convCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

    //    auto result = m_appPostgresql->deleteConversation(conv_id);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_400, "删除会话失败");
    //    #else
    //    OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_400, "删除会话失败");
    //    #endif
    //    return true;
    //}

    oatpp::Boolean markPrivateConversationRead(const oatpp::String& currentUserIdHeader, const oatpp::String& targetUserUuid) {
        OATPP_ASSERT_HTTP(currentUserIdHeader && !currentUserIdHeader->empty(), Status::CODE_400, "用户ID不能为空");
        OATPP_ASSERT_HTTP(targetUserUuid && !targetUserUuid->empty(), Status::CODE_400, "目标用户ID不能为空");

        auto userId = m_idCache->getUserId(currentUserIdHeader);
        OATPP_ASSERT_HTTP(userId > 0, Status::CODE_401, "用户不存在或已失效");

        auto targetUserId = m_idCache->getUserId(targetUserUuid);
        OATPP_ASSERT_HTTP(targetUserId > 0, Status::CODE_404, "目标用户不存在");

        auto result = m_appPostgresql->markPrivateConversationRead(userId, targetUserId);
        #ifdef SQLCHECK
        if(!result->isSuccess()) {
            OATPP_LOGE("SQL_ERROR", "%s", result->getErrorMessage()->c_str());
        }
        #endif
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, "标记已读失败");
        return true;
    }

    oatpp::Boolean markGroupConversationRead(const oatpp::String& currentUserIdHeader, const oatpp::String& groupUuid) {
        OATPP_ASSERT_HTTP(currentUserIdHeader && !currentUserIdHeader->empty(), Status::CODE_400, "用户ID不能为空");
        OATPP_ASSERT_HTTP(groupUuid && !groupUuid->empty(), Status::CODE_400, "群组ID不能为空");

        auto userId = m_idCache->getUserId(currentUserIdHeader);
        OATPP_ASSERT_HTTP(userId > 0, Status::CODE_401, "用户不存在或已失效");

        auto groupId = m_idCache->getGroupId(groupUuid);
        OATPP_ASSERT_HTTP(groupId > 0, Status::CODE_404, "群组不存在");

        auto result = m_appPostgresql->markGroupConversationRead(userId, groupId);
        #ifdef SQLCHECK
        if(!result->isSuccess()) {
            OATPP_LOGE("SQL_ERROR", "%s", result->getErrorMessage()->c_str());
        }
        #endif
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, "标记已读失败");
        return true;
    }

    oatpp::Boolean mutePrivateConversation(const oatpp::String& currentUserIdHeader, const oatpp::String& targetUserUuid) {
        OATPP_ASSERT_HTTP(currentUserIdHeader && !currentUserIdHeader->empty(), Status::CODE_400, "用户ID不能为空");
        OATPP_ASSERT_HTTP(targetUserUuid && !targetUserUuid->empty(), Status::CODE_400, "目标用户ID不能为空");

        auto userId = m_idCache->getUserId(currentUserIdHeader);
        OATPP_ASSERT_HTTP(userId > 0, Status::CODE_401, "用户不存在或已失效");

        auto targetUserId = m_idCache->getUserId(targetUserUuid);
        OATPP_ASSERT_HTTP(targetUserId > 0, Status::CODE_404, "目标用户不存在");

        auto result = m_appPostgresql->mutePrivateConversation(userId, targetUserId);
        #ifdef SQLCHECK
        if(!result->isSuccess()) {
            OATPP_LOGE("SQL_ERROR", "%s", result->getErrorMessage()->c_str());
        }
        #endif
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, "设置免打扰失败");
        return true;
    }

    oatpp::Boolean muteGroupConversation(const oatpp::String& currentUserIdHeader, const oatpp::String& groupUuid) {
        OATPP_ASSERT_HTTP(currentUserIdHeader && !currentUserIdHeader->empty(), Status::CODE_400, "用户ID不能为空");
        OATPP_ASSERT_HTTP(groupUuid && !groupUuid->empty(), Status::CODE_400, "群组ID不能为空");

        auto userId = m_idCache->getUserId(currentUserIdHeader);
        OATPP_ASSERT_HTTP(userId > 0, Status::CODE_401, "用户不存在或已失效");

        auto groupId = m_idCache->getGroupId(groupUuid);
        OATPP_ASSERT_HTTP(groupId > 0, Status::CODE_404, "群组不存在");

        auto result = m_appPostgresql->muteGroupConversation(userId, groupId);
        #ifdef SQLCHECK
        if(!result->isSuccess()) {
            OATPP_LOGE("SQL_ERROR", "%s", result->getErrorMessage()->c_str());
        }
        #endif
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, "设置免打扰失败");
        return true;
    }

    oatpp::Boolean unmutePrivateConversation(const oatpp::String& currentUserIdHeader, const oatpp::String& targetUserUuid) {
        OATPP_ASSERT_HTTP(currentUserIdHeader && !currentUserIdHeader->empty(), Status::CODE_400, "用户ID不能为空");
        OATPP_ASSERT_HTTP(targetUserUuid && !targetUserUuid->empty(), Status::CODE_400, "目标用户ID不能为空");

        auto userId = m_idCache->getUserId(currentUserIdHeader);
        OATPP_ASSERT_HTTP(userId > 0, Status::CODE_401, "用户不存在或已失效");

        auto targetUserId = m_idCache->getUserId(targetUserUuid);
        OATPP_ASSERT_HTTP(targetUserId > 0, Status::CODE_404, "目标用户不存在");

        auto result = m_appPostgresql->unmutePrivateConversation(userId, targetUserId);
        #ifdef SQLCHECK
        if(!result->isSuccess()) {
            OATPP_LOGE("SQL_ERROR", "%s", result->getErrorMessage()->c_str());
        }
        #endif
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, "取消免打扰失败");
        return true;
    }

    oatpp::Boolean unmuteGroupConversation(const oatpp::String& currentUserIdHeader, const oatpp::String& groupUuid) {
        OATPP_ASSERT_HTTP(currentUserIdHeader && !currentUserIdHeader->empty(), Status::CODE_400, "用户ID不能为空");
        OATPP_ASSERT_HTTP(groupUuid && !groupUuid->empty(), Status::CODE_400, "群组ID不能为空");

        auto userId = m_idCache->getUserId(currentUserIdHeader);
        OATPP_ASSERT_HTTP(userId > 0, Status::CODE_401, "用户不存在或已失效");

        auto groupId = m_idCache->getGroupId(groupUuid);
        OATPP_ASSERT_HTTP(groupId > 0, Status::CODE_404, "群组不存在");

        auto result = m_appPostgresql->unmuteGroupConversation(userId, groupId);
        #ifdef SQLCHECK
        if(!result->isSuccess()) {
            OATPP_LOGE("SQL_ERROR", "%s", result->getErrorMessage()->c_str());
        }
        #endif
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, "取消免打扰失败");
        return true;
    }

    //oatpp::Vector<oatpp::Object<ConversationMemberVO>> getConversationMembers(const oatpp::String& currentUserIdHeader, const oatpp::String& convUuid) {
    //    OATPP_ASSERT_HTTP(currentUserIdHeader && !currentUserIdHeader->empty(), Status::CODE_400, "用户ID不能为空");
    //    OATPP_ASSERT_HTTP(convUuid && !convUuid->empty(), Status::CODE_400, "会话ID不能为空");
    //    auto userCheck = m_appPostgresql->getUserIdByUuid(currentUserIdHeader);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(userCheck->isSuccess(), Status::CODE_500, userCheck->getErrorMessage());
    //    OATPP_ASSERT_HTTP(userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
    //    #else
    //    OATPP_ASSERT_HTTP(userCheck->isSuccess(), Status::CODE_500, userCheck->getErrorMessage());
    //    #endif
    //    auto user_id = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

    //    auto convCheck = m_appPostgresql->getConversationIdByUuid(convUuid);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(convCheck->isSuccess(), Status::CODE_500, convCheck->getErrorMessage());
    //    OATPP_ASSERT_HTTP(convCheck->hasMoreToFetch(), Status::CODE_404, "会话不存在");
    //    #else
    //    OATPP_ASSERT_HTTP(convCheck->isSuccess() && convCheck->hasMoreToFetch(), Status::CODE_404, "会话不存在");
    //    #endif
    //    auto conv_id = convCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

    //    auto result = m_appPostgresql->getConversationMembers(conv_id);
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
    //    auto convCheck = m_appPostgresql->getConversationIdByUuid(convUuid);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(convCheck->isSuccess(), Status::CODE_500, convCheck->getErrorMessage());
    //    OATPP_ASSERT_HTTP(convCheck->hasMoreToFetch(), Status::CODE_404, "会话不存在");
    //    #else
    //    OATPP_ASSERT_HTTP(convCheck->isSuccess() && convCheck->hasMoreToFetch(), Status::CODE_404, "会话不存在");
    //    #endif
    //    auto conv_id = convCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

    //    auto transaction = m_appPostgresql->beginTransaction();
    //    for (auto userId : *request->userIds) {
    //        OATPP_ASSERT_HTTP(userId && !userId->empty(), Status::CODE_400, "用户ID不能为空");
    //        auto userCheck = m_appPostgresql->getUserIdByUuid(userId);
    //        #ifdef SQLCHECK
    //        OATPP_ASSERT_HTTP(userCheck->isSuccess() && userCheck->hasMoreToFetch(), Status::CODE_404, "用户不存在");
    //        #else
    //        OATPP_ASSERT_HTTP(userCheck->isSuccess(), Status::CODE_500, userCheck->getErrorMessage());
    //        #endif
    //        auto user_id = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;
    //        auto result = m_appPostgresql->addConversationMembers(conv_id, user_id);
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
    //    auto userCheck = m_appPostgresql->getUserIdByUuid(currentUserIdHeader);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(userCheck->isSuccess(), Status::CODE_500, userCheck->getErrorMessage());
    //    OATPP_ASSERT_HTTP(userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
    //    #else
    //    OATPP_ASSERT_HTTP(userCheck->isSuccess(), Status::CODE_500, userCheck->getErrorMessage());
    //    #endif
    //    auto user_id = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

    //    auto convCheck = m_appPostgresql->getConversationIdByUuid(convUuid);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(convCheck->isSuccess(), Status::CODE_500, convCheck->getErrorMessage());
    //    OATPP_ASSERT_HTTP(convCheck->hasMoreToFetch(), Status::CODE_404, "会话不存在");
    //    #else
    //    OATPP_ASSERT_HTTP(convCheck->isSuccess() && convCheck->hasMoreToFetch(), Status::CODE_404, "会话不存在");
    //    #endif
    //    auto conv_id = convCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

    //    auto result = m_appPostgresql->leaveConversation(conv_id, user_id);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_400, "退出会话失败");
    //    #else
    //    OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_400, "退出会话失败");
    //    #endif
    //    return true;
    //}
};
