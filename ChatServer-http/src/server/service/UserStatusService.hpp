#pragma once

#include "global.h"
#include "../dto/UserStatusDto.hpp"
#include "../vo/UserStatusVo.hpp"
#include "../vo/FriendVo.hpp"
#include "../postgresql/AppClient.hpp"

class StatusService {
private:
    std::shared_ptr<AppClient> m_appClient;
    using Status = oatpp::web::protocol::http::Status;
public:
    StatusService(const std::shared_ptr<AppClient>& appClient) : m_appClient(appClient) {}

    oatpp::Object<UserStatusVO> updateStatus(const oatpp::String& currentUserIdHeader, const oatpp::Object<UpdateStatusRequestDTO>& request) {

        OATPP_ASSERT_HTTP(request->status && !request->status->empty(), Status::CODE_400, "状态参数不能为空");

        auto validStatuses = { "online", "offline", "away", "busy" };
        bool isValid = false;
        for (const auto& s : validStatuses) {
            if (request->status == s) {
                isValid = true;
                break;
            }
        }
        OATPP_ASSERT_HTTP(isValid, Status::CODE_400, "无效的状态值，必须是 online, offline, away, busy 之一");

        auto userCheck = m_appClient->getUserIdByUuid(currentUserIdHeader);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(userCheck->isSuccess(), Status::CODE_500, userCheck->getErrorMessage());
        OATPP_ASSERT_HTTP(userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
        #else
        OATPP_ASSERT_HTTP(userCheck->isSuccess() && userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
        #endif
        auto userId = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        auto result = m_appClient->updateUserStatus(userId, request->status);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, result->getErrorMessage());
        #else
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_404, "用户不存在");
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
        OATPP_ASSERT_HTTP(userCheck->isSuccess(), Status::CODE_500, userCheck->getErrorMessage());
        OATPP_ASSERT_HTTP(userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
        #else
        OATPP_ASSERT_HTTP(userCheck->isSuccess() && userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
        #endif
        OATPP_ASSERT_HTTP(request->userUuids, Status::CODE_401, "查询的用户为空");
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

//    oatpp::Object<UserInfoVO> getCurrentUser(const oatpp::String& userUuid) {
//        //OATPP_LOGD("PATH", "经过");
//        auto userCheck = m_appClient->getUserIdByUuid(userUuid);
//#ifdef SQLCHECK
//        OATPP_ASSERT_HTTP(userCheck->isSuccess(), Status::CODE_500, userCheck->getErrorMessage());
//        OATPP_ASSERT_HTTP(userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
//#else
//        OATPP_ASSERT_HTTP(userCheck->isSuccess() && userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
//#endif
//
//        auto id = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;
//        auto result = m_appClient->getUserInfoById(id);
//#ifdef SQLCHECK
//        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, result->getErrorMessage());
//        OATPP_ASSERT_HTTP(result->hasMoreToFetch(), Status::CODE_404, "用户不存在");
//#else
//        OATPP_ASSERT_HTTP(result->isSuccess() && result->hasMoreToFetch(), Status::CODE_404, "用户不存在");
//#endif
//
//        return result->fetch<oatpp::Vector<oatpp::Object<UserInfoVO>>>()[0];
//    }


    oatpp::Object<UserInfoVO> updateUserInfo(const oatpp::Object<UpdateProfileRequestDTO>& request, const oatpp::String& userUuid) {
        auto userCheck = m_appClient->getUserIdByUuid(userUuid);
#ifdef SQLCHECK
        OATPP_ASSERT_HTTP(userCheck->isSuccess(), Status::CODE_500, userCheck->getErrorMessage());
        OATPP_ASSERT_HTTP(userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
#else
        OATPP_ASSERT_HTTP(userCheck->isSuccess() && userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
#endif

        auto userId = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;
        auto result = m_appClient->updateUser(userId, request->username, request->avatarUrl);
#ifdef SQLCHECK
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, result->getErrorMessage());
        OATPP_ASSERT_HTTP(result->hasMoreToFetch(), Status::CODE_404, "用户不存在");
#else
        OATPP_ASSERT_HTTP(result->isSuccess() && result->hasMoreToFetch(), Status::CODE_404, "用户不存在");
#endif

        auto updatedUser = m_appClient->getUserInfoById(userId);
#ifdef SQLCHECK
        OATPP_ASSERT_HTTP(updatedUser->isSuccess(), Status::CODE_500, updatedUser->getErrorMessage());
        OATPP_ASSERT_HTTP(updatedUser->hasMoreToFetch(), Status::CODE_404, "用户不存在");
#else
        OATPP_ASSERT_HTTP(updatedUser->isSuccess() && updatedUser->hasMoreToFetch(), Status::CODE_404, "用户不存在");
#endif

        return updatedUser->fetch<oatpp::Vector<oatpp::Object<UserInfoVO>>>()[0];
    }

    oatpp::Vector<oatpp::Object<FriendInfoVO>> searchUsers(const oatpp::String& keyword, const oatpp::String& userUuid) {
        auto userCheck = m_appClient->getUserIdByUuid(userUuid);
#ifdef SQLCHECK
        OATPP_ASSERT_HTTP(userCheck->isSuccess(), Status::CODE_500, userCheck->getErrorMessage());
        OATPP_ASSERT_HTTP(userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
#else
        OATPP_ASSERT_HTTP(userCheck->isSuccess() && userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
#endif

        auto userId = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        OATPP_ASSERT_HTTP(keyword && !keyword->empty(), Status::CODE_400, "搜索关键词不能为空");
        auto searchKeyword = "%" + keyword + "%";
        auto result = m_appClient->searchUsers(searchKeyword,userId);
#ifdef SQLCHECK
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, result->getErrorMessage());
#else
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, "搜索用户失败");
#endif
        auto users = result->fetch<oatpp::Vector<oatpp::Object<FriendInfoVO>>>();
        if (!users) {
            users = oatpp::Vector<oatpp::Object<FriendInfoVO>>::createShared();
        }
        return users;
    }

    oatpp::Object<UserInfoVO> getUserInfoByUuid(const oatpp::String& userUuid) {
        auto userCheck = m_appClient->getUserIdByUuid(userUuid);
#ifdef SQLCHECK
        OATPP_ASSERT_HTTP(userCheck->isSuccess(), Status::CODE_500, userCheck->getErrorMessage());
        OATPP_ASSERT_HTTP(userCheck->hasMoreToFetch(), Status::CODE_404, "用户不存在");
#else
        OATPP_ASSERT_HTTP(userCheck->isSuccess() && userCheck->hasMoreToFetch(), Status::CODE_404, "用户不存在");
#endif
        auto id = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        // 获取用户基本信息
        auto userResult = m_appClient->getUserInfoById(id);
#ifdef SQLCHECK
        OATPP_ASSERT_HTTP(userResult->isSuccess(), Status::CODE_500, userResult->getErrorMessage());
        OATPP_ASSERT_HTTP(userResult->hasMoreToFetch(), Status::CODE_404, "用户不存在");
#else
        OATPP_ASSERT_HTTP(userResult->isSuccess() && userResult->hasMoreToFetch(), Status::CODE_404, "用户不存在");
#endif
        //auto userProfile = userResult->fetch<oatpp::Vector<oatpp::Object<UserInfoVO>>>()[0];
        return userResult->fetch<oatpp::Vector<oatpp::Object<UserInfoVO>>>()[0];
        //// 获取用户状态信息（包含lastSeen）
        //auto statusResult = m_appClient->getUserStatus(id);
        //oatpp::String lastSeen = nullptr;
        //oatpp::String status = nullptr;
        //if (statusResult->isSuccess() && statusResult->hasMoreToFetch()) {
        //    auto statusData = statusResult->fetch<oatpp::Vector<oatpp::Object<UserStatusVO>>>()[0];
        //    status = statusData->status;
        //    lastSeen = statusData->lastSeen;
        //}

        //// 创建UserInfoVO，使用getUserById的数据作为基础
        //auto userInfo = UserInfoVO::createShared();
        //userInfo->userUuid = userUuid;
        //userInfo->username = userProfile->username;
        //userInfo->email = userProfile->email;
        //userInfo->avatarUrl = userProfile->avatarUrl;
        //// 优先使用getUserStatus返回的status和lastSeen
        //userInfo->status = status ? status : (userProfile->status ? userProfile->status : oatpp::String("offline"));
        //userInfo->lastSeen = lastSeen ? lastSeen : userProfile->lastSeen;

        //return userInfo;
    }
};
