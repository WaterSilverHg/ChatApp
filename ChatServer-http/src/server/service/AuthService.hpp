#pragma once

#include "global.h"
#include "server/dto/AuthDto.hpp"
#include "server/vo/AuthVo.hpp"
#include"server/dto/GeneralDto.hpp"
#include "server/postgresql/AppPostgresql.hpp"
#include "jwt/Appjwt.h"
#include "smtp/AppEmail.hpp"

class AuthService {
    using Status = oatpp::web::protocol::http::Status;
private:
    std::shared_ptr<AppPostgresql> m_appPostgresql;
    std::shared_ptr<Appjwt> m_jwt;
    std::shared_ptr<AppRedis> m_redis;
    std::shared_ptr<AppEmail> m_email;

    bool verifyPassword(const oatpp::String& plainPassword, const oatpp::String& storedHash) {
        return BCrypt::validatePassword(plainPassword->c_str(), storedHash->c_str());
    }

public:
    AuthService(const std::shared_ptr<AppPostgresql>& appClient,
                const std::shared_ptr<Appjwt>& jwt,
                const std::shared_ptr<AppRedis>& redis,
                const std::shared_ptr<AppEmail>& email = nullptr)
        : m_appPostgresql(appClient), m_jwt(jwt), m_redis(redis), m_email(email) {}

    oatpp::Object<LoginResponseVO> login(const oatpp::Object<LoginRequestDTO>& request) {
        OATPP_ASSERT_HTTP(request->email, Status::CODE_400, "邮箱不能为空");
        OATPP_ASSERT_HTTP(request->password, Status::CODE_400, "密码不能为空");
        auto userResult = m_appPostgresql->getUserByEmail(request->email);
        #ifdef SQLCHECK
        if(!userResult->isSuccess()) {
            OATPP_LOGE("SQL_ERROR", "%s", userResult->getErrorMessage()->c_str());
        }
        #endif
        OATPP_ASSERT_HTTP(userResult->isSuccess(), Status::CODE_500, "查询用户失败");
        OATPP_ASSERT_HTTP(userResult->hasMoreToFetch(), Status::CODE_401, "邮箱或密码错误");
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
        // 验证验证码
        OATPP_ASSERT_HTTP(request->verificationCode && !request->verificationCode->empty(),
            Status::CODE_400, "验证码不能为空");

        std::string email = request->email->c_str();
        std::string storedCode = m_redis->getVerificationCode(email);
        OATPP_ASSERT_HTTP(!storedCode.empty(), Status::CODE_400, "验证码不存在或已过期");

        std::string inputCode = request->verificationCode->c_str();
        OATPP_ASSERT_HTTP(storedCode == inputCode, Status::CODE_400, "验证码错误");

        // 验证码正确，删除已使用的验证码（防止重复使用）
        m_redis->deleteVerificationCode(email);

        auto result = m_appPostgresql->createUser(request->username, request->email, request->phone, request->password);
        #ifdef SQLCHECK
        if(!result->isSuccess()) {
            OATPP_LOGE("SQL_ERROR", "%s", result->getErrorMessage()->c_str());
        }
        #endif
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, "创建用户失败");
        OATPP_ASSERT_HTTP(result->hasMoreToFetch(), Status::CODE_400, "邮箱错误或已存在");
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
        // 验证验证码
        OATPP_ASSERT_HTTP(request->verificationCode, Status::CODE_400, "验证码不能为空");
        std::string storedCode = m_redis->getVerificationCode(request->email->c_str());
        OATPP_ASSERT_HTTP(!storedCode.empty(), Status::CODE_401, "验证码不存在或已过期");
        OATPP_ASSERT_HTTP(storedCode == request->verificationCode->c_str(), Status::CODE_401, "验证码错误");

        auto userResult = m_appPostgresql->getUserByEmail(request->email);
#ifdef SQLCHECK
        if(!userResult->isSuccess()) {
            OATPP_LOGE("SQL_ERROR", "%s", userResult->getErrorMessage()->c_str());
        }
#endif
        OATPP_ASSERT_HTTP(userResult->isSuccess(), Status::CODE_500, "查询用户失败");
        OATPP_ASSERT_HTTP(userResult->hasMoreToFetch(), Status::CODE_401, "邮箱或密码错误");
        auto v_userWithPass = userResult->fetch<oatpp::Vector<oatpp::Object<UserProfileWithPassDTO>>>();
        OATPP_ASSERT_HTTP(v_userWithPass->size() == 1, Status::CODE_500, "Unknown error");
        auto userWithPass = v_userWithPass[0];

        OATPP_ASSERT_HTTP(!verifyPassword(request->newPassword, userWithPass->passwordHash),
            Status::CODE_401, "新密码不能和旧密码相同");


