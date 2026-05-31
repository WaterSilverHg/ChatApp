#pragma once

#include "global.h"
#include "../dto/AuthDto.hpp"
#include "../dto/FriendDto.hpp"
#include "../dto/MessageDto.hpp"
#include "../dto/GroupDto.hpp"
#include "../dto/ConversationDto.hpp"
#include "../dto/FileDto.hpp"
#include "../dto/UserStatusDto.hpp"


//即使你sql语句中用了大写的变量，它映射还是映射在小写上

#include OATPP_CODEGEN_BEGIN(DbClient) 

class AppPostgresql : public oatpp::orm::DbClient {
public:
    AppPostgresql(const std::shared_ptr<oatpp::orm::Executor>& executor)
        : oatpp::orm::DbClient(executor) {}

    // ==================== 用户相关 ====================
    // 根据邮箱查找用户（包含密码哈希，用于登录验证）
    QUERY(getUserByEmail, 
          "SELECT CAST(uuid AS VARCHAR) AS uuid, username, email, password_hash AS passwordhash, avatar_url AS avatarurl, TO_CHAR(created_at, 'YYYY-MM-DD HH24:MI:SS') AS createdat FROM users WHERE email = :email AND deleted_at IS NULL;",
          PARAM(oatpp::String, email))

    //// 根据邮箱查找用户（不包含密码）
    //QUERY(findUserByEmail, 
    //      "SELECT id, username, email, avatar_url, status, created_at FROM users WHERE email = :email AND deleted_at IS NULL;",
    //      PARAM(oatpp::String, email))

    // 根据ID查找用户
    QUERY(getUserById, 
          "SELECT CAST(uuid AS VARCHAR) AS uuid, username, email, avatar_url AS avatarurl, status, TO_CHAR(created_at, 'YYYY-MM-DD HH24:MI:SS') AS createdat FROM users WHERE id = :userId AND deleted_at IS NULL;",
          PARAM(oatpp::Int64, userId))

    // 创建用户
    QUERY(createUser, 
          "INSERT INTO users (username, email, phone, password_hash) VALUES (:username, :email, :phone, crypt(:password, gen_salt('bf'))) RETURNING CAST(uuid AS VARCHAR) AS uuid;",
          PARAM(oatpp::String, username),
          PARAM(oatpp::String, email),
          PARAM(oatpp::String, phone),
          PARAM(oatpp::String, password))

    // 登录用户
    QUERY(loginUser, 
          "SELECT CAST(uuid AS VARCHAR) AS uuid, username, email, avatar_url AS avatarurl, TO_CHAR(created_at, 'YYYY-MM-DD HH24:MI:SS') AS createdat FROM users WHERE email = :email AND password_hash = :passwordHash AND deleted_at IS NULL;",
          PARAM(oatpp::String, email),
          PARAM(oatpp::String, passwordHash))


    QUERY(getPasswordHashById,
        "SELECT password_hash FROM users WHERE id = :userId;",
        PARAM(oatpp::Int64, userId))

    // 重置密码
        QUERY(resetPasswordByEmail,
            "UPDATE users SET password_hash = crypt(:newPassword, gen_salt('bf')), updated_at = NOW() "
            "WHERE email = :email AND deleted_at IS NULL;",
            PARAM(oatpp::String, email),
            PARAM(oatpp::String, newPassword))

    // 更新用户信息
    QUERY(updateUser, 
          "UPDATE users SET username = :username, avatar_url = :avatarUrl WHERE id = :userId AND deleted_at IS NULL;",
          PARAM(oatpp::Int64, userId),
          PARAM(oatpp::String, username),
          PARAM(oatpp::String, avatarUrl))

    // 修改密码
    QUERY(changePassword, 
          "UPDATE users SET password_hash = crypt(:newPasswordHash, gen_salt('bf')) WHERE id = :userId;",
          PARAM(oatpp::Int64, userId),
          PARAM(oatpp::String, newPasswordHash))

    // ==================== 好友相关 ====================
    // 检查好友关系状态
    QUERY(checkFriendshipStatus,
          "SELECT status FROM friendships WHERE user_id = :userId AND friend_id = :friendId;",
          PARAM(oatpp::Int64, userId),
          PARAM(oatpp::Int64, friendId))

    // 检查好友请求是否已存在（返回 UUID）
    QUERY(checkFriendRequestExists,
          "SELECT CAST(uuid AS VARCHAR) AS uuid FROM friend_requests WHERE (from_user_id = :fromUserId AND to_user_id = :toUserId) OR (from_user_id = :toUserId AND to_user_id = :fromUserId);",
          PARAM(oatpp::Int64, fromUserId),
          PARAM(oatpp::Int64, toUserId))

    // 重新发送好友请求 —— 仅当请求被拒绝/取消时才重置为 pending，已 pending 或已接受的跳过
    QUERY(updateFriendRequestToPending,
          "UPDATE friend_requests SET status = 'pending', message = :message, updated_at = NOW() WHERE ((from_user_id = :fromUserId AND to_user_id = :toUserId) OR (from_user_id = :toUserId AND to_user_id = :fromUserId)) AND status IN ('rejected', 'canceled') RETURNING CAST(uuid AS VARCHAR) AS uuid;",
          PARAM(oatpp::Int64, fromUserId),
          PARAM(oatpp::Int64, toUserId),
          PARAM(oatpp::String, message))

    // 发送好友请求
    QUERY(sendFriendRequest, 
          "INSERT INTO friend_requests (from_user_id, to_user_id, message) VALUES (:fromUserId, :toUserId, :message) RETURNING CAST(uuid AS VARCHAR) AS uuid;",
          PARAM(oatpp::Int64, fromUserId),
          PARAM(oatpp::Int64, toUserId),
          PARAM(oatpp::String, message))

