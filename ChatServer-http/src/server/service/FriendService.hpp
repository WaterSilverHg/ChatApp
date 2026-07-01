#pragma once

#include "global.h"
#include "server/dto/FriendDto.hpp"
#include "server/vo/FriendVo.hpp"
#include "server/postgresql/AppPostgresql.hpp"
#include "redis/AppRedis.hpp"
#include "tool/HashUtils.hpp"
#include "tool/UuidIdCache.hpp"

class FriendService {
private:
    std::shared_ptr<AppPostgresql> m_appPostgresql;
    std::shared_ptr<AppRedis> m_redis;
    std::shared_ptr<UuidIdCache> m_idCache;
    using Status = oatpp::web::protocol::http::Status;

    // 辅助：防重检查封装
    bool checkDedup(const oatpp::String& userUuid, const char* action,
                    const oatpp::String& target, int ttl = 3) {
        return m_redis->tryAcquireDedupLock(userUuid->c_str(), action, target->c_str(), "", ttl);
    }

public:
    FriendService(const std::shared_ptr<AppPostgresql>& appPostgresql, 
                  const std::shared_ptr<AppRedis>& redis,
                  const std::shared_ptr<UuidIdCache>& idCache)
        : m_appPostgresql(appPostgresql), m_redis(redis), m_idCache(idCache) {}

    //oatpp::Object<FriendResponseVO> sendFriendRequest(const oatpp::String& currentUserIdHeader, const oatpp::Object<SendFriendRequestDTO>& request) {
    //    OATPP_ASSERT_HTTP(currentUserIdHeader && !currentUserIdHeader->empty(), Status::CODE_400, "用户ID不能为空");
    //    OATPP_ASSERT_HTTP(request, Status::CODE_400, "请求参数不能为空");
    //    OATPP_ASSERT_HTTP(request->toUserUuid && !request->toUserUuid->empty(), Status::CODE_400, "目标用户ID不能为空");
    //    OATPP_ASSERT_HTTP(request->message && !request->message->empty(), Status::CODE_400, "请求消息不能为空");
    //    auto userCheck = m_appPostgresql->getUserIdByUuid(currentUserIdHeader);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(userCheck->isSuccess(), Status::CODE_500, userCheck->getErrorMessage());
    //    OATPP_ASSERT_HTTP(userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
    //    #else
    //    OATPP_ASSERT_HTTP(userCheck->isSuccess() && userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
    //    #endif
    //    auto user_id = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

    //    auto targetCheck = m_appPostgresql->getUserIdByUuid(request->toUserUuid);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(targetCheck->isSuccess(), Status::CODE_500, targetCheck->getErrorMessage());
    //    OATPP_ASSERT_HTTP(targetCheck->hasMoreToFetch(), Status::CODE_404, "目标用户不存在");
    //    #else
    //    OATPP_ASSERT_HTTP(targetCheck->isSuccess() && targetCheck->hasMoreToFetch(), Status::CODE_404, "目标用户不存在");
    //    #endif
    //    auto tuser_id = targetCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

    //    // 检查好友关系状态
    //    auto friendshipStatus1Result = m_appPostgresql->checkFriendshipStatus(user_id, tuser_id);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(friendshipStatus1Result->isSuccess(), Status::CODE_500, friendshipStatus1Result->getErrorMessage());
    //    #else
    //    OATPP_ASSERT_HTTP(friendshipStatus1Result->isSuccess(), Status::CODE_400, "检查好友关系状态失败");
    //    #endif

    //    if (friendshipStatus1Result->hasMoreToFetch()) {
    //        auto friendshipStatus = friendshipStatus1Result->fetch<oatpp::Vector<oatpp::Object<StatusDTO>>>()[0]->status;
    //        if (friendshipStatus == "accepted") {
    //            OATPP_ASSERT_HTTP(false, Status::CODE_400, "两人已经是好友，不能发送好友请求");
    //        } else if (friendshipStatus == "blocked") {
    //            OATPP_ASSERT_HTTP(false, Status::CODE_400, "对方已被拉黑，不能发送好友请求");
    //        }
    //        auto friendshipStatus2Result = m_appPostgresql->checkFriendshipStatus(tuser_id, user_id);
    //        #ifdef SQLCHECK
    //        OATPP_ASSERT_HTTP(friendshipStatus2Result->isSuccess(), Status::CODE_500, friendshipStatus2Result->getErrorMessage());
    //        #else
    //        OATPP_ASSERT_HTTP(friendshipStatus2Result->isSuccess(), Status::CODE_400, "检查好友关系状态失败");
    //        #endif
    //        if (friendshipStatus2Result->hasMoreToFetch()) {
    //            friendshipStatus = friendshipStatus2Result->fetch<oatpp::Vector<oatpp::Object<StatusDTO>>>()[0]->status;
    //            if (friendshipStatus == "blocked") {
    //                OATPP_ASSERT_HTTP(false, Status::CODE_400, "对方已拉黑你，不能发送好友请求");
    //            }
    //        }
    //    }

