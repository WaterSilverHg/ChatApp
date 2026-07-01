#pragma once

#include "global.h"
#include "server/dto/GroupDto.hpp"
#include "server/vo/GroupVo.hpp"
#include "server/postgresql/AppPostgresql.hpp"
#include "tool/UuidIdCache.hpp"

class GroupService {
private:
    std::shared_ptr<AppPostgresql> m_appPostgresql;
    std::shared_ptr<UuidIdCache> m_idCache;
    using Status = oatpp::web::protocol::http::Status;
public:
    GroupService(const std::shared_ptr<AppPostgresql>& appClient, const std::shared_ptr<UuidIdCache>& idCache) 
        : m_appPostgresql(appClient), m_idCache(idCache) {}

    oatpp::Object<GroupInfoVO> createGroup(const oatpp::String& currentUserIdHeader, const oatpp::Object<CreateGroupRequestDTO>& request) {
        ASYNC_THROW_IF(currentUserIdHeader && !currentUserIdHeader->empty(), "User ID cannot be empty");
        ASYNC_THROW_IF(request, "Request parameters cannot be empty");
        ASYNC_THROW_IF(request->name && !request->name->empty(), "Group name cannot be empty");
        ASYNC_THROW_IF(request->memberUuids && !request->memberUuids->empty(), "Member list cannot be empty");
        auto userId = m_idCache->getUserId(currentUserIdHeader);
        ASYNC_THROW_IF(userId > 0, "User does not exist or has been deactivated");

        // 预先检查所有成员是否存在，避免在事务中失败
        std::vector<oatpp::Int64> memberIds;
        for (auto& memberUuid : *request->memberUuids) {
            auto memberId = m_idCache->getUserId(memberUuid);
            ASYNC_THROW_IF(memberId > 0, "User does not exist or has been deactivated");
            memberIds.push_back(memberId);
        }

        // 在事务中执行所有操作
        auto transaction = m_appPostgresql->beginTransaction();

        auto result = m_appPostgresql->createGroup(request->name, request->description, request->avatarUrl, userId, request->maxMembers, request->isPublic);
        #ifdef SQLCHECK
        if (!result->isSuccess()) {
            transaction.rollback();
            OATPP_LOGD("GroupService", "Error: %s", result->getErrorMessage()->c_str());
            throw std::runtime_error("Failed to create group");
        }
        if (!result->hasMoreToFetch()) {
            transaction.rollback();
            OATPP_LOGD("GroupService", "Error: %s", "Failed to create group");
            throw std::runtime_error("Failed to create group");
        }
        #else
        if (!result->isSuccess() || !result->hasMoreToFetch()) {
            transaction.rollback();
            throw std::runtime_error("Failed to create group");
        }
        #endif

        auto group = result->fetch<oatpp::Vector<oatpp::Object<GroupInfoVO>>>()[0];
        group->memberCount = request->memberUuids->size() + 1;

        auto groupId = m_idCache->getGroupId(group->uuid.getValue(""));
        if (groupId <= 0) {
            transaction.rollback();
            throw std::runtime_error("Group does not exist or has been deactivated");
        }

        auto ownerResult = m_appPostgresql->addGroupMember(groupId, userId, "owner");
        if (!ownerResult->isSuccess()) {
            transaction.rollback();
            #ifdef SQLCHECK
            OATPP_LOGD("GroupService", "Error: %s", ownerResult->getErrorMessage()->c_str());
            #endif
            throw std::runtime_error("Failed to add group owner");
        }

        auto ownerConvResult = m_appPostgresql->createGroupConversation(userId, groupId);
        if (!ownerConvResult->isSuccess()) {
            transaction.rollback();
        #ifdef SQLCHECK
            OATPP_LOGD("GroupService", "Error: %s", ownerConvResult->getErrorMessage()->c_str());
        #endif
            throw std::runtime_error("Failed to create owner conversation");
        }

        for (size_t i = 0; i < memberIds.size(); ++i) {
            auto memberId = memberIds[i];
            auto memberResult = m_appPostgresql->addGroupMember(groupId, memberId, "member");
            if (!memberResult->isSuccess()) {
                transaction.rollback();
            #ifdef SQLCHECK
                OATPP_LOGD("GroupService", "Error: %s", memberResult->getErrorMessage()->c_str());
            #endif
                throw std::runtime_error("Failed to add member");
            }
            auto convResult = m_appPostgresql->createGroupConversation(memberId, groupId);
            if (!convResult->isSuccess()) {
                transaction.rollback();
            #ifdef SQLCHECK
                OATPP_LOGD("GroupService", "Error: %s", convResult->getErrorMessage()->c_str());
            #endif
                throw std::runtime_error("Failed to create conversation");
            }
        }
        transaction.commit();
        return group;
    }

