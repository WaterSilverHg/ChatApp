#pragma once

#include "global.h"
#include"server/dto/AuthDto.hpp"

#include OATPP_CODEGEN_BEGIN(DTO) 
// 用户资料
class UserProfileVO : public oatpp::DTO {
    DTO_INIT(UserProfileVO, DTO)
        DTO_FIELD(String, uuid, "uuid");
    DTO_FIELD(String, username, "username");
    DTO_FIELD(String, email, "email");
    DTO_FIELD(String, avatarUrl, "avatarurl");
    DTO_FIELD(String, status, "status");
    DTO_FIELD(String, createdAt, "createdat");
};
// 认证响应
class AuthResponseVO : public oatpp::DTO {
    DTO_INIT(AuthResponseVO, DTO)
    DTO_FIELD(String, token, "token");
    DTO_FIELD(Int32, expiresIn, "expiresIn");
};

// 登录响应
class LoginResponseVO : public oatpp::DTO {
    DTO_INIT(LoginResponseVO, DTO)
        DTO_FIELD(String, token, "token");
    DTO_FIELD(Int32, expiresIn, "expiresIn");
    DTO_FIELD(Object<UserProfileVO>, user, "user");
};

#include OATPP_CODEGEN_END(DTO) 