    // 获取收到的好友请求
        QUERY(getReceivedFriendRequests,
            "SELECT "
            "  CAST(fr.uuid AS VARCHAR) AS uuid,"
            "  CAST(from_user.uuid AS VARCHAR) AS fromuseruuid, "
            "  CAST(to_user.uuid AS VARCHAR) AS touseruuid, "
            "  fr.status, "
            "  TO_CHAR(fr.created_at, 'YYYY-MM-DD HH24:MI:SS') AS createdat, "
            "  fr.message "
            "FROM friend_requests fr "
            "JOIN users from_user ON fr.from_user_id = from_user.id "
            "JOIN users to_user ON fr.to_user_id = to_user.id "
            "WHERE fr.to_user_id = :userId AND fr.status = 'pending' "
            "ORDER BY fr.created_at DESC;",
            PARAM(oatpp::Int64, userId))

    // 获取已发送的好友请求
    QUERY(getSentFriendRequests,
        "SELECT "
        "  CAST(fr.uuid AS VARCHAR) AS uuid,"
        "  CAST(u.uuid AS VARCHAR) AS fromuseruuid, "
        "  CAST(to_user.uuid AS VARCHAR) AS touseruuid, "
        "  fr.status, "
        "  TO_CHAR(fr.created_at, 'YYYY-MM-DD HH24:MI:SS') AS createdat, "
        "  fr.message "
        "FROM friend_requests fr "
        "JOIN users u ON fr.from_user_id = u.id "
        "JOIN users to_user ON fr.to_user_id = to_user.id " 
        "WHERE fr.from_user_id = :userId AND fr.status = 'pending' "
        "ORDER BY fr.created_at DESC;",
        PARAM(oatpp::Int64, userId))

        QUERY(getFriendRequestByUuid,
            "SELECT "
            "  CAST(fr.uuid AS VARCHAR) AS uuid, "
            "  CAST(from_user.uuid AS VARCHAR) AS fromuseruuid, "
            "  CAST(to_user.uuid AS VARCHAR) AS touseruuid, "
            "  fr.status, "
            "  TO_CHAR(fr.created_at, 'YYYY-MM-DD HH24:MI:SS') AS createdat, "
            "  fr.message "
            "FROM friend_requests fr "
            "JOIN users from_user ON fr.from_user_id = from_user.id "
            "JOIN users to_user ON fr.to_user_id = to_user.id "
            "WHERE fr.uuid = CAST(:uuid AS uuid);",
            PARAM(oatpp::String, uuid))

    // 更新好友请求状态 (with identity check and status guard)
    // For accept/reject: current user must be to_user_id
    // For cancel: current user must be from_user_id
    // Always check status = 'pending'
    QUERY(handleFriendRequest,
        "UPDATE friend_requests "
        "SET status = :status, updated_at = NOW() "
        "WHERE uuid = CAST(:requestUuid AS uuid) "
        "  AND status = 'pending' "
        "  AND ("
        "      (:status IN ('accepted', 'rejected') AND to_user_id = :currentUserId) "
        "      OR (:status = 'canceled' AND from_user_id = :currentUserId)"
        "  ) "
        "RETURNING from_user_id, to_user_id;",
        PARAM(oatpp::String, requestUuid),
        PARAM(oatpp::String, status),
        PARAM(oatpp::Int64, currentUserId))

    // 取消好友请求 (deprecated, use handleFriendRequest instead)
    QUERY(cancelFriendRequest, 
          "DELETE FROM friend_requests WHERE uuid = CAST(:requestId AS uuid);",
          PARAM(oatpp::String, requestId))

    // 旧的更新方法 (deprecated, use handleFriendRequest instead)
    QUERY(updateFriendRequestStatus,
        "UPDATE friend_requests SET status = :status, updated_at = NOW() WHERE uuid = CAST(:uuid AS uuid);",
        PARAM(oatpp::String, uuid),
        PARAM(oatpp::String, status))

    // 创建好友关系
    QUERY(createFriendship,
          "INSERT INTO friendships (user_id, friend_id, status) VALUES (:userId, :friendId, 'accepted'), (:friendId, :userId, 'accepted');",
          PARAM(oatpp::Int64, userId),
          PARAM(oatpp::Int64, friendId))

    // 更新好友关系状态（如果存在）
    QUERY(acceptFriendshipStatus,
          "UPDATE friendships SET status = 'accepted' WHERE (user_id = :userId AND friend_id = :friendId) OR (user_id = :friendId AND friend_id = :userId);",
          PARAM(oatpp::Int64, userId),
          PARAM(oatpp::Int64, friendId))

    // 获取两个用户之间的私聊会话ID
    QUERY(getPrivateConversationId,
          "SELECT id FROM conversations WHERE type = 'private' AND ((user1_id = :userId1 AND user2_id = :userId2) OR (user1_id = :userId2 AND user2_id = :userId1));",
          PARAM(oatpp::Int64, userId1),
          PARAM(oatpp::Int64, userId2))

    // 获取好友列表
    QUERY(getFriends, 
          "SELECT CAST(u.uuid AS VARCHAR) AS frienduuid, u.username, u.avatar_url AS avatarurl, u.status, f.remark, f.group_name AS groupname "
        " FROM friendships f "
        "JOIN users u ON f.friend_id = u.id "
        " WHERE f.user_id = :userId AND f.status = 'accepted' AND u.deleted_at IS NULL; ",
          PARAM(oatpp::Int64, userId))

    // 更新好友备注
    QUERY(updateFriendRemark, 
          "UPDATE friendships SET remark = :remark WHERE user_id = :userId AND friend_id = :friendId;",
          PARAM(oatpp::Int64, userId),
          PARAM(oatpp::Int64, friendId),
          PARAM(oatpp::String, remark))

    // 更新好友分组
    QUERY(updateFriendGroup, 
          "UPDATE friendships SET group_name = :groupName WHERE user_id = :userId AND friend_id = :friendId;",
          PARAM(oatpp::Int64, userId),
          PARAM(oatpp::Int64, friendId),
          PARAM(oatpp::String, groupName))

    // 删除好友
    QUERY(deleteFriend, 
          "UPDATE friendships SET status = 'deleted' WHERE (user_id = :userId AND friend_id = :friendId) OR (user_id = :friendId AND friend_id = :userId);",
          PARAM(oatpp::Int64, userId),
          PARAM(oatpp::Int64, friendId))

