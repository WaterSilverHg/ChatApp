#pragma once

#include "global.h"

#include OATPP_CODEGEN_BEGIN(DTO) 

// 好友请求
class FriendResponseVO : public oatpp::DTO {
    DTO_INIT(FriendResponseVO, DTO)
    DTO_FIELD(String, uuid, "uuid");
    DTO_FIELD(String, fromUserUuid, "fromuseruuid");
    DTO_FIELD(String, toUserUuid, "touseruuid");
    DTO_FIELD(String, status, "status"); // pending, accepted, rejected
    DTO_FIELD(String, createdAt, "createdat");
    DTO_FIELD(String, message, "message");
};

// 好友信息
class FriendInfoVO : public oatpp::DTO {
    DTO_INIT(FriendInfoVO, DTO)
    DTO_FIELD(String, friendUuid, "frienduuid");
    DTO_FIELD(String, username, "username");
    DTO_FIELD(String, avatarUrl, "avatarurl");
    DTO_FIELD(String, remark, "remark");
    DTO_FIELD(String, groupName, "groupname");
    DTO_FIELD(String, status, "status"); // online, offline
    DTO_FIELD(String, createdAt, "createdat");
};
#include OATPP_CODEGEN_END(DTO) 
