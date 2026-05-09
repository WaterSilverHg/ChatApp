#pragma once

#include "global.h"

#include OATPP_CODEGEN_BEGIN(DTO) 

// 用户在线状态
class UserStatusVO : public oatpp::DTO {
    DTO_INIT(UserStatusVO, DTO)
    DTO_FIELD(String, userId, "userid");
    DTO_FIELD(String, status, "status"); // online, offline, away, busy
};

// 用户信息
class UserInfoVO : public oatpp::DTO {
    DTO_INIT(UserInfoVO, DTO)
    DTO_FIELD(String, userUuid, "useruuid");
    DTO_FIELD(String, username, "username");
    //DTO_FIELD(String, email, "email");
    DTO_FIELD(String, avatarUrl, "avatarurl");
    DTO_FIELD(String, status, "status");
    DTO_FIELD(String, lastSeen, "lastseen");
};


#include OATPP_CODEGEN_END(DTO) 