    oatpp::Vector<oatpp::Object<GroupInfoVO>> getMyGroups(const oatpp::String& currentUserIdHeader) {
        ASYNC_THROW_IF(currentUserIdHeader && !currentUserIdHeader->empty(), "User ID cannot be empty");
        auto userId = m_idCache->getUserId(currentUserIdHeader);
        ASYNC_THROW_IF(userId > 0, "User does not exist or has been deactivated");

        auto result = m_appPostgresql->getMyGroups(userId);
        #ifdef SQLCHECK
        OATPP_LOGD("GroupService", "Error: %s", result->getErrorMessage()->c_str());
        #endif
        ASYNC_THROW_IF(result->isSuccess(), "Failed to get group list");

        auto groups = result->fetch<oatpp::Vector<oatpp::Object<GroupInfoVO>>>();
        if (!groups) {
            groups = oatpp::Vector<oatpp::Object<GroupInfoVO>>::createShared();
        }
        return groups;
    }

    oatpp::Object<GroupDetailInfoVO> getGroupDetail(const oatpp::String& currentUserIdHeader, const oatpp::String& groupUuid) {
        ASYNC_THROW_IF(currentUserIdHeader && !currentUserIdHeader->empty(), "User ID cannot be empty");
        ASYNC_THROW_IF(groupUuid && !groupUuid->empty(), "Group ID cannot be empty");
        
        auto userId = m_idCache->getUserId(currentUserIdHeader);
        ASYNC_THROW_IF(userId > 0, "User does not exist or has been deactivated");

        auto groupId = m_idCache->getGroupId(groupUuid);
        ASYNC_THROW_IF(groupId > 0, "Group does not exist or has been deactivated");

        auto memberCheck = m_appPostgresql->checkUserInGroup(groupId, userId);
        #ifdef SQLCHECK
        if (memberCheck->isSuccess()) {
            OATPP_LOGD("GroupService", "Error: %s", memberCheck->getErrorMessage()->c_str());
            throw std::runtime_error("You are not a member of this group and do not have permission to view");
        }
        if (memberCheck->hasMoreToFetch()) {
            OATPP_LOGD("GroupService", "Error: %s", "You are not a member of this group and do not have permission to view");
            throw std::runtime_error("You are not a member of this group and do not have permission to view");
        }
        #else
        ASYNC_THROW_IF(memberCheck->isSuccess() && memberCheck->hasMoreToFetch(), "You are not a member of this group and do not have permission to view");
        #endif

        auto result = m_appPostgresql->getGroupDetail(groupId);
        #ifdef SQLCHECK
        if (!result->isSuccess()) {
            OATPP_LOGD("GroupService", "Error: %s", result->getErrorMessage()->c_str());
            throw std::runtime_error("Group does not exist");
        }
        if (!result->hasMoreToFetch()) {
            OATPP_LOGD("GroupService", "Error: %s", "Group does not exist");
            throw std::runtime_error("Group does not exist");
        }
        #else
        ASYNC_THROW_IF(result->isSuccess() && result->hasMoreToFetch(), "Group does not exist");
        #endif

        return result->fetch<oatpp::Vector<oatpp::Object<GroupDetailInfoVO>>>()[0];
    }

