#pragma once

#include "global.h"

#include OATPP_CODEGEN_BEGIN(DTO) 

class UserProfileWithPassDTO : public oatpp::DTO {
    DTO_INIT(UserProfileWithPassDTO, DTO)
        DTO_FIELD(String, userUuid, "useruuid");
    DTO_FIELD(String, username, "username");
    DTO_FIELD(String, email, "email");
    DTO_FIELD(String, passwordHash, "passwordhash");
    DTO_FIELD(String, avatarUrl, "avatarurl");
    DTO_FIELD(String, lastSeen, "lastseen");
};

// 注册请求
class RegisterRequestDTO : public oatpp::DTO {
    DTO_INIT(RegisterRequestDTO, DTO)
        DTO_FIELD(String, username, "username");
    DTO_FIELD(String, email, "email");
    DTO_FIELD(String, phone, "phone");      // 可选
    DTO_FIELD(String, password, "password");
};

// 发送验证码请求
class SendVerificationCodeRequestDTO : public oatpp::DTO {
    DTO_INIT(SendVerificationCodeRequestDTO, DTO)
        DTO_FIELD(String, email, "email");
};

//// 验证验证码请求
//class VerifyCodeRequestDTO : public oatpp::DTO {
//    DTO_INIT(VerifyCodeRequestDTO, DTO)
//        DTO_FIELD(String, email, "email");
//};

// 登录请求
class LoginRequestDTO : public oatpp::DTO {
    DTO_INIT(LoginRequestDTO, DTO)
        DTO_FIELD(String, email, "email");
    DTO_FIELD(String, password, "password");
};

// 登出请求
class LogoutRequestDTO : public oatpp::DTO {
    DTO_INIT(LogoutRequestDTO, DTO)
        DTO_FIELD(String, refreshToken, "refreshtoken");
};

// 刷新令牌请求
class RefreshTokenRequestDTO : public oatpp::DTO {
    DTO_INIT(RefreshTokenRequestDTO, DTO)
        DTO_FIELD(String, token, "token");
};

//// 忘记密码请求
//class ForgotPasswordRequestDTO : public oatpp::DTO {
//    DTO_INIT(ForgotPasswordRequestDTO, DTO)
//        DTO_FIELD(String, email, "email");
//};

// 重置密码请求
class ResetPasswordRequestDTO : public oatpp::DTO {
    DTO_INIT(ResetPasswordRequestDTO, DTO)
        DTO_FIELD(String, email, "email");
    DTO_FIELD(String, verificationCode, "verificationCode");
    //DTO_FIELD(String, oldPassword, "oldPassword");
    DTO_FIELD(String, code, "code");
    DTO_FIELD(String, newPassword, "newPassword");
};





// 修改密码请求
//class ChangePasswordRequestDTO : public oatpp::DTO {
//    DTO_INIT(ChangePasswordRequestDTO, DTO)
//        DTO_FIELD(String, oldPassword, "oldPassword");
//    DTO_FIELD(String, newPassword, "newPassword");
//};

//// 用户状态更新请求
//class UpdateStatusRequestDTO : public oatpp::DTO {
//    DTO_INIT(UpdateStatusRequestDTO, DTO)
//        DTO_FIELD(String, status, "status");
//};

class PasswordHashDTO : public oatpp::DTO {
    DTO_INIT(PasswordHashDTO, DTO)
        DTO_FIELD(String, passwordHash, "passwordhash");
};

#include OATPP_CODEGEN_END(DTO)