        auto result = m_appPostgresql->resetPasswordByEmail(request->email,request->newPassword);
        #ifdef SQLCHECK
        if(!result->isSuccess()) {
            OATPP_LOGE("SQL_ERROR", "%s", result->getErrorMessage()->c_str());
        }
        #endif
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, "重置密码失败");

        // 删除已使用的验证码
        m_redis->deleteVerificationCode(request->email->c_str());

        return true;
    }

    //oatpp::Object<UserInfoVO> getCurrentUser(const oatpp::String& userId) {
    //    auto userCheck = m_appPostgresql->getUserIdByUuid(userId);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(userCheck->isSuccess(), Status::CODE_500, userCheck->getErrorMessage());
    //    OATPP_ASSERT_HTTP(userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
    //    #else
    //    OATPP_ASSERT_HTTP(userCheck->isSuccess() && userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
    //    #endif

    //    auto id = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

    //    // 获取用户基本信息
    //    auto userResult = m_appPostgresql->getUserById(id);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(userResult->isSuccess(), Status::CODE_500, userResult->getErrorMessage());
    //    OATPP_ASSERT_HTTP(userResult->hasMoreToFetch(), Status::CODE_404, "用户不存在");
    //    #else
    //    OATPP_ASSERT_HTTP(userResult->isSuccess() && userResult->hasMoreToFetch(), Status::CODE_404, "用户不存在");
    //    #endif
    //    auto userProfile = userResult->fetch<oatpp::Vector<oatpp::Object<UserProfileVO>>>()[0];

    //    // 获取用户状态信息（包含lastSeen）
    //    auto statusResult = m_appPostgresql->getUserStatus(id);
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
    //     auto userCheck = m_appPostgresql->getUserIdByUuid(userId);
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
    //    auto userCheck = m_appPostgresql->getUserIdByUuid(userId);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(userCheck->isSuccess(), Status::CODE_500, userCheck->getErrorMessage());
    //    OATPP_ASSERT_HTTP(userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
    //    #else
    //    OATPP_ASSERT_HTTP(userCheck->isSuccess() && userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
    //    #endif

    //    auto id = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;
    //    auto result = m_appPostgresql->updateUser(id, request->username, request->avatarUrl);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, result->getErrorMessage());
    //    OATPP_ASSERT_HTTP(result->hasMoreToFetch(), Status::CODE_404, "用户不存在");
    //    #else
    //    OATPP_ASSERT_HTTP(result->isSuccess() && result->hasMoreToFetch(), Status::CODE_404, "用户不存在");
    //    #endif

    //    // 获取更新后的用户信息
    //    auto updatedUser = m_appPostgresql->getUserById(id);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(updatedUser->isSuccess(), Status::CODE_500, updatedUser->getErrorMessage());
    //    OATPP_ASSERT_HTTP(updatedUser->hasMoreToFetch(), Status::CODE_404, "用户不存在");
    //    #else
    //    OATPP_ASSERT_HTTP(updatedUser->isSuccess() && updatedUser->hasMoreToFetch(), Status::CODE_404, "用户不存在");
    //    #endif
    //    auto userProfile = updatedUser->fetch<oatpp::Vector<oatpp::Object<UserProfileVO>>>()[0];

    //    // 获取用户状态信息（包含lastSeen）
    //    auto statusResult = m_appPostgresql->getUserStatus(id);
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

    //    auto userCheck = m_appPostgresql->getUserIdByUuid(userId);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(userCheck->isSuccess(), Status::CODE_500, userCheck->getErrorMessage());
    //    OATPP_ASSERT_HTTP(userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
    //    #else
    //    OATPP_ASSERT_HTTP(userCheck->isSuccess() && userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
    //    #endif

    //    auto id = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

    //    // 1. 查询用户当前的密码哈希
    //    auto passwordHashResult = m_appPostgresql->getPasswordHashById(id);
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
    //    auto updateResult = m_appPostgresql->changePassword(id, request->newPassword);
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
            OATPP_ASSERT_HTTP(ttl <= 240, Status::CODE_400, "验证码请求频繁");
            OATPP_ASSERT_HTTP(ttl > 0, Status::CODE_400, "验证码已过期或不存在");
        }

        std::string code = generateVerificationCode();
        OATPP_ASSERT_HTTP(m_redis->setVerificationCode(email, code, 300), Status::CODE_500, "服务器保存验证码失败");

        // 通过 SMTP 发送验证码邮件
        if (m_email) {
            OATPP_ASSERT_HTTP(m_email->sendVerificationCode(code, email),
                Status::CODE_500, "邮件发送失败，请稍后重试");
        }

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