    //    auto checkResult = m_appPostgresql->checkFriendRequestExists(user_id, tuser_id);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(checkResult->isSuccess(), Status::CODE_500, checkResult->getErrorMessage());
    //    #else
    //    OATPP_ASSERT_HTTP(checkResult->isSuccess(), Status::CODE_400, "发送好友请求失败");
    //    #endif

    //    oatpp::Object<UuidDTO> ruuid;
    //    if (checkResult->hasMoreToFetch()) {
    //        auto updateResult = m_appPostgresql->updateFriendRequestToPending(user_id, tuser_id, request->message);
    //        #ifdef SQLCHECK
    //        OATPP_ASSERT_HTTP(updateResult->isSuccess(), Status::CODE_500, updateResult->getErrorMessage());
    //        OATPP_ASSERT_HTTP(updateResult->hasMoreToFetch(), Status::CODE_400, "更新好友请求失败");
    //        #else
    //        OATPP_ASSERT_HTTP(updateResult->isSuccess() && updateResult->hasMoreToFetch(), Status::CODE_400, "更新好友请求失败");
    //        #endif
    //        ruuid = updateResult->fetch<oatpp::Vector<oatpp::Object<UuidDTO>>>()[0];
    //    } else {
    //        auto result = m_appPostgresql->sendFriendRequest(user_id, tuser_id, request->message);
    //        #ifdef SQLCHECK
    //        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, result->getErrorMessage());
    //        OATPP_ASSERT_HTTP(result->hasMoreToFetch(), Status::CODE_400, "发送好友请求失败");
    //        #else
    //        OATPP_ASSERT_HTTP(result->isSuccess() && result->hasMoreToFetch(), Status::CODE_400, "发送好友请求失败");
    //        #endif
    //        ruuid = result->fetch<oatpp::Vector<oatpp::Object<UuidDTO>>>()[0];
    //    }

    //    auto response = FriendResponseVO::createShared();
    //    response->uuid = ruuid->uuid;
    //    response->fromUserUuid = currentUserIdHeader;
    //    response->toUserUuid = request->toUserUuid;
    //    response->message = request->message;
    //    response->status = "pending";
    //    return response;
    //}

    oatpp::Vector<oatpp::Object<SentFriendRequestVO>> getSentRequests(const oatpp::String& currentUserIdHeader) {
        OATPP_ASSERT_HTTP(currentUserIdHeader && !currentUserIdHeader->empty(), Status::CODE_400, "用户ID不能为空");
        auto user_id = m_idCache->getUserId(currentUserIdHeader);
        OATPP_ASSERT_HTTP(user_id > 0, Status::CODE_401, "用户不存在或已失效");

        auto result = m_appPostgresql->getSentFriendRequestsWithUserInfo(user_id);
        #ifdef SQLCHECK
        if(!result->isSuccess()) {
            OATPP_LOGE("SQL_ERROR", "%s", result->getErrorMessage()->c_str());
        }
        #endif
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, "获取发送的请求失败");
        
        auto requests = result->fetch<oatpp::Vector<oatpp::Object<SentFriendRequestDetailDTO>>>();
        auto response = oatpp::Vector<oatpp::Object<SentFriendRequestVO>>::createShared();
        
        for (const auto& request : *requests) {
            auto vo = SentFriendRequestVO::createShared();
            vo->uuid = request->uuid;
            vo->status = request->status;
            vo->createdAt = request->createdAt;
            vo->message = request->message;
            
            // 直接使用查询得到的用户信息
            auto userInfo = UserInfoVO::createShared();
            userInfo->userUuid = request->toUserUuid;
            userInfo->username = request->toUsername;
            userInfo->avatarUrl = request->toAvatarUrl;
            vo->receiver = userInfo;
            
            response->push_back(vo);
        }
        
