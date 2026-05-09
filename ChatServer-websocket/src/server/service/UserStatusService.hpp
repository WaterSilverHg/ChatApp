#pragma once

#include "global.h"
#include "../dto/UserStatusDto.hpp"
#include "../vo/UserStatusVo.hpp"
#include "../postgresql/AppClient.hpp"

class UserStatusService {
private:
    std::shared_ptr<AppClient> m_appClient;
    using Status = oatpp::web::protocol::http::Status;
public:
    UserStatusService(const std::shared_ptr<AppClient>& appClient) : m_appClient(appClient) {}

    oatpp::Object<UserStatusVO> updateStatus(const oatpp::String& currentUserIdHeader, const oatpp::Object<UpdateStatusRequestDTO>& request) {

        ASYNC_THROW_IF(request->status && !request->status->empty(), "Status parameter cannot be empty");

        auto validStatuses = { "online", "offline", "away", "busy" };
        bool isValid = false;
        for (const auto& s : validStatuses) {
            if (request->status == s) {
                isValid = true;
                break;
            }
        }
        ASYNC_THROW_IF(isValid, "Invalid status value, must be one of: online, offline, away, busy");

        auto userCheck = m_appClient->getUserIdByUuid(currentUserIdHeader);
#ifdef SQLCHECK
        ASYNC_THROW_IF(userCheck->isSuccess(), userCheck->getErrorMessage());
        ASYNC_THROW_IF(userCheck->hasMoreToFetch(), "User does not exist or has expired");
#else
        ASYNC_THROW_IF(userCheck->isSuccess() && userCheck->hasMoreToFetch(), "User does not exist or has expired");
#endif
        auto userId = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        auto result = m_appClient->updateUserStatus(userId, request->status);
#ifdef SQLCHECK
        ASYNC_THROW_IF(result->isSuccess(), result->getErrorMessage());
#else
        ASYNC_THROW_IF(result->isSuccess(), "User does not exist");
#endif
        auto status = UserStatusVO::createShared();
        status->userId = currentUserIdHeader;
        status->status = request->status;
        return status;
    }

    //oatpp::Object<UserStatusVO> getUserStatus(const oatpp::String& targetUserUuid) {
    //    auto userCheck = m_appClient->getUserIdByUuid(targetUserUuid);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(userCheck->isSuccess(), Status::CODE_500, userCheck->getErrorMessage());
    //    OATPP_ASSERT_HTTP(userCheck->hasMoreToFetch(), Status::CODE_404, "用户不存在");
    //    #else
    //    OATPP_ASSERT_HTTP(userCheck->isSuccess() && userCheck->hasMoreToFetch(), Status::CODE_404, "用户不存在");
    //    #endif
    //    auto id = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

    //    auto result = m_appClient->getUserStatus(id);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_404, "用户不存在");
    //    OATPP_ASSERT_HTTP(result->hasMoreToFetch(), Status::CODE_404, "用户状态不存在");
    //    #else
    //    OATPP_ASSERT_HTTP(result->isSuccess() && result->hasMoreToFetch(), Status::CODE_404, "用户状态不存在");
    //    #endif
    //    return result->fetch<oatpp::Vector<oatpp::Object<UserStatusVO>>>()[0];
    //}

    oatpp::Vector<oatpp::Object<UserStatusVO>> getMultipleStatuses(const oatpp::String& currentUserIdHeader, const oatpp::Object<BatchStatusRequestDTO>& request) {
        auto userCheck = m_appClient->getUserIdByUuid(currentUserIdHeader);
        #ifdef SQLCHECK
        ASYNC_THROW_IF(userCheck->isSuccess(), userCheck->getErrorMessage());
        ASYNC_THROW_IF(userCheck->hasMoreToFetch(), "User does not exist or has expired");
        #else
        ASYNC_THROW_IF(userCheck->isSuccess() && userCheck->hasMoreToFetch(), "User does not exist or has expired");
        #endif
        ASYNC_THROW_IF(request->userUuids, "Query users cannot be empty");
        auto statuses = oatpp::Vector<oatpp::Object<UserStatusVO>>::createShared();
        for (const auto& targetUserIdStr : *request->userUuids) {
            auto uuidResult = m_appClient->getUserIdByUuid(targetUserIdStr);
            if (!uuidResult->isSuccess() || !uuidResult->hasMoreToFetch()) {
                continue;
            }
            auto targetUserId = uuidResult->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

            auto result = m_appClient->getUserStatus(targetUserId);
            if (result->isSuccess()) {
                auto status = result->fetch<oatpp::Vector<oatpp::Object<UserStatusVO>>>()[0];
                statuses->push_back(status);
            }
        }
        return statuses;
    }

