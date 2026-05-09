#pragma once

#include "global.h"
#include "../service/AuthService.hpp"
#include "../handler/AppAuthHandler.h"

#include OATPP_CODEGEN_BEGIN(ApiController) 

class AuthController : public oatpp::web::server::api::ApiController {
private:
    std::shared_ptr<AuthService> m_authService;

public:
    AuthController(OATPP_COMPONENT(std::shared_ptr<ObjectMapper>, objectMapper),
                   OATPP_COMPONENT(std::shared_ptr<AppClient>, appClient),
                   OATPP_COMPONENT(std::shared_ptr<Appjwt>, jwt),
                   OATPP_COMPONENT(std::shared_ptr<AppRedis>, redis))
        : oatpp::web::server::api::ApiController(objectMapper),
          m_authService(std::make_shared<AuthService>(appClient, jwt, redis)) {
        setDefaultAuthorizationHandler(std::make_shared<AppAuthHandler>());
    }

    static std::shared_ptr<AuthController> createShared(
        OATPP_COMPONENT(std::shared_ptr<ObjectMapper>, objectMapper),
        OATPP_COMPONENT(std::shared_ptr<AppClient>, appClient),
        OATPP_COMPONENT(std::shared_ptr<Appjwt>, jwt),
        OATPP_COMPONENT(std::shared_ptr<AppRedis>, redis)
    ) {
        return std::make_shared<AuthController>(objectMapper, appClient, jwt, redis);
    }

    ENDPOINT_INFO(login) {
        info->summary = "用户登录";
        info->description = "通过邮箱和密码进行用户登录，返回JWT令牌和用户信息";
        info->addResponse<Object<LoginResponseVO>>(Status::CODE_200, "application/json", "登录成功");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_401, "application/json", "邮箱或密码错误");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_400, "application/json", "请求参数错误");
        info->addConsumes<Object<LoginRequestDTO>>("application/json");
    }
    ENDPOINT("POST", "/api/auth/login", login, BODY_DTO(Object<LoginRequestDTO>, request)) {
        return createDtoResponse(Status::CODE_200, m_authService->login(request));
    }

    ENDPOINT_INFO(signup) {
        info->summary = "用户注册";
        info->description = "创建新用户账号，返回JWT令牌和用户信息";
        info->addResponse<Object<LoginResponseVO>>(Status::CODE_200, "application/json", "注册成功");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_400, "application/json", "注册失败，邮箱已存在");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_400, "application/json", "请求参数错误");
        info->addConsumes<Object<RegisterRequestDTO>>("application/json");
    }
    ENDPOINT("POST", "/api/auth/register", signup, BODY_DTO(Object<RegisterRequestDTO>, request)) {
        return createDtoResponse(Status::CODE_200, m_authService->signup(request));
    }

    ENDPOINT_INFO(resetPassword) {
        info->summary = "密码重置";
        info->description = "通过邮箱重置用户密码";
        info->addResponse<Boolean>(Status::CODE_200, "application/json", "密码重置成功");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_400, "application/json", "密码重置失败");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_400, "application/json", "请求参数错误");
        info->addConsumes<Object<ResetPasswordRequestDTO>>("application/json");
    }
    ENDPOINT("POST", "/api/auth/reset-password", resetPassword, BODY_DTO(Object<ResetPasswordRequestDTO>, request)) {
        return createDtoResponse(Status::CODE_200, m_authService->resetPassword(request));
    }

    ENDPOINT_INFO(logout) {
        info->summary = "用户登出";
        info->description = "用户登出，删除Redis中的令牌";
        info->addResponse<Boolean>(Status::CODE_200, "application/json", "登出成功");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_401, "application/json", "用户不存在或已失效");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_400, "application/json", "请求参数错误");
        info->addSecurityRequirement("BearerAuth");
    }
    ENDPOINT("POST", "/api/auth/logout", logout, AUTHORIZATION(std::shared_ptr<Appjwt::Payload>, authObject)) {
        return createDtoResponse(Status::CODE_200, m_authService->logout(authObject->userUuid));
    }

    ENDPOINT_INFO(sendVerificationCode) {
        info->summary = "发送验证码";
        info->description = "向指定邮箱发送验证码，验证码有效期5分钟，60秒内不能重复发送";
        info->addResponse<Boolean>(Status::CODE_200, "application/json", "发送成功");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_400, "application/json", "请求参数错误");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_400, "application/json", "验证码请求频繁");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_500, "application/json", "服务器发送验证码失败");
        info->addConsumes<Object<SendVerificationCodeRequestDTO>>("application/json");
    }
    ENDPOINT("POST", "/api/auth/send-code", sendVerificationCode, BODY_DTO(Object<SendVerificationCodeRequestDTO>, request)) {
        return createDtoResponse(Status::CODE_200, m_authService->sendVerificationCode(request));
    }
};

#include OATPP_CODEGEN_END(ApiController)