    // 拉黑用户
    QUERY(blockUser, 
          "UPDATE friendships SET status = 'blocked' WHERE user_id = :userId AND friend_id = :friendId;",
          PARAM(oatpp::Int64, userId),
          PARAM(oatpp::Int64, friendId))

    // 取消拉黑
    QUERY(unblockUser, 
          "UPDATE friendships SET status = 'accepted' WHERE user_id = :userId AND friend_id = :friendId;",
          PARAM(oatpp::Int64, userId),
          PARAM(oatpp::Int64, friendId))

    // ==================== 消息相关 ====================
// 发送私聊消息
QUERY(sendPrivateMessage,
    "INSERT INTO messages (from_user_id, to_user_id, message_type, content, file_url, file_size, file_name, mime_type) "
    "VALUES (:fromUserId, :toUserId, :messageType, :content, :fileUrl, :fileSize, :fileName, :mimeType) "
    "RETURNING "
    "  CAST(uuid AS VARCHAR) AS msguuid, "
    "  :messageType AS messagetype, "
    "  :content AS content, "
    "  :fileUrl AS fileurl, "
    "  :fileSize AS filesize, "
    "  :fileName AS filename, "
    "  :mimeType AS mimetype, "
    "  TO_CHAR(created_at, 'YYYY-MM-DD HH24:MI:SS') AS createdat; ",
    PARAM(oatpp::Int64, fromUserId),
    PARAM(oatpp::Int64, toUserId),
    PARAM(oatpp::String, messageType),
    PARAM(oatpp::String, content),
    PARAM(oatpp::String, fileUrl),
    PARAM(oatpp::Int64, fileSize),
    PARAM(oatpp::String, fileName),
    PARAM(oatpp::String, mimeType))

    // 获取私聊消息（拉取时自动将当前用户的会话 unread_count 归零）
    QUERY(getPrivateMessages,
        "WITH mark_read AS ("
        "  UPDATE conversations SET unread_count = 0, last_read_time = CURRENT_TIMESTAMP, updated_at = NOW() "
        "  WHERE user_id = :userId AND target_user_id = :friendId"
        ") "
        "SELECT "
        "  CAST(m.uuid AS VARCHAR) AS msguuid, "
        "  CAST(from_user.uuid AS VARCHAR) AS fromuseruuid, "
        "  CAST(to_user.uuid AS VARCHAR) AS touseruuid, "
        "  m.message_type AS messagetype, "
        "  m.content, "
        "  m.file_url AS fileurl, "
        "  m.file_size AS filesize, "
        "  m.file_name AS filename, "
        "  m.mime_type AS mimetype, "
        "  TO_CHAR(m.created_at, 'YYYY-MM-DD HH24:MI:SS') AS createdat "
        "FROM messages m "
        "JOIN users from_user ON m.from_user_id = from_user.id "
        "JOIN users to_user ON m.to_user_id = to_user.id "
        "WHERE (m.from_user_id = :userId AND m.to_user_id = :friendId) "
        "   OR (m.from_user_id = :friendId AND m.to_user_id = :userId) "
        "ORDER BY m.created_at DESC "
        "LIMIT :limit OFFSET :offset;",
        PARAM(oatpp::Int64, userId),
        PARAM(oatpp::Int64, friendId),
        PARAM(oatpp::Int32, limit),
        PARAM(oatpp::Int32, offset))

    // 获取消息详情（用于撤回时通知相关用户）
    QUERY(getMessageInfo,
          "SELECT "
          "  m.id, "
          "  CAST(m.uuid AS VARCHAR) AS msguuid, "
          "  m.from_user_id AS fromuserid, "
          "  CAST(from_user.uuid AS VARCHAR) AS fromuseruuid, "
          "  m.to_user_id AS touserid, "
          "  m.to_group_id AS togroupid, "
          "  m.message_type AS messagetype "
          "FROM messages m "
          "JOIN users from_user ON m.from_user_id = from_user.id "
          "WHERE m.id = :messageId;",
          PARAM(oatpp::Int64, messageId))

    // 撤回消息
    QUERY(recallMessage,
          "UPDATE messages SET message_type = 'recalled', content = '', file_url = NULL, file_size = NULL, file_name = NULL, mime_type = NULL WHERE id = :messageId AND from_user_id = :userId AND message_type != 'recalled';",
          PARAM(oatpp::Int64, messageId),
          PARAM(oatpp::Int64, userId))

    // ==================== 会话更新（发送消息时） ====================
    // 私聊会话：发送方自身，unread_count = 0（自己发出的消息不需要未读计数）
    QUERY(upsertConversationSenderPrivate,
          "INSERT INTO conversations (user_id, target_user_id, last_message_id, unread_count, created_at, updated_at) "
          "VALUES (:userId, :targetUserId, :messageId, 0, NOW(), NOW()) "
          "ON CONFLICT (user_id, target_user_id) "
          "DO UPDATE SET last_message_id = :messageId, unread_count = 0, updated_at = NOW()",
          PARAM(oatpp::Int64, userId),
          PARAM(oatpp::Int64, targetUserId),
          PARAM(oatpp::Int64, messageId))

    // 私聊会话：接收方，unread_count + 1（拉取消息时归零）
    QUERY(incrementConversationUnreadPrivate,
          "INSERT INTO conversations (user_id, target_user_id, last_message_id, unread_count, created_at, updated_at) "
          "VALUES (:userId, :targetUserId, :messageId, 1, NOW(), NOW()) "
          "ON CONFLICT (user_id, target_user_id) "
          "DO UPDATE SET last_message_id = :messageId, unread_count = conversations.unread_count + 1, updated_at = NOW()",
          PARAM(oatpp::Int64, userId),
          PARAM(oatpp::Int64, targetUserId),
          PARAM(oatpp::Int64, messageId))

