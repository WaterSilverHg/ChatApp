#pragma once

#include "global.h"
#include "../dto/GroupDto.hpp"
#include "../vo/GroupVo.hpp"
#include "../postgresql/AppClient.hpp"

class GroupService {
private:
    std::shared_ptr<AppClient> m_appClient;
    using Status = oatpp::web::protocol::http::Status;
public:
    GroupService(const std::shared_ptr<AppClient>& appClient) : m_appClient(appClient) {}

    oatpp::Object<GroupInfoVO> createGroup(const oatpp::String& currentUserIdHeader, const oatpp::Object<CreateGroupRequestDTO>& request) {
        OATPP_ASSERT_HTTP(currentUserIdHeader && !currentUserIdHeader->empty(), Status::CODE_400, "用户ID不能为空");
        OATPP_ASSERT_HTTP(request, Status::CODE_400, "请求参数不能为空");
        OATPP_ASSERT_HTTP(request->name && !request->name->empty(), Status::CODE_400, "群组名称不能为空");
        OATPP_ASSERT_HTTP(request->memberUuids && !request->memberUuids->empty(), Status::CODE_400, "成员列表不能为空");
        auto userCheck = m_appClient->getUserIdByUuid(currentUserIdHeader);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(userCheck->isSuccess(), Status::CODE_500, userCheck->getErrorMessage());
        OATPP_ASSERT_HTTP(userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
        #else
        OATPP_ASSERT_HTTP(userCheck->isSuccess() && userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
        #endif
        auto userId = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        auto result = m_appClient->createGroup(request->name, request->description, request->avatarUrl, userId, request->maxMembers, request->isPublic);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, result->getErrorMessage());
        OATPP_ASSERT_HTTP(result->hasMoreToFetch(), Status::CODE_400, "创建群组失败");
        #else
        OATPP_ASSERT_HTTP(result->isSuccess() && result->hasMoreToFetch(), Status::CODE_400, "创建群组失败");
        #endif

        auto group = result->fetch<oatpp::Vector<oatpp::Object<GroupInfoVO>>>()[0];
        group->memberCount = request->memberUuids->size() + 1;
        auto transaction = m_appClient->beginTransaction();
        auto groupCheck = m_appClient->getGroupIdByUuid(group->uuid.getValue(""));
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(groupCheck->isSuccess(), Status::CODE_500, groupCheck->getErrorMessage());
        OATPP_ASSERT_HTTP(groupCheck->hasMoreToFetch(), Status::CODE_401, "群组不存在或已失效");
        #else
        OATPP_ASSERT_HTTP(groupCheck->isSuccess() && groupCheck->hasMoreToFetch(), Status::CODE_401, "群组不存在或已失效");
        #endif
        auto groupId = groupCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;
        auto ownerResult = m_appClient->addGroupMember(groupId, userId, "owner");
        if (!ownerResult->isSuccess()) {
            transaction.rollback();
            OATPP_ASSERT_HTTP(false, Status::CODE_400, "添加组长失败");
        }

        auto ownerConvResult = m_appClient->createGroupConversation(userId, groupId);
        if (!ownerConvResult->isSuccess()) {
            transaction.rollback();
        #ifdef SQLCHECK
            OATPP_ASSERT_HTTP(false, Status::CODE_500, ownerConvResult->getErrorMessage());
        #else
            OATPP_ASSERT_HTTP(false, Status::CODE_500, "组长添加会话失败");
        #endif
        }
        for (auto& memberUuid : *request->memberUuids) {
            auto memberCheck = m_appClient->getUserIdByUuid(memberUuid);
            #ifdef SQLCHECK
            OATPP_ASSERT_HTTP(memberCheck->isSuccess(), Status::CODE_500, memberCheck->getErrorMessage());
            OATPP_ASSERT_HTTP(memberCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
            #else
            OATPP_ASSERT_HTTP(memberCheck->isSuccess() && memberCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
            #endif
            auto memberId = memberCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;
            auto memberResult = m_appClient->addGroupMember(groupId, memberId, "member");
            if (!memberResult->isSuccess()) {
                transaction.rollback();
            #ifdef SQLCHECK
                OATPP_ASSERT_HTTP(false, Status::CODE_500, memberResult->getErrorMessage());
            #else
                OATPP_ASSERT_HTTP(false, Status::CODE_500, "添加组员失败");
            #endif
            }
            auto convResult = m_appClient->createGroupConversation(memberId, groupId);
            if (!convResult->isSuccess()) {
                transaction.rollback();
            #ifdef SQLCHECK
                OATPP_ASSERT_HTTP(false, Status::CODE_500, convResult->getErrorMessage());
            #else
                OATPP_ASSERT_HTTP(false, Status::CODE_500, "添加会话失败");
            #endif
            }
        }
        transaction.commit();
        return group;
    }

    oatpp::Vector<oatpp::Object<GroupInfoVO>> getMyGroups(const oatpp::String& currentUserIdHeader) {
        OATPP_ASSERT_HTTP(currentUserIdHeader && !currentUserIdHeader->empty(), Status::CODE_400, "用户ID不能为空");
        auto userCheck = m_appClient->getUserIdByUuid(currentUserIdHeader);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(userCheck->isSuccess(), Status::CODE_500, userCheck->getErrorMessage());
        OATPP_ASSERT_HTTP(userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
        #else
        OATPP_ASSERT_HTTP(userCheck->isSuccess() && userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
        #endif
        auto userId = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        auto result = m_appClient->getMyGroups(userId);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, result->getErrorMessage());
        #else
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, "获取群组列表失败");
        #endif

        auto groups = result->fetch<oatpp::Vector<oatpp::Object<GroupInfoVO>>>();
        if (!groups) {
            groups = oatpp::Vector<oatpp::Object<GroupInfoVO>>::createShared();
        }
        return groups;
    }

    oatpp::Object<GroupDetailInfoVO> getGroupDetail(const oatpp::String& currentUserIdHeader, const oatpp::String& groupUuid) {
        OATPP_ASSERT_HTTP(currentUserIdHeader && !currentUserIdHeader->empty(), Status::CODE_400, "用户ID不能为空");
        OATPP_ASSERT_HTTP(groupUuid && !groupUuid->empty(), Status::CODE_400, "群组ID不能为空");
        auto userCheck = m_appClient->getUserIdByUuid(currentUserIdHeader);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(userCheck->isSuccess(), Status::CODE_500, userCheck->getErrorMessage());
        OATPP_ASSERT_HTTP(userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
        #else
        OATPP_ASSERT_HTTP(userCheck->isSuccess() && userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
        #endif
        auto userId = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        auto groupCheck = m_appClient->getGroupIdByUuid(groupUuid);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(groupCheck->isSuccess(), Status::CODE_500, groupCheck->getErrorMessage());
        OATPP_ASSERT_HTTP(groupCheck->hasMoreToFetch(), Status::CODE_401, "群组不存在或已失效");
        #else
        OATPP_ASSERT_HTTP(groupCheck->isSuccess() && groupCheck->hasMoreToFetch(), Status::CODE_401, "群组不存在或已失效");
        #endif
        auto groupId = groupCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        auto memberCheck = m_appClient->checkUserInGroup(groupId, userId);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(memberCheck->isSuccess(), Status::CODE_500, memberCheck->getErrorMessage());
        OATPP_ASSERT_HTTP(memberCheck->hasMoreToFetch(), Status::CODE_403, "您不是该群组的成员，无权查看");
        #else
        OATPP_ASSERT_HTTP(memberCheck->isSuccess() && memberCheck->hasMoreToFetch(), Status::CODE_403, "您不是该群组的成员，无权查看");
        #endif

        auto result = m_appClient->getGroupDetail(groupId);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, result->getErrorMessage());
        OATPP_ASSERT_HTTP(result->hasMoreToFetch(), Status::CODE_404, "群组不存在");
        #else
        OATPP_ASSERT_HTTP(result->isSuccess() && result->hasMoreToFetch(), Status::CODE_404, "群组不存在");
        #endif

        return result->fetch<oatpp::Vector<oatpp::Object<GroupDetailInfoVO>>>()[0];
    }

    oatpp::Object<GroupDetailInfoVO> updateGroup(const oatpp::String& currentUserIdHeader, const oatpp::String& groupUuid, const oatpp::Object<UpdateGroupRequestDTO>& request) {
        OATPP_ASSERT_HTTP(currentUserIdHeader && !currentUserIdHeader->empty(), Status::CODE_400, "用户ID不能为空");
        OATPP_ASSERT_HTTP(groupUuid && !groupUuid->empty(), Status::CODE_400, "群组ID不能为空");
        OATPP_ASSERT_HTTP(request, Status::CODE_400, "请求参数不能为空");
        auto userCheck = m_appClient->getUserIdByUuid(currentUserIdHeader);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(userCheck->isSuccess(), Status::CODE_500, userCheck->getErrorMessage());
        OATPP_ASSERT_HTTP(userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
        #else
        OATPP_ASSERT_HTTP(userCheck->isSuccess() && userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
        #endif
        auto userId = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        auto groupCheck = m_appClient->getGroupIdByUuid(groupUuid);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(groupCheck->isSuccess(), Status::CODE_500, groupCheck->getErrorMessage());
        OATPP_ASSERT_HTTP(groupCheck->hasMoreToFetch(), Status::CODE_401, "群组不存在或已失效");
        #else
        OATPP_ASSERT_HTTP(groupCheck->isSuccess() && groupCheck->hasMoreToFetch(), Status::CODE_401, "群组不存在或已失效");
        #endif
        auto groupId = groupCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;
        //感觉可以优化成一次查询
        auto result = m_appClient->updateGroup(groupId, userId, request->name, request->description, request->avatarUrl, request->maxMembers, request->isPublic);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_403, result->getErrorMessage());
        #else
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_403, "无权限修改群组信息");
        #endif

        auto updatedGroup = m_appClient->getGroupDetail(groupId);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(updatedGroup->isSuccess(), Status::CODE_500, updatedGroup->getErrorMessage());
        OATPP_ASSERT_HTTP(updatedGroup->hasMoreToFetch(), Status::CODE_404, "群组不存在");
        #else
        OATPP_ASSERT_HTTP(updatedGroup->isSuccess() && updatedGroup->hasMoreToFetch(), Status::CODE_404, "群组不存在");
        #endif

        return updatedGroup->fetch<oatpp::Vector<oatpp::Object<GroupDetailInfoVO>>>()[0];
    }

    oatpp::Boolean dissolveGroup(const oatpp::String& currentUserIdHeader, const oatpp::String& groupUuid) {
        OATPP_ASSERT_HTTP(currentUserIdHeader && !currentUserIdHeader->empty(), Status::CODE_400, "用户ID不能为空");
        OATPP_ASSERT_HTTP(groupUuid && !groupUuid->empty(), Status::CODE_400, "群组ID不能为空");
        auto userCheck = m_appClient->getUserIdByUuid(currentUserIdHeader);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(userCheck->isSuccess(), Status::CODE_500, userCheck->getErrorMessage());
        OATPP_ASSERT_HTTP(userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
        #else
        OATPP_ASSERT_HTTP(userCheck->isSuccess() && userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
        #endif
        auto userId = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        auto groupCheck = m_appClient->getGroupIdByUuid(groupUuid);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(groupCheck->isSuccess(), Status::CODE_500, groupCheck->getErrorMessage());
        OATPP_ASSERT_HTTP(groupCheck->hasMoreToFetch(), Status::CODE_401, "群组不存在或已失效");
        #else
        OATPP_ASSERT_HTTP(groupCheck->isSuccess() && groupCheck->hasMoreToFetch(), Status::CODE_401, "群组不存在或已失效");
        #endif
        auto groupId = groupCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        auto dsresult = m_appClient->dissolveGroup(groupId, userId);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(dsresult->isSuccess(), Status::CODE_403, dsresult->getErrorMessage());
        #else
        OATPP_ASSERT_HTTP(dsresult->isSuccess(), Status::CODE_403, "无权限解散群组");
        #endif

        //auto convIdResult = m_appClient->getGroupConversationId(groupId, userId);
        //if (convIdResult->isSuccess() && convIdResult->hasMoreToFetch()) {
        //    auto convId = convIdResult->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;
        //    auto deleteConvResult = m_appClient->deleteConversation(convId);
        //    OATPP_ASSERT_HTTP(deleteConvResult->isSuccess(), Status::CODE_400, "删除会话失败");
        //}

        return true;
    }

    oatpp::Vector<oatpp::Object<GroupMemberVO>> getGroupMembers(const oatpp::String& groupUuid) {
        OATPP_ASSERT_HTTP(groupUuid && !groupUuid->empty(), Status::CODE_400, "群组ID不能为空");

        auto groupCheck = m_appClient->getGroupIdByUuid(groupUuid);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(groupCheck->isSuccess(), Status::CODE_500, groupCheck->getErrorMessage());
        OATPP_ASSERT_HTTP(groupCheck->hasMoreToFetch(), Status::CODE_401, "群组不存在或已失效");
        #else
        OATPP_ASSERT_HTTP(groupCheck->isSuccess() && groupCheck->hasMoreToFetch(), Status::CODE_401, "群组不存在或已失效");
        #endif
        auto groupId = groupCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        auto result = m_appClient->getGroupMembers(groupId);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_404, result->getErrorMessage());
        #else
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_404, "群组不存在");
        #endif

        return result->fetch<oatpp::Vector<oatpp::Object<GroupMemberVO>>>();
    }

    oatpp::Boolean addGroupMembers(const oatpp::String& currentUserIdHeader, const oatpp::String& groupUuid, const oatpp::Object<AddGroupMemberRequestDTO>& request) {
        OATPP_ASSERT_HTTP(currentUserIdHeader && !currentUserIdHeader->empty(), Status::CODE_400, "用户ID不能为空");
        OATPP_ASSERT_HTTP(groupUuid && !groupUuid->empty(), Status::CODE_400, "群组ID不能为空");
        OATPP_ASSERT_HTTP(request, Status::CODE_400, "请求参数不能为空");
        OATPP_ASSERT_HTTP(request->userIds && !request->userIds->empty(), Status::CODE_400, "用户ID列表不能为空");
        auto userCheck = m_appClient->getUserIdByUuid(currentUserIdHeader);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(userCheck->isSuccess(), Status::CODE_500, userCheck->getErrorMessage());
        OATPP_ASSERT_HTTP(userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
        #else
        OATPP_ASSERT_HTTP(userCheck->isSuccess() && userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
        #endif
        auto userId = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        auto groupCheck = m_appClient->getGroupIdByUuid(groupUuid);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(groupCheck->isSuccess(), Status::CODE_500, groupCheck->getErrorMessage());
        OATPP_ASSERT_HTTP(groupCheck->hasMoreToFetch(), Status::CODE_401, "群组不存在或已失效");
        #else
        OATPP_ASSERT_HTTP(groupCheck->isSuccess() && groupCheck->hasMoreToFetch(), Status::CODE_401, "群组不存在或已失效");
        #endif
        auto groupId = groupCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        for (const auto& memberId : *request->userIds) {
            auto result = m_appClient->addGroupMember(groupId, memberId, "member");
            #ifdef SQLCHECK
            OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, result->getErrorMessage());
            #else
            OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_400, "添加成员失败");
            #endif
        }
        return true;
    }

    oatpp::Boolean removeGroupMember(const oatpp::String& currentUserIdHeader, const oatpp::String& groupUuid, const oatpp::String& targetUserUuid) {
        OATPP_ASSERT_HTTP(currentUserIdHeader && !currentUserIdHeader->empty(), Status::CODE_400, "用户ID不能为空");
        OATPP_ASSERT_HTTP(groupUuid && !groupUuid->empty(), Status::CODE_400, "群组ID不能为空");
        OATPP_ASSERT_HTTP(targetUserUuid && !targetUserUuid->empty(), Status::CODE_400, "目标用户ID不能为空");
        auto userCheck = m_appClient->getUserIdByUuid(currentUserIdHeader);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(userCheck->isSuccess(), Status::CODE_500, userCheck->getErrorMessage());
        OATPP_ASSERT_HTTP(userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
        #else
        OATPP_ASSERT_HTTP(userCheck->isSuccess() && userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
        #endif
        auto currentUserId = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        auto groupCheck = m_appClient->getGroupIdByUuid(groupUuid);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(groupCheck->isSuccess(), Status::CODE_500, groupCheck->getErrorMessage());
        OATPP_ASSERT_HTTP(groupCheck->hasMoreToFetch(), Status::CODE_401, "群组不存在或已失效");
        #else
        OATPP_ASSERT_HTTP(groupCheck->isSuccess() && groupCheck->hasMoreToFetch(), Status::CODE_401, "群组不存在或已失效");
        #endif
        auto groupId = groupCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        auto targetUserResult = m_appClient->getUserIdByUuid(targetUserUuid);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(targetUserResult->isSuccess(), Status::CODE_500, targetUserResult->getErrorMessage());
        OATPP_ASSERT_HTTP(targetUserResult->hasMoreToFetch(), Status::CODE_404, "目标用户不存在或已失效");
        #else
        OATPP_ASSERT_HTTP(targetUserResult->isSuccess() && targetUserResult->hasMoreToFetch(), Status::CODE_404, "目标用户不存在或已失效");
        #endif
        auto targetUserId = targetUserResult->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        OATPP_ASSERT_HTTP(currentUserId != targetUserId, Status::CODE_400, "不能通过此接口移除自己，请使用退出群聊接口");

        auto currentRoleResult = m_appClient->getUserRoleInGroup(groupId, currentUserId);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(currentRoleResult->isSuccess(), Status::CODE_500, currentRoleResult->getErrorMessage());
        OATPP_ASSERT_HTTP(currentRoleResult->hasMoreToFetch(), Status::CODE_403, "您不是该群组的成员，无权操作");
        #else
        OATPP_ASSERT_HTTP(currentRoleResult->isSuccess() && currentRoleResult->hasMoreToFetch(), Status::CODE_403, "您不是该群组的成员，无权操作");
        #endif
        auto currentRole = currentRoleResult->fetch<oatpp::Vector<oatpp::Object<UserRoleDTO>>>()[0]->role;

        auto targetRoleResult = m_appClient->getUserRoleInGroup(groupId, targetUserId);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(targetRoleResult->isSuccess(), Status::CODE_500, targetRoleResult->getErrorMessage());
        OATPP_ASSERT_HTTP(targetRoleResult->hasMoreToFetch(), Status::CODE_404, "目标用户不在该群组中");
        #else
        OATPP_ASSERT_HTTP(targetRoleResult->isSuccess() && targetRoleResult->hasMoreToFetch(), Status::CODE_404, "目标用户不在该群组中");
        #endif
        auto targetRole = targetRoleResult->fetch<oatpp::Vector<oatpp::Object<UserRoleDTO>>>()[0]->role;

        bool hasPermission = false;
        if (currentRole == "owner") {
            hasPermission = true;
        } else if (currentRole == "admin" && targetRole == "member") {
            hasPermission = true;
        }

        OATPP_ASSERT_HTTP(hasPermission, Status::CODE_403, "您没有权限移除该成员");

        auto result = m_appClient->removeGroupMember(groupId, targetUserId);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, result->getErrorMessage());
        #else
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, "移除成员失败");
        #endif

        auto deleteConvResult = m_appClient->deleteConversationForGroupMember(groupId, targetUserId);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(deleteConvResult->isSuccess(), Status::CODE_500, deleteConvResult->getErrorMessage());
        #else
        OATPP_ASSERT_HTTP(deleteConvResult->isSuccess(), Status::CODE_500, "删除会话失败");
        #endif
        return true;
    }

    oatpp::Boolean setMemberRole(const oatpp::String& currentUserIdHeader, const oatpp::String& groupUuid, const oatpp::String& targetUserUuid, const oatpp::Object<SetMemberRoleRequestDTO>& request) {
        OATPP_ASSERT_HTTP(currentUserIdHeader && !currentUserIdHeader->empty(), Status::CODE_400, "用户ID不能为空");
        OATPP_ASSERT_HTTP(groupUuid && !groupUuid->empty(), Status::CODE_400, "群组ID不能为空");
        OATPP_ASSERT_HTTP(targetUserUuid && !targetUserUuid->empty(), Status::CODE_400, "目标用户ID不能为空");
        OATPP_ASSERT_HTTP(request, Status::CODE_400, "请求参数不能为空");
        OATPP_ASSERT_HTTP(request->role && !request->role->empty(), Status::CODE_400, "角色参数不能为空");
        OATPP_ASSERT_HTTP(request->role == "admin" || request->role == "member", Status::CODE_400, "角色只能是 'admin' 或 'member'");

        auto currentUserResult = m_appClient->getUserIdByUuid(currentUserIdHeader);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(currentUserResult->isSuccess(), Status::CODE_500, currentUserResult->getErrorMessage());
        OATPP_ASSERT_HTTP(currentUserResult->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
        #else
        OATPP_ASSERT_HTTP(currentUserResult->isSuccess() && currentUserResult->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
        #endif
        auto currentUserId = currentUserResult->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        auto groupIdResult = m_appClient->getGroupIdByUuid(groupUuid);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(groupIdResult->isSuccess(), Status::CODE_500, groupIdResult->getErrorMessage());
        OATPP_ASSERT_HTTP(groupIdResult->hasMoreToFetch(), Status::CODE_404, "群组不存在或已失效");
        #else
        OATPP_ASSERT_HTTP(groupIdResult->isSuccess() && groupIdResult->hasMoreToFetch(), Status::CODE_404, "群组不存在或已失效");
        #endif
        auto groupId = groupIdResult->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        auto targetUserResult = m_appClient->getUserIdByUuid(targetUserUuid);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(targetUserResult->isSuccess(), Status::CODE_500, targetUserResult->getErrorMessage());
        OATPP_ASSERT_HTTP(targetUserResult->hasMoreToFetch(), Status::CODE_404, "目标用户不存在或已失效");
        #else
        OATPP_ASSERT_HTTP(targetUserResult->isSuccess() && targetUserResult->hasMoreToFetch(), Status::CODE_404, "目标用户不存在或已失效");
        #endif
        auto targetUserId = targetUserResult->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        OATPP_ASSERT_HTTP(currentUserId != targetUserId, Status::CODE_400, "不能修改自己的角色");

        auto currentRoleResult = m_appClient->getUserRoleInGroup(groupId, currentUserId);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(currentRoleResult->isSuccess(), Status::CODE_500, currentRoleResult->getErrorMessage());
        OATPP_ASSERT_HTTP(currentRoleResult->hasMoreToFetch(), Status::CODE_403, "您不是该群组的成员，无权操作");
        #else
        OATPP_ASSERT_HTTP(currentRoleResult->isSuccess(), Status::CODE_500, currentRoleResult->getErrorMessage());
        #endif
        auto currentRole = currentRoleResult->fetch<oatpp::Vector<oatpp::Object<UserRoleDTO>>>()[0]->role;

        OATPP_ASSERT_HTTP(currentRole == "owner", Status::CODE_403, "只有群主可以设置管理员");

        auto targetRoleResult = m_appClient->getUserRoleInGroup(groupId, targetUserId);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(targetRoleResult->isSuccess(), Status::CODE_500, targetRoleResult->getErrorMessage());
        OATPP_ASSERT_HTTP(targetRoleResult->hasMoreToFetch(), Status::CODE_404, "目标用户不在该群组中");
        #else
        OATPP_ASSERT_HTTP(targetRoleResult->isSuccess(), Status::CODE_500, targetRoleResult->getErrorMessage());
        #endif
        auto targetRole = targetRoleResult->fetch<oatpp::Vector<oatpp::Object<UserRoleDTO>>>()[0]->role;

        OATPP_ASSERT_HTTP(!(request->role == "admin" && targetRole == "owner"), Status::CODE_403, "不能将群主设置为管理员");
        OATPP_ASSERT_HTTP(request->role != targetRole, Status::CODE_400, targetRole == "admin" ? "用户已经是管理员" : "用户已经是普通成员");

        auto result = m_appClient->setMemberRole(groupId, targetUserId, request->role);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, result->getErrorMessage());
        #else
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, "设置角色失败");
        #endif

        return true;
    }

    oatpp::Boolean joinGroup(const oatpp::String& currentUserIdHeader, const oatpp::String& groupUuid) {
        OATPP_ASSERT_HTTP(currentUserIdHeader && !currentUserIdHeader->empty(), Status::CODE_400, "用户ID不能为空");
        OATPP_ASSERT_HTTP(groupUuid && !groupUuid->empty(), Status::CODE_400, "群组ID不能为空");
        auto currentUserResult = m_appClient->getUserIdByUuid(currentUserIdHeader);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(currentUserResult->isSuccess(), Status::CODE_500, currentUserResult->getErrorMessage());
        OATPP_ASSERT_HTTP(currentUserResult->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
        #else
        OATPP_ASSERT_HTTP(currentUserResult->isSuccess() && currentUserResult->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
        #endif
        auto currentUserId = currentUserResult->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        auto groupIdResult = m_appClient->getGroupIdByUuid(groupUuid);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(groupIdResult->isSuccess(), Status::CODE_500, groupIdResult->getErrorMessage());
        OATPP_ASSERT_HTTP(groupIdResult->hasMoreToFetch(), Status::CODE_404, "群组不存在或已失效");
        #else
        OATPP_ASSERT_HTTP(groupIdResult->isSuccess() && groupIdResult->hasMoreToFetch(), Status::CODE_404, "群组不存在或已失效");
        #endif
        auto groupId = groupIdResult->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        auto result = m_appClient->addGroupMember(groupId, currentUserId, "member");
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, result->getErrorMessage());
        #else
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_400, "加入群组失败");
        #endif

        return true;
    }

    oatpp::Boolean leaveGroup(const oatpp::String& currentUserIdHeader, const oatpp::String& groupUuid) {
        OATPP_ASSERT_HTTP(currentUserIdHeader && !currentUserIdHeader->empty(), Status::CODE_400, "用户ID不能为空");
        OATPP_ASSERT_HTTP(groupUuid && !groupUuid->empty(), Status::CODE_400, "群组ID不能为空");
        auto currentUserResult = m_appClient->getUserIdByUuid(currentUserIdHeader);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(currentUserResult->isSuccess(), Status::CODE_500, currentUserResult->getErrorMessage());
        OATPP_ASSERT_HTTP(currentUserResult->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
        #else
        OATPP_ASSERT_HTTP(currentUserResult->isSuccess() && currentUserResult->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
        #endif
        auto currentUserId = currentUserResult->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        auto groupIdResult = m_appClient->getGroupIdByUuid(groupUuid);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(groupIdResult->isSuccess(), Status::CODE_500, groupIdResult->getErrorMessage());
        OATPP_ASSERT_HTTP(groupIdResult->hasMoreToFetch(), Status::CODE_404, "群组不存在或已失效");
        #else
        OATPP_ASSERT_HTTP(groupIdResult->isSuccess() && groupIdResult->hasMoreToFetch(), Status::CODE_404, "群组不存在或已失效");
        #endif
        auto groupId = groupIdResult->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;



        // 获取群成员列表
        auto membersResult = m_appClient->getGroupMembers(groupId);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(membersResult->isSuccess(), Status::CODE_500, membersResult->getErrorMessage());
        #else
        OATPP_ASSERT_HTTP(membersResult->isSuccess(), Status::CODE_500, "获取群成员列表失败");
        #endif
        auto members = membersResult->fetch<oatpp::Vector<oatpp::Object<GroupMemberVO>>>();

        // 计算剩余成员数量（减1是因为当前用户要离开）
        int remainingMembers = members->size() - 1;

        // 如果只剩最后一个成员，直接解散群聊
        if (remainingMembers == 1) {
            auto dissolveResult = m_appClient->dissolveGroup(groupId, currentUserId);
            #ifdef SQLCHECK
            OATPP_ASSERT_HTTP(dissolveResult->isSuccess(), Status::CODE_403, dissolveResult->getErrorMessage());
            #else
            OATPP_ASSERT_HTTP(dissolveResult->isSuccess(), Status::CODE_403, "无权限解散群组");
            #endif
            return true;
        }
        // 检查当前用户是否是群主
        auto currentRoleResult = m_appClient->getUserRoleInGroup(groupId, currentUserId);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(currentRoleResult->isSuccess(), Status::CODE_500, currentRoleResult->getErrorMessage());
        OATPP_ASSERT_HTTP(currentRoleResult->hasMoreToFetch(), Status::CODE_403, "您不是该群组的成员，无权操作");
        #else
        OATPP_ASSERT_HTTP(currentRoleResult->isSuccess() && currentRoleResult->hasMoreToFetch(), Status::CODE_403, "您不是该群组的成员，无权操作");
        #endif
        auto currentRole = currentRoleResult->fetch<oatpp::Vector<oatpp::Object<UserRoleDTO>>>()[0]->role;

        // 如果是群主，需要选择新群主
        if (currentRole == "owner") {


            // 优先选择管理员作为新群主
            oatpp::Int64 newOwnerId = -1;
            for (const auto& member : *members) {
                if (member->role == "admin" && member->userUuid != currentUserIdHeader) {
                    auto memberIdResult = m_appClient->getUserIdByUuid(member->userUuid);
                    if (memberIdResult->isSuccess() && memberIdResult->hasMoreToFetch()) {
                        newOwnerId = memberIdResult->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;
                        break;
                    }
                }
            }

            // 如果没有管理员，选择普通成员作为新群主
            if (newOwnerId == -1) {
                for (const auto& member : *members) {
                    if (member->role == "member" && member->userUuid != currentUserIdHeader) {
                        auto memberIdResult = m_appClient->getUserIdByUuid(member->userUuid);
                        if (memberIdResult->isSuccess() && memberIdResult->hasMoreToFetch()) {
                            newOwnerId = memberIdResult->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;
                            break;
                        }
                    }
                }
            }

            // 如果找到新群主，更新其角色为owner
            if (newOwnerId != -1) {
                auto updateResult = m_appClient->setMemberRole(groupId, newOwnerId, "owner");
                #ifdef SQLCHECK
                OATPP_ASSERT_HTTP(updateResult->isSuccess(), Status::CODE_500, updateResult->getErrorMessage());
                #else
                OATPP_ASSERT_HTTP(updateResult->isSuccess(), Status::CODE_500, "设置新群主失败");
                #endif
            }
        }

        // 执行退出群组操作
        auto result = m_appClient->leaveGroup(groupId, currentUserId);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, result->getErrorMessage());
        #else
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_400, "退出群组失败");
        #endif

        return true;
    }

    oatpp::Vector<oatpp::Object<GroupInfoVO>> searchGroups(const oatpp::String& keyword, const oatpp::String& currentUserUuid) {
        OATPP_ASSERT_HTTP(keyword && !keyword->empty(), Status::CODE_400, "搜索关键词不能为空");
        OATPP_ASSERT_HTTP(currentUserUuid && !currentUserUuid->empty(), Status::CODE_400, "用户ID不能为空");


        auto userCheck = m_appClient->getUserIdByUuid(currentUserUuid);
#ifdef SQLCHECK
        OATPP_ASSERT_HTTP(userCheck->isSuccess(), Status::CODE_500, userCheck->getErrorMessage());
        OATPP_ASSERT_HTTP(userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
#else
        OATPP_ASSERT_HTTP(userCheck->isSuccess() && userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
#endif
        auto userId = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        
        auto searchKeyword = "%" + keyword + "%";
        auto result = m_appClient->searchGroups(searchKeyword,userId);
#ifdef SQLCHECK
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, result->getErrorMessage());
#else
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, "搜索群组失败");
#endif
  
        auto groups = result->fetch<oatpp::Vector<oatpp::Object<GroupInfoVO>>>();
        
        std::sort(groups->begin(), groups->end(), [](const oatpp::Object<GroupInfoVO>& a, const oatpp::Object<GroupInfoVO>& b) {
            if (a->memberCount != b->memberCount) {
                return a->memberCount > b->memberCount;
            }
            return false;
        });
        
        return groups;
    }
};
