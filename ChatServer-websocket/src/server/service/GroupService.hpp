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
        ASYNC_THROW_IF(currentUserIdHeader && !currentUserIdHeader->empty(), "User ID cannot be empty");
        ASYNC_THROW_IF(request, "Request parameters cannot be empty");
        ASYNC_THROW_IF(request->name && !request->name->empty(), "Group name cannot be empty");
        ASYNC_THROW_IF(request->memberUuids && !request->memberUuids->empty(), "Member list cannot be empty");
        auto userCheck = m_appClient->getUserIdByUuid(currentUserIdHeader);
        #ifdef SQLCHECK
        ASYNC_THROW_IF(userCheck->isSuccess(), userCheck->getErrorMessage());
        ASYNC_THROW_IF(userCheck->hasMoreToFetch(), "User does not exist or has been deactivated");
        #else
        ASYNC_THROW_IF(userCheck->isSuccess() && userCheck->hasMoreToFetch(), "User does not exist or has been deactivated");
        #endif
        auto userId = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        // 预先检查所有成员是否存在，避免在事务中失败
        std::vector<oatpp::Int64> memberIds;
        for (auto& memberUuid : *request->memberUuids) {
            auto memberCheck = m_appClient->getUserIdByUuid(memberUuid);
            #ifdef SQLCHECK
            ASYNC_THROW_IF(memberCheck->isSuccess(), memberCheck->getErrorMessage());
            ASYNC_THROW_IF(memberCheck->hasMoreToFetch(), "User does not exist or has been deactivated");
            #else
            ASYNC_THROW_IF(memberCheck->isSuccess() && memberCheck->hasMoreToFetch(), "User does not exist or has been deactivated");
            #endif
            memberIds.push_back(memberCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id);
        }

        // 在事务中执行所有操作
        auto transaction = m_appClient->beginTransaction();

        auto result = m_appClient->createGroup(request->name, request->description, request->avatarUrl, userId, request->maxMembers, request->isPublic);
        #ifdef SQLCHECK
        if (result->isSuccess()) {
            transaction.rollback();
            throw std::runtime_error(result->getErrorMessage());
        }
        if (!result->hasMoreToFetch()) {
            transaction.rollback();
             throw std::runtime_error("Failed to create group");
        }
        #else
        if (result->isSuccess() || !result->hasMoreToFetch()) {
            transaction.rollback();
            throw std::runtime_error("Failed to create group");
        }
        #endif

        auto group = result->fetch<oatpp::Vector<oatpp::Object<GroupInfoVO>>>()[0];
        group->memberCount = request->memberUuids->size() + 1;

        auto groupCheck = m_appClient->getGroupIdByUuid(group->uuid.getValue(""));
        #ifdef SQLCHECK
        if (groupCheck->isSuccess()) {
            transaction.rollback();
            throw std::runtime_error(groupCheck->getErrorMessage());
        }
        if (!groupCheck->hasMoreToFetch()) {
            transaction.rollback();
            throw std::runtime_error("Group does not exist or has been deactivated");
        }
        #else
        if (groupCheck->isSuccess() || !groupCheck->hasMoreToFetch()) {
            transaction.rollback();
            throw std::runtime_error("Group does not exist or has been deactivated");
        }
        #endif
        auto groupId = groupCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        auto ownerResult = m_appClient->addGroupMember(groupId, userId, "owner");
        if (!ownerResult->isSuccess()) {
            transaction.rollback();
            #ifdef SQLCHECK
            throw std::runtime_error(ownerResult->getErrorMessage());
            #else
            throw std::runtime_error("Failed to add group owner");
            #endif
        }

        auto ownerConvResult = m_appClient->createGroupConversation(userId, groupId);
        if (!ownerConvResult->isSuccess()) {
            transaction.rollback();
        #ifdef SQLCHECK
            throw std::runtime_error(ownerConvResult->getErrorMessage());
        #else
            throw std::runtime_error("Failed to create owner conversation");
        #endif
        }

        for (size_t i = 0; i < memberIds.size(); ++i) {
            auto memberId = memberIds[i];
            auto memberResult = m_appClient->addGroupMember(groupId, memberId, "member");
            if (!memberResult->isSuccess()) {
                transaction.rollback();
            #ifdef SQLCHECK
                throw std::runtime_error(memberResult->getErrorMessage());
            #else
                throw std::runtime_error("Failed to add member");
            #endif
            }
            auto convResult = m_appClient->createGroupConversation(memberId, groupId);
            if (!convResult->isSuccess()) {
                transaction.rollback();
            #ifdef SQLCHECK
                throw std::runtime_error(convResult->getErrorMessage());
            #else
                throw std::runtime_error("Failed to create conversation");
            #endif
            }
        }
        transaction.commit();
        return group;
    }

    oatpp::Vector<oatpp::Object<GroupInfoVO>> getMyGroups(const oatpp::String& currentUserIdHeader) {
        ASYNC_THROW_IF(currentUserIdHeader && !currentUserIdHeader->empty(), "User ID cannot be empty");
        auto userCheck = m_appClient->getUserIdByUuid(currentUserIdHeader);
        #ifdef SQLCHECK
        ASYNC_THROW_IF(userCheck->isSuccess(), userCheck->getErrorMessage());
        ASYNC_THROW_IF(userCheck->hasMoreToFetch(), "User does not exist or has been deactivated");
        #else
        ASYNC_THROW_IF(userCheck->isSuccess() && userCheck->hasMoreToFetch(), "User does not exist or has been deactivated");
        #endif
        auto userId = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        auto result = m_appClient->getMyGroups(userId);
        #ifdef SQLCHECK
        ASYNC_THROW_IF(result->isSuccess(), result->getErrorMessage());
        #else
        ASYNC_THROW_IF(result->isSuccess(), "Failed to get group list");
        #endif

        auto groups = result->fetch<oatpp::Vector<oatpp::Object<GroupInfoVO>>>();
        if (!groups) {
            groups = oatpp::Vector<oatpp::Object<GroupInfoVO>>::createShared();
        }
        return groups;
    }

    oatpp::Object<GroupDetailInfoVO> getGroupDetail(const oatpp::String& currentUserIdHeader, const oatpp::String& groupUuid) {
        ASYNC_THROW_IF(currentUserIdHeader && !currentUserIdHeader->empty(), "User ID cannot be empty");
        ASYNC_THROW_IF(groupUuid && !groupUuid->empty(), "Group ID cannot be empty");
        auto userCheck = m_appClient->getUserIdByUuid(currentUserIdHeader);
        #ifdef SQLCHECK
        ASYNC_THROW_IF(userCheck->isSuccess(), userCheck->getErrorMessage());
        ASYNC_THROW_IF(userCheck->hasMoreToFetch(), "User does not exist or has been deactivated");
        #else
        ASYNC_THROW_IF(userCheck->isSuccess() && userCheck->hasMoreToFetch(), "User does not exist or has been deactivated");
        #endif
        auto userId = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        auto groupCheck = m_appClient->getGroupIdByUuid(groupUuid);
        #ifdef SQLCHECK
        ASYNC_THROW_IF(groupCheck->isSuccess(), groupCheck->getErrorMessage());
        ASYNC_THROW_IF(groupCheck->hasMoreToFetch(), "Group does not exist or has been deactivated");
        #else
        ASYNC_THROW_IF(groupCheck->isSuccess() && groupCheck->hasMoreToFetch(), "Group does not exist or has been deactivated");
        #endif
        auto groupId = groupCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        auto memberCheck = m_appClient->checkUserInGroup(groupId, userId);
        #ifdef SQLCHECK
        ASYNC_THROW_IF(memberCheck->isSuccess(), memberCheck->getErrorMessage());
        ASYNC_THROW_IF(memberCheck->hasMoreToFetch(), "You are not a member of this group and do not have permission to view");
        #else
        ASYNC_THROW_IF(memberCheck->isSuccess() && memberCheck->hasMoreToFetch(), "You are not a member of this group and do not have permission to view");
        #endif

        auto result = m_appClient->getGroupDetail(groupId);
        #ifdef SQLCHECK
        ASYNC_THROW_IF(result->isSuccess(), result->getErrorMessage());
        ASYNC_THROW_IF(result->hasMoreToFetch(), "Group does not exist");
        #else
        ASYNC_THROW_IF(result->isSuccess() && result->hasMoreToFetch(), "Group does not exist");
        #endif

        return result->fetch<oatpp::Vector<oatpp::Object<GroupDetailInfoVO>>>()[0];
    }

    oatpp::Object<GroupDetailInfoVO> updateGroup(const oatpp::String& currentUserIdHeader, const oatpp::Object<UpdateGroupRequestDTO>& request) {
        ASYNC_THROW_IF(currentUserIdHeader && !currentUserIdHeader->empty(), "User ID cannot be empty");
        ASYNC_THROW_IF(request, "Request parameters cannot be empty");
        ASYNC_THROW_IF(request->groupUuid && !request->groupUuid->empty(), "Group ID cannot be empty");
        auto userCheck = m_appClient->getUserIdByUuid(currentUserIdHeader);
        #ifdef SQLCHECK
        ASYNC_THROW_IF(userCheck->isSuccess(), userCheck->getErrorMessage());
        ASYNC_THROW_IF(userCheck->hasMoreToFetch(), "User does not exist or has been deactivated");
        #else
        ASYNC_THROW_IF(userCheck->isSuccess() && userCheck->hasMoreToFetch(), "User does not exist or has been deactivated");
        #endif
        auto userId = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        auto groupCheck = m_appClient->getGroupIdByUuid(request->groupUuid);
        #ifdef SQLCHECK
        ASYNC_THROW_IF(groupCheck->isSuccess(), groupCheck->getErrorMessage());
        ASYNC_THROW_IF(groupCheck->hasMoreToFetch(), "Group does not exist or has been deactivated");
        #else
        ASYNC_THROW_IF(groupCheck->isSuccess() && groupCheck->hasMoreToFetch(), "Group does not exist or has been deactivated");
        #endif
        auto groupId = groupCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;
        //感觉可以优化成一次查询
        auto result = m_appClient->updateGroup(groupId, userId, request->name, request->description, request->avatarUrl, request->maxMembers, request->isPublic);
        #ifdef SQLCHECK
        ASYNC_THROW_IF(result->isSuccess(), result->getErrorMessage());
        #else
        ASYNC_THROW_IF(result->isSuccess(), "No permission to modify group information");
        #endif

        auto updatedGroup = m_appClient->getGroupDetail(groupId);
        #ifdef SQLCHECK
        ASYNC_THROW_IF(updatedGroup->isSuccess(), updatedGroup->getErrorMessage());
        ASYNC_THROW_IF(updatedGroup->hasMoreToFetch(), "Group does not exist");
        #else
        ASYNC_THROW_IF(updatedGroup->isSuccess() && updatedGroup->hasMoreToFetch(), "Group does not exist");
        #endif

        return updatedGroup->fetch<oatpp::Vector<oatpp::Object<GroupDetailInfoVO>>>()[0];
    }

    oatpp::Boolean dissolveGroup(const oatpp::String& currentUserIdHeader, const oatpp::String& groupUuid) {
        ASYNC_THROW_IF(currentUserIdHeader && !currentUserIdHeader->empty(), "User ID cannot be empty");
        ASYNC_THROW_IF(groupUuid && !groupUuid->empty(), "Group ID cannot be empty");
        auto userCheck = m_appClient->getUserIdByUuid(currentUserIdHeader);
        #ifdef SQLCHECK
        ASYNC_THROW_IF(userCheck->isSuccess(), userCheck->getErrorMessage());
        ASYNC_THROW_IF(userCheck->hasMoreToFetch(), "User does not exist or has been deactivated");
        #else
        ASYNC_THROW_IF(userCheck->isSuccess() && userCheck->hasMoreToFetch(), "User does not exist or has been deactivated");
        #endif
        auto userId = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        auto groupCheck = m_appClient->getGroupIdByUuid(groupUuid);
        #ifdef SQLCHECK
        ASYNC_THROW_IF(groupCheck->isSuccess(), groupCheck->getErrorMessage());
        ASYNC_THROW_IF(groupCheck->hasMoreToFetch(), "Group does not exist or has been deactivated");
        #else
        ASYNC_THROW_IF(groupCheck->isSuccess() && groupCheck->hasMoreToFetch(), "Group does not exist or has been deactivated");
        #endif
        auto groupId = groupCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        auto dsresult = m_appClient->dissolveGroup(groupId, userId);
        #ifdef SQLCHECK
        ASYNC_THROW_IF(dsresult->isSuccess(), dsresult->getErrorMessage());
        #else
        ASYNC_THROW_IF(dsresult->isSuccess(), "No permission to dissolve the group");
        #endif

        //auto convIdResult = m_appClient->getGroupConversationId(groupId, userId);
        //if (convIdResult->isSuccess() && convIdResult->hasMoreToFetch()) {
        //    auto convId = convIdResult->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;
        //    auto deleteConvResult = m_appClient->deleteConversation(convId);
        //    ASYNC_THROW_IF(!deleteConvResult->isSuccess(), "删除会话失败");
        //}

        return true;
    }

    oatpp::Vector<oatpp::Object<GroupMemberVO>> getGroupMembers(const oatpp::String& groupUuid) {
        ASYNC_THROW_IF(groupUuid && !groupUuid->empty(), "Group ID cannot be empty");

        auto groupCheck = m_appClient->getGroupIdByUuid(groupUuid);
        #ifdef SQLCHECK
        ASYNC_THROW_IF(groupCheck->isSuccess(), groupCheck->getErrorMessage());
        ASYNC_THROW_IF(groupCheck->hasMoreToFetch(), "Group does not exist or has been deactivated");
        #else
        ASYNC_THROW_IF(groupCheck->isSuccess() && groupCheck->hasMoreToFetch(), "Group does not exist or has been deactivated");
        #endif
        auto groupId = groupCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        auto result = m_appClient->getGroupMembers(groupId);
        #ifdef SQLCHECK
        ASYNC_THROW_IF(result->isSuccess(), result->getErrorMessage());
        #else
        ASYNC_THROW_IF(result->isSuccess(), "Group does not exist");
        #endif

        return result->fetch<oatpp::Vector<oatpp::Object<GroupMemberVO>>>();
    }

    oatpp::Boolean addGroupMembers(const oatpp::String& currentUserIdHeader, const oatpp::String& groupUuid, const oatpp::Object<AddGroupMemberRequestDTO>& request) {
        ASYNC_THROW_IF(currentUserIdHeader && !currentUserIdHeader->empty(), "User ID cannot be empty");
        ASYNC_THROW_IF(groupUuid && !groupUuid->empty(), "Group ID cannot be empty");
        ASYNC_THROW_IF(request, "Request parameters cannot be empty");
        ASYNC_THROW_IF(request->userUuids && !request->userUuids->empty(), "User ID list cannot be empty");
        auto userCheck = m_appClient->getUserIdByUuid(currentUserIdHeader);
        #ifdef SQLCHECK
        ASYNC_THROW_IF(userCheck->isSuccess(), userCheck->getErrorMessage());
        ASYNC_THROW_IF(userCheck->hasMoreToFetch(), "User does not exist or has been deactivated");
        #else
        ASYNC_THROW_IF(userCheck->isSuccess() && userCheck->hasMoreToFetch(), "User does not exist or has been deactivated");
        #endif
        auto userId = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        auto groupCheck = m_appClient->getGroupIdByUuid(groupUuid);
        #ifdef SQLCHECK
        ASYNC_THROW_IF(groupCheck->isSuccess(), groupCheck->getErrorMessage());
        ASYNC_THROW_IF(groupCheck->hasMoreToFetch(), "Group does not exist or has been deactivated");
        #else
        ASYNC_THROW_IF(groupCheck->isSuccess() && groupCheck->hasMoreToFetch(), "Group does not exist or has been deactivated");
        #endif
        auto groupId = groupCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        // 在事务中执行所有操作
        auto transaction = m_appClient->beginTransaction();

        for (const auto& memberUuid : *request->userUuids) {
            auto memberCheck = m_appClient->getUserIdByUuid(memberUuid);
#ifdef SQLCHECK
            if (memberCheck->isSuccess()) {
                transaction.rollback();
                throw std::runtime_error(memberCheck->getErrorMessage());
            }
            if (!memberCheck->hasMoreToFetch()) {
                transaction.rollback();
                throw std::runtime_error("User does not exist or has been deactivated");
            }
#else
            if (memberCheck->isSuccess() || !memberCheck->hasMoreToFetch()) {
                transaction.rollback();
                throw std::runtime_error("User does not exist or has been deactivated");
            }
#endif
            auto memberId = memberCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;
            auto result = m_appClient->addGroupMember(groupId, memberId, "member");
            #ifdef SQLCHECK
            if (result->isSuccess()) {
                transaction.rollback();
                throw std::runtime_error(result->getErrorMessage());
            }
            #else
            if (result->isSuccess()) {
                transaction.rollback();
                throw std::runtime_error("Failed to add member");
            }
            #endif
            auto convResult = m_appClient->createGroupConversation(memberId, groupId);
            if (!convResult->isSuccess()) {
                transaction.rollback();
                #ifdef SQLCHECK
                throw std::runtime_error(convResult->getErrorMessage());
                #else
                throw std::runtime_error("Failed to create conversation for member");
                #endif
            }
        }

        transaction.commit();
        return true;
    }

    oatpp::Boolean removeGroupMember(const oatpp::String& currentUserIdHeader, const oatpp::String& groupUuid, const oatpp::String& targetUserUuid) {
        ASYNC_THROW_IF(currentUserIdHeader && !currentUserIdHeader->empty(), "User ID cannot be empty");
        ASYNC_THROW_IF(groupUuid && !groupUuid->empty(), "Group ID cannot be empty");
        ASYNC_THROW_IF(targetUserUuid && !targetUserUuid->empty(), "Target user ID cannot be empty");

        auto userCheck = m_appClient->getUserIdByUuid(currentUserIdHeader);
        #ifdef SQLCHECK
        ASYNC_THROW_IF(userCheck->isSuccess(), userCheck->getErrorMessage());
        ASYNC_THROW_IF(userCheck->hasMoreToFetch(), "User does not exist or has been deactivated");
        #else
        ASYNC_THROW_IF(userCheck->isSuccess() && userCheck->hasMoreToFetch(), "User does not exist or has been deactivated");
        #endif
        auto currentUserId = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        auto groupCheck = m_appClient->getGroupIdByUuid(groupUuid);
        #ifdef SQLCHECK
        ASYNC_THROW_IF(groupCheck->isSuccess(), groupCheck->getErrorMessage());
        ASYNC_THROW_IF(groupCheck->hasMoreToFetch(), "Group does not exist or has been deactivated");
        #else
        ASYNC_THROW_IF(groupCheck->isSuccess() && groupCheck->hasMoreToFetch(), "Group does not exist or has been deactivated");
        #endif
        auto groupId = groupCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        auto targetUserResult = m_appClient->getUserIdByUuid(targetUserUuid);
        #ifdef SQLCHECK
        ASYNC_THROW_IF(targetUserResult->isSuccess(), targetUserResult->getErrorMessage());
        ASYNC_THROW_IF(targetUserResult->hasMoreToFetch(), "Target user does not exist or has been deactivated");
        #else
        ASYNC_THROW_IF(targetUserResult->isSuccess() && targetUserResult->hasMoreToFetch(), "Target user does not exist or has been deactivated");
        #endif
        auto targetUserId = targetUserResult->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        ASYNC_THROW_IF(currentUserId == targetUserId, "You cannot remove yourself through this interface, please use the quit group interface");

        auto result = m_appClient->removeGroupMember(groupId, targetUserId, currentUserId);
        #ifdef SQLCHECK
        ASYNC_THROW_IF(result->isSuccess(), result->getErrorMessage());
        ASYNC_THROW_IF(result->hasMoreToFetch(), "You do not have permission to remove this member or member does not exist");
        #else
        ASYNC_THROW_IF(result->isSuccess() && !result->hasMoreToFetch(), "You do not have permission to remove this member or member does not exist");
        #endif

        auto deleteConvResult = m_appClient->deleteConversationForGroupMember(groupId, targetUserId);
        #ifdef SQLCHECK
        ASYNC_THROW_IF(deleteConvResult->isSuccess(), deleteConvResult->getErrorMessage());
        #else
        ASYNC_THROW_IF(deleteConvResult->isSuccess(), "Failed to delete conversation");
        #endif
        return true;
    }

    oatpp::Boolean setMemberRole(const oatpp::String& currentUserIdHeader,const oatpp::Object<SetMemberRoleRequestDTO>& request) {
        ASYNC_THROW_IF(currentUserIdHeader && !currentUserIdHeader->empty(), "User ID cannot be empty");
        ASYNC_THROW_IF(request->groupUuid && !request->groupUuid->empty(), "Group ID cannot be empty");
        ASYNC_THROW_IF(request->targetUserUuid && !request->targetUserUuid->empty(), "Target user ID cannot be empty");
        ASYNC_THROW_IF(request, "Request parameters cannot be empty");
        ASYNC_THROW_IF(request->role && !request->role->empty(), "Role parameter cannot be empty");
        ASYNC_THROW_IF((request->role == "admin" || request->role == "member"), "Role can only be 'admin' or 'member'");

        auto currentUserResult = m_appClient->getUserIdByUuid(currentUserIdHeader);
        #ifdef SQLCHECK
        ASYNC_THROW_IF(currentUserResult->isSuccess(), currentUserResult->getErrorMessage());
        ASYNC_THROW_IF(currentUserResult->hasMoreToFetch(), "User does not exist or has been deactivated");
        #else
        ASYNC_THROW_IF(currentUserResult->isSuccess() && currentUserResult->hasMoreToFetch(), "User does not exist or has been deactivated");
        #endif
        auto currentUserId = currentUserResult->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        auto groupIdResult = m_appClient->getGroupIdByUuid(request->groupUuid);
        #ifdef SQLCHECK
        ASYNC_THROW_IF(groupIdResult->isSuccess(), groupIdResult->getErrorMessage());
        ASYNC_THROW_IF(groupIdResult->hasMoreToFetch(), "Group does not exist or has been deactivated");
        #else
        ASYNC_THROW_IF(groupIdResult->isSuccess() && groupIdResult->hasMoreToFetch(), "Group does not exist or has been deactivated");
        #endif
        auto groupId = groupIdResult->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        auto targetUserResult = m_appClient->getUserIdByUuid(request->targetUserUuid);
        #ifdef SQLCHECK
        ASYNC_THROW_IF(targetUserResult->isSuccess(), targetUserResult->getErrorMessage());
        ASYNC_THROW_IF(targetUserResult->hasMoreToFetch(), "Target user does not exist or has been deactivated");
        #else
        ASYNC_THROW_IF(targetUserResult->isSuccess() && targetUserResult->hasMoreToFetch(), "Target user does not exist or has been deactivated");
        #endif
        auto targetUserId = targetUserResult->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        ASYNC_THROW_IF(currentUserId == targetUserId, "Cannot modify your own role");

        auto currentRoleResult = m_appClient->getUserRoleInGroup(groupId, currentUserId);
        #ifdef SQLCHECK
        ASYNC_THROW_IF(currentRoleResult->isSuccess(), currentRoleResult->getErrorMessage());
        ASYNC_THROW_IF(currentRoleResult->hasMoreToFetch(), "You are not a member of this group and do not have permission to operate");
        #else
        ASYNC_THROW_IF(currentRoleResult->isSuccess() && currentRoleResult->hasMoreToFetch(), "You are not a member of this group and do not have permission to operate");
        #endif
        auto currentRole = currentRoleResult->fetch<oatpp::Vector<oatpp::Object<UserRoleDTO>>>()[0]->role;

        ASYNC_THROW_IF(currentRole == "owner", "Only the group owner can set administrators");

        auto targetRoleResult = m_appClient->getUserRoleInGroup(groupId, targetUserId);
        #ifdef SQLCHECK
        ASYNC_THROW_IF(targetRoleResult->isSuccess(), targetRoleResult->getErrorMessage());
        ASYNC_THROW_IF(targetRoleResult->hasMoreToFetch(), "Target user is not in this group");
        #else
        ASYNC_THROW_IF(targetRoleResult->isSuccess() && targetRoleResult->hasMoreToFetch(), "Target user is not in this group");
        #endif
        auto targetRole = targetRoleResult->fetch<oatpp::Vector<oatpp::Object<UserRoleDTO>>>()[0]->role;

        ASYNC_THROW_IF((request->role == "admin" && targetRole == "owner"), "Cannot set the group owner as an administrator");
        ASYNC_THROW_IF(request->role != targetRole, targetRole == "admin" ? "User is already an administrator" : "User is already a regular member");

        auto result = m_appClient->setMemberRole(groupId, targetUserId, request->role);
        #ifdef SQLCHECK
        ASYNC_THROW_IF(result->isSuccess(), result->getErrorMessage());
        #else
        ASYNC_THROW_IF(result->isSuccess(), "Failed to set role");
        #endif

        return true;
    }

    oatpp::Boolean joinGroup(const oatpp::String& currentUserIdHeader, const oatpp::String& groupUuid) {
        ASYNC_THROW_IF(currentUserIdHeader && !currentUserIdHeader->empty(), "User ID cannot be empty");
        ASYNC_THROW_IF(groupUuid && !groupUuid->empty(), "Group ID cannot be empty");
        auto currentUserResult = m_appClient->getUserIdByUuid(currentUserIdHeader);
        #ifdef SQLCHECK
        ASYNC_THROW_IF(currentUserResult->isSuccess(), currentUserResult->getErrorMessage());
        ASYNC_THROW_IF(currentUserResult->hasMoreToFetch(), "User does not exist or has been deactivated");
        #else
        ASYNC_THROW_IF(currentUserResult->isSuccess() && currentUserResult->hasMoreToFetch(), "User does not exist or has been deactivated");
        #endif
        auto currentUserId = currentUserResult->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        auto groupIdResult = m_appClient->getGroupIdByUuid(groupUuid);
        #ifdef SQLCHECK
        ASYNC_THROW_IF(groupIdResult->isSuccess(), groupIdResult->getErrorMessage());
        ASYNC_THROW_IF(groupIdResult->hasMoreToFetch(), "Group does not exist or has been deactivated");
        #else
        ASYNC_THROW_IF(groupIdResult->isSuccess() && groupIdResult->hasMoreToFetch(), "Group does not exist or has been deactivated");
        #endif
        auto groupId = groupIdResult->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        auto result = m_appClient->addGroupMember(groupId, currentUserId, "member");
        #ifdef SQLCHECK
        ASYNC_THROW_IF(result->isSuccess(), result->getErrorMessage());
        #else
        ASYNC_THROW_IF(result->isSuccess(), "Failed to join group");
        #endif

        return true;
    }

    oatpp::Boolean leaveGroup(const oatpp::String& currentUserIdHeader, const oatpp::String& groupUuid) {
        ASYNC_THROW_IF(currentUserIdHeader && !currentUserIdHeader->empty(), "User ID cannot be empty");
        ASYNC_THROW_IF(groupUuid && !groupUuid->empty(), "Group ID cannot be empty");
        auto currentUserResult = m_appClient->getUserIdByUuid(currentUserIdHeader);
        #ifdef SQLCHECK
        ASYNC_THROW_IF(currentUserResult->isSuccess(), currentUserResult->getErrorMessage());
        ASYNC_THROW_IF(currentUserResult->hasMoreToFetch(), "User does not exist or has been deactivated");
        #else
        ASYNC_THROW_IF(currentUserResult->isSuccess() && currentUserResult->hasMoreToFetch(), "User does not exist or has been deactivated");
        #endif
        auto currentUserId = currentUserResult->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        auto groupIdResult = m_appClient->getGroupIdByUuid(groupUuid);
        #ifdef SQLCHECK
        ASYNC_THROW_IF(groupIdResult->isSuccess(), groupIdResult->getErrorMessage());
        ASYNC_THROW_IF(groupIdResult->hasMoreToFetch(), "Group does not exist or has been deactivated");
        #else
        ASYNC_THROW_IF(groupIdResult->isSuccess() && groupIdResult->hasMoreToFetch(), "Group does not exist or has been deactivated");
        #endif
        auto groupId = groupIdResult->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        // 在事务中执行所有操作，避免 TOCTOU
        auto transaction = m_appClient->beginTransaction();

        // 在事务内重新获取群成员列表（REPEATABLE READ 快照）
        auto membersResult = m_appClient->getGroupMembers(groupId);
        #ifdef SQLCHECK
        if (membersResult->isSuccess()) {
            transaction.rollback();
            throw std::runtime_error("Failed to get group member list");
        }
        #else
        if (membersResult->isSuccess()) {
            transaction.rollback();
            throw std::runtime_error("Failed to get group member list");
        }
        #endif
        auto members = membersResult->fetch<oatpp::Vector<oatpp::Object<GroupMemberVO>>>();

        // 计算剩余成员数量（减1是因为当前用户要离开）
        int remainingMembers = members->size() - 1;

        // 如果只剩最后一个成员，直接解散群聊
        if (remainingMembers == 1) {
            auto dissolveResult = m_appClient->dissolveGroup(groupId, currentUserId);
            #ifdef SQLCHECK
            if (dissolveResult->isSuccess()) {
                transaction.rollback();
                throw std::runtime_error(dissolveResult->getErrorMessage());
            }
            #else
            if (dissolveResult->isSuccess()) {
                transaction.rollback();
                throw std::runtime_error("No permission to dissolve the group");
            }
            #endif
            transaction.commit();
            return true;
        }

        // 检查当前用户是否是群主
        auto currentRoleResult = m_appClient->getUserRoleInGroup(groupId, currentUserId);
        #ifdef SQLCHECK
        if (currentRoleResult->isSuccess() || !currentRoleResult->hasMoreToFetch()) {
            transaction.rollback();
            throw std::runtime_error("You are not a member of this group and do not have permission to operate");
        }
        #else
        if (currentRoleResult->isSuccess() || !currentRoleResult->hasMoreToFetch()) {
            transaction.rollback();
            throw std::runtime_error("You are not a member of this group and do not have permission to operate");
        }
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
                if (updateResult->isSuccess() || !updateResult->hasMoreToFetch()) {
                    transaction.rollback();
                    throw std::runtime_error("Failed to set new group owner - member may have left");
                }
                #else
                if (updateResult->isSuccess() || !updateResult->hasMoreToFetch()) {
                    transaction.rollback();
                    throw std::runtime_error("Failed to set new group owner - member may have left");
                }
                #endif
            }
        }

        // 执行退出群组操作
        auto result = m_appClient->leaveGroup(groupId, currentUserId);
        #ifdef SQLCHECK
        if (result->isSuccess()) {
            transaction.rollback();
            throw std::runtime_error(result->getErrorMessage());
        }
        #else
        if (result->isSuccess()) {
            transaction.rollback();
            throw std::runtime_error("Failed to leave group");
        }
        #endif

        transaction.commit();
        return true;
    }


    // 发送群聊请求
    oatpp::Boolean sendGroupRequest(const oatpp::String& currentUserIdHeader, const oatpp::String& groupUuid, const oatpp::Object<SendGroupRequestDTO>& request) {
        ASYNC_THROW_IF(currentUserIdHeader && !currentUserIdHeader->empty(), "用户ID不能为空");
        ASYNC_THROW_IF(groupUuid && !groupUuid->empty(), "群组ID不能为空");
        ASYNC_THROW_IF(request, "请求参数不能为空");

        auto userCheck = m_appClient->getUserIdByUuid(currentUserIdHeader);
#ifdef SQLCHECK
        ASYNC_THROW_IF(userCheck->isSuccess(), userCheck->getErrorMessage());
        ASYNC_THROW_IF(userCheck->hasMoreToFetch(), "用户不存在或已失效");
#else
        ASYNC_THROW_IF(userCheck->isSuccess() && userCheck->hasMoreToFetch(), "用户不存在或已失效");
#endif
        auto userId = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        auto groupCheck = m_appClient->getGroupIdByUuid(groupUuid);
#ifdef SQLCHECK
        ASYNC_THROW_IF(groupCheck->isSuccess(), groupCheck->getErrorMessage());
        ASYNC_THROW_IF(groupCheck->hasMoreToFetch(), "群组不存在或已失效");
#else
        ASYNC_THROW_IF(groupCheck->isSuccess() && groupCheck->hasMoreToFetch(), "群组不存在或已失效");
#endif
        auto groupId = groupCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        // 检查是否已在群中
        auto memberCheck = m_appClient->checkUserInGroup(groupId, userId);
        if (memberCheck->isSuccess() && memberCheck->hasMoreToFetch()) {
            throw std::runtime_error("您已经是该群成员");
        }

        auto result = m_appClient->sendGroupRequest(groupId, userId, request->message);
#ifdef SQLCHECK
        ASYNC_THROW_IF(result->isSuccess(), result->getErrorMessage());
#else
        ASYNC_THROW_IF(result->isSuccess(), "发送群聊请求失败");
#endif

        return true;
    }

    // 处理群聊请求（通过/拒绝）
    oatpp::Boolean handleGroupRequest(const oatpp::String& currentUserIdHeader, const oatpp::String& requestUuid, const oatpp::Object<HandleGroupRequestDTO>& request) {
        ASYNC_THROW_IF(currentUserIdHeader && !currentUserIdHeader->empty(), "用户ID不能为空");
        ASYNC_THROW_IF(requestUuid && !requestUuid->empty(), "请求ID不能为空");
        ASYNC_THROW_IF(request && request->status, "处理状态不能为空");
        ASYNC_THROW_IF(request->status == "accepted" || request->status == "rejected", "状态只能是 'accepted' 或 'rejected'");

        auto userCheck = m_appClient->getUserIdByUuid(currentUserIdHeader);
#ifdef SQLCHECK
        ASYNC_THROW_IF(userCheck->isSuccess(), userCheck->getErrorMessage());
        ASYNC_THROW_IF(userCheck->hasMoreToFetch(), "用户不存在或已失效");
#else
        ASYNC_THROW_IF(userCheck->isSuccess() && userCheck->hasMoreToFetch(), "用户不存在或已失效");
#endif
        auto userId = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        // 先获取请求详情
        auto requestDetailResult = m_appClient->getGroupRequestByUuid(requestUuid);
#ifdef SQLCHECK
        ASYNC_THROW_IF(requestDetailResult->isSuccess(), requestDetailResult->getErrorMessage());
        ASYNC_THROW_IF(requestDetailResult->hasMoreToFetch(), "群聊请求不存在");
#else
        ASYNC_THROW_IF(requestDetailResult->isSuccess() && requestDetailResult->hasMoreToFetch(), "群聊请求不存在");
#endif
        auto requestDetail = requestDetailResult->fetch<oatpp::Vector<oatpp::Object<GroupRequestDetailDTO>>>()[0];

        auto transaction = m_appClient->beginTransaction();

        // 更新请求状态
        auto updateResult = m_appClient->handleGroupRequest(requestUuid, request->status, userId);
        if (!updateResult->isSuccess()) {
            transaction.rollback();
#ifdef SQLCHECK
            throw std::runtime_error(updateResult->getErrorMessage());
#else
            throw std::runtime_error("处理群聊请求失败");
#endif
        }

        // 检查是否真的更新了行（通过 getKnownCount()）
        if (updateResult->getKnownCount() == 0) {
            transaction.rollback();
            throw std::runtime_error("请求已被处理或不存在");
        }

        // 如果是批准，添加成员并创建会话
        if (request->status == "accepted") {
            // 添加成员到 group_members（带冲突处理）
            auto addMemberResult = m_appClient->addGroupMemberWithConflict(requestDetail->groupId, requestDetail->requesterId, "member");
            if (!addMemberResult->isSuccess()) {
                transaction.rollback();
#ifdef SQLCHECK
                throw std::runtime_error(addMemberResult->getErrorMessage());
#else
                throw std::runtime_error("添加群成员失败");
#endif
            }

            // 创建群聊会话
            auto convResult = m_appClient->createGroupConversation(requestDetail->requesterId, requestDetail->groupId);
            if (!convResult->isSuccess()) {
                transaction.rollback();
#ifdef SQLCHECK
                throw std::runtime_error(convResult->getErrorMessage());
#else
                throw std::runtime_error("创建群聊会话失败");
#endif
            }
        }

        transaction.commit();
        return true;
    }
};
