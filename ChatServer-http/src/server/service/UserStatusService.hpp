#pragma once

#include "global.h"
#include "../dto/UserStatusDto.hpp"
#include "../vo/UserStatusVo.hpp"
#include "../postgresql/AppPostgresql.hpp"
#include "../../redis/AppRedis.hpp"
#include "../../tool/UuidIdCache.hpp"

class StatusService {
private:
    std::shared_ptr<AppPostgresql> m_appPostgresql;
    std::shared_ptr<AppRedis> m_redis;
    std::shared_ptr<UuidIdCache> m_idCache;
    using Status = oatpp::web::protocol::http::Status;
public:
    StatusService(const std::shared_ptr<AppPostgresql>& appClient, 
                  const std::shared_ptr<AppRedis>& redis,
                  const std::shared_ptr<UuidIdCache>& idCache)
        : m_appPostgresql(appClient), m_redis(redis), m_idCache(idCache) {}

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

        auto userId = m_idCache->getUserId(currentUserIdHeader);
        OATPP_ASSERT_HTTP(userId > 0, Status::CODE_401, "用户不存在或已失效");

        auto result = m_appPostgresql->updateUserStatus(userId, request->status);
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
    //    auto userCheck = m_appPostgresql->getUserIdByUuid(targetUserUuid);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(userCheck->isSuccess(), Status::CODE_500, userCheck->getErrorMessage());
    //    OATPP_ASSERT_HTTP(userCheck->hasMoreToFetch(), Status::CODE_404, "用户不存在");
    //    #else
    //    OATPP_ASSERT_HTTP(userCheck->isSuccess() && userCheck->hasMoreToFetch(), Status::CODE_404, "用户不存在");
    //    #endif
    //    auto id = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

    //    auto result = m_appPostgresql->getUserStatus(id);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_404, "用户不存在");
    //    OATPP_ASSERT_HTTP(result->hasMoreToFetch(), Status::CODE_404, "用户状态不存在");
    //    #else
    //    OATPP_ASSERT_HTTP(result->isSuccess() && result->hasMoreToFetch(), Status::CODE_404, "用户状态不存在");
    //    #endif
    //    return result->fetch<oatpp::Vector<oatpp::Object<UserStatusVO>>>()[0];
    //}

    oatpp::Vector<oatpp::Object<UserStatusVO>> getMultipleStatuses(const oatpp::String& currentUserIdHeader, const oatpp::Object<BatchStatusRequestDTO>& request) {
        OATPP_ASSERT_HTTP(request->userUuids, Status::CODE_401, "查询的用户为空");
        auto statuses = oatpp::Vector<oatpp::Object<UserStatusVO>>::createShared();
        for (const auto& targetUserIdStr : *request->userUuids) {
            auto targetUserId = m_idCache->getUserId(targetUserIdStr);
            if (targetUserId <= 0) {
                continue;
            }

            auto result = m_appPostgresql->getUserStatus(targetUserId);
            if (result->isSuccess()) {
                auto status = result->fetch<oatpp::Vector<oatpp::Object<UserStatusVO>>>()[0];
                statuses->push_back(status);
            }
        }
        return statuses;
    }

    //oatpp::Vector<oatpp::Object<UserInfoVO>> getOnlineUsers(const oatpp::String& currentUserIdHeader) {
    //    auto userCheck = m_appPostgresql->getUserIdByUuid(currentUserIdHeader);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(userCheck->isSuccess(), Status::CODE_500, userCheck->getErrorMessage());
    //    OATPP_ASSERT_HTTP(userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
    //    #else
    //    OATPP_ASSERT_HTTP(userCheck->isSuccess() && userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
    //    #endif

    //    auto result = m_appPostgresql->getOnlineUsers();
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, result->getErrorMessage());
    //    #else
    //    OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, "获取在线用户失败");
    //    #endif
    //    return result->fetch<oatpp::Vector<oatpp::Object<UserInfoVO>>>();
    //}

    //oatpp::Vector<oatpp::Object<UserStatusVO>> getFriendsStatus(const oatpp::String& currentUserIdHeader) {
    //    auto userCheck = m_appPostgresql->getUserIdByUuid(currentUserIdHeader);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(userCheck->isSuccess(), Status::CODE_500, userCheck->getErrorMessage());
    //    OATPP_ASSERT_HTTP(userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
    //    #else
    //    OATPP_ASSERT_HTTP(userCheck->isSuccess() && userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
    //    #endif
    //    auto id = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

    //    auto result = m_appPostgresql->getFriendsStatus(id);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, result->getErrorMessage());
    //    #else
    //    OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, "获取好友状态失败");
    //    #endif
    //    return result->fetch<oatpp::Vector<oatpp::Object<UserStatusVO>>>();
    //}

