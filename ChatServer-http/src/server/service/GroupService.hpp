#pragma once

#include "global.h"
#include "server/dto/GroupDto.hpp"
#include "server/vo/GroupVo.hpp"
#include "server/postgresql/AppPostgresql.hpp"
#include "redis/AppRedis.hpp"
#include "tool/HashUtils.hpp"
#include "tool/UuidIdCache.hpp"

class GroupService {
private:
    std::shared_ptr<AppPostgresql> m_appPostgresql;
    std::shared_ptr<AppRedis> m_redis;
    std::shared_ptr<UuidIdCache> m_idCache;
    using Status = oatpp::web::protocol::http::Status;

public:
    GroupService(const std::shared_ptr<AppPostgresql>& appClient, 
                 const std::shared_ptr<AppRedis>& redis,
                 const std::shared_ptr<UuidIdCache>& idCache)
        : m_appPostgresql(appClient), m_redis(redis), m_idCache(idCache) {}

    oatpp::Object<GroupInfoVO> createGroup(const oatpp::String& currentUserIdHeader, const oatpp::Object<CreateGroupRequestDTO>& request) {
        OATPP_ASSERT_HTTP(currentUserIdHeader && !currentUserIdHeader->empty(), Status::CODE_400, "用户ID不能为空");
        OATPP_ASSERT_HTTP(request, Status::CODE_400, "请求参数不能为空");
        OATPP_ASSERT_HTTP(request->name && !request->name->empty(), Status::CODE_400, "群组名称不能为空");
        OATPP_ASSERT_HTTP(request->memberUuids && !request->memberUuids->empty(), Status::CODE_400, "成员列表不能为空");
        auto userId = m_idCache->getUserId(currentUserIdHeader);
        OATPP_ASSERT_HTTP(userId > 0, Status::CODE_401, "用户不存在或已失效");

        auto result = m_appPostgresql->createGroup(request->name, request->description, request->avatarUrl, userId, request->maxMembers, request->isPublic);
        #ifdef SQLCHECK
        if(!result->isSuccess()) {
            OATPP_LOGE("SQL_ERROR", "%s", result->getErrorMessage()->c_str());
        }
        #endif
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, "创建群组失败");
        OATPP_ASSERT_HTTP(result->hasMoreToFetch(), Status::CODE_400, "创建群组失败");

        auto group = result->fetch<oatpp::Vector<oatpp::Object<GroupInfoVO>>>()[0];
        group->memberCount = request->memberUuids->size() + 1;
        auto transaction = m_appPostgresql->beginTransaction();
        auto groupId = m_idCache->getGroupId(group->uuid.getValue(""));
        OATPP_ASSERT_HTTP(groupId > 0, Status::CODE_401, "群组不存在或已失效");

        auto ownerResult = m_appPostgresql->addGroupMember(groupId, userId, "owner");
        if (!ownerResult->isSuccess()) {
            transaction.rollback();
            OATPP_ASSERT_HTTP(false, Status::CODE_400, "添加组长失败");
        }

