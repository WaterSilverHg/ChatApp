#pragma once

#include "global.h"

#include OATPP_CODEGEN_BEGIN(DTO) 


// 创建群组请求
class CreateGroupRequestDTO : public oatpp::DTO {
    DTO_INIT(CreateGroupRequestDTO, DTO)
    DTO_FIELD(String, name, "name");
    DTO_FIELD(String, description, "description");
    DTO_FIELD(String, avatarUrl, "avatarurl");
    DTO_FIELD(Int32, maxMembers, "maxmembers");
    DTO_FIELD(Boolean, isPublic, "ispublic");
    DTO_FIELD(oatpp::Vector<String>, memberUuids, "memberuuids");
};

// 更新群组信息请求
class UpdateGroupRequestDTO : public oatpp::DTO {
    DTO_INIT(UpdateGroupRequestDTO, DTO)
        DTO_FIELD(String, groupUuid, "groupuuid");
    DTO_FIELD(String, name, "name");
    DTO_FIELD(String, description, "description");
    DTO_FIELD(String, avatarUrl, "avatarurl");
    DTO_FIELD(Int32, maxMembers, "maxmembers");
    DTO_FIELD(Boolean, isPublic, "ispublic");
};


// 添加群成员请求
class AddGroupMemberRequestDTO : public oatpp::DTO {
    DTO_INIT(AddGroupMemberRequestDTO, DTO)
        DTO_FIELD(String, groupUuid, "groupuuid");
    DTO_FIELD(oatpp::Vector<String>, userUuids, "useruuids");
};

// 设置群成员角色请求
class SetMemberRoleRequestDTO : public oatpp::DTO {
    DTO_INIT(SetMemberRoleRequestDTO, DTO)
        DTO_FIELD(String, groupUuid, "groupuuid");
    DTO_FIELD(String, targetUserUuid, "targetuseruuid");
    DTO_FIELD(String, role, "role");
};

class UserRoleDTO : public oatpp::DTO {
    DTO_INIT(UserRoleDTO, DTO)
        DTO_FIELD(String, role, "role");
};

//// 发送群聊请求（HTTP）
//class SendGroupRequestDTO : public oatpp::DTO {
//    DTO_INIT(SendGroupRequestDTO, DTO)
//        DTO_FIELD(String, message, "message");
//};

// WebSocket 发送群聊请求（包含 groupUuid）
class SendGroupRequestDTO : public oatpp::DTO {
    DTO_INIT(SendGroupRequestDTO, DTO)
        DTO_FIELD(String, groupUuid, "groupuuid");
    DTO_FIELD(String, message, "message");
};

// 处理群聊请求
class HandleGroupRequestDTO : public oatpp::DTO {
    DTO_INIT(HandleGroupRequestDTO, DTO)
        DTO_FIELD(String, grUuid, "gruuid");
    DTO_FIELD(String, status, "status"); // approved, rejected
};

// 群聊请求响应 DTO
class GroupRequestResponseDTO : public oatpp::DTO {
    DTO_INIT(GroupRequestResponseDTO, DTO)
        DTO_FIELD(String, uuid, "uuid");
    DTO_FIELD(String, groupUuid, "groupuuid");
    DTO_FIELD(String, groupName, "groupname");
    DTO_FIELD(String, requesterUuid, "requesteruuid");
    DTO_FIELD(String, requesterName, "requestername");
    DTO_FIELD(String, requesterAvatar, "requesteravatar");
    DTO_FIELD(String, groupAvatar, "groupavatar");
    DTO_FIELD(String, message, "message");
    DTO_FIELD(String, status, "status");
    DTO_FIELD(String, createdAt, "createdat");
};

// 群聊请求详情 DTO（内部使用）
class GroupRequestDetailDTO : public oatpp::DTO {
    DTO_INIT(GroupRequestDetailDTO, DTO)
        DTO_FIELD(Int64, id, "id");
    DTO_FIELD(Int64, groupId, "groupid");
    DTO_FIELD(Int64, requesterId, "requesterid");
};

#include OATPP_CODEGEN_END(DTO) 
