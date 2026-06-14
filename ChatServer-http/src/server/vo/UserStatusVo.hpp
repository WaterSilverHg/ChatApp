#pragma once

#include "global.h"

#include OATPP_CODEGEN_BEGIN(DTO) 

// 用户在线状态
class UserStatusVO : public oatpp::DTO {
    DTO_INIT(UserStatusVO, DTO)
    DTO_FIELD(String, userId, "userid");
    DTO_FIELD(String, status, "status"); // online, offline, away, busy
    DTO_FIELD(String, lastSeen, "lastseen");
};

// 用户信息
class UserInfoVO : public oatpp::DTO {
    DTO_INIT(UserInfoVO, DTO)
    DTO_FIELD(String, userUuid, "useruuid");
    DTO_FIELD(String, username, "username");
    DTO_FIELD(String, email, "email");
    DTO_FIELD(String, avatarUrl, "avatarurl");
    DTO_FIELD(String, status, "status");
    DTO_FIELD(String, lastSeen, "lastseen");
};

// 搜索结果中的用户信息（UserInfoVO + 好友关系状态）
class SearchUserVO : public oatpp::DTO {
    DTO_INIT(SearchUserVO, DTO)
    DTO_FIELD(String, userUuid, "useruuid");
    DTO_FIELD(String, username, "username");
    DTO_FIELD(String, email, "email");
    DTO_FIELD(String, avatarUrl, "avatarurl");
    DTO_FIELD(String, status, "status");
    DTO_FIELD(String, lastSeen, "lastseen");
    DTO_FIELD(String, friendshipStatus, "friendshipstatus"); // "accepted"/"pending"/"block"/"blocked"/"none"
};


#include OATPP_CODEGEN_END(DTO) 