        auto ownerConvResult = m_appPostgresql->createGroupConversation(userId, groupId);
        if (!ownerConvResult->isSuccess()) {
            transaction.rollback();
        #ifdef SQLCHECK
            OATPP_LOGE("SQL_ERROR", "%s", ownerConvResult->getErrorMessage()->c_str());
        #endif
            OATPP_ASSERT_HTTP(false, Status::CODE_500, "组长添加会话失败");
        }
        for (auto& memberUuid : *request->memberUuids) {
            auto memberId = m_idCache->getUserId(memberUuid);
            OATPP_ASSERT_HTTP(memberId > 0, Status::CODE_401, "用户不存在或已失效");

            auto memberResult = m_appPostgresql->addGroupMember(groupId, memberId, "member");
            if (!memberResult->isSuccess()) {
                transaction.rollback();
            #ifdef SQLCHECK
                OATPP_LOGE("SQL_ERROR", "%s", memberResult->getErrorMessage()->c_str());
            #endif
                OATPP_ASSERT_HTTP(false, Status::CODE_500, "添加组员失败");
            }
            auto convResult = m_appPostgresql->createGroupConversation(memberId, groupId);
            if (!convResult->isSuccess()) {
                transaction.rollback();
            #ifdef SQLCHECK
                OATPP_LOGE("SQL_ERROR", "%s", convResult->getErrorMessage()->c_str());
            #endif
                OATPP_ASSERT_HTTP(false, Status::CODE_500, "添加会话失败");
            }
        }
        transaction.commit();
        return group;
    }

    oatpp::Vector<oatpp::Object<GroupInfoVO>> getMyGroups(const oatpp::String& currentUserIdHeader) {
        OATPP_ASSERT_HTTP(currentUserIdHeader && !currentUserIdHeader->empty(), Status::CODE_400, "用户ID不能为空");
        auto userId = m_idCache->getUserId(currentUserIdHeader);
        OATPP_ASSERT_HTTP(userId > 0, Status::CODE_401, "用户不存在或已失效");

        auto result = m_appPostgresql->getMyGroups(userId);
        #ifdef SQLCHECK
        if(!result->isSuccess()) {
            OATPP_LOGE("SQL_ERROR", "%s", result->getErrorMessage()->c_str());
        }
        #endif
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, "获取群组列表失败");

        auto groups = result->fetch<oatpp::Vector<oatpp::Object<GroupInfoVO>>>();
        if (!groups) {
            groups = oatpp::Vector<oatpp::Object<GroupInfoVO>>::createShared();
        }
        return groups;
    }

    oatpp::Object<GroupDetailInfoVO> getGroupDetail(const oatpp::String& currentUserIdHeader, const oatpp::String& groupUuid) {
        OATPP_ASSERT_HTTP(currentUserIdHeader && !currentUserIdHeader->empty(), Status::CODE_400, "用户ID不能为空");
        OATPP_ASSERT_HTTP(groupUuid && !groupUuid->empty(), Status::CODE_400, "群组ID不能为空");
        auto userId = m_idCache->getUserId(currentUserIdHeader);
        OATPP_ASSERT_HTTP(userId > 0, Status::CODE_401, "用户不存在或已失效");

        auto groupId = m_idCache->getGroupId(groupUuid);
        OATPP_ASSERT_HTTP(groupId > 0, Status::CODE_401, "群组不存在或已失效");

        //auto memberCheck = m_appPostgresql->checkUserInGroup(groupId, userId);
        //#ifdef SQLCHECK
        //if(!memberCheck->isSuccess()) {
        //    OATPP_LOGE("SQL_ERROR", "%s", memberCheck->getErrorMessage()->c_str());
        //}
        //#endif
        //OATPP_ASSERT_HTTP(memberCheck->isSuccess(), Status::CODE_500, "检查群组成员失败");
        //OATPP_ASSERT_HTTP(memberCheck->hasMoreToFetch(), Status::CODE_403, "您不是该群组的成员，无权查看");

        auto result = m_appPostgresql->getGroupDetail(groupId);
        #ifdef SQLCHECK
        if(!result->isSuccess()) {
            OATPP_LOGE("SQL_ERROR", "%s", result->getErrorMessage()->c_str());
        }
        #endif
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, "获取群组详情失败");
        OATPP_ASSERT_HTTP(result->hasMoreToFetch(), Status::CODE_404, "群组不存在");

        return result->fetch<oatpp::Vector<oatpp::Object<GroupDetailInfoVO>>>()[0];
    }

    oatpp::Object<GroupDetailInfoVO> updateGroup(const oatpp::String& currentUserIdHeader, const oatpp::String& groupUuid, const oatpp::Object<UpdateGroupRequestDTO>& request) {
        OATPP_ASSERT_HTTP(currentUserIdHeader && !currentUserIdHeader->empty(), Status::CODE_400, "用户ID不能为空");
        OATPP_ASSERT_HTTP(groupUuid && !groupUuid->empty(), Status::CODE_400, "群组ID不能为空");
        OATPP_ASSERT_HTTP(request, Status::CODE_400, "请求参数不能为空");
        auto userId = m_idCache->getUserId(currentUserIdHeader);
        OATPP_ASSERT_HTTP(userId > 0, Status::CODE_401, "用户不存在或已失效");

        auto groupId = m_idCache->getGroupId(groupUuid);
        OATPP_ASSERT_HTTP(groupId > 0, Status::CODE_401, "群组不存在或已失效");

        auto result = m_appPostgresql->updateGroup(groupId, userId, request->name, request->description, request->avatarUrl, request->maxMembers, request->isPublic);
        #ifdef SQLCHECK
        if(!result->isSuccess()) {
            OATPP_LOGE("SQL_ERROR", "%s", result->getErrorMessage()->c_str());
        }
        #endif
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_403, "无权限修改群组信息");

        auto updatedGroup = m_appPostgresql->getGroupDetail(groupId);
        #ifdef SQLCHECK
        if(!updatedGroup->isSuccess()) {
            OATPP_LOGE("SQL_ERROR", "%s", updatedGroup->getErrorMessage()->c_str());
        }
        #endif
        OATPP_ASSERT_HTTP(updatedGroup->isSuccess(), Status::CODE_500, "获取群组详情失败");
        OATPP_ASSERT_HTTP(updatedGroup->hasMoreToFetch(), Status::CODE_404, "群组不存在");

        return updatedGroup->fetch<oatpp::Vector<oatpp::Object<GroupDetailInfoVO>>>()[0];
    }

    oatpp::Boolean dissolveGroup(const oatpp::String& currentUserIdHeader, const oatpp::String& groupUuid) {
        OATPP_ASSERT_HTTP(currentUserIdHeader && !currentUserIdHeader->empty(), Status::CODE_400, "用户ID不能为空");
        OATPP_ASSERT_HTTP(groupUuid && !groupUuid->empty(), Status::CODE_400, "群组ID不能为空");
        auto userId = m_idCache->getUserId(currentUserIdHeader);
        OATPP_ASSERT_HTTP(userId > 0, Status::CODE_401, "用户不存在或已失效");

        auto groupId = m_idCache->getGroupId(groupUuid);
        OATPP_ASSERT_HTTP(groupId > 0, Status::CODE_401, "群组不存在或已失效");

        auto dsresult = m_appPostgresql->dissolveGroup(groupId, userId);
        #ifdef SQLCHECK
        if(!dsresult->isSuccess()) {
            OATPP_LOGE("SQL_ERROR", "%s", dsresult->getErrorMessage()->c_str());
        }
        #endif
        OATPP_ASSERT_HTTP(dsresult->isSuccess(), Status::CODE_403, "无权限解散群组");

        return true;
    }

    oatpp::Vector<oatpp::Object<GroupMemberVO>> getGroupMembers(const oatpp::String& groupUuid) {
        OATPP_ASSERT_HTTP(groupUuid && !groupUuid->empty(), Status::CODE_400, "群组ID不能为空");

        auto groupId = m_idCache->getGroupId(groupUuid);
        OATPP_ASSERT_HTTP(groupId > 0, Status::CODE_401, "群组不存在或已失效");

        auto result = m_appPostgresql->getGroupMembers(groupId);
        #ifdef SQLCHECK
        if(!result->isSuccess()) {
            OATPP_LOGE("SQL_ERROR", "%s", result->getErrorMessage()->c_str());
        }
        #endif
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_404, "群组不存在");

        return result->fetch<oatpp::Vector<oatpp::Object<GroupMemberVO>>>();
    }

    oatpp::Boolean addGroupMembers(const oatpp::String& currentUserIdHeader, const oatpp::String& groupUuid, const oatpp::Object<AddGroupMemberRequestDTO>& request) {
        OATPP_ASSERT_HTTP(currentUserIdHeader && !currentUserIdHeader->empty(), Status::CODE_400, "用户ID不能为空");
        OATPP_ASSERT_HTTP(groupUuid && !groupUuid->empty(), Status::CODE_400, "群组ID不能为空");
        OATPP_ASSERT_HTTP(request, Status::CODE_400, "请求参数不能为空");
        OATPP_ASSERT_HTTP(request->userIds && !request->userIds->empty(), Status::CODE_400, "用户ID列表不能为空");
        auto userId = m_idCache->getUserId(currentUserIdHeader);
        OATPP_ASSERT_HTTP(userId > 0, Status::CODE_401, "用户不存在或已失效");

        auto groupId = m_idCache->getGroupId(groupUuid);
        OATPP_ASSERT_HTTP(groupId > 0, Status::CODE_401, "群组不存在或已失效");

        auto transaction = m_appPostgresql->beginTransaction();
        for (const auto& memberId : *request->userIds) {
            auto result = m_appPostgresql->addGroupMember(groupId, memberId, "member");
            if (!result->isSuccess()) {
                transaction.rollback();
                #ifdef SQLCHECK
                OATPP_LOGE("SQL_ERROR", "%s", result->getErrorMessage()->c_str());
                #endif
                OATPP_ASSERT_HTTP(false, Status::CODE_400, "添加成员失败");
            }
            auto convResult = m_appPostgresql->createGroupConversation(memberId, groupId);
            if (!convResult->isSuccess()) {
                transaction.rollback();
                #ifdef SQLCHECK
                OATPP_LOGE("SQL_ERROR", "%s", convResult->getErrorMessage()->c_str());
                #endif
                OATPP_ASSERT_HTTP(false, Status::CODE_500, "添加会话失败");
            }
        }
        transaction.commit();
        return true;
    }

    oatpp::Boolean removeGroupMember(const oatpp::String& currentUserIdHeader, const oatpp::String& groupUuid, const oatpp::String& targetUserUuid) {
        OATPP_ASSERT_HTTP(currentUserIdHeader && !currentUserIdHeader->empty(), Status::CODE_400, "用户ID不能为空");
        OATPP_ASSERT_HTTP(groupUuid && !groupUuid->empty(), Status::CODE_400, "群组ID不能为空");
        OATPP_ASSERT_HTTP(targetUserUuid && !targetUserUuid->empty(), Status::CODE_400, "目标用户ID不能为空");
        auto currentUserId = m_idCache->getUserId(currentUserIdHeader);
        OATPP_ASSERT_HTTP(currentUserId > 0, Status::CODE_401, "用户不存在或已失效");

        auto groupId = m_idCache->getGroupId(groupUuid);
        OATPP_ASSERT_HTTP(groupId > 0, Status::CODE_401, "群组不存在或已失效");

        auto targetUserId = m_idCache->getUserId(targetUserUuid);
        OATPP_ASSERT_HTTP(targetUserId > 0, Status::CODE_404, "目标用户不存在或已失效");

        OATPP_ASSERT_HTTP(currentUserId != targetUserId, Status::CODE_400, "不能通过此接口移除自己，请使用退出群聊接口");

        auto currentRoleResult = m_appPostgresql->getUserRoleInGroup(groupId, currentUserId);
        #ifdef SQLCHECK
        if(!currentRoleResult->isSuccess()) {
            OATPP_LOGE("SQL_ERROR", "%s", currentRoleResult->getErrorMessage()->c_str());
        }
        #endif
        OATPP_ASSERT_HTTP(currentRoleResult->isSuccess(), Status::CODE_500, "获取用户角色失败");
        OATPP_ASSERT_HTTP(currentRoleResult->hasMoreToFetch(), Status::CODE_403, "您不是该群组的成员，无权操作");
        auto currentRole = currentRoleResult->fetch<oatpp::Vector<oatpp::Object<UserRoleDTO>>>()[0]->role;

        auto targetRoleResult = m_appPostgresql->getUserRoleInGroup(groupId, targetUserId);
        #ifdef SQLCHECK
        if(!targetRoleResult->isSuccess()) {
            OATPP_LOGE("SQL_ERROR", "%s", targetRoleResult->getErrorMessage()->c_str());
        }
        #endif
        OATPP_ASSERT_HTTP(targetRoleResult->isSuccess(), Status::CODE_500, "获取目标用户角色失败");
        OATPP_ASSERT_HTTP(targetRoleResult->hasMoreToFetch(), Status::CODE_404, "目标用户不在该群组中");
        auto targetRole = targetRoleResult->fetch<oatpp::Vector<oatpp::Object<UserRoleDTO>>>()[0]->role;

        bool hasPermission = false;
        if (currentRole == "owner") {
            hasPermission = true;
        } else if (currentRole == "admin" && targetRole == "member") {
            hasPermission = true;
        }

        OATPP_ASSERT_HTTP(hasPermission, Status::CODE_403, "您没有权限移除该成员");

        auto result = m_appPostgresql->removeGroupMember(groupId, targetUserId);
        #ifdef SQLCHECK
        if(!result->isSuccess()) {
            OATPP_LOGE("SQL_ERROR", "%s", result->getErrorMessage()->c_str());
        }
        #endif
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, "移除成员失败");

        auto deleteConvResult = m_appPostgresql->deleteConversationForGroupMember(groupId, targetUserId);
        #ifdef SQLCHECK
        if(!deleteConvResult->isSuccess()) {
            OATPP_LOGE("SQL_ERROR", "%s", deleteConvResult->getErrorMessage()->c_str());
        }
        #endif
        OATPP_ASSERT_HTTP(deleteConvResult->isSuccess(), Status::CODE_500, "删除会话失败");
        return true;
    }

    oatpp::Boolean setMemberRole(const oatpp::String& currentUserIdHeader, const oatpp::String& groupUuid, const oatpp::String& targetUserUuid, const oatpp::Object<SetMemberRoleRequestDTO>& request) {
        OATPP_ASSERT_HTTP(currentUserIdHeader && !currentUserIdHeader->empty(), Status::CODE_400, "用户ID不能为空");
        OATPP_ASSERT_HTTP(groupUuid && !groupUuid->empty(), Status::CODE_400, "群组ID不能为空");
        OATPP_ASSERT_HTTP(targetUserUuid && !targetUserUuid->empty(), Status::CODE_400, "目标用户ID不能为空");
        OATPP_ASSERT_HTTP(request, Status::CODE_400, "请求参数不能为空");
        OATPP_ASSERT_HTTP(request->role && !request->role->empty(), Status::CODE_400, "角色参数不能为空");
        OATPP_ASSERT_HTTP(request->role == "admin" || request->role == "member", Status::CODE_400, "角色只能是 'admin' 或 'member'");

        auto currentUserId = m_idCache->getUserId(currentUserIdHeader);
        OATPP_ASSERT_HTTP(currentUserId > 0, Status::CODE_401, "用户不存在或已失效");

        auto groupId = m_idCache->getGroupId(groupUuid);
        OATPP_ASSERT_HTTP(groupId > 0, Status::CODE_404, "群组不存在或已失效");

        auto targetUserId = m_idCache->getUserId(targetUserUuid);
        OATPP_ASSERT_HTTP(targetUserId > 0, Status::CODE_404, "目标用户不存在或已失效");

        OATPP_ASSERT_HTTP(currentUserId != targetUserId, Status::CODE_400, "不能修改自己的角色");

        auto currentRoleResult = m_appPostgresql->getUserRoleInGroup(groupId, currentUserId);
        #ifdef SQLCHECK
        if(!currentRoleResult->isSuccess()) {
            OATPP_LOGE("SQL_ERROR", "%s", currentRoleResult->getErrorMessage()->c_str());
        }
        #endif
        OATPP_ASSERT_HTTP(currentRoleResult->isSuccess(), Status::CODE_500, "获取用户角色失败");
        OATPP_ASSERT_HTTP(currentRoleResult->hasMoreToFetch(), Status::CODE_403, "您不是该群组的成员，无权操作");
        auto currentRole = currentRoleResult->fetch<oatpp::Vector<oatpp::Object<UserRoleDTO>>>()[0]->role;

        OATPP_ASSERT_HTTP(currentRole == "owner", Status::CODE_403, "只有群主可以设置管理员");

        auto targetRoleResult = m_appPostgresql->getUserRoleInGroup(groupId, targetUserId);
        #ifdef SQLCHECK
        if(!targetRoleResult->isSuccess()) {
            OATPP_LOGE("SQL_ERROR", "%s", targetRoleResult->getErrorMessage()->c_str());
        }
        #endif
        OATPP_ASSERT_HTTP(targetRoleResult->isSuccess(), Status::CODE_500, "获取目标用户角色失败");
        OATPP_ASSERT_HTTP(targetRoleResult->hasMoreToFetch(), Status::CODE_404, "目标用户不在该群组中");
        auto targetRole = targetRoleResult->fetch<oatpp::Vector<oatpp::Object<UserRoleDTO>>>()[0]->role;

        OATPP_ASSERT_HTTP(!(request->role == "admin" && targetRole == "owner"), Status::CODE_403, "不能将群主设置为管理员");
        OATPP_ASSERT_HTTP(request->role != targetRole, Status::CODE_400, targetRole == "admin" ? "用户已经是管理员" : "用户已经是普通成员");

        auto result = m_appPostgresql->setMemberRole(groupId, targetUserId, request->role);
        #ifdef SQLCHECK
        if(!result->isSuccess()) {
            OATPP_LOGE("SQL_ERROR", "%s", result->getErrorMessage()->c_str());
        }
        #endif
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, "设置角色失败");

        return true;
    }

    oatpp::Boolean joinGroup(const oatpp::String& currentUserIdHeader, const oatpp::String& groupUuid) {
        OATPP_ASSERT_HTTP(currentUserIdHeader && !currentUserIdHeader->empty(), Status::CODE_400, "用户ID不能为空");
        OATPP_ASSERT_HTTP(groupUuid && !groupUuid->empty(), Status::CODE_400, "群组ID不能为空");

        // 去重检查
        if (!m_redis->tryAcquireDedupLock(currentUserIdHeader->c_str(), "joinGroup", groupUuid->c_str())) {
            OATPP_ASSERT_HTTP(false, Status::CODE_429, "请求频繁");
        }

        auto currentUserId = m_idCache->getUserId(currentUserIdHeader);
        OATPP_ASSERT_HTTP(currentUserId > 0, Status::CODE_401, "用户不存在或已失效");

        auto groupId = m_idCache->getGroupId(groupUuid);
        OATPP_ASSERT_HTTP(groupId > 0, Status::CODE_404, "群组不存在或已失效");

        auto transaction = m_appPostgresql->beginTransaction();
        auto result = m_appPostgresql->addGroupMember(groupId, currentUserId, "member");
        if (!result->isSuccess()) {
            transaction.rollback();
            #ifdef SQLCHECK
            OATPP_LOGE("SQL_ERROR", "%s", result->getErrorMessage()->c_str());
            #endif
            OATPP_ASSERT_HTTP(false, Status::CODE_400, "加入群组失败");
        }
        auto convResult = m_appPostgresql->createGroupConversation(currentUserId, groupId);
        if (!convResult->isSuccess()) {
            transaction.rollback();
            #ifdef SQLCHECK
            OATPP_LOGE("SQL_ERROR", "%s", convResult->getErrorMessage()->c_str());
            #endif
            OATPP_ASSERT_HTTP(false, Status::CODE_500, "添加会话失败");
        }
        transaction.commit();

        return true;
    }

    oatpp::Boolean leaveGroup(const oatpp::String& currentUserIdHeader, const oatpp::String& groupUuid) {
        OATPP_ASSERT_HTTP(currentUserIdHeader && !currentUserIdHeader->empty(), Status::CODE_400, "用户ID不能为空");
        OATPP_ASSERT_HTTP(groupUuid && !groupUuid->empty(), Status::CODE_400, "群组ID不能为空");
        auto currentUserId = m_idCache->getUserId(currentUserIdHeader);
        OATPP_ASSERT_HTTP(currentUserId > 0, Status::CODE_401, "用户不存在或已失效");

        auto groupId = m_idCache->getGroupId(groupUuid);
        OATPP_ASSERT_HTTP(groupId > 0, Status::CODE_404, "群组不存在或已失效");



        // 获取群成员列表
        auto membersResult = m_appPostgresql->getGroupMembers(groupId);
        #ifdef SQLCHECK
        if(!membersResult->isSuccess()) {
            OATPP_LOGE("SQL_ERROR", "%s", membersResult->getErrorMessage()->c_str());
        }
        #endif
        OATPP_ASSERT_HTTP(membersResult->isSuccess(), Status::CODE_500, "获取群成员列表失败");
        auto members = membersResult->fetch<oatpp::Vector<oatpp::Object<GroupMemberVO>>>();

        // 计算剩余成员数量（减1是因为当前用户要离开）
        int remainingMembers = members->size() - 1;

        // 如果只剩最后一个成员，直接解散群聊
        //原本是remainingMembers == 1的，其实只有群主一个人的话是无法成立群聊的
        if (remainingMembers <= 1) {
            auto dissolveResult = m_appPostgresql->dissolveGroup(groupId, currentUserId);
            #ifdef SQLCHECK
            if(!dissolveResult->isSuccess()) {
                OATPP_LOGE("SQL_ERROR", "%s", dissolveResult->getErrorMessage()->c_str());
            }
            #endif
            OATPP_ASSERT_HTTP(dissolveResult->isSuccess(), Status::CODE_403, "无权限解散群组");
            return true;
        }
        // 检查当前用户是否是群主
        auto currentRoleResult = m_appPostgresql->getUserRoleInGroup(groupId, currentUserId);
        #ifdef SQLCHECK
        if(!currentRoleResult->isSuccess()) {
            OATPP_LOGE("SQL_ERROR", "%s", currentRoleResult->getErrorMessage()->c_str());
        }
        #endif
        OATPP_ASSERT_HTTP(currentRoleResult->isSuccess(), Status::CODE_500, "获取用户角色失败");
        OATPP_ASSERT_HTTP(currentRoleResult->hasMoreToFetch(), Status::CODE_403, "您不是该群组的成员，无权操作");
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
                if(!updateResult->isSuccess()) {
                    OATPP_LOGE("SQL_ERROR", "%s", updateResult->getErrorMessage()->c_str());
                }
                #endif
                OATPP_ASSERT_HTTP(updateResult->isSuccess(), Status::CODE_500, "设置新群主失败");
            }
        }

        // 执行退出群组操作
        auto result = m_appPostgresql->leaveGroup(groupId, currentUserId);
        #ifdef SQLCHECK
        if(!result->isSuccess()) {
            OATPP_LOGE("SQL_ERROR", "%s", result->getErrorMessage()->c_str());
        }
        #endif
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_400, "退出群组失败");

        // 删除该用户的群会话
        auto deleteConvResult = m_appPostgresql->deleteConversationForGroupMember(groupId, currentUserId);
        #ifdef SQLCHECK
        if(!deleteConvResult->isSuccess()) {
            OATPP_LOGE("SQL_ERROR", "%s", deleteConvResult->getErrorMessage()->c_str());
        }
        #endif
        // 会话删除失败不阻断退出流程，仅记录日志

        return true;
    }

    oatpp::Vector<oatpp::Object<GroupInfoVO>> searchGroups(const oatpp::String& keyword, const oatpp::String& currentUserUuid) {
        OATPP_ASSERT_HTTP(keyword && !keyword->empty(), Status::CODE_400, "搜索关键词不能为空");
        OATPP_ASSERT_HTTP(currentUserUuid && !currentUserUuid->empty(), Status::CODE_400, "用户ID不能为空");


        auto userId = m_idCache->getUserId(currentUserUuid);
        OATPP_ASSERT_HTTP(userId > 0, Status::CODE_401, "用户不存在或已失效");

        
        auto searchKeyword = "%" + keyword + "%";
        //OATPP_LOGD("Search", "Received keyword: %s", searchKeyword->c_str());
        auto result = m_appPostgresql->searchGroups(searchKeyword,userId);
#ifdef SQLCHECK
        if(!result->isSuccess()) {
            OATPP_LOGE("SQL_ERROR", "%s", result->getErrorMessage()->c_str());
        }
#endif
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, "搜索群组失败");
  
        auto groups = result->fetch<oatpp::Vector<oatpp::Object<GroupInfoVO>>>();
        
        std::sort(groups->begin(), groups->end(), [](const oatpp::Object<GroupInfoVO>& a, const oatpp::Object<GroupInfoVO>& b) {
            if (a->memberCount != b->memberCount) {
                return a->memberCount > b->memberCount;
            }
            return false;
        });
        
        return groups;
    }

    // 获取收到的群聊请求（作为管理员/群主）
    oatpp::Vector<oatpp::Object<ReceivedGroupRequestVO>> getReceivedGroupRequests(const oatpp::String& currentUserIdHeader) {
        OATPP_ASSERT_HTTP(currentUserIdHeader && !currentUserIdHeader->empty(), Status::CODE_400, "用户ID不能为空");
        
        auto userId = m_idCache->getUserId(currentUserIdHeader);
        OATPP_ASSERT_HTTP(userId > 0, Status::CODE_401, "用户不存在或已失效");

        auto result = m_appPostgresql->getReceivedGroupRequests(userId);
        #ifdef SQLCHECK
        if(!result->isSuccess()) {
            OATPP_LOGE("SQL_ERROR", "%s", result->getErrorMessage()->c_str());
        }
        #endif
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, "获取群聊请求失败");

        auto requests = result->fetch<oatpp::Vector<oatpp::Object<GroupRequestResponseDTO>>>();
        auto response = oatpp::Vector<oatpp::Object<ReceivedGroupRequestVO>>::createShared();

        for (const auto& request : *requests) {
            auto vo = ReceivedGroupRequestVO::createShared();
            vo->uuid = request->uuid;
            vo->groupUuid = request->groupUuid;
            vo->groupName = request->groupName;
            vo->message = request->message;
            vo->status = request->status;
            vo->createdAt = request->createdAt;

            // 组装请求者信息
            auto requesterInfo = UserInfoVO::createShared();
            requesterInfo->userUuid = request->requesterUuid;
            requesterInfo->username = request->requesterName;
            requesterInfo->avatarUrl = request->requesterAvatar;
            vo->requester = requesterInfo;

            response->push_back(vo);
        }

        return response;
    }

    // 获取发送的群聊请求
    oatpp::Vector<oatpp::Object<SentGroupRequestVO>> getSentGroupRequests(const oatpp::String& currentUserIdHeader) {
        OATPP_ASSERT_HTTP(currentUserIdHeader && !currentUserIdHeader->empty(), Status::CODE_400, "用户ID不能为空");
        
        auto userId = m_idCache->getUserId(currentUserIdHeader);
        OATPP_ASSERT_HTTP(userId > 0, Status::CODE_401, "用户不存在或已失效");

        auto result = m_appPostgresql->getSentGroupRequests(userId);
        #ifdef SQLCHECK
        if(!result->isSuccess()) {
            OATPP_LOGE("SQL_ERROR", "%s", result->getErrorMessage()->c_str());
        }
        #endif
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, "获取群聊请求失败");

        auto requests = result->fetch<oatpp::Vector<oatpp::Object<GroupRequestResponseDTO>>>();
        auto response = oatpp::Vector<oatpp::Object<SentGroupRequestVO>>::createShared();

        for (const auto& request : *requests) {
            auto vo = SentGroupRequestVO::createShared();
            vo->uuid = request->uuid;
            vo->message = request->message;
            vo->status = request->status;
            vo->createdAt = request->createdAt;

            // 组装群组信息
            auto groupInfo = GroupInfoVO::createShared();
            groupInfo->uuid = request->groupUuid;
            groupInfo->name = request->groupName;
            groupInfo->avatarUrl = request->groupAvatar;
            vo->group = groupInfo;

            response->push_back(vo);
        }

        return response;
    }

    //// 发送群聊请求
    //oatpp::Boolean sendGroupRequest(const oatpp::String& currentUserIdHeader, const oatpp::String& groupUuid, const oatpp::Object<SendGroupRequestDTO>& request) {
    //    OATPP_ASSERT_HTTP(currentUserIdHeader && !currentUserIdHeader->empty(), Status::CODE_400, "用户ID不能为空");
    //    OATPP_ASSERT_HTTP(groupUuid && !groupUuid->empty(), Status::CODE_400, "群组ID不能为空");
    //    OATPP_ASSERT_HTTP(request, Status::CODE_400, "请求参数不能为空");

    //    auto userCheck = m_appPostgresql->getUserIdByUuid(currentUserIdHeader);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(userCheck->isSuccess(), Status::CODE_500, userCheck->getErrorMessage());
    //    OATPP_ASSERT_HTTP(userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
    //    #else
    //    OATPP_ASSERT_HTTP(userCheck->isSuccess() && userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
    //    #endif
    //    auto userId = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

    //    auto groupCheck = m_appPostgresql->getGroupIdByUuid(groupUuid);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(groupCheck->isSuccess(), Status::CODE_500, groupCheck->getErrorMessage());
    //    OATPP_ASSERT_HTTP(groupCheck->hasMoreToFetch(), Status::CODE_404, "群组不存在或已失效");
    //    #else
    //    OATPP_ASSERT_HTTP(groupCheck->isSuccess() && groupCheck->hasMoreToFetch(), Status::CODE_404, "群组不存在或已失效");
    //    #endif
    //    auto groupId = groupCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

    //    // 检查是否已在群中
    //    auto memberCheck = m_appPostgresql->checkUserInGroup(groupId, userId);
    //    if (memberCheck->isSuccess() && memberCheck->hasMoreToFetch()) {
    //        OATPP_ASSERT_HTTP(false, Status::CODE_400, "您已经是该群成员");
    //    }

    //    auto result = m_appPostgresql->sendGroupRequest(groupId, userId, request->message);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, result->getErrorMessage());
    //    //OATPP_ASSERT_HTTP(result->hasMoreToFetch(), Status::CODE_400, "发送群聊请求失败");
    //    #else
    //    OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_400, "发送群聊请求失败");
    //    #endif

    //    return true;
    //}

    //// 处理群聊请求（通过/拒绝）
    //oatpp::Boolean handleGroupRequest(const oatpp::String& currentUserIdHeader, const oatpp::String& requestUuid, const oatpp::Object<HandleGroupRequestDTO>& request) {
    //    OATPP_ASSERT_HTTP(currentUserIdHeader && !currentUserIdHeader->empty(), Status::CODE_400, "用户ID不能为空");
    //    OATPP_ASSERT_HTTP(requestUuid && !requestUuid->empty(), Status::CODE_400, "请求ID不能为空");
    //    OATPP_ASSERT_HTTP(request && request->status, Status::CODE_400, "处理状态不能为空");
    //    OATPP_ASSERT_HTTP(request->status == "approved" || request->status == "rejected", Status::CODE_400, "状态只能是 'approved' 或 'rejected'");

    //    auto userCheck = m_appPostgresql->getUserIdByUuid(currentUserIdHeader);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(userCheck->isSuccess(), Status::CODE_500, userCheck->getErrorMessage());
    //    OATPP_ASSERT_HTTP(userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
    //    #else
    //    OATPP_ASSERT_HTTP(userCheck->isSuccess() && userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
    //    #endif
    //    auto userId = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

    //    auto result = m_appPostgresql->handleGroupRequest(requestUuid, request->status, userId);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, result->getErrorMessage());
    //    #else
    //    OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, "处理群聊请求失败");
    //    #endif

    //    return true;
    //}
};
