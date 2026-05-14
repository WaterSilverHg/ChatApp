#pragma once

#include "global.h"
#include "../dto/ConversationDto.hpp"
#include "../vo/ConversationVo.hpp"
#include "../postgresql/AppClient.hpp"

class ConversationService {
private:
    std::shared_ptr<AppClient> m_appClient;
    using Status = oatpp::web::protocol::http::Status;
public:
    ConversationService(const std::shared_ptr<AppClient>& appClient) : m_appClient(appClient) {}

    oatpp::Vector<oatpp::Object<ConversationVO>> getConversations(const oatpp::String& currentUserIdHeader) {
        OATPP_ASSERT_HTTP(currentUserIdHeader && !currentUserIdHeader->empty(), Status::CODE_400, "用户ID不能为空");
        auto result = m_appClient->getUserIdByUuid(currentUserIdHeader);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, result->getErrorMessage());
        OATPP_ASSERT_HTTP(result->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
        #else
        OATPP_ASSERT_HTTP(result->isSuccess() && result->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
        #endif
        auto id = result->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        result = m_appClient->getConversations(id);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, result->getErrorMessage());
        #else
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, "获取会话列表失败");
        #endif
        return result->fetch<oatpp::Vector<oatpp::Object<ConversationVO>>>();
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

    oatpp::Boolean markConversationRead(const oatpp::String& currentUserIdHeader, const oatpp::String& convUuid) {
        OATPP_ASSERT_HTTP(currentUserIdHeader && !currentUserIdHeader->empty(), Status::CODE_400, "用户ID不能为空");
        OATPP_ASSERT_HTTP(convUuid && !convUuid->empty(), Status::CODE_400, "会话ID不能为空");

        auto userCheck = m_appClient->getUserIdByUuid(currentUserIdHeader);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(userCheck->isSuccess(), Status::CODE_500, userCheck->getErrorMessage());
        OATPP_ASSERT_HTTP(userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
        #else
        OATPP_ASSERT_HTTP(userCheck->isSuccess() && userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
        #endif
        auto userId = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        // 先尝试查询是否为私聊会话（convUuid是对端用户的uuid）
        auto targetUserResult = m_appClient->getUserIdByUuid(convUuid);
        oatpp::Int64 targetId = -1;
        bool isUser = false;
        
        if (targetUserResult->isSuccess() && targetUserResult->hasMoreToFetch()) {
            targetId = targetUserResult->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;
            isUser = true;
        } else {
            // 尝试查询是否为群聊会话（convUuid是群聊的uuid）
            auto targetGroupResult = m_appClient->getGroupIdByUuid(convUuid);
            if (targetGroupResult->isSuccess() && targetGroupResult->hasMoreToFetch()) {
                targetId = targetGroupResult->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;
                isUser = false;
            } else {
                OATPP_ASSERT_HTTP(false, Status::CODE_404, "会话不存在");
            }
        }

        auto convIdResult = m_appClient->getConversationId(userId, targetId);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(convIdResult->isSuccess(), Status::CODE_500, convIdResult->getErrorMessage());
        OATPP_ASSERT_HTTP(convIdResult->hasMoreToFetch(), Status::CODE_404, "会话不存在");
        #else
        OATPP_ASSERT_HTTP(convIdResult->isSuccess() && convIdResult->hasMoreToFetch(), Status::CODE_404, "会话不存在");
        #endif
        auto conversationId = convIdResult->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        auto result = m_appClient->markConversationRead(conversationId, userId);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, result->getErrorMessage());
        #else
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, "标记已读失败");
        #endif
        return true;
    }

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