    // 群聊会话：发送方自身，unread_count = 0
    QUERY(upsertConversationSenderGroup,
          "INSERT INTO conversations (user_id, target_group_id, last_message_id, unread_count, created_at, updated_at) "
          "VALUES (:userId, :groupId, :messageId, 0, NOW(), NOW()) "
          "ON CONFLICT (user_id, target_group_id) "
          "DO UPDATE SET last_message_id = :messageId, unread_count = 0, updated_at = NOW()",
          PARAM(oatpp::Int64, userId),
          PARAM(oatpp::Int64, groupId),
          PARAM(oatpp::Int64, messageId))

    // 群聊会话：接收方，unread_count + 1（拉取消息时归零）
    QUERY(incrementConversationUnreadGroup,
          "INSERT INTO conversations (user_id, target_group_id, last_message_id, unread_count, created_at, updated_at) "
          "VALUES (:userId, :groupId, :messageId, 1, NOW(), NOW()) "
          "ON CONFLICT (user_id, target_group_id) "
          "DO UPDATE SET last_message_id = :messageId, unread_count = conversations.unread_count + 1, updated_at = NOW()",
          PARAM(oatpp::Int64, userId),
          PARAM(oatpp::Int64, groupId),
          PARAM(oatpp::Int64, messageId))

    // 发送群消息
    QUERY(sendGroupMessage,
        "INSERT INTO messages (from_user_id, to_group_id, message_type, content, file_url, file_size, file_name, mime_type) "
        "VALUES (:fromUserId, :groupId, :messageType, :content, :fileUrl, :fileSize, :fileName, :mimeType) "
        "RETURNING "
        "  CAST(uuid AS VARCHAR) AS msguuid, "
        "  :messageType AS messagetype, "
        "  :content AS content, "
        "  :fileUrl AS fileurl, "
        "  :fileSize AS filesize, "
        "  :fileName AS filename, "
        "  :mimeType AS mimetype, "
        "  TO_CHAR(created_at, 'YYYY-MM-DD HH24:MI:SS') AS createdat; ",
        PARAM(oatpp::Int64, fromUserId),
        PARAM(oatpp::Int64, groupId),
        PARAM(oatpp::String, messageType),
        PARAM(oatpp::String, content),
        PARAM(oatpp::String, fileUrl),
        PARAM(oatpp::Int64, fileSize),
        PARAM(oatpp::String, fileName),
        PARAM(oatpp::String, mimeType))

    QUERY(getGroupMessages,
        "WITH mark_read AS ("
        "  UPDATE conversations SET unread_count = 0, last_read_time = CURRENT_TIMESTAMP, updated_at = NOW() "
        "  WHERE user_id = :userId AND target_group_id = :groupId"
        ") "
        "SELECT "
        "  CAST(m.uuid AS VARCHAR) AS msguuid, "
        "  CAST(from_user.uuid AS VARCHAR) AS fromuseruuid, "
        "  CAST(g.uuid AS VARCHAR) AS groupuuid, "
        "  m.message_type AS messagetype, "
        "  m.content, "
        "  m.file_url AS fileurl, "
        "  m.file_size AS filesize, "
        "  m.file_name AS filename, "
        "  m.mime_type AS mimetype, "
        "  TO_CHAR(m.created_at, 'YYYY-MM-DD HH24:MI:SS') AS createdat "
        "FROM messages m "
        "JOIN users from_user ON m.from_user_id = from_user.id "
        "JOIN groups g ON m.to_group_id = g.id "
        "WHERE m.to_group_id = :groupId "
        "ORDER BY m.id DESC "
        "LIMIT :limit OFFSET :offset;",
        PARAM(oatpp::Int64, userId),
        PARAM(oatpp::Int64, groupId),
        PARAM(oatpp::Int32, limit),
        PARAM(oatpp::Int32, offset))

        // 处理群聊请求（通过/拒绝）
        QUERY(handleGroupRequest,
            "UPDATE group_requests SET status = :status, reviewer_id = :reviewerId, reviewed_at = NOW() WHERE uuid = CAST(:requestUuid AS uuid) AND status = 'pending';",
            PARAM(oatpp::String, requestUuid),
            PARAM(oatpp::String, status),
            PARAM(oatpp::Int64, reviewerId))

    // 获取群聊请求详情
    QUERY(getGroupRequestByUuid,
        "SELECT id, group_id AS groupid, requester_id AS requesterid FROM group_requests WHERE uuid = CAST(:requestUuid AS uuid);",
        PARAM(oatpp::String, requestUuid))

    // 添加群成员（带冲突处理）
    QUERY(addGroupMemberWithConflict, 
          "INSERT INTO group_members (group_id, user_id, role) VALUES (:groupId, :userId, :role) ON CONFLICT (group_id, user_id) DO NOTHING;",
          PARAM(oatpp::Int64, groupId),
          PARAM(oatpp::Int64, userId),
          PARAM(oatpp::String, role))

    // 检查群聊请求是否已存在
    QUERY(checkGroupRequestExists,
        "SELECT CAST(uuid AS VARCHAR) AS uuid, status FROM group_requests WHERE group_id = :groupId AND requester_id = :requesterId;",
        PARAM(oatpp::Int64, groupId),
        PARAM(oatpp::Int64, requesterId))

    // 重新发送群聊请求 —— 仅当请求被拒绝/取消时才重置为 pending
    QUERY(updateGroupRequestToPending,
        "UPDATE group_requests SET status = 'pending', message = :message, updated_at = NOW() WHERE group_id = :groupId AND requester_id = :requesterId AND status IN ('rejected', 'canceled') RETURNING CAST(uuid AS VARCHAR) AS uuid;",
        PARAM(oatpp::Int64, groupId),
        PARAM(oatpp::Int64, requesterId),
        PARAM(oatpp::String, message))

        // 发送群聊请求
        QUERY(sendGroupRequest,
            "INSERT INTO group_requests (group_id, requester_id, message) VALUES (:groupId, :requesterId, :message);",
            PARAM(oatpp::Int64, groupId),
            PARAM(oatpp::Int64, requesterId),
            PARAM(oatpp::String, message))