        return response;
    }

    oatpp::Vector<oatpp::Object<ReceivedFriendRequestVO>> getReceivedRequests(const oatpp::String& currentUserIdHeader) {
        OATPP_ASSERT_HTTP(currentUserIdHeader && !currentUserIdHeader->empty(), Status::CODE_400, "用户ID不能为空");
        auto user_id = m_idCache->getUserId(currentUserIdHeader);
        OATPP_ASSERT_HTTP(user_id > 0, Status::CODE_401, "用户不存在或已失效");

        auto result = m_appPostgresql->getReceivedFriendRequestsWithUserInfo(user_id);
        #ifdef SQLCHECK
        if(!result->isSuccess()) {
            OATPP_LOGE("SQL_ERROR", "%s", result->getErrorMessage()->c_str());
        }
        #endif
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, "获取收到的请求失败");
        
        auto requests = result->fetch<oatpp::Vector<oatpp::Object<ReceivedFriendRequestDetailDTO>>>();
        auto response = oatpp::Vector<oatpp::Object<ReceivedFriendRequestVO>>::createShared();
        
        for (const auto& request : *requests) {
            auto vo = ReceivedFriendRequestVO::createShared();
            vo->uuid = request->uuid;
            vo->status = request->status;
            vo->createdAt = request->createdAt;
            vo->message = request->message;
            
            // 直接使用查询得到的用户信息
            auto userInfo = UserInfoVO::createShared();
            userInfo->userUuid = request->fromUserUuid;
            userInfo->username = request->fromUsername;
            userInfo->avatarUrl = request->fromAvatarUrl;
            vo->requester = userInfo;
            
            response->push_back(vo);
        }
        
        return response;
    }

    //oatpp::Boolean acceptFriendRequest(const oatpp::String& currentUserIdHeader, const oatpp::String& requestUuid) {
    //    OATPP_ASSERT_HTTP(currentUserIdHeader && !currentUserIdHeader->empty(), Status::CODE_400, "用户ID不能为空");
    //    OATPP_ASSERT_HTTP(requestUuid && !requestUuid->empty(), Status::CODE_400, "请求ID不能为空");

    //    auto userCheck = m_appPostgresql->getUserIdByUuid(currentUserIdHeader);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(userCheck->isSuccess(), Status::CODE_500, userCheck->getErrorMessage());
    //    OATPP_ASSERT_HTTP(userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
    //    #else
    //    OATPP_ASSERT_HTTP(userCheck->isSuccess() && userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
    //    #endif
    //    oatpp::Int64 currentUserId = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

    //    auto requestResult = m_appPostgresql->getFriendRequestByUuid(requestUuid);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(requestResult->isSuccess(), Status::CODE_500, requestResult->getErrorMessage());
    //    OATPP_ASSERT_HTTP(requestResult->hasMoreToFetch(), Status::CODE_404, "好友请求不存在");
    //    #else
    //    OATPP_ASSERT_HTTP(requestResult->isSuccess() && requestResult->hasMoreToFetch(), Status::CODE_404, "好友请求不存在");
    //    #endif

    //    auto friendRequest = requestResult->fetch<oatpp::Vector<oatpp::Object<FriendRequestResponseVO>>>()[0];

    //    OATPP_ASSERT_HTTP(friendRequest->toUserUuid == currentUserIdHeader, Status::CODE_403, "您没有权限处理此请求");

    //    auto fromUserResult = m_appPostgresql->getUserIdByUuid(friendRequest->fromUserUuid);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(fromUserResult->isSuccess(), Status::CODE_500, fromUserResult->getErrorMessage());
    //    OATPP_ASSERT_HTTP(fromUserResult->hasMoreToFetch(), Status::CODE_404, "发送者用户不存在");
    //    #else
    //    OATPP_ASSERT_HTTP(fromUserResult->isSuccess() && fromUserResult->hasMoreToFetch(), Status::CODE_404, "发送者用户不存在");
    //    #endif
    //    oatpp::Int64 fromUserId = fromUserResult->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

    //    auto toUserResult = m_appPostgresql->getUserIdByUuid(friendRequest->toUserUuid);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(toUserResult->isSuccess(), Status::CODE_500, toUserResult->getErrorMessage());
    //    OATPP_ASSERT_HTTP(toUserResult->hasMoreToFetch(), Status::CODE_404, "接收者用户不存在");
    //    #else
    //    OATPP_ASSERT_HTTP(toUserResult->isSuccess() && toUserResult->hasMoreToFetch(), Status::CODE_404, "接收者用户不存在");
    //    #endif
    //    oatpp::Int64 toUserId = toUserResult->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

    //    auto updateResult = m_appPostgresql->updateFriendRequestStatus(requestUuid, "accepted");
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(updateResult->isSuccess(), Status::CODE_500, updateResult->getErrorMessage());
    //    OATPP_ASSERT_HTTP(updateResult->hasMoreToFetch(), Status::CODE_404, "好友请求不存在");
    //    #else
    //    OATPP_ASSERT_HTTP(updateResult->isSuccess() && updateResult->hasMoreToFetch(), Status::CODE_404, "好友请求不存在");
    //    #endif

    //    auto checkFriendshipResult = m_appPostgresql->checkFriendshipStatus(fromUserId, toUserId);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(checkFriendshipResult->isSuccess(), Status::CODE_500, checkFriendshipResult->getErrorMessage());
    //    #else
    //    OATPP_ASSERT_HTTP(checkFriendshipResult->isSuccess(), Status::CODE_500, "检查好友关系失败");
    //    #endif

    //    if (checkFriendshipResult->hasMoreToFetch()) {
    //        auto updateFriendshipResult = m_appPostgresql->acceptFriendshipStatus(fromUserId, toUserId);
    //        #ifdef SQLCHECK
    //        OATPP_ASSERT_HTTP(updateFriendshipResult->isSuccess(), Status::CODE_500, updateFriendshipResult->getErrorMessage());
    //        #else
    //        OATPP_ASSERT_HTTP(updateFriendshipResult->isSuccess(), Status::CODE_500, "更新好友关系失败");
    //        #endif
    //    } else {
    //        auto createFriendshipResult = m_appPostgresql->createFriendship(fromUserId, toUserId);
    //        #ifdef SQLCHECK
    //        OATPP_ASSERT_HTTP(createFriendshipResult->isSuccess(), Status::CODE_500, createFriendshipResult->getErrorMessage());
    //        #else
    //        OATPP_ASSERT_HTTP(createFriendshipResult->isSuccess(), Status::CODE_500, "创建好友关系失败");
    //        #endif
    //    }

    //    auto createConv1Result = m_appPostgresql->createPrivateConversation(fromUserId, toUserId);
    //    #ifdef SQLCHECK
    //    if (!createConv1Result->isSuccess()) {
    //        OATPP_LOGD("WARRING", "%s", createConv1Result->getErrorMessage());
    //    }
    //    #endif

    //    auto createConv2Result = m_appPostgresql->createPrivateConversation(toUserId, fromUserId);
    //    #ifdef SQLCHECK
    //    if (!createConv2Result->isSuccess()) {
    //        OATPP_LOGD("WARRING", "%s", createConv2Result->getErrorMessage());
    //    }
    //    #endif

    //    return true;
    //}

    //oatpp::Boolean rejectFriendRequest(const oatpp::String& requestUuid) {
    //    OATPP_ASSERT_HTTP(requestUuid && !requestUuid->empty(), Status::CODE_400, "请求ID不能为空");
    //    auto result = m_appPostgresql->updateFriendRequestStatus(requestUuid, "reject");
    //    OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_400, "拒绝请求失败");
    //    return true;
    //}

    //oatpp::Boolean cancelFriendRequest(const oatpp::String& requestUuid) {
    //    OATPP_ASSERT_HTTP(requestUuid && !requestUuid->empty(), Status::CODE_400, "请求ID不能为空");
    //    auto result = m_appPostgresql->cancelFriendRequest(requestUuid);
    //    OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_400, "取消请求失败");
    //    return true;
    //}

    oatpp::Vector<oatpp::Object<FriendInfoVO>> getFriends(const oatpp::String& currentUserIdHeader) {
        OATPP_ASSERT_HTTP(currentUserIdHeader && !currentUserIdHeader->empty(), Status::CODE_400, "用户ID不能为空");
        auto user_id = m_idCache->getUserId(currentUserIdHeader);
        OATPP_ASSERT_HTTP(user_id > 0, Status::CODE_401, "用户不存在或已失效");

        auto result = m_appPostgresql->getFriends(user_id);
        #ifdef SQLCHECK
        if(!result->isSuccess()) {
            OATPP_LOGE("SQL_ERROR", "%s", result->getErrorMessage()->c_str());
        }
        #endif
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, "获取好友列表失败");
        return result->fetch<oatpp::Vector<oatpp::Object<FriendInfoVO>>>();
    }

    oatpp::Object<UserDetailInfoVO> getUserDetail(const oatpp::String& currentUserIdHeader, const oatpp::String& friendUuid) {
        OATPP_ASSERT_HTTP(currentUserIdHeader && !currentUserIdHeader->empty(), Status::CODE_400, "用户ID不能为空");
        OATPP_ASSERT_HTTP(friendUuid && !friendUuid->empty(), Status::CODE_400, "好友ID不能为空");
        auto userId = m_idCache->getUserId(currentUserIdHeader);
        OATPP_ASSERT_HTTP(userId > 0, Status::CODE_401, "用户不存在或已失效");

        auto friendId = m_idCache->getUserId(friendUuid);
        OATPP_ASSERT_HTTP(friendId > 0, Status::CODE_404, "好友不存在");

        auto result = m_appPostgresql->getFriendDetail(userId, friendId);
        #ifdef SQLCHECK
        if(!result->isSuccess()) {
            OATPP_LOGE("SQL_ERROR", "%s", result->getErrorMessage()->c_str());
        }
        #endif
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, "获取好友详情失败");
        OATPP_ASSERT_HTTP(result->hasMoreToFetch(), Status::CODE_404, "好友不存在或不是您的好友");

        return result->fetch<oatpp::Vector<oatpp::Object<UserDetailInfoVO>>>()[0];
    }

    oatpp::Vector<oatpp::Object<FriendInfoVO>> getBlockedUsers(const oatpp::String& currentUserIdHeader) {
        OATPP_ASSERT_HTTP(currentUserIdHeader && !currentUserIdHeader->empty(), Status::CODE_400, "用户ID不能为空");
        auto user_id = m_idCache->getUserId(currentUserIdHeader);
        OATPP_ASSERT_HTTP(user_id > 0, Status::CODE_401, "用户不存在或已失效");

        auto result = m_appPostgresql->getBlockedUsers(user_id);
        #ifdef SQLCHECK
        if(!result->isSuccess()) {
            OATPP_LOGE("SQL_ERROR", "%s", result->getErrorMessage()->c_str());
        }
        #endif
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, "获取黑名单列表失败");
        return result->fetch<oatpp::Vector<oatpp::Object<FriendInfoVO>>>();
    }

    //oatpp::Vector<oatpp::Object<FriendInfoVO>> getOnlineFriends(const oatpp::String& currentUserIdHeader) {
    //    OATPP_ASSERT_HTTP(currentUserIdHeader && !currentUserIdHeader->empty(), Status::CODE_400, "用户ID不能为空");
    //    auto userCheck = m_appPostgresql->getUserIdByUuid(currentUserIdHeader);
    //    OATPP_ASSERT_HTTP(userCheck->isSuccess(), Status::CODE_500, userCheck->getErrorMessage());
    //    OATPP_ASSERT_HTTP(userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
    //    auto user_id = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

    //    auto result = m_appPostgresql->getFriends(user_id);
    //    OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, result->getErrorMessage());

    //    auto friends = result->fetch<oatpp::Vector<oatpp::Object<FriendInfoVO>>>();
    //    auto onlineFriends = oatpp::Vector<oatpp::Object<FriendInfoVO>>::createShared();
    //    for (const auto& friendInfo : *friends) {
    //        if (friendInfo->status && friendInfo->status.getValue("") == "online") {
    //            onlineFriends->push_back(friendInfo);
    //        }
    //    }
    //    return onlineFriends;
    //}

    oatpp::Boolean updateFriendRemark(const oatpp::String& currentUserIdHeader, const oatpp::String& friendUuid, const oatpp::Object<UpdateFriendRemarkDTO>& request) {
        OATPP_ASSERT_HTTP(currentUserIdHeader && !currentUserIdHeader->empty(), Status::CODE_400, "用户ID不能为空");
        OATPP_ASSERT_HTTP(friendUuid && !friendUuid->empty(), Status::CODE_400, "好友ID不能为空");
        OATPP_ASSERT_HTTP(request, Status::CODE_400, "请求参数不能为空");
        OATPP_ASSERT_HTTP(request->remark != nullptr, Status::CODE_400, "备注不能为空");
        auto user_id = m_idCache->getUserId(currentUserIdHeader);
        OATPP_ASSERT_HTTP(user_id > 0, Status::CODE_401, "用户不存在或已失效");

        auto fuser_id = m_idCache->getUserId(friendUuid);
        OATPP_ASSERT_HTTP(fuser_id > 0, Status::CODE_404, "好友不存在");

        auto result = m_appPostgresql->updateFriendRemark(user_id, fuser_id, request->remark);
        #ifdef SQLCHECK
        if(!result->isSuccess()) {
            OATPP_LOGE("SQL_ERROR", "%s", result->getErrorMessage()->c_str());
        }
        #endif
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_400, "修改备注失败");
        return true;
    }

    oatpp::Boolean updateFriendGroup(const oatpp::String& currentUserIdHeader, const oatpp::String& friendUuid, const oatpp::Object<UpdateFriendGroupDTO>& request) {
        OATPP_ASSERT_HTTP(currentUserIdHeader && !currentUserIdHeader->empty(), Status::CODE_400, "用户ID不能为空");
        OATPP_ASSERT_HTTP(friendUuid && !friendUuid->empty(), Status::CODE_400, "好友ID不能为空");
        OATPP_ASSERT_HTTP(request, Status::CODE_400, "请求参数不能为空");
        OATPP_ASSERT_HTTP(request->group && !request->group->empty(), Status::CODE_400, "分组名称不能为空");
        auto user_id = m_idCache->getUserId(currentUserIdHeader);
        OATPP_ASSERT_HTTP(user_id > 0, Status::CODE_401, "用户不存在或已失效");

        auto fuser_id = m_idCache->getUserId(friendUuid);
        OATPP_ASSERT_HTTP(fuser_id > 0, Status::CODE_404, "好友不存在");

        auto result = m_appPostgresql->updateFriendGroup(user_id, fuser_id, request->group);
        #ifdef SQLCHECK
        if(!result->isSuccess()) {
            OATPP_LOGE("SQL_ERROR", "%s", result->getErrorMessage()->c_str());
        }
        #endif
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_400, "移动分组失败");
        return true;
    }

    oatpp::Boolean deleteFriend(const oatpp::String& currentUserIdHeader, const oatpp::String& friendUuid) {
        OATPP_ASSERT_HTTP(currentUserIdHeader && !currentUserIdHeader->empty(), Status::CODE_400, "用户ID不能为空");
        OATPP_ASSERT_HTTP(friendUuid && !friendUuid->empty(), Status::CODE_400, "好友ID不能为空");
        auto user_id = m_idCache->getUserId(currentUserIdHeader);
        OATPP_ASSERT_HTTP(user_id > 0, Status::CODE_401, "用户不存在或已失效");

        auto fuser_id = m_idCache->getUserId(friendUuid);
        OATPP_ASSERT_HTTP(fuser_id > 0, Status::CODE_404, "好友不存在");

        auto result = m_appPostgresql->deleteFriend(user_id, fuser_id);
        #ifdef SQLCHECK
        if(!result->isSuccess()) {
            OATPP_LOGE("SQL_ERROR", "%s", result->getErrorMessage()->c_str());
        }
        #endif
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_400, "删除好友失败");

        return true;
    }

    oatpp::Boolean blockUser(const oatpp::String& currentUserIdHeader, const oatpp::String& targetUserUuid) {
        OATPP_ASSERT_HTTP(currentUserIdHeader && !currentUserIdHeader->empty(), Status::CODE_400, "用户ID不能为空");
        OATPP_ASSERT_HTTP(targetUserUuid && !targetUserUuid->empty(), Status::CODE_400, "目标用户ID不能为空");

        // 去重检查
        if (!m_redis->tryAcquireDedupLock(currentUserIdHeader->c_str(), "block", targetUserUuid->c_str())) {
            OATPP_ASSERT_HTTP(false, Status::CODE_429, "请求频繁");
        }

        auto user_id = m_idCache->getUserId(currentUserIdHeader);
        OATPP_ASSERT_HTTP(user_id > 0, Status::CODE_401, "用户不存在或已失效");

        auto target_id = m_idCache->getUserId(targetUserUuid);
        OATPP_ASSERT_HTTP(target_id > 0, Status::CODE_404, "目标用户不存在");

        // 检查好友关系状态
        auto friendshipStatusResult = m_appPostgresql->checkFriendshipStatus(user_id, target_id);
        #ifdef SQLCHECK
        if(!friendshipStatusResult->isSuccess()) {
            OATPP_LOGE("SQL_ERROR", "%s", friendshipStatusResult->getErrorMessage()->c_str());
        }
        #endif
        OATPP_ASSERT_HTTP(friendshipStatusResult->isSuccess(), Status::CODE_500, "检查好友关系失败");

        // 只有好友关系才能拉黑
        if (friendshipStatusResult->hasMoreToFetch()) {
            auto friendshipStatus = friendshipStatusResult->fetch<oatpp::Vector<oatpp::Object<StatusDTO>>>()[0]->status;
            if (friendshipStatus != "accepted") {
                OATPP_ASSERT_HTTP(false, Status::CODE_400, "只有好友关系才能拉黑");
            }
        } else {
            OATPP_ASSERT_HTTP(false, Status::CODE_400, "只有好友关系才能拉黑");
        }

        auto result = m_appPostgresql->blockUser(user_id, target_id);
        #ifdef SQLCHECK
        if(!result->isSuccess()) {
            OATPP_LOGE("SQL_ERROR", "%s", result->getErrorMessage()->c_str());
        }
        #endif
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_400, "拉黑用户失败");
        return true;
    }

    oatpp::Boolean unblockUser(const oatpp::String& currentUserIdHeader, const oatpp::String& targetUserUuid) {
        OATPP_ASSERT_HTTP(currentUserIdHeader && !currentUserIdHeader->empty(), Status::CODE_400, "用户ID不能为空");
        OATPP_ASSERT_HTTP(targetUserUuid && !targetUserUuid->empty(), Status::CODE_400, "目标用户ID不能为空");
        auto user_id = m_idCache->getUserId(currentUserIdHeader);
        OATPP_ASSERT_HTTP(user_id > 0, Status::CODE_401, "用户不存在或已失效");

        auto target_id = m_idCache->getUserId(targetUserUuid);
        OATPP_ASSERT_HTTP(target_id > 0, Status::CODE_404, "目标用户不存在");

        // 检查好友关系状态
        auto friendshipStatusResult = m_appPostgresql->checkFriendshipStatus(user_id, target_id);
        #ifdef SQLCHECK
        if(!friendshipStatusResult->isSuccess()) {
            OATPP_LOGE("SQL_ERROR", "%s", friendshipStatusResult->getErrorMessage()->c_str());
        }
        #endif
        OATPP_ASSERT_HTTP(friendshipStatusResult->isSuccess(), Status::CODE_500, "检查好友关系失败");

        // 只有被拉黑的关系才能取消拉黑
        if (friendshipStatusResult->hasMoreToFetch()) {
            auto friendshipStatus = friendshipStatusResult->fetch<oatpp::Vector<oatpp::Object<StatusDTO>>>()[0]->status;
            if (friendshipStatus != "block") {
                OATPP_ASSERT_HTTP(false, Status::CODE_400, "只有被拉黑的关系才能取消拉黑");
            }
        } else {
            OATPP_ASSERT_HTTP(false, Status::CODE_400, "只有被拉黑的关系才能取消拉黑");
        }

        auto result = m_appPostgresql->unblockUser(user_id, target_id);
        #ifdef SQLCHECK
        if(!result->isSuccess()) {
            OATPP_LOGE("SQL_ERROR", "%s", result->getErrorMessage()->c_str());
        }
        #endif
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_400, "取消拉黑用户失败");
        return true;
    }

    // ==================== 好友分组管理 ====================

    oatpp::Vector<oatpp::Object<GroupNameVO>> getFriendGroups(const oatpp::String& currentUserIdHeader) {
        OATPP_ASSERT_HTTP(currentUserIdHeader && !currentUserIdHeader->empty(), Status::CODE_400, "用户ID不能为空");
        auto user_id = m_idCache->getUserId(currentUserIdHeader);
        OATPP_ASSERT_HTTP(user_id > 0, Status::CODE_401, "用户不存在或已失效");

        auto result = m_appPostgresql->getFriendGroupNames(user_id);
        #ifdef SQLCHECK
        if(!result->isSuccess()) {
            OATPP_LOGE("SQL_ERROR", "%s", result->getErrorMessage()->c_str());
        }
        #endif
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, "获取分组列表失败");
        return result->fetch<oatpp::Vector<oatpp::Object<GroupNameVO>>>();
    }

    oatpp::Boolean createFriendGroup(const oatpp::String& currentUserIdHeader, const oatpp::Object<CreateFriendGroupDTO>& request) {
        OATPP_ASSERT_HTTP(currentUserIdHeader && !currentUserIdHeader->empty(), Status::CODE_400, "用户ID不能为空");
        OATPP_ASSERT_HTTP(request, Status::CODE_400, "请求参数不能为空");
        OATPP_ASSERT_HTTP(request->name && !request->name->empty(), Status::CODE_400, "分组名称不能为空");
        OATPP_ASSERT_HTTP(request->name != "默认分组", Status::CODE_400, "不能与默认分组重名");
        OATPP_ASSERT_HTTP(request->name->length() <= 50, Status::CODE_400, "分组名称过长");

        auto user_id = m_idCache->getUserId(currentUserIdHeader);
        OATPP_ASSERT_HTTP(user_id > 0, Status::CODE_401, "用户不存在或已失效");

        // 检查分组名是否已存在
        auto existing = m_appPostgresql->getFriendGroupNames(user_id);
        if (existing->isSuccess() && existing->hasMoreToFetch()) {
            auto groups = existing->fetch<oatpp::Vector<oatpp::Object<GroupNameVO>>>();
            for (auto& g : *groups) {
                if (g->groupName == request->name) {
                    OATPP_ASSERT_HTTP(false, Status::CODE_409, "分组名称已存在");
                }
            }
        }

        // 如果有初始成员，移动他们到新分组
        if (request->memberUuids && !request->memberUuids->empty()) {
            for (auto& friendUuid : *request->memberUuids) {
                auto friend_id = m_idCache->getUserId(friendUuid);
                if (friend_id > 0) {
                    m_appPostgresql->updateFriendGroup(user_id, friend_id, request->name);
                }
            }
        }

        return true;
    }

    oatpp::Boolean deleteFriendGroup(const oatpp::String& currentUserIdHeader, const oatpp::String& groupName) {
        OATPP_ASSERT_HTTP(currentUserIdHeader && !currentUserIdHeader->empty(), Status::CODE_400, "用户ID不能为空");
        OATPP_ASSERT_HTTP(groupName && !groupName->empty(), Status::CODE_400, "分组名称不能为空");
        OATPP_ASSERT_HTTP(groupName != "默认分组", Status::CODE_400, "默认分组不能删除");

        auto user_id = m_idCache->getUserId(currentUserIdHeader);
        OATPP_ASSERT_HTTP(user_id > 0, Status::CODE_401, "用户不存在或已失效");

        auto result = m_appPostgresql->resetFriendGroupToDefault(user_id, groupName);
        #ifdef SQLCHECK
        if(!result->isSuccess()) {
            OATPP_LOGE("SQL_ERROR", "%s", result->getErrorMessage()->c_str());
        }
        #endif
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, "删除分组失败");
        return true;
    }

    oatpp::Boolean renameFriendGroup(const oatpp::String& currentUserIdHeader, const oatpp::String& oldName, const oatpp::Object<RenameFriendGroupDTO>& request) {
        OATPP_ASSERT_HTTP(currentUserIdHeader && !currentUserIdHeader->empty(), Status::CODE_400, "用户ID不能为空");
        OATPP_ASSERT_HTTP(oldName && !oldName->empty(), Status::CODE_400, "原分组名称不能为空");
        OATPP_ASSERT_HTTP(request && request->newName && !request->newName->empty(), Status::CODE_400, "新分组名称不能为空");
        OATPP_ASSERT_HTTP(oldName != "默认分组", Status::CODE_400, "默认分组不能重命名");
        OATPP_ASSERT_HTTP(request->newName->length() <= 50, Status::CODE_400, "分组名称过长");

        auto user_id = m_idCache->getUserId(currentUserIdHeader);
        OATPP_ASSERT_HTTP(user_id > 0, Status::CODE_401, "用户不存在或已失效");

        auto result = m_appPostgresql->renameFriendGroup(user_id, oldName, request->newName);
        #ifdef SQLCHECK
        if(!result->isSuccess()) {
            OATPP_LOGE("SQL_ERROR", "%s", result->getErrorMessage()->c_str());
        }
        #endif
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, "重命名分组失败");
        return true;
    }
};