    oatpp::Object<GroupDetailInfoVO> updateGroup(const oatpp::String& currentUserIdHeader, const oatpp::Object<UpdateGroupRequestDTO>& request) {
        ASYNC_THROW_IF(currentUserIdHeader && !currentUserIdHeader->empty(), "User ID cannot be empty");
        ASYNC_THROW_IF(request, "Request parameters cannot be empty");
        ASYNC_THROW_IF(request->groupUuid && !request->groupUuid->empty(), "Group ID cannot be empty");
        
        auto userId = m_idCache->getUserId(currentUserIdHeader);
        ASYNC_THROW_IF(userId > 0, "User does not exist or has been deactivated");

        auto groupId = m_idCache->getGroupId(request->groupUuid);
        ASYNC_THROW_IF(groupId > 0, "Group does not exist or has been deactivated");
        //感觉可以优化成一次查询
        auto result = m_appPostgresql->updateGroup(groupId, userId, request->name, request->description, request->avatarUrl, request->maxMembers, request->isPublic);
        #ifdef SQLCHECK
        if (!result->isSuccess()) {
            OATPP_LOGD("GroupService", "Error: %s", result->getErrorMessage()->c_str());
            throw std::runtime_error("No permission to modify group information");
        }
        #else
        ASYNC_THROW_IF(result->isSuccess(), "No permission to modify group information");
        #endif

        auto updatedGroup = m_appPostgresql->getGroupDetail(groupId);
        #ifdef SQLCHECK
        if (!updatedGroup->isSuccess()) {
            OATPP_LOGD("GroupService", "Error: %s", updatedGroup->getErrorMessage()->c_str());
            throw std::runtime_error("Group does not exist");
        }
        if (!updatedGroup->hasMoreToFetch()) {
            OATPP_LOGD("GroupService", "Error: %s", "Group does not exist");
            throw std::runtime_error("Group does not exist");
        }
        #else
        ASYNC_THROW_IF(updatedGroup->isSuccess() && updatedGroup->hasMoreToFetch(), "Group does not exist");
        #endif

        return updatedGroup->fetch<oatpp::Vector<oatpp::Object<GroupDetailInfoVO>>>()[0];
    }

    oatpp::Boolean dissolveGroup(const oatpp::String& currentUserIdHeader, const oatpp::String& groupUuid) {
        ASYNC_THROW_IF(currentUserIdHeader && !currentUserIdHeader->empty(), "User ID cannot be empty");
        ASYNC_THROW_IF(groupUuid && !groupUuid->empty(), "Group ID cannot be empty");
        
        auto userId = m_idCache->getUserId(currentUserIdHeader);
        ASYNC_THROW_IF(userId > 0, "User does not exist or has been deactivated");

        auto groupId = m_idCache->getGroupId(groupUuid);
        ASYNC_THROW_IF(groupId > 0, "Group does not exist or has been deactivated");

        auto dsresult = m_appPostgresql->dissolveGroup(groupId, userId);
        #ifdef SQLCHECK
        if (dsresult->isSuccess()) {
            OATPP_LOGD("GroupService", "Error: %s", dsresult->getErrorMessage()->c_str());
            throw std::runtime_error("No permission to dissolve the group");
        }
        #else
        ASYNC_THROW_IF(dsresult->isSuccess(), "No permission to dissolve the group");
        #endif

        //auto convIdResult = m_appPostgresql->getGroupConversationId(groupId, userId);
        //if (convIdResult->isSuccess() && convIdResult->hasMoreToFetch()) {
        //    auto convId = convIdResult->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;
        //    auto deleteConvResult = m_appPostgresql->deleteConversation(convId);
        //    ASYNC_THROW_IF(!deleteConvResult->isSuccess(), "删除会话失败");
        //}

        return true;
    }