    // ==================== 群组相关 ====================
    // 创建群组
    QUERY(createGroup,
        "INSERT INTO groups (name, description, avatar_url, owner_id, max_members, is_public) "
        "VALUES (:name, :description, :avatarUrl, :ownerId, :maxMembers, :isPublic) "
        "RETURNING "
        "  CAST(uuid AS VARCHAR) AS uuid, "
        "  :name AS name, "
        "  :description AS description, "
        "  :avatarUrl AS avatarurl, "
        "  (SELECT CAST(uuid AS VARCHAR) FROM users WHERE id = :ownerId) AS owneruuid, "
        "  1 AS membercount, "
        "  TO_CHAR(created_at, 'YYYY-MM-DD HH24:MI:SS') AS createdat;",
        PARAM(oatpp::String, name),
        PARAM(oatpp::String, description),
        PARAM(oatpp::String, avatarUrl),
        PARAM(oatpp::Int64, ownerId),
        PARAM(oatpp::Int32, maxMembers),
        PARAM(oatpp::Boolean, isPublic))

    // 添加群成员
    QUERY(addGroupMember, 
          "INSERT INTO group_members (group_id, user_id, role) VALUES (:groupId, :userId, :role);",
          PARAM(oatpp::Int64, groupId),
          PARAM(oatpp::Int64, userId),
          PARAM(oatpp::String, role))

    QUERY(checkUserInGroup,
        "SELECT 1 FROM group_members WHERE group_id = :groupId AND user_id = :userId;",
        PARAM(oatpp::Int64, groupId),
        PARAM(oatpp::Int64, userId))

    // 获取我的群组
        QUERY(getMyGroups,
            "SELECT "
            "  CAST(g.uuid AS VARCHAR) AS uuid, "
            "  g.name, "
            "  g.description, "
            "  g.avatar_url AS avatarurl, "
            "  CAST(u.uuid AS VARCHAR) AS owneruuid, "
            "  (SELECT COUNT(*) FROM group_members WHERE group_id = g.id) AS membercount, "
            "  TO_CHAR(g.created_at, 'YYYY-MM-DD HH24:MI:SS') AS createdat "
            "FROM group_members gm "
            "JOIN groups g ON gm.group_id = g.id "
            "JOIN users u ON g.owner_id = u.id "
            "WHERE gm.user_id = :userId AND g.deleted_at IS NULL "
            "ORDER BY g.created_at DESC;",
            PARAM(oatpp::Int64, userId))

    // 获取群组详情
    QUERY(getGroupDetail,
        "SELECT "
        "  CAST(g.uuid AS VARCHAR) AS uuid, "
        "  g.name, "
        "  g.description, "
        "  g.avatar_url AS avatarurl, "
        "  (SELECT COUNT(*) FROM group_members WHERE group_id = g.id) AS membercount, "
        "  g.max_members AS maxmembers, "
        "  TO_CHAR(g.created_at, 'YYYY-MM-DD HH24:MI:SS') AS createdat "
        "FROM groups g "
        "WHERE g.id = :groupId AND g.deleted_at IS NULL;",
        PARAM(oatpp::Int64, groupId))

    //QUERY(checkUserInGroup,
    //    "SELECT COUNT(*) FROM group_members WHERE group_id = :groupId AND user_id = :userId;",
    //    PARAM(oatpp::Int64, groupId),
    //    PARAM(oatpp::Int64, userId))
    // 更新群组信息
    QUERY(updateGroup, 
          "UPDATE groups SET name = :name, description = :description, avatar_url = :avatarUrl, max_members = :maxMembers, is_public = :isPublic "
           "WHERE id = :groupId AND owner_id = :userId;",
          PARAM(oatpp::Int64, groupId),
          PARAM(oatpp::Int64, userId),
          PARAM(oatpp::String, name),
          PARAM(oatpp::String, description),
          PARAM(oatpp::String, avatarUrl),
          PARAM(oatpp::Int32, maxMembers),
          PARAM(oatpp::Boolean, isPublic))

    // 解散群组
    QUERY(dissolveGroup, 
          "UPDATE groups SET deleted_at = CURRENT_TIMESTAMP WHERE id = :groupId AND owner_id = :userId;",
          PARAM(oatpp::Int64, groupId),
          PARAM(oatpp::Int64, userId))

            // 获取群成员
            QUERY(getGroupMembers,
                "SELECT "
                "  CAST(u.uuid AS VARCHAR) AS useruuid, "
                "  u.username, "
                "  u.avatar_url AS avatarurl, "
                "  gm.role, "
                "  TO_CHAR(gm.joined_at, 'YYYY-MM-DD HH24:MI:SS') AS joinedat "
                "FROM group_members gm "
                "JOIN users u ON gm.user_id = u.id "
                "WHERE gm.group_id = :groupId "
                "ORDER BY "
                "  CASE gm.role "
                "    WHEN 'owner' THEN 1 "
                "    WHEN 'admin' THEN 2 "
                "    ELSE 3 "
                "  END;",
                PARAM(oatpp::Int64, groupId))

    // 移除群成员 (with embedded permission check to avoid TOCTOU)
    // Owner can remove anyone; admin can only remove members
    QUERY(removeGroupMember,
          "DELETE FROM group_members "
          "WHERE group_id = :groupId AND user_id = :targetUserId "
          "AND EXISTS ("
          "    SELECT 1 FROM group_members AS gm "
          "    WHERE gm.group_id = :groupId AND gm.user_id = :currentUserId "
          "    AND ("
          "        gm.role = 'owner' "
          "        OR (gm.role = 'admin' AND EXISTS ("
          "            SELECT 1 FROM group_members "
          "            WHERE group_id = :groupId AND user_id = :targetUserId AND role = 'member'"
          "        ))"
          "    )"
          ");",
          PARAM(oatpp::Int64, groupId),
          PARAM(oatpp::Int64, targetUserId),
          PARAM(oatpp::Int64, currentUserId))