//    oatpp::Object<UserInfoVO> getCurrentUser(const oatpp::String& userUuid) {
//        //OATPP_LOGD("PATH", "经过");
//        auto userCheck = m_appPostgresql->getUserIdByUuid(userUuid);
//#ifdef SQLCHECK
//        OATPP_ASSERT_HTTP(userCheck->isSuccess(), Status::CODE_500, userCheck->getErrorMessage());
//        OATPP_ASSERT_HTTP(userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
//#else
//        OATPP_ASSERT_HTTP(userCheck->isSuccess() && userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
//#endif
//
//        auto id = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;
//        auto result = m_appPostgresql->getUserInfoById(id);
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
        auto userId = m_idCache->getUserId(userUuid);
        OATPP_ASSERT_HTTP(userId > 0, Status::CODE_401, "用户不存在或已失效");

        auto result = m_appPostgresql->updateUser(userId, request->username, request->avatarUrl);
#ifdef SQLCHECK
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, result->getErrorMessage());
        OATPP_ASSERT_HTTP(result->hasMoreToFetch(), Status::CODE_404, "用户不存在");
#else
        OATPP_ASSERT_HTTP(result->isSuccess() && result->hasMoreToFetch(), Status::CODE_404, "用户不存在");
#endif

        auto updatedUser = m_appPostgresql->getUserInfoById(userId);
#ifdef SQLCHECK
        OATPP_ASSERT_HTTP(updatedUser->isSuccess(), Status::CODE_500, updatedUser->getErrorMessage());
        OATPP_ASSERT_HTTP(updatedUser->hasMoreToFetch(), Status::CODE_404, "用户不存在");
#else
        OATPP_ASSERT_HTTP(updatedUser->isSuccess() && updatedUser->hasMoreToFetch(), Status::CODE_404, "用户不存在");
#endif

        return updatedUser->fetch<oatpp::Vector<oatpp::Object<UserInfoVO>>>()[0];
    }

    oatpp::Vector<oatpp::Object<SearchUserVO>> searchUsers(const oatpp::String& keyword, const oatpp::String& userUuid) {
        auto userId = m_idCache->getUserId(userUuid);
        OATPP_ASSERT_HTTP(userId > 0, Status::CODE_401, "用户不存在或已失效");

        OATPP_ASSERT_HTTP(keyword && !keyword->empty(), Status::CODE_400, "搜索关键词不能为空");
        auto searchKeyword = "%" + keyword + "%";
        auto result = m_appPostgresql->searchUsers(searchKeyword, userId);
#ifdef SQLCHECK
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, result->getErrorMessage());
#else
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, "搜索用户失败");
#endif
        auto users = result->fetch<oatpp::Vector<oatpp::Object<SearchUserVO>>>();
        if (!users) {
            users = oatpp::Vector<oatpp::Object<SearchUserVO>>::createShared();
        }
        return users;
    }

    oatpp::Object<UserInfoVO> getUserInfoByUuid(const oatpp::String& userUuid) {
        auto id = m_idCache->getUserId(userUuid);
        OATPP_ASSERT_HTTP(id > 0, Status::CODE_404, "用户不存在");

        // 获取用户基本信息
        auto userResult = m_appPostgresql->getUserInfoById(id);
#ifdef SQLCHECK
        OATPP_ASSERT_HTTP(userResult->isSuccess(), Status::CODE_500, userResult->getErrorMessage());
        OATPP_ASSERT_HTTP(userResult->hasMoreToFetch(), Status::CODE_404, "用户不存在");
#else
        OATPP_ASSERT_HTTP(userResult->isSuccess() && userResult->hasMoreToFetch(), Status::CODE_404, "用户不存在");
#endif
        return userResult->fetch<oatpp::Vector<oatpp::Object<UserInfoVO>>>()[0];

    }
};