    oatpp::Vector<oatpp::Object<GroupMemberVO>> getGroupMembers(const oatpp::String& groupUuid) {
        ASYNC_THROW_IF(groupUuid && !groupUuid->empty(), "Group ID cannot be empty");

        auto groupId = m_idCache->getGroupId(groupUuid);
        ASYNC_THROW_IF(groupId > 0, "Group does not exist or has been deactivated");

        auto result = m_appPostgresql->getGroupMembers(groupId);
        #ifdef SQLCHECK
        if (!result->isSuccess()) {
            OATPP_LOGD("GroupService", "Error: %s", result->getErrorMessage()->c_str());
            throw std::runtime_error("Group does not exist");
        }
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

        auto userId = m_idCache->getUserId(currentUserIdHeader);
        ASYNC_THROW_IF(userId > 0, "User does not exist or has been deactivated");

        auto groupId = m_idCache->getGroupId(groupUuid);
        ASYNC_THROW_IF(groupId > 0, "Group does not exist or has been deactivated");

        // 检查当前用户是否有权限（群主或管理员）
        auto roleResult = m_appPostgresql->getUserRoleInGroup(groupId, userId);
        #ifdef SQLCHECK
        if (!roleResult->isSuccess() || !roleResult->hasMoreToFetch()) {
            throw std::runtime_error("You are not a member of this group and do not have permission to add members");
        }
        #else
        if (!roleResult->isSuccess() || !roleResult->hasMoreToFetch()) {
            throw std::runtime_error("You are not a member of this group and do not have permission to add members");
        }
        #endif
        auto role = roleResult->fetch<oatpp::Vector<oatpp::Object<UserRoleDTO>>>()[0]->role;
        if (role != "owner" && role != "admin") {
            throw std::runtime_error("Only group owner or admin can add members");
        }

        // 在事务中执行所有操作
        auto transaction = m_appPostgresql->beginTransaction();

        for (const auto& memberUuid : *request->userUuids) {
            auto memberId = m_idCache->getUserId(memberUuid);
            if (memberId <= 0) {
                transaction.rollback();
                throw std::runtime_error("User does not exist or has been deactivated");
            }
            auto result = m_appPostgresql->addGroupMember(groupId, memberId, "member");
            #ifdef SQLCHECK
            if (!result->isSuccess()) {
                transaction.rollback();
                OATPP_LOGD("GroupService", "Error: %s", result->getErrorMessage()->c_str());
                throw std::runtime_error("Failed to add member");
            }
            #else
            if (!result->isSuccess()) {
                transaction.rollback();
                throw std::runtime_error("Failed to add member");
            }
            #endif
            auto convResult = m_appPostgresql->createGroupConversation(memberId, groupId);
            if (!convResult->isSuccess()) {
                transaction.rollback();
                #ifdef SQLCHECK
                OATPP_LOGD("GroupService", "Error: %s", convResult->getErrorMessage()->c_str());
                #endif
                throw std::runtime_error("Failed to create conversation for member");
            }
        }

        transaction.commit();
        return true;
    }