    // 可选：删除成员的会话记录
    QUERY(deleteConversationForGroupMember,
        "DELETE FROM conversations WHERE user_id = :userId AND target_group_id = :groupId;",
        PARAM(oatpp::Int64, groupId),
        PARAM(oatpp::Int64, userId))
    // 设置成员角色 (RETURNING 1 to check affected rows)
    QUERY(setMemberRole, 
          "UPDATE group_members SET role = :role WHERE group_id = :groupId AND user_id = :userId RETURNING 1;",
          PARAM(oatpp::Int64, groupId),
          PARAM(oatpp::Int64, userId),
          PARAM(oatpp::String, role))

    // 退出群组
    QUERY(leaveGroup, 
          "DELETE FROM group_members WHERE group_id = :groupId AND user_id = :userId;",
          PARAM(oatpp::Int64, groupId),
          PARAM(oatpp::Int64, userId))

    // ==================== 会话相关 ====================
    // 获取会话列表
    QUERY(getConversations,
        "SELECT "
        "  CASE "
        "    WHEN c.target_user_id IS NOT NULL THEN 'private' "
        "    ELSE 'group' "
        "  END AS type, "
        "  COALESCE( "
        "    CAST(u.uuid AS VARCHAR), "
        "    CAST(g.uuid AS VARCHAR) "
        "  ) AS targetid, "
        "  COALESCE(u.username, g.name) AS targetname, "
        "  COALESCE(u.avatar_url, g.avatar_url) AS targetavatar, "
        "  m.content AS lastmessage, "
        "  m.message_type AS lastmessagetype, "
        "  TO_CHAR(m.created_at, 'YYYY-MM-DD HH24:MI:SS') AS lastmessagetime, "
        "  c.unread_count AS unreadcount, "
        "  COALESCE(gm.is_muted, false) AS ismuted "
        "FROM conversations c "
        "LEFT JOIN users u ON c.target_user_id = u.id AND u.deleted_at IS NULL "
        "LEFT JOIN groups g ON c.target_group_id = g.id AND g.deleted_at IS NULL "
        "LEFT JOIN messages m ON c.last_message_id = m.id "
        "LEFT JOIN group_members gm ON gm.group_id = c.target_group_id AND gm.user_id = c.user_id "
        "WHERE c.user_id = :userId "
        "ORDER BY c.updated_at DESC;",
        PARAM(oatpp::Int64, userId))

    // 获取会话详情
    QUERY(getConversationDetail, 
          "SELECT CAST(uuid AS VARCHAR) AS uuid, type, name, avatar_url AS avatarurl, TO_CHAR(created_at, 'YYYY-MM-DD HH24:MI:SS') AS createdat "
        "  FROM conversations "
           "WHERE id = :conversationId;",
          PARAM(oatpp::Int64, conversationId))

        // 创建单聊会话
    QUERY(createPrivateConversation,
        "INSERT INTO conversations (user_id, target_user_id, created_at, updated_at) "
        "VALUES (:userId, :targetUserId, NOW(), NOW()) "
        "ON CONFLICT (user_id, target_user_id) DO UPDATE SET updated_at = NOW() ;",//加了个分号和逗号，如果还要下面的部分记得删除
        //"RETURNING "
        //"  'private' AS type, "
        //"  (SELECT CAST(uuid AS VARCHAR) FROM users WHERE id = :targetUserId) AS targetUuid, "
        //"  (SELECT username FROM users WHERE id = :targetUserId) AS targetName, "
        //"  (SELECT avatar_url FROM users WHERE id = :targetUserId) AS targetAvatar, "
        //"  NULL AS lastMessage, "
        //"  NULL AS lastMessageType, "
        //"  NULL AS lastMessageTime, "
        //"  0 AS unreadCount, "
        //"  false AS isMuted;",
        PARAM(oatpp::Int64, userId),
        PARAM(oatpp::Int64, targetUserId))

    // 创建群聊会话（为单个成员，返回 uuid）
    QUERY(createGroupConversation,
        "INSERT INTO conversations (user_id, target_group_id, created_at, updated_at) "
        "VALUES (:userId, :groupId, NOW(), NOW()) "
        "ON CONFLICT (user_id, target_group_id) DO UPDATE SET updated_at = NOW() ;",//加了个分号和逗号，如果还要下面的部分记得删除
        //"RETURNING "
        //"  'group' AS type, "
        //"  (SELECT CAST(uuid AS VARCHAR) FROM groups WHERE id = :groupId) AS targetUuid, "
        //"  (SELECT name FROM groups WHERE id = :groupId) AS targetName, "
        //"  (SELECT avatar_url FROM groups WHERE id = :groupId) AS targetAvatar, "
        //"  NULL AS lastMessage, "
        //"  NULL AS lastMessageType, "
        //"  NULL AS lastMessageTime, "
        //"  0 AS unreadCount, "
        //"  false AS isMuted;",
        PARAM(oatpp::Int64, userId),
        PARAM(oatpp::Int64, groupId))

        QUERY(getConversationByid,
            "SELECT "
            "  c.id, "
            "  c.user_id AS userId, "
            "  CASE "
            "    WHEN c.target_user_id IS NOT NULL THEN 'private' "
            "    ELSE 'group' "
            "  END AS type, "
            "  COALESCE(c.target_user_id, c.target_group_id) AS targetId "
            "FROM conversations c "
            "WHERE c.id = :id;",
            PARAM(oatpp::Int64, id))

    //QUERY(createConversation, 
    //      "INSERT INTO conversations (type, name, avatar_url) VALUES (:type, :name, :avatarUrl) RETURNING id;",
    //      PARAM(oatpp::String, type),
    //      PARAM(oatpp::String, name),
    //      PARAM(oatpp::String, avatarUrl))

    // 添加会话成员
    //QUERY(addConversationMember, 
    //      "INSERT INTO conversation_members (conversation_id, user_id) VALUES (:conversationId, :userId);",
    //      PARAM(oatpp::Int64, conversationId),
    //      PARAM(oatpp::Int64, userId))

