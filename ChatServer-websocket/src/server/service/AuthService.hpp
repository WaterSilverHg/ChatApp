#pragma once

#include "global.h"
#include "../dto/AuthDto.hpp"
#include "../vo/AuthVo.hpp"
#include"../dto/GeneralDto.hpp"
#include "../postgresql/AppClient.hpp"
#include "../../jwt/Appjwt.h"

class AuthService {
    using Status = oatpp::web::protocol::http::Status;
private:
    std::shared_ptr<AppClient> m_appClient;
    std::shared_ptr<Appjwt> m_jwt;
    std::shared_ptr<AppRedis> m_redis;
    // std::shared_ptr<AppEmail> m_email;

    bool verifyPassword(const oatpp::String& plainPassword, const oatpp::String& storedHash) {
        return BCrypt::validatePassword(plainPassword->c_str(), storedHash->c_str());
    }

public:
    AuthService(const std::shared_ptr<AppClient>& appClient,
                const std::shared_ptr<Appjwt>& jwt,
                const std::shared_ptr<AppRedis>& redis)
        : m_appClient(appClient), m_jwt(jwt), m_redis(redis) {}

    oatpp::Object<LoginResponseVO> login(const oatpp::Object<LoginRequestDTO>& request) {
        ASYNC_THROW_IF(request->email, "邮箱不能为空");
        ASYNC_THROW_IF(request->password, "密码不能为空");
        auto userResult = m_appClient->getUserByEmail(request->email);
        #ifdef SQLCHECK
        ASYNC_THROW_IF(userResult->isSuccess(), userResult->getErrorMessage());
        ASYNC_THROW_IF(userResult->hasMoreToFetch(), "邮箱或密码错误");
        #else
        ASYNC_THROW_IF(userResult->isSuccess() && userResult->hasMoreToFetch(), "邮箱或密码错误");
        #endif
        auto v_userWithPass = userResult->fetch<oatpp::Vector<oatpp::Object<UserProfileWithPassDTO>>>();
        ASYNC_THROW_IF(v_userWithPass->size() == 1, "Unknown error");
        auto userWithPass = v_userWithPass[0];
        
        ASYNC_THROW_IF(verifyPassword(request->password, userWithPass->passwordHash), "邮箱或密码错误");

        auto user = UserProfileVO::createShared();
        user->uuid = userWithPass->uuid;
        user->username = userWithPass->username;
        user->email = userWithPass->email;
        user->avatarUrl = userWithPass->avatarUrl;
        user->createdAt = userWithPass->createdAt;

        auto response = LoginResponseVO::createShared();
        auto payload = std::make_shared<Appjwt::Payload>();
        payload->userUuid = user->uuid;
        response->token = m_jwt->createToken(payload);

        m_redis->createToken(user->uuid, response->token.getValue(""), EXPIRESIN);

        response->user = user;
        response->expiresIn = EXPIRESIN;
        return response;
    }

    oatpp::Object<LoginResponseVO> signup(const oatpp::Object<RegisterRequestDTO>& request) {
        auto result = m_appClient->createUser(request->username, request->email, request->phone, request->password);
        #ifdef SQLCHECK
        ASYNC_THROW_IF(result->isSuccess(), result->getErrorMessage());
        ASYNC_THROW_IF(result->hasMoreToFetch(), "创建用户失败");
        #else
        ASYNC_THROW_IF(result->isSuccess() && result->hasMoreToFetch(), "邮箱错误或已存在");
        #endif
        auto uuid = result->fetch< oatpp::Vector<oatpp::Object<UuidDTO>>>()[0];
        //auto uuid = result->fetch<oatpp::String>();
        auto user = UserProfileVO::createShared();
        user->uuid = uuid->uuid;
        user->email = request->email;
        user->username = request->username;

        auto response = LoginResponseVO::createShared();
        auto payload = std::make_shared<Appjwt::Payload>();
        payload->userUuid = user->uuid;
        response->token = m_jwt->createToken(payload);

        m_redis->createToken(payload->userUuid, response->token.getValue(""), EXPIRESIN);

        response->user = user;
        response->expiresIn = EXPIRESIN;
        return response;
    }

