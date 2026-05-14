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
        OATPP_ASSERT_HTTP(request->email, Status::CODE_400, "邮箱不能为空");
        OATPP_ASSERT_HTTP(request->password, Status::CODE_400, "密码不能为空");
        auto userResult = m_appClient->getUserByEmail(request->email);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(userResult->isSuccess(), Status::CODE_500, userResult->getErrorMessage());
        OATPP_ASSERT_HTTP(userResult->hasMoreToFetch(), Status::CODE_401, "邮箱或密码错误");
        #else
        OATPP_ASSERT_HTTP(userResult->isSuccess() && userResult->hasMoreToFetch(), Status::CODE_401, "邮箱或密码错误");
        #endif
        auto v_userWithPass = userResult->fetch<oatpp::Vector<oatpp::Object<UserProfileWithPassDTO>>>();
        OATPP_ASSERT_HTTP(v_userWithPass->size() == 1, Status::CODE_500, "Unknown error");
        auto userWithPass = v_userWithPass[0];
        
        OATPP_ASSERT_HTTP(verifyPassword(request->password, userWithPass->passwordHash), 
                          Status::CODE_401, "邮箱或密码错误");

        auto user = UserInfoVO::createShared();
        user->userUuid = userWithPass->userUuid;
        user->username = userWithPass->username;
        user->email = userWithPass->email;
        user->avatarUrl = userWithPass->avatarUrl;
        user->lastSeen = userWithPass->lastSeen;

        auto response = LoginResponseVO::createShared();
        auto payload = std::make_shared<Appjwt::Payload>();
        payload->userUuid = user->userUuid;
        // 生成唯一的sessionId
        payload->sessionId = m_jwt->generateSessionId();
        response->token = m_jwt->createToken(payload);

        m_redis->createSession(user->userUuid, payload->sessionId.getValue(""), EXPIRESIN);

        response->user = user;
        response->expiresIn = EXPIRESIN;
        return response;
    }

    oatpp::Object<LoginResponseVO> signup(const oatpp::Object<RegisterRequestDTO>& request) {
        auto result = m_appClient->createUser(request->username, request->email, request->phone, request->password);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_400, result->getErrorMessage());
        OATPP_ASSERT_HTTP(result->hasMoreToFetch(), Status::CODE_400, "创建用户失败");
        #else
        OATPP_ASSERT_HTTP(result->isSuccess() && result->hasMoreToFetch(), Status::CODE_400, "邮箱错误或已存在");
        #endif
        auto uuid = result->fetch< oatpp::Vector<oatpp::Object<UuidDTO>>>()[0];
        //auto uuid = result->fetch<oatpp::String>();
        auto user = UserInfoVO::createShared();
        user->userUuid = uuid->uuid;
        user->email = request->email;
        user->username = request->username;
        // 新注册用户没有头像，设置为空
        user->avatarUrl = nullptr;
        // 新注册用户状态默认为online
        //user->status = oatpp::String("online");
        user->lastSeen = nullptr;

        auto response = LoginResponseVO::createShared();
        auto payload = std::make_shared<Appjwt::Payload>();
        payload->userUuid = user->userUuid;
        // 生成唯一的sessionId
        payload->sessionId = m_jwt->generateSessionId();
        response->token = m_jwt->createToken(payload);

        m_redis->createSession(payload->userUuid, payload->sessionId.getValue(""), EXPIRESIN);

        response->user = user;
        response->expiresIn = EXPIRESIN;
        return response;
    }

    oatpp::Boolean resetPassword(const oatpp::Object<ResetPasswordRequestDTO>& request) {
        auto userResult = m_appClient->getUserByEmail(request->email);
#ifdef SQLCHECK
        OATPP_ASSERT_HTTP(userResult->isSuccess(), Status::CODE_500, userResult->getErrorMessage());
        OATPP_ASSERT_HTTP(userResult->hasMoreToFetch(), Status::CODE_401, "邮箱或密码错误");
#else
        OATPP_ASSERT_HTTP(userResult->isSuccess() && userResult->hasMoreToFetch(), Status::CODE_401, "邮箱或密码错误");
#endif
        auto v_userWithPass = userResult->fetch<oatpp::Vector<oatpp::Object<UserProfileWithPassDTO>>>();
        OATPP_ASSERT_HTTP(v_userWithPass->size() == 1, Status::CODE_500, "Unknown error");
        auto userWithPass = v_userWithPass[0];

        OATPP_ASSERT_HTTP(!verifyPassword(request->newPassword, userWithPass->passwordHash),
            Status::CODE_401, "新密码不能和旧密码相同");


        auto result = m_appClient->resetPasswordByEmail(request->email,request->newPassword);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_400, result->getErrorMessage());
        #else
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, "重置密码错误");
        #endif
        return true;
    }

    //oatpp::Object<UserInfoVO> getCurrentUser(const oatpp::String& userId) {
    //    auto userCheck = m_appClient->getUserIdByUuid(userId);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(userCheck->isSuccess(), Status::CODE_500, userCheck->getErrorMessage());
    //    OATPP_ASSERT_HTTP(userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
    //    #else
    //    OATPP_ASSERT_HTTP(userCheck->isSuccess() && userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
    //    #endif

    //    auto id = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

    //    // 获取用户基本信息
    //    auto userResult = m_appClient->getUserById(id);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(userResult->isSuccess(), Status::CODE_500, userResult->getErrorMessage());
    //    OATPP_ASSERT_HTTP(userResult->hasMoreToFetch(), Status::CODE_404, "用户不存在");
    //    #else
    //    OATPP_ASSERT_HTTP(userResult->isSuccess() && userResult->hasMoreToFetch(), Status::CODE_404, "用户不存在");
    //    #endif
    //    auto userProfile = userResult->fetch<oatpp::Vector<oatpp::Object<UserProfileVO>>>()[0];

    //    // 获取用户状态信息（包含lastSeen）
    //    auto statusResult = m_appClient->getUserStatus(id);
    //    oatpp::String lastSeen = nullptr;
    //    oatpp::String status = nullptr;
    //    if (statusResult->isSuccess() && statusResult->hasMoreToFetch()) {
    //        auto statusData = statusResult->fetch<oatpp::Vector<oatpp::Object<UserStatusVO>>>()[0];
    //        status = statusData->status;
    //        lastSeen = statusData->lastSeen;
    //    }

    //    // 创建UserInfoVO
    //    auto userInfo = UserInfoVO::createShared();
    //    userInfo->userUuid = userId;
    //    userInfo->username = userProfile->username;
    //    userInfo->email = userProfile->email;
    //    userInfo->avatarUrl = userProfile->avatarUrl;
    //    userInfo->status = status ? status : oatpp::String("offline");
    //    userInfo->lastSeen = lastSeen;

    //    return userInfo;
    //}

    // oatpp::Boolean logout(const oatpp::String& userId) {
    //     auto userCheck = m_appClient->getUserIdByUuid(userId);
    //     #ifdef SQLCHECK
    //     OATPP_ASSERT_HTTP(userCheck->isSuccess(), Status::CODE_500, userCheck->getErrorMessage());
    //     OATPP_ASSERT_HTTP(userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
    //     #else
    //     OATPP_ASSERT_HTTP(userCheck->isSuccess() && userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
    //     #endif

    //     //m_redis->deleteSession(userId.getValue(""));
    //     return true;
    // }

    //oatpp::Object<UserInfoVO> updateUserInfo(const oatpp::Object<UpdateProfileRequestDTO>& request, const oatpp::String& userId) {
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

    //    // 获取更新后的用户信息
    //    auto updatedUser = m_appClient->getUserById(id);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(updatedUser->isSuccess(), Status::CODE_500, updatedUser->getErrorMessage());
    //    OATPP_ASSERT_HTTP(updatedUser->hasMoreToFetch(), Status::CODE_404, "用户不存在");
    //    #else
    //    OATPP_ASSERT_HTTP(updatedUser->isSuccess() && updatedUser->hasMoreToFetch(), Status::CODE_404, "用户不存在");
    //    #endif
    //    auto userProfile = updatedUser->fetch<oatpp::Vector<oatpp::Object<UserProfileVO>>>()[0];

    //    // 获取用户状态信息（包含lastSeen）
    //    auto statusResult = m_appClient->getUserStatus(id);
    //    oatpp::String lastSeen = nullptr;
    //    oatpp::String status = nullptr;
    //    if (statusResult->isSuccess() && statusResult->hasMoreToFetch()) {
    //        auto statusData = statusResult->fetch<oatpp::Vector<oatpp::Object<UserStatusVO>>>()[0];
    //        status = statusData->status;
    //        lastSeen = statusData->lastSeen;
    //    }

    //    // 创建UserInfoVO
    //    auto userInfo = UserInfoVO::createShared();
    //    userInfo->userUuid = userId;
    //    userInfo->username = userProfile->username;
    //    userInfo->email = userProfile->email;
    //    userInfo->avatarUrl = userProfile->avatarUrl;
    //    userInfo->status = status ? status : oatpp::String("offline");
    //    userInfo->lastSeen = lastSeen;

    //    return userInfo;
    //}

    //oatpp::Boolean changePassword(const oatpp::Object<ChangePasswordRequestDTO>& request, const oatpp::String& userId) {
    //    OATPP_ASSERT_HTTP(request, Status::CODE_400, "请求参数不能为空");
    //    OATPP_ASSERT_HTTP(request->oldPassword && !request->oldPassword->empty(), Status::CODE_400, "旧密码不能为空");
    //    OATPP_ASSERT_HTTP(request->newPassword && !request->newPassword->empty(), Status::CODE_400, "新密码不能为空");
    //    OATPP_ASSERT_HTTP(request->oldPassword != request->newPassword, Status::CODE_400, "新密码不能与旧密码相同");

    //    auto userCheck = m_appClient->getUserIdByUuid(userId);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(userCheck->isSuccess(), Status::CODE_500, userCheck->getErrorMessage());
    //    OATPP_ASSERT_HTTP(userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
    //    #else
    //    OATPP_ASSERT_HTTP(userCheck->isSuccess() && userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
    //    #endif

    //    auto id = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

    //    // 1. 查询用户当前的密码哈希
    //    auto passwordHashResult = m_appClient->getPasswordHashById(id);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(passwordHashResult->isSuccess(), Status::CODE_500, passwordHashResult->getErrorMessage());
    //    OATPP_ASSERT_HTTP(passwordHashResult->hasMoreToFetch(), Status::CODE_404, "用户不存在");
    //    #else
    //    OATPP_ASSERT_HTTP(passwordHashResult->isSuccess() && passwordHashResult->hasMoreToFetch(), Status::CODE_404, "用户不存在");
    //    #endif
    //    auto pass = passwordHashResult->fetch<oatpp::Vector<oatpp::Object<PasswordHashDTO>>>()[0];

    //    // 2. 验证旧密码
    //    bool isValid = verifyPassword(request->oldPassword, pass->passwordHash);
    //    OATPP_ASSERT_HTTP(isValid, Status::CODE_400, "修改密码失败，旧密码错误");

    //    // 3. 验证通过后，更新为新密码
    //    auto updateResult = m_appClient->changePassword(id, request->newPassword);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(updateResult->isSuccess(), Status::CODE_500, updateResult->getErrorMessage());
    //    OATPP_ASSERT_HTTP(updateResult->hasMoreToFetch(), Status::CODE_404, "用户不存在");
    //    #else
    //    OATPP_ASSERT_HTTP(updateResult->isSuccess() && updateResult->hasMoreToFetch(), Status::CODE_404, "用户不存在");
    //    #endif

    //    return true;
    //}

    oatpp::Boolean sendVerificationCode(const oatpp::Object<SendVerificationCodeRequestDTO>& request) {
        OATPP_ASSERT_HTTP(request, Status::CODE_400, "请求参数不能为空");
        OATPP_ASSERT_HTTP(request->email && !request->email->empty(), Status::CODE_400, "邮箱不能为空");

        std::string email = request->email->c_str();

        if (m_redis->existsVerificationCode(email)) {
            long long ttl = m_redis->getVerificationCodeTTL(email);
            OATPP_ASSERT_HTTP(ttl <= 60 || ttl < 0, Status::CODE_400, "验证码请求频繁");
        }

        std::string code = generateVerificationCode();
        ;
        OATPP_ASSERT_HTTP(!m_redis->setVerificationCode(email, code, 300), Status::CODE_500, "服务器发送验证码失败");
        // ||!m_email->sendVerificationCode(code,email)
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