    // 获取群聊会话ID
    QUERY(getGroupConversationId,
        "SELECT id FROM conversations WHERE type = 'group' AND group_id = :groupId AND user_id = :userId;",
        PARAM(oatpp::Int64, groupId),
        PARAM(oatpp::Int64, userId))
    // 更新会话信息
    QUERY(updateConversation, 
          "UPDATE conversations SET name = :name, avatar_url = :avatarUrl WHERE id = :conversationId;",
          PARAM(oatpp::Int64, conversationId),
          PARAM(oatpp::String, name),
          PARAM(oatpp::String, avatarUrl))

    // 删除会话
    QUERY(deleteConversation, 
          "DELETE FROM conversations WHERE id = :conversationId;",
          PARAM(oatpp::Int64, conversationId))

    // 移除会话成员
    QUERY(removeConversationMember, 
          "DELETE FROM conversation_members WHERE conversation_id = :conversationId AND user_id = :userId;",
          PARAM(oatpp::Int64, conversationId),
          PARAM(oatpp::Int64, userId))


    QUERY(getUserRoleInGroup,
        "SELECT role FROM group_members WHERE group_id = :groupId AND user_id = :userId;",
        PARAM(oatpp::Int64, groupId),
        PARAM(oatpp::Int64, userId))
    // 添加多个会话成员
    //QUERY(addConversationMembers, 
    //      "INSERT INTO conversation_members (conversation_id, user_id) VALUES (:conversationId, :userId);",
    //      PARAM(oatpp::Int64, conversationId),
    //      PARAM(oatpp::Int64, userId))

// 标记会话已读
    QUERY(markConversationRead,
        "UPDATE conversations SET last_read_time = CURRENT_TIMESTAMP, unread_count = 0 "
        "WHERE id = :conversationId AND user_id = :userId;",
        PARAM(oatpp::Int64, conversationId),
        PARAM(oatpp::Int64, userId))

    // 标记会话已读（私聊）
    QUERY(markPrivateConversationRead,
        "UPDATE conversations SET last_read_time = CURRENT_TIMESTAMP, unread_count = 0 "
        "WHERE user_id = :userId AND target_user_id = :targetUserId;",
        PARAM(oatpp::Int64, userId),
        PARAM(oatpp::Int64, targetUserId))

    // 标记会话已读（群聊）
    QUERY(markGroupConversationRead,
        "UPDATE conversations SET last_read_time = CURRENT_TIMESTAMP, unread_count = 0 "
        "WHERE user_id = :userId AND target_group_id = :targetGroupId;",
        PARAM(oatpp::Int64, userId),
        PARAM(oatpp::Int64, targetGroupId))

    // 消息免打扰（私聊）
    QUERY(mutePrivateConversation,
        "UPDATE conversations SET is_muted = true WHERE user_id = :userId AND target_user_id = :targetUserId;",
        PARAM(oatpp::Int64, userId),
        PARAM(oatpp::Int64, targetUserId))

    // 消息免打扰（群聊）
    QUERY(muteGroupConversation,
        "UPDATE conversations SET is_muted = true WHERE user_id = :userId AND target_group_id = :targetGroupId;",
        PARAM(oatpp::Int64, userId),
        PARAM(oatpp::Int64, targetGroupId))

    // 取消消息免打扰（私聊）
    QUERY(unmutePrivateConversation,
        "UPDATE conversations SET is_muted = false WHERE user_id = :userId AND target_user_id = :targetUserId;",
        PARAM(oatpp::Int64, userId),
        PARAM(oatpp::Int64, targetUserId))

    // 取消消息免打扰（群聊）
    QUERY(unmuteGroupConversation,
        "UPDATE conversations SET is_muted = false WHERE user_id = :userId AND target_group_id = :targetGroupId;",
        PARAM(oatpp::Int64, userId),
        PARAM(oatpp::Int64, targetGroupId))

    // 获取会话成员
    //QUERY(getConversationMembers, 
    //      "SELECT cm.id, u.id as userId, u.username, u.avatar_url AS avatarUrl, cm.role, cm.joined_at AS joinedAt "
    //    "FROM conversation_members cm "
    //    "JOIN users u ON cm.user_id = u.id "
    //    "WHERE cm.conversation_id = :conversationId;",
    //      PARAM(oatpp::Int64, conversationId))

    //// 退出会话
    //QUERY(leaveConversation, 
    //      "DELETE FROM conversation_members WHERE conversation_id = :conversationId AND user_id = :userId;",
    //      PARAM(oatpp::Int64, conversationId),
    //      PARAM(oatpp::Int64, userId))

    // ==================== 文件相关 ====================
    // 上传文件
    QUERY(uploadFile, 
          "INSERT INTO files (file_name, file_size, mime_type, file_path, file_type, user_id) "
           "VALUES(:fileName, :fileSize, :mimeType, :fileUrl, :fileType, :userId) RETURNING id, file_name AS fileName, file_size AS fileSize, mime_type AS mimeType, file_path AS fileUrl, user_id AS userId, TO_CHAR(created_at, 'YYYY-MM-DD HH24:MI:SS') AS createdAt; ",
          PARAM(oatpp::String, fileName),
          PARAM(oatpp::Int64, fileSize),
          PARAM(oatpp::String, mimeType),
          PARAM(oatpp::String, fileUrl),
          PARAM(oatpp::String, fileType),
          PARAM(oatpp::Int64, userId))

    // 获取文件列表
    QUERY(getFileList, 
          "SELECT id, file_name AS fileName, file_type AS fileType, file_size AS fileSize, file_path AS fileUrl, user_id AS uploaderId, TO_CHAR(created_at, 'YYYY-MM-DD HH24:MI:SS') AS createdAt "
        "FROM files "
        "WHERE user_id = :userId "
        " ORDER BY created_at DESC;",
          PARAM(oatpp::Int64, userId))

    // 获取文件详情
    QUERY(getFileDetail, 
          "SELECT id, file_name AS fileName, file_type AS fileType, file_size AS fileSize, file_path AS fileUrl, user_id AS uploaderId, TO_CHAR(created_at, 'YYYY-MM-DD HH24:MI:SS') AS createdAt "
        "FROM files "
           "WHERE id = :id;",
          PARAM(oatpp::Int64, id))