    oatpp::Boolean resetPassword(const oatpp::Object<ResetPasswordRequestDTO>& request) {
        auto userResult = m_appClient->getUserByEmail(request->email);
        #ifdef SQLCHECK
        ASYNC_THROW_IF(userResult->isSuccess(), userResult->getErrorMessage());
        ASYNC_THROW_IF(userResult->hasMoreToFetch(), "邮箱或密码错误");
        #else
        ASYNC_THROW_IF(userResult->isSuccess() && userResult->hasMoreToFetch(), "邮箱或密码错误");
        #endif
        auto v_userWithPass = userResult->fetch<oatpp::Vector<oatpp::Object<UserProfileWithPassDTO>>>();
        ASYNC_THROW_IF(v_userWithPass->size() == 1, "Unknown error");
        auto userWithPass = v_userWithPass[0];

        ASYNC_THROW_IF(verifyPassword(request->oldPassword, userWithPass->passwordHash), "新密码不能和旧密码相同");


        auto result = m_appClient->resetPasswordByEmail(request->email,request->newPassword);
        ASYNC_THROW_IF(result->isSuccess(), result->getErrorMessage());
        return true;
    }

    //oatpp::Object<UserProfileVO> getCurrentUser(const oatpp::String& userId) {
    //    auto userCheck = m_appClient->getUserIdByUuid(userId);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(userCheck->isSuccess(), Status::CODE_500, userCheck->getErrorMessage());
    //    OATPP_ASSERT_HTTP(userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
    //    #else
    //    OATPP_ASSERT_HTTP(userCheck->isSuccess() && userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
    //    #endif

    //    auto id = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;
    //    auto result = m_appClient->getUserById(id);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, result->getErrorMessage());
    //    OATPP_ASSERT_HTTP(result->hasMoreToFetch(), Status::CODE_404, "用户不存在");
    //    #else
    //    OATPP_ASSERT_HTTP(result->isSuccess() && result->hasMoreToFetch(), Status::CODE_404, "用户不存在");
    //    #endif

    //    return result->fetch<oatpp::Vector<oatpp::Object<UserProfileVO>>>()[0];
    //}

    oatpp::Boolean logout(const oatpp::String& userId) {
        auto userCheck = m_appClient->getUserIdByUuid(userId);
        #ifdef SQLCHECK
        ASYNC_THROW_IF(userCheck->isSuccess(), userCheck->getErrorMessage());
        ASYNC_THROW_IF(userCheck->hasMoreToFetch(), "用户不存在或已失效");
        #else
        ASYNC_THROW_IF(userCheck->isSuccess() && userCheck->hasMoreToFetch(), "用户不存在或已失效");
        #endif

        m_redis->deleteToken(userId.getValue(""));
        return true;
    }

    //oatpp::Object<UserProfileVO> updateUserInfo(const oatpp::Object<UpdateProfileRequestDTO>& request, const oatpp::String& userId) {
    //    auto userCheck = m_appClient->getUserIdByUuid(userId);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(userCheck->isSuccess(), Status::CODE_500, userCheck->getErrorMessage());
    //    OATPP_ASSERT_HTTP(userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
    //    #else
    //    OATPP_ASSERT_HTTP(userCheck->isSuccess() && userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
    //    #endif

    //    auto id = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;
    //    auto result = m_appClient->updateUser(id, request->username, request->avatarUrl);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, result->getErrorMessage());
    //    OATPP_ASSERT_HTTP(result->hasMoreToFetch(), Status::CODE_404, "用户不存在");
    //    #else
    //    OATPP_ASSERT_HTTP(result->isSuccess() && result->hasMoreToFetch(), Status::CODE_404, "用户不存在");
    //    #endif

    //    auto updatedUser = m_appClient->getUserById(id);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(updatedUser->isSuccess(), Status::CODE_500, updatedUser->getErrorMessage());
    //    OATPP_ASSERT_HTTP(updatedUser->hasMoreToFetch(), Status::CODE_404, "用户不存在");
    //    #else
    //    OATPP_ASSERT_HTTP(updatedUser->isSuccess() && updatedUser->hasMoreToFetch(), Status::CODE_404, "用户不存在");
    //    #endif

    //    return updatedUser->fetch<oatpp::Vector<oatpp::Object<UserProfileVO>>>()[0];
    //}

    oatpp::Boolean changePassword(const oatpp::Object<ChangePasswordRequestDTO>& request, const oatpp::String& userId) {
        ASYNC_THROW_IF(request, "请求参数不能为空");
        ASYNC_THROW_IF(request->oldPassword && !request->oldPassword->empty(), "旧密码不能为空");
        ASYNC_THROW_IF(request->newPassword && !request->newPassword->empty(), "新密码不能为空");
        ASYNC_THROW_IF(!(request->oldPassword == request->newPassword), "新密码不能与旧密码相同");

        auto userCheck = m_appClient->getUserIdByUuid(userId);
        #ifdef SQLCHECK
        ASYNC_THROW_IF(userCheck->isSuccess(), userCheck->getErrorMessage());
        ASYNC_THROW_IF(userCheck->hasMoreToFetch(), "用户不存在或已失效");
        #else
        ASYNC_THROW_IF(userCheck->isSuccess() && userCheck->hasMoreToFetch(), "用户不存在或已失效");
        #endif

        auto id = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        auto passwordHashResult = m_appClient->getPasswordHashById(id);
        #ifdef SQLCHECK
        ASYNC_THROW_IF(passwordHashResult->isSuccess(), passwordHashResult->getErrorMessage());
        ASYNC_THROW_IF(passwordHashResult->hasMoreToFetch(), "用户不存在");
        #else
        ASYNC_THROW_IF(passwordHashResult->isSuccess() && passwordHashResult->hasMoreToFetch(), "用户不存在");
        #endif
        auto pass = passwordHashResult->fetch<oatpp::Vector<oatpp::Object<PasswordHashDTO>>>()[0];

        bool isValid = verifyPassword(request->oldPassword, pass->passwordHash);
        ASYNC_THROW_IF(isValid, "修改密码失败，旧密码错误");

        auto updateResult = m_appClient->changePassword(id, request->newPassword);
        #ifdef SQLCHECK
        ASYNC_THROW_IF(updateResult->isSuccess(), updateResult->getErrorMessage());
        ASYNC_THROW_IF(updateResult->hasMoreToFetch(), "用户不存在");
        #else
        ASYNC_THROW_IF(updateResult->isSuccess() && updateResult->hasMoreToFetch(), "用户不存在");
        #endif

        return true;
    }

    oatpp::Boolean sendVerificationCode(const oatpp::Object<RegisterRequestDTO>& request) {
        ASYNC_THROW_IF(request, "请求参数不能为空");
        ASYNC_THROW_IF(request->email && !request->email->empty(), "邮箱不能为空");

        std::string email = request->email->c_str();

        if (m_redis->existsVerificationCode(email)) {
            long long ttl = m_redis->getVerificationCodeTTL(email);
            ASYNC_THROW_IF(ttl <= 60 || ttl < 0, "验证码请求频繁");
        }

        std::string code = generateVerificationCode();
        ASYNC_THROW_IF(m_redis->setVerificationCode(email, code, 300), "服务器发送验证码失败");
        return true;
    }

private:
    std::string generateVerificationCode() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 999999);
        int code = dis(gen);
        char buffer[7];
        snprintf(buffer, sizeof(buffer), "%06d", code);
        return std::string(buffer);
    }
};