    //oatpp::Vector<oatpp::Object<UserInfoVO>> getOnlineUsers(const oatpp::String& currentUserIdHeader) {
    //    auto userCheck = m_appClient->getUserIdByUuid(currentUserIdHeader);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(userCheck->isSuccess(), Status::CODE_500, userCheck->getErrorMessage());
    //    OATPP_ASSERT_HTTP(userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
    //    #else
    //    OATPP_ASSERT_HTTP(userCheck->isSuccess() && userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
    //    #endif

    //    auto result = m_appClient->getOnlineUsers();
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, result->getErrorMessage());
    //    #else
    //    OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, "获取在线用户失败");
    //    #endif
    //    return result->fetch<oatpp::Vector<oatpp::Object<UserInfoVO>>>();
    //}

    //oatpp::Vector<oatpp::Object<UserStatusVO>> getFriendsStatus(const oatpp::String& currentUserIdHeader) {
    //    auto userCheck = m_appClient->getUserIdByUuid(currentUserIdHeader);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(userCheck->isSuccess(), Status::CODE_500, userCheck->getErrorMessage());
    //    OATPP_ASSERT_HTTP(userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
    //    #else
    //    OATPP_ASSERT_HTTP(userCheck->isSuccess() && userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
    //    #endif
    //    auto id = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

    //    auto result = m_appClient->getFriendsStatus(id);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, result->getErrorMessage());
    //    #else
    //    OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, "获取好友状态失败");
    //    #endif
    //    return result->fetch<oatpp::Vector<oatpp::Object<UserStatusVO>>>();
    //}

    oatpp::Object<UserProfileVO> getCurrentUser(const oatpp::String& userId) {
        auto userCheck = m_appClient->getUserIdByUuid(userId);
#ifdef SQLCHECK
        ASYNC_THROW_IF(userCheck->isSuccess(), userCheck->getErrorMessage());
        ASYNC_THROW_IF(userCheck->hasMoreToFetch(), "User does not exist or has expired");
#else
        ASYNC_THROW_IF(userCheck->isSuccess() && userCheck->hasMoreToFetch(), "User does not exist or has expired");
#endif

        auto id = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;
        auto result = m_appClient->getUserById(id);
#ifdef SQLCHECK
        ASYNC_THROW_IF(result->isSuccess(), result->getErrorMessage());
        ASYNC_THROW_IF(result->hasMoreToFetch(), "User does not exist");
#else
        ASYNC_THROW_IF(result->isSuccess() && result->hasMoreToFetch(), "User does not exist");
#endif

        return result->fetch<oatpp::Vector<oatpp::Object<UserProfileVO>>>()[0];
    }


    oatpp::Object<UserProfileVO> updateUserInfo(const oatpp::Object<UpdateProfileRequestDTO>& request, const oatpp::String& userId) {
        auto userCheck = m_appClient->getUserIdByUuid(userId);
#ifdef SQLCHECK
        ASYNC_THROW_IF(userCheck->isSuccess(), userCheck->getErrorMessage());
        ASYNC_THROW_IF(userCheck->hasMoreToFetch(), "User does not exist or has expired");
#else
        ASYNC_THROW_IF(userCheck->isSuccess() && userCheck->hasMoreToFetch(), "User does not exist or has expired");
#endif

        auto id = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;
        auto result = m_appClient->updateUser(id, request->username, request->avatarUrl);
#ifdef SQLCHECK
        ASYNC_THROW_IF(result->isSuccess(), result->getErrorMessage());
        ASYNC_THROW_IF(result->hasMoreToFetch(), "User does not exist");
#else
        ASYNC_THROW_IF(result->isSuccess() && result->hasMoreToFetch(), "User does not exist");
#endif

        auto updatedUser = m_appClient->getUserById(id);
#ifdef SQLCHECK
        ASYNC_THROW_IF(updatedUser->isSuccess(), updatedUser->getErrorMessage());
        ASYNC_THROW_IF(updatedUser->hasMoreToFetch(), "User does not exist");
        #else
        ASYNC_THROW_IF(updatedUser->isSuccess() && updatedUser->hasMoreToFetch(), "User does not exist");
        #endif

        return updatedUser->fetch<oatpp::Vector<oatpp::Object<UserProfileVO>>>()[0];
    }
};