    // 删除文件
    QUERY(deleteFile, 
          "DELETE FROM files WHERE id = :fileId AND user_id = :userId;",
          PARAM(oatpp::Int64, fileId),
          PARAM(oatpp::Int64, userId))

    // 搜索文件
    QUERY(searchFiles, 
          "SELECT id, file_name AS fileName, file_type AS fileType, file_size AS fileSize, file_path AS fileUrl, user_id AS uploaderId, TO_CHAR(created_at, 'YYYY-MM-DD HH24:MI:SS') AS createdAt "
        " FROM files "
        "  WHERE user_id = :userId AND file_name LIKE :keyword "
          " ORDER BY created_at DESC;",
          PARAM(oatpp::String, keyword),
          PARAM(oatpp::Int64, userId))

    // 获取文件统计
    QUERY(getFileStatistics, 
          "SELECT COUNT(*) AS totalFiles, SUM(file_size) AS totalSize "
        " FROM files "
           "WHERE user_id = :userId;",
          PARAM(oatpp::Int64, userId))

    // ==================== 通知相关 ====================
    // 获取通知列表
    QUERY(getNotifications, 
          "SELECT id, user_id AS userId, type, title, content, is_read AS isRead, TO_CHAR(created_at, 'YYYY-MM-DD HH24:MI:SS') AS createdAt "
        " FROM notifications "
        "WHERE user_id = :userId "
           "ORDER BY created_at DESC;",
          PARAM(oatpp::Int64, userId))

    // 获取未读通知数量
    QUERY(getUnreadCount, 
          "SELECT COUNT(*) AS count "
        " FROM notifications "
          " WHERE user_id = :userId AND is_read = false;",
          PARAM(oatpp::Int64, userId))

    // 标记通知为已读
    QUERY(markNotificationRead, 
          "UPDATE notifications SET is_read = true WHERE id = :notificationId AND user_id = :userId;",
          PARAM(oatpp::Int64, notificationId),
          PARAM(oatpp::Int64, userId))

    // 标记所有通知为已读
    QUERY(markAllNotificationsRead, 
          "UPDATE notifications SET is_read = true WHERE user_id = :userId;",
          PARAM(oatpp::Int64, userId))

    // 删除通知
    QUERY(deleteNotification, 
          "DELETE FROM notifications WHERE id = :notificationId AND user_id = :userId;",
          PARAM(oatpp::Int64, notificationId),
          PARAM(oatpp::Int64, userId))

    // 清空通知
    QUERY(clearNotifications, 
          "DELETE FROM notifications WHERE user_id = :userId;",
          PARAM(oatpp::Int64, userId))

    // ==================== 状态相关 ====================
    // 获取用户状态
    QUERY(getUserStatus,
        "SELECT CAST(uuid AS VARCHAR) AS userId, status, TO_CHAR(last_login, 'YYYY-MM-DD HH24:MI:SS') AS lastSeen "
        "FROM users "
        "WHERE id = :userId;",
        PARAM(oatpp::Int64, userId))
    // 获取好友状态
    //QUERY(getFriendsStatus, 
    //      "SELECT u.id, u.status, TO_CHAR(u.last_login, 'YYYY-MM-DD HH24:MI:SS') AS lastSeen "
    //    "FROM friendships f"
    //    "JOIN users u ON f.friend_id = u.id "
    //       "WHERE f.user_id = :userId AND f.status = 'accepted';",
    //      PARAM(oatpp::Int64, userId))

    //// 获取在线用户
    //QUERY(getOnlineUsers, 
    //      "SELECT CAST(uuid AS VARCHAR) AS userUuid, username, avatar_url AS avatarUrl, status, TO_CHAR(last_login, 'YYYY-MM-DD HH24:MI:SS') AS lastSeen "
    //    "FROM users "
    //       "WHERE status = 'online' AND deleted_at IS NULL;")

    // 更新用户状态
    QUERY(updateUserStatus,
        "UPDATE users SET status = :status, updated_at = CURRENT_TIMESTAMP WHERE id = :userId;",
        PARAM(oatpp::Int64, userId),
        PARAM(oatpp::String, status))

    //==================其他=====================

        // 根据用户UUID获取用户ID
        QUERY(getUserIdByUuid,
            "SELECT id FROM users WHERE uuid = CAST(:uuid AS uuid) AND deleted_at IS NULL;",
            PARAM(oatpp::String, uuid))
        // 根据用户ID获取用户UUID
        QUERY(getUserUuidById,
            "SELECT uuid FROM users WHERE id = :id AND deleted_at IS NULL;",
            PARAM(oatpp::Int64, id))

        // 根据群组UUID获取群组ID
        QUERY(getGroupIdByUuid,
            "SELECT id FROM groups WHERE uuid = CAST(:uuid AS uuid) AND deleted_at IS NULL;",
            PARAM(oatpp::String, uuid))

        // 根据群组ID获取群组UUID
        QUERY(getGroupUuidById,
            "SELECT uuid FROM groups WHERE id = :id AND deleted_at IS NULL;",
            PARAM(oatpp::Int64, id))

        // 根据会话UUID获取会话ID（根据参数判断是私聊还是群聊）
            QUERY(getConversationId,
                "SELECT id FROM conversations "
                "WHERE user_id = :userId "
                "  AND (target_user_id = :targetId OR target_group_id = :targetId);",
                PARAM(oatpp::Int64, userId),
                PARAM(oatpp::Int64, targetId))

        // 根据消息UUID获取消息ID
        QUERY(getMessageIdByUuid,
            "SELECT id FROM messages WHERE uuid = CAST(:uuid AS uuid);",
            PARAM(oatpp::String, uuid))

        // 根据文件UUID获取文件ID
        QUERY(getFileIdByUuid,
            "SELECT id FROM files WHERE uuid = CAST(:uuid AS uuid);",
            PARAM(oatpp::String, uuid))
};

#include OATPP_CODEGEN_END(DbClient)