    oatpp::Boolean removeGroupMember(const oatpp::String& currentUserIdHeader, const oatpp::String& groupUuid, const oatpp::String& targetUserUuid) {
        ASYNC_THROW_IF(currentUserIdHeader && !currentUserIdHeader->empty(), "User ID cannot be empty");
        ASYNC_THROW_IF(groupUuid && !groupUuid->empty(), "Group ID cannot be empty");
        ASYNC_THROW_IF(targetUserUuid && !targetUserUuid->empty(), "Target user ID cannot be empty");

        auto currentUserId = m_idCache->getUserId(currentUserIdHeader);
        ASYNC_THROW_IF(currentUserId > 0, "User does not exist or has been deactivated");

        auto groupId = m_idCache->getGroupId(groupUuid);
        ASYNC_THROW_IF(groupId > 0, "Group does not exist or has been deactivated");

        auto targetUserId = m_idCache->getUserId(targetUserUuid);
        ASYNC_THROW_IF(targetUserId > 0, "Target user does not exist or has been deactivated");

        ASYNC_THROW_IF(currentUserId != targetUserId, "You cannot remove yourself through this interface, please use the quit group interface");

        auto result = m_appPostgresql->removeGroupMember(groupId, targetUserId, currentUserId);
        #ifdef SQLCHECK
        if (!result->isSuccess()) {
            OATPP_LOGD("GroupService", "Error: %s", result->getErrorMessage()->c_str());
            throw std::runtime_error("You do not have permission to remove this member or member does not exist");
        }
        if (result->hasMoreToFetch()) {
            OATPP_LOGD("GroupService", "Error: %s", "You do not have permission to remove this member or member does not exist");
            throw std::runtime_error("You do not have permission to remove this member or member does not exist");
        }
        #else
        ASYNC_THROW_IF(result->isSuccess() && !result->hasMoreToFetch(), "You do not have permission to remove this member or member does not exist");
        #endif

        auto deleteConvResult = m_appPostgresql->deleteConversationForGroupMember(groupId, targetUserId);
        #ifdef SQLCHECK
        if (!deleteConvResult->isSuccess()) {
            OATPP_LOGD("GroupService", "Error: %s", deleteConvResult->getErrorMessage()->c_str());
            throw std::runtime_error("Failed to delete conversation");
        }
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

        auto currentUserId = m_idCache->getUserId(currentUserIdHeader);
        ASYNC_THROW_IF(currentUserId > 0, "User does not exist or has been deactivated");

        auto groupId = m_idCache->getGroupId(request->groupUuid);
        ASYNC_THROW_IF(groupId > 0, "Group does not exist or has been deactivated");

        auto targetUserId = m_idCache->getUserId(request->targetUserUuid);
        ASYNC_THROW_IF(targetUserId > 0, "Target user does not exist or has been deactivated");

        ASYNC_THROW_IF(currentUserId != targetUserId, "Cannot modify your own role");

        auto currentRoleResult = m_appPostgresql->getUserRoleInGroup(groupId, currentUserId);
        #ifdef SQLCHECK
        if (!currentRoleResult->isSuccess()) {
            OATPP_LOGD("GroupService", "Error: %s", currentRoleResult->getErrorMessage()->c_str());
            throw std::runtime_error("You are not a member of this group and do not have permission to operate");
        }
        if (!currentRoleResult->hasMoreToFetch()) {
            OATPP_LOGD("GroupService", "Error: %s", "You are not a member of this group and do not have permission to operate");
            throw std::runtime_error("You are not a member of this group and do not have permission to operate");
        }
        #else
        ASYNC_THROW_IF(currentRoleResult->isSuccess() && currentRoleResult->hasMoreToFetch(), "You are not a member of this group and do not have permission to operate");
        #endif
        auto currentRole = currentRoleResult->fetch<oatpp::Vector<oatpp::Object<UserRoleDTO>>>()[0]->role;

        ASYNC_THROW_IF(currentRole == "owner", "Only the group owner can set administrators");

        auto targetRoleResult = m_appPostgresql->getUserRoleInGroup(groupId, targetUserId);
        #ifdef SQLCHECK
        if (!targetRoleResult->isSuccess()) {
            OATPP_LOGD("GroupService", "Error: %s", targetRoleResult->getErrorMessage()->c_str());
            throw std::runtime_error("Target user is not in this group");
        }
        if (!targetRoleResult->hasMoreToFetch()) {
            OATPP_LOGD("GroupService", "Error: %s", "Target user is not in this group");
            throw std::runtime_error("Target user is not in this group");
        }
        #else
        ASYNC_THROW_IF(targetRoleResult->isSuccess() && targetRoleResult->hasMoreToFetch(), "Target user is not in this group");
        #endif
        auto targetRole = targetRoleResult->fetch<oatpp::Vector<oatpp::Object<UserRoleDTO>>>()[0]->role;
        //暂为只有群主能提权，且只能提为管理员,降权只能将管理员降为普通成员
        ASYNC_THROW_IF((request->role == "admin" && targetRole == "member")||(request->role == "member"&&targetRole == "admin"), "Invalid permission setting");
        //ASYNC_THROW_IF((request->role == "admin" && targetRole == "owner"), "Cannot set the group owner as an administrator");
        //ASYNC_THROW_IF(request->role != targetRole, targetRole == "admin" ? "User is already an administrator" : "User is already a regular member");

        auto result = m_appPostgresql->setMemberRole(groupId, targetUserId, request->role);
        #ifdef SQLCHECK
        if (!result->isSuccess()) {
            OATPP_LOGD("GroupService", "Error: %s", result->getErrorMessage()->c_str());
            throw std::runtime_error("Failed to set role");
        }
        #else
        ASYNC_THROW_IF(result->isSuccess(), "Failed to set role");
        #endif

        return true;
    }

    oatpp::Boolean joinGroup(const oatpp::String& currentUserIdHeader, const oatpp::String& groupUuid) {
        ASYNC_THROW_IF(currentUserIdHeader && !currentUserIdHeader->empty(), "User ID cannot be empty");
        ASYNC_THROW_IF(groupUuid && !groupUuid->empty(), "Group ID cannot be empty");
        
        auto currentUserId = m_idCache->getUserId(currentUserIdHeader);
        ASYNC_THROW_IF(currentUserId > 0, "User does not exist or has been deactivated");

        auto groupId = m_idCache->getGroupId(groupUuid);
        ASYNC_THROW_IF(groupId > 0, "Group does not exist or has been deactivated");

        auto result = m_appPostgresql->addGroupMember(groupId, currentUserId, "member");
        #ifdef SQLCHECK
        if (!result->isSuccess()) {
            OATPP_LOGD("GroupService", "Error: %s", result->getErrorMessage()->c_str());
            throw std::runtime_error("Failed to join group");
        }
        #else
        ASYNC_THROW_IF(result->isSuccess(), "Failed to join group");
        #endif

        return true;
    }

    oatpp::Boolean leaveGroup(const oatpp::String& currentUserIdHeader, const oatpp::String& groupUuid) {
        ASYNC_THROW_IF(currentUserIdHeader && !currentUserIdHeader->empty(), "User ID cannot be empty");
        ASYNC_THROW_IF(groupUuid && !groupUuid->empty(), "Group ID cannot be empty");
        
        auto currentUserId = m_idCache->getUserId(currentUserIdHeader);
        ASYNC_THROW_IF(currentUserId > 0, "User does not exist or has been deactivated");

        auto groupId = m_idCache->getGroupId(groupUuid);
        ASYNC_THROW_IF(groupId > 0, "Group does not exist or has been deactivated");

        // 在事务中执行所有操作，避免 TOCTOU
        auto transaction = m_appPostgresql->beginTransaction();

        // 在事务内重新获取群成员列表（REPEATABLE READ 快照）
        auto membersResult = m_appPostgresql->getGroupMembers(groupId);
        #ifdef SQLCHECK
        if (!membersResult->isSuccess()) {
            transaction.rollback();
            throw std::runtime_error("Failed to get group member list");
        }
        #else
        if (!membersResult->isSuccess()) {
            transaction.rollback();
            throw std::runtime_error("Failed to get group member list");
        }
        #endif
        auto members = membersResult->fetch<oatpp::Vector<oatpp::Object<GroupMemberVO>>>();

        // 计算剩余成员数量（减1是因为当前用户要离开）
        int remainingMembers = members->size() - 1;

        // 如果只剩最后一个成员，直接解散群聊
        if (remainingMembers == 1) {
            auto dissolveResult = m_appPostgresql->dissolveGroup(groupId, currentUserId);
            #ifdef SQLCHECK
            if (dissolveResult->isSuccess()) {
                transaction.rollback();
                OATPP_LOGD("GroupService", "Error: %s", dissolveResult->getErrorMessage()->c_str());
                throw std::runtime_error("No permission to dissolve the group");
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
        auto currentRoleResult = m_appPostgresql->getUserRoleInGroup(groupId, currentUserId);
        #ifdef SQLCHECK
        if (!currentRoleResult->isSuccess() || !currentRoleResult->hasMoreToFetch()) {
            transaction.rollback();
            OATPP_LOGD("GroupService", "Error: %s", currentRoleResult->getErrorMessage()->c_str());
            throw std::runtime_error("You are not a member of this group and do not have permission to operate");
        }
        #else
        if (!currentRoleResult->isSuccess() || !currentRoleResult->hasMoreToFetch()) {
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
                    newOwnerId = m_idCache->getUserId(member->userUuid);
                    if (newOwnerId > 0) {
                        break;
                    }
                }
            }

            // 如果没有管理员，选择普通成员作为新群主
            if (newOwnerId == -1) {
                for (const auto& member : *members) {
                    if (member->role == "member" && member->userUuid != currentUserIdHeader) {
                        newOwnerId = m_idCache->getUserId(member->userUuid);
                        if (newOwnerId > 0) {
                            break;
                        }
                    }
                }
            }

            // 如果找到新群主，更新其角色为owner
            if (newOwnerId != -1) {
                auto updateResult = m_appPostgresql->setMemberRole(groupId, newOwnerId, "owner");
                #ifdef SQLCHECK
                if (!updateResult->isSuccess() || !updateResult->hasMoreToFetch()) {
                    transaction.rollback();
                    OATPP_LOGD("GroupService", "Error: %s", updateResult->getErrorMessage()->c_str());
                    throw std::runtime_error("Failed to set new group owner - member may have left");
                }
                #else
                if (!updateResult->isSuccess() || !updateResult->hasMoreToFetch()) {
                    transaction.rollback();
                    throw std::runtime_error("Failed to set new group owner - member may have left");
                }
                #endif
            }
        }

        // 执行退出群组操作
        auto result = m_appPostgresql->leaveGroup(groupId, currentUserId);
        #ifdef SQLCHECK
        if (!result->isSuccess()) {
            transaction.rollback();
            OATPP_LOGD("GroupService", "Error: %s", result->getErrorMessage()->c_str());
            throw std::runtime_error("Failed to leave group");
        }
        #else
        if (!result->isSuccess()) {
            transaction.rollback();
            throw std::runtime_error("Failed to leave group");
        }
        #endif

        // 删除该用户的群会话记录
        auto convResult = m_appPostgresql->deleteConversationForGroupMember(groupId, currentUserId);
        #ifdef SQLCHECK
        if (!convResult->isSuccess()) {
            OATPP_LOGD("GroupService", "Warning: Failed to delete conversation: %s", convResult->getErrorMessage()->c_str());
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

        auto userId = m_idCache->getUserId(currentUserIdHeader);
        ASYNC_THROW_IF(userId > 0, "用户不存在或已失效");

        auto groupId = m_idCache->getGroupId(groupUuid);
        ASYNC_THROW_IF(groupId > 0, "群组不存在或已失效");

        // 检查是否已在群中
        auto memberCheck = m_appPostgresql->checkUserInGroup(groupId, userId);
        if (memberCheck->isSuccess() && memberCheck->hasMoreToFetch()) {
            throw std::runtime_error("您已经是该群成员");
        }

        // 检查是否已有请求
        auto checkResult = m_appPostgresql->checkGroupRequestExists(groupId, userId);
        #ifdef SQLCHECK
        if (!checkResult->isSuccess()) {
            OATPP_LOGD("GroupService", "Error: %s", checkResult->getErrorMessage()->c_str());
            throw std::runtime_error("检查群聊请求失败");
        }
        #else
        ASYNC_THROW_IF(checkResult->isSuccess(), "检查群聊请求失败");
        #endif

        if (checkResult->hasMoreToFetch()) {
            auto updateResult = m_appPostgresql->updateGroupRequestToPending(groupId, userId, request->message);
            #ifdef SQLCHECK
            if (!updateResult->isSuccess()) {
                OATPP_LOGD("GroupService", "Error: %s", updateResult->getErrorMessage()->c_str());
                throw std::runtime_error("发送群聊请求失败");
            }
            #else
            ASYNC_THROW_IF(updateResult->isSuccess(), "发送群聊请求失败");
            #endif
            return true;
        }

        auto result = m_appPostgresql->sendGroupRequest(groupId, userId, request->message);
        #ifdef SQLCHECK
        if (!result->isSuccess()) {
            OATPP_LOGD("GroupService", "Error: %s", result->getErrorMessage()->c_str());
            throw std::runtime_error("发送群聊请求失败");
        }
        #else
        ASYNC_THROW_IF(result->isSuccess(), "发送群聊请求失败");
        #endif

        return true;
    }

    oatpp::Object<GroupRequestDetailDTO> handleGroupRequest(const oatpp::String& currentUserIdHeader, const oatpp::String& requestUuid, const oatpp::Object<HandleGroupRequestDTO>& request) {
        ASYNC_THROW_IF(currentUserIdHeader && !currentUserIdHeader->empty(), "用户ID不能为空");
        ASYNC_THROW_IF(requestUuid && !requestUuid->empty(), "请求ID不能为空");
        ASYNC_THROW_IF(request && request->status, "处理状态不能为空");
        ASYNC_THROW_IF(request->status == "accepted" || request->status == "rejected", "状态只能是 'accepted' 或 'rejected'");

        auto userId = m_idCache->getUserId(currentUserIdHeader);
        ASYNC_THROW_IF(userId > 0, "用户不存在或已失效");

        auto requestDetailResult = m_appPostgresql->getGroupRequestByUuid(requestUuid);
        #ifdef SQLCHECK
        if (!requestDetailResult->isSuccess()) {
            OATPP_LOGD("GroupService", "Error: %s", requestDetailResult->getErrorMessage()->c_str());
            throw std::runtime_error("群聊请求不存在");
        }
        if (!requestDetailResult->hasMoreToFetch()) {
            OATPP_LOGD("GroupService", "Error: %s", "群聊请求不存在");
            throw std::runtime_error("群聊请求不存在");
        }
        #else
        ASYNC_THROW_IF(requestDetailResult->isSuccess() && requestDetailResult->hasMoreToFetch(), "群聊请求不存在");
        #endif
        auto requestDetail = requestDetailResult->fetch<oatpp::Vector<oatpp::Object<GroupRequestDetailDTO>>>()[0];

        auto transaction = m_appPostgresql->beginTransaction();

        auto updateResult = m_appPostgresql->handleGroupRequest(requestUuid, request->status, userId);
        if (!updateResult->isSuccess()) {
            transaction.rollback();
            #ifdef SQLCHECK
            OATPP_LOGD("GroupService", "Error: %s", updateResult->getErrorMessage()->c_str());
            #endif
            throw std::runtime_error("处理群聊请求失败");
        }

        if (updateResult->getKnownCount() == 0) {
            transaction.rollback();
            throw std::runtime_error("请求已被处理或不存在");
        }

        if (request->status == "accepted") {
            auto addMemberResult = m_appPostgresql->addGroupMemberWithConflict(requestDetail->groupId, requestDetail->requesterId, "member");
            if (!addMemberResult->isSuccess()) {
                transaction.rollback();
                #ifdef SQLCHECK
                OATPP_LOGD("GroupService", "Error: %s", addMemberResult->getErrorMessage()->c_str());
                #endif
                throw std::runtime_error("添加群成员失败");
            }

            auto convResult = m_appPostgresql->createGroupConversation(requestDetail->requesterId, requestDetail->groupId);
            if (!convResult->isSuccess()) {
                transaction.rollback();
                #ifdef SQLCHECK
                OATPP_LOGD("GroupService", "Error: %s", convResult->getErrorMessage()->c_str());
                #endif
                throw std::runtime_error("创建群聊会话失败");
            }
        }

        transaction.commit();
        return requestDetail;
    }
};
