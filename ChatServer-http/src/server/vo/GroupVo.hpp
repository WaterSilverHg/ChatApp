#pragma once

#include "global.h"
#include "UserStatusVo.hpp"

#include OATPP_CODEGEN_BEGIN(DTO) 

// 群组信息
class GroupInfoVO : public oatpp::DTO {
    DTO_INIT(GroupInfoVO, DTO)
    DTO_FIELD(String, uuid, "uuid");
    DTO_FIELD(String, name, "name");
    DTO_FIELD(String, description, "description");
    DTO_FIELD(String, avatarUrl, "avatarurl");
    DTO_FIELD(String, ownerUuid, "owneruuid");
    DTO_FIELD(Int32, memberCount, "membercount");
    DTO_FIELD(Boolean, isJoined, "isjoined");
    DTO_FIELD(String, createdAt, "createdat");
};

class GroupDetailInfoVO : public oatpp::DTO {
    DTO_INIT(GroupDetailInfoVO, DTO)

        DTO_FIELD(String, uuid, "uuid");
    DTO_FIELD(String, name, "name");
    DTO_FIELD(String, description, "description");
    DTO_FIELD(String, avatarUrl, "avatarurl");
    DTO_FIELD(Int32, memberCount, "membercount");
    DTO_FIELD(Int32, maxMembers, "maxmembers");
    DTO_FIELD(String, role, "role");
    DTO_FIELD(Boolean, isMuted, "ismuted");
    DTO_FIELD(String, createdAt, "createdat");
};


// 群成员信息
class GroupMemberVO : public oatpp::DTO {
    DTO_INIT(GroupMemberVO, DTO)
    DTO_FIELD(String, userUuid, "useruuid");
    DTO_FIELD(String, username, "username");
    DTO_FIELD(String, avatarUrl, "avatarurl");
    DTO_FIELD(String, role, "role"); // owner, admin, member
    DTO_FIELD(String, joinedAt, "joinedat");
};

// 收到的群聊请求 VO
class ReceivedGroupRequestVO : public oatpp::DTO {
    DTO_INIT(ReceivedGroupRequestVO, DTO)
    DTO_FIELD(String, uuid, "uuid");
    DTO_FIELD(String, groupUuid, "groupuuid");
    DTO_FIELD(String, groupName, "groupname");
    DTO_FIELD(Object<UserInfoVO>, requester, "requester"); // 请求者信息
    DTO_FIELD(String, message, "message");
    DTO_FIELD(String, status, "status");
    DTO_FIELD(String, createdAt, "createdat");
};

// 发送的群聊请求 VO
class SentGroupRequestVO : public oatpp::DTO {
    DTO_INIT(SentGroupRequestVO, DTO)
    DTO_FIELD(String, uuid, "uuid");
    DTO_FIELD(Object<GroupInfoVO>, group, "group"); // 目标群组信息
    DTO_FIELD(String, message, "message");
    DTO_FIELD(String, status, "status");
    DTO_FIELD(String, createdAt, "createdat");
};

#include OATPP_CODEGEN_END(DTO) 
