#pragma once

#include "global.h"
#include "../dto/FriendDto.hpp"
#include "../vo/FriendVo.hpp"
#include "../postgresql/AppClient.hpp"

class FriendService {
private:
    std::shared_ptr<AppClient> m_appClient;
    using Status = oatpp::web::protocol::http::Status;
public:
    FriendService(const std::shared_ptr<AppClient>& appClient) : m_appClient(appClient) {}

    //oatpp::Object<FriendResponseVO> sendFriendRequest(const oatpp::String& currentUserIdHeader, const oatpp::Object<SendFriendRequestDTO>& request) {
    //    OATPP_ASSERT_HTTP(currentUserIdHeader && !currentUserIdHeader->empty(), Status::CODE_400, "用户ID不能为空");
    //    OATPP_ASSERT_HTTP(request, Status::CODE_400, "请求参数不能为空");
    //    OATPP_ASSERT_HTTP(request->toUserUuid && !request->toUserUuid->empty(), Status::CODE_400, "目标用户ID不能为空");
    //    OATPP_ASSERT_HTTP(request->message && !request->message->empty(), Status::CODE_400, "请求消息不能为空");
    //    auto userCheck = m_appClient->getUserIdByUuid(currentUserIdHeader);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(userCheck->isSuccess(), Status::CODE_500, userCheck->getErrorMessage());
    //    OATPP_ASSERT_HTTP(userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
    //    #else
    //    OATPP_ASSERT_HTTP(userCheck->isSuccess() && userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
    //    #endif
    //    auto user_id = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

    //    auto targetCheck = m_appClient->getUserIdByUuid(request->toUserUuid);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(targetCheck->isSuccess(), Status::CODE_500, targetCheck->getErrorMessage());
    //    OATPP_ASSERT_HTTP(targetCheck->hasMoreToFetch(), Status::CODE_404, "目标用户不存在");
    //    #else
    //    OATPP_ASSERT_HTTP(targetCheck->isSuccess() && targetCheck->hasMoreToFetch(), Status::CODE_404, "目标用户不存在");
    //    #endif
    //    auto tuser_id = targetCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

    //    // 检查好友关系状态
    //    auto friendshipStatus1Result = m_appClient->checkFriendshipStatus(user_id, tuser_id);
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
    //        auto friendshipStatus2Result = m_appClient->checkFriendshipStatus(tuser_id, user_id);
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

    //    auto checkResult = m_appClient->checkFriendRequestExists(user_id, tuser_id);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(checkResult->isSuccess(), Status::CODE_500, checkResult->getErrorMessage());
    //    #else
    //    OATPP_ASSERT_HTTP(checkResult->isSuccess(), Status::CODE_400, "发送好友请求失败");
    //    #endif

    //    oatpp::Object<UuidDTO> ruuid;
    //    if (checkResult->hasMoreToFetch()) {
    //        auto updateResult = m_appClient->updateFriendRequestToPending(user_id, tuser_id, request->message);
    //        #ifdef SQLCHECK
    //        OATPP_ASSERT_HTTP(updateResult->isSuccess(), Status::CODE_500, updateResult->getErrorMessage());
    //        OATPP_ASSERT_HTTP(updateResult->hasMoreToFetch(), Status::CODE_400, "更新好友请求失败");
    //        #else
    //        OATPP_ASSERT_HTTP(updateResult->isSuccess() && updateResult->hasMoreToFetch(), Status::CODE_400, "更新好友请求失败");
    //        #endif
    //        ruuid = updateResult->fetch<oatpp::Vector<oatpp::Object<UuidDTO>>>()[0];
    //    } else {
    //        auto result = m_appClient->sendFriendRequest(user_id, tuser_id, request->message);
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
        auto userCheck = m_appClient->getUserIdByUuid(currentUserIdHeader);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(userCheck->isSuccess(), Status::CODE_500, userCheck->getErrorMessage());
        OATPP_ASSERT_HTTP(userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
        #else
        OATPP_ASSERT_HTTP(userCheck->isSuccess() && userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
        #endif
        auto user_id = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        auto result = m_appClient->getSentFriendRequestsWithUserInfo(user_id);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, result->getErrorMessage());
        #else
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, "获取发送的请求失败");
        #endif
        
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
        auto userCheck = m_appClient->getUserIdByUuid(currentUserIdHeader);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(userCheck->isSuccess(), Status::CODE_500, userCheck->getErrorMessage());
        OATPP_ASSERT_HTTP(userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
        #else
        OATPP_ASSERT_HTTP(userCheck->isSuccess() && userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
        #endif
        auto user_id = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        auto result = m_appClient->getReceivedFriendRequestsWithUserInfo(user_id);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, result->getErrorMessage());
        #else
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, "获取收到的请求失败");
        #endif
        
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

    //    auto userCheck = m_appClient->getUserIdByUuid(currentUserIdHeader);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(userCheck->isSuccess(), Status::CODE_500, userCheck->getErrorMessage());
    //    OATPP_ASSERT_HTTP(userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
    //    #else
    //    OATPP_ASSERT_HTTP(userCheck->isSuccess() && userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
    //    #endif
    //    oatpp::Int64 currentUserId = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

    //    auto requestResult = m_appClient->getFriendRequestByUuid(requestUuid);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(requestResult->isSuccess(), Status::CODE_500, requestResult->getErrorMessage());
    //    OATPP_ASSERT_HTTP(requestResult->hasMoreToFetch(), Status::CODE_404, "好友请求不存在");
    //    #else
    //    OATPP_ASSERT_HTTP(requestResult->isSuccess() && requestResult->hasMoreToFetch(), Status::CODE_404, "好友请求不存在");
    //    #endif

    //    auto friendRequest = requestResult->fetch<oatpp::Vector<oatpp::Object<FriendRequestResponseVO>>>()[0];

    //    OATPP_ASSERT_HTTP(friendRequest->toUserUuid == currentUserIdHeader, Status::CODE_403, "您没有权限处理此请求");

    //    auto fromUserResult = m_appClient->getUserIdByUuid(friendRequest->fromUserUuid);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(fromUserResult->isSuccess(), Status::CODE_500, fromUserResult->getErrorMessage());
    //    OATPP_ASSERT_HTTP(fromUserResult->hasMoreToFetch(), Status::CODE_404, "发送者用户不存在");
    //    #else
    //    OATPP_ASSERT_HTTP(fromUserResult->isSuccess() && fromUserResult->hasMoreToFetch(), Status::CODE_404, "发送者用户不存在");
    //    #endif
    //    oatpp::Int64 fromUserId = fromUserResult->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

    //    auto toUserResult = m_appClient->getUserIdByUuid(friendRequest->toUserUuid);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(toUserResult->isSuccess(), Status::CODE_500, toUserResult->getErrorMessage());
    //    OATPP_ASSERT_HTTP(toUserResult->hasMoreToFetch(), Status::CODE_404, "接收者用户不存在");
    //    #else
    //    OATPP_ASSERT_HTTP(toUserResult->isSuccess() && toUserResult->hasMoreToFetch(), Status::CODE_404, "接收者用户不存在");
    //    #endif
    //    oatpp::Int64 toUserId = toUserResult->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

    //    auto updateResult = m_appClient->updateFriendRequestStatus(requestUuid, "accepted");
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(updateResult->isSuccess(), Status::CODE_500, updateResult->getErrorMessage());
    //    OATPP_ASSERT_HTTP(updateResult->hasMoreToFetch(), Status::CODE_404, "好友请求不存在");
    //    #else
    //    OATPP_ASSERT_HTTP(updateResult->isSuccess() && updateResult->hasMoreToFetch(), Status::CODE_404, "好友请求不存在");
    //    #endif

    //    auto checkFriendshipResult = m_appClient->checkFriendshipStatus(fromUserId, toUserId);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(checkFriendshipResult->isSuccess(), Status::CODE_500, checkFriendshipResult->getErrorMessage());
    //    #else
    //    OATPP_ASSERT_HTTP(checkFriendshipResult->isSuccess(), Status::CODE_500, "检查好友关系失败");
    //    #endif

    //    if (checkFriendshipResult->hasMoreToFetch()) {
    //        auto updateFriendshipResult = m_appClient->acceptFriendshipStatus(fromUserId, toUserId);
    //        #ifdef SQLCHECK
    //        OATPP_ASSERT_HTTP(updateFriendshipResult->isSuccess(), Status::CODE_500, updateFriendshipResult->getErrorMessage());
    //        #else
    //        OATPP_ASSERT_HTTP(updateFriendshipResult->isSuccess(), Status::CODE_500, "更新好友关系失败");
    //        #endif
    //    } else {
    //        auto createFriendshipResult = m_appClient->createFriendship(fromUserId, toUserId);
    //        #ifdef SQLCHECK
    //        OATPP_ASSERT_HTTP(createFriendshipResult->isSuccess(), Status::CODE_500, createFriendshipResult->getErrorMessage());
    //        #else
    //        OATPP_ASSERT_HTTP(createFriendshipResult->isSuccess(), Status::CODE_500, "创建好友关系失败");
    //        #endif
    //    }

    //    auto createConv1Result = m_appClient->createPrivateConversation(fromUserId, toUserId);
    //    #ifdef SQLCHECK
    //    if (!createConv1Result->isSuccess()) {
    //        OATPP_LOGD("WARRING", "%s", createConv1Result->getErrorMessage());
    //    }
    //    #endif

    //    auto createConv2Result = m_appClient->createPrivateConversation(toUserId, fromUserId);
    //    #ifdef SQLCHECK
    //    if (!createConv2Result->isSuccess()) {
    //        OATPP_LOGD("WARRING", "%s", createConv2Result->getErrorMessage());
    //    }
    //    #endif

    //    return true;
    //}

    //oatpp::Boolean rejectFriendRequest(const oatpp::String& requestUuid) {
    //    OATPP_ASSERT_HTTP(requestUuid && !requestUuid->empty(), Status::CODE_400, "请求ID不能为空");
    //    auto result = m_appClient->updateFriendRequestStatus(requestUuid, "reject");
    //    OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_400, "拒绝请求失败");
    //    return true;
    //}

    //oatpp::Boolean cancelFriendRequest(const oatpp::String& requestUuid) {
    //    OATPP_ASSERT_HTTP(requestUuid && !requestUuid->empty(), Status::CODE_400, "请求ID不能为空");
    //    auto result = m_appClient->cancelFriendRequest(requestUuid);
    //    OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_400, "取消请求失败");
    //    return true;
    //}

    oatpp::Vector<oatpp::Object<FriendInfoVO>> getFriends(const oatpp::String& currentUserIdHeader) {
        OATPP_ASSERT_HTTP(currentUserIdHeader && !currentUserIdHeader->empty(), Status::CODE_400, "用户ID不能为空");
        auto userCheck = m_appClient->getUserIdByUuid(currentUserIdHeader);
        OATPP_ASSERT_HTTP(userCheck->isSuccess(), Status::CODE_500, userCheck->getErrorMessage());
        OATPP_ASSERT_HTTP(userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
        auto user_id = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        auto result = m_appClient->getFriends(user_id);
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, result->getErrorMessage());
        return result->fetch<oatpp::Vector<oatpp::Object<FriendInfoVO>>>();
    }

    //oatpp::Vector<oatpp::Object<FriendInfoVO>> getOnlineFriends(const oatpp::String& currentUserIdHeader) {
    //    OATPP_ASSERT_HTTP(currentUserIdHeader && !currentUserIdHeader->empty(), Status::CODE_400, "用户ID不能为空");
    //    auto userCheck = m_appClient->getUserIdByUuid(currentUserIdHeader);
    //    OATPP_ASSERT_HTTP(userCheck->isSuccess(), Status::CODE_500, userCheck->getErrorMessage());
    //    OATPP_ASSERT_HTTP(userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
    //    auto user_id = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

    //    auto result = m_appClient->getFriends(user_id);
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
        auto userCheck = m_appClient->getUserIdByUuid(currentUserIdHeader);
        OATPP_ASSERT_HTTP(userCheck->isSuccess(), Status::CODE_500, userCheck->getErrorMessage());
        OATPP_ASSERT_HTTP(userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
        auto user_id = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        auto friendCheck = m_appClient->getUserIdByUuid(friendUuid);
        OATPP_ASSERT_HTTP(friendCheck->isSuccess(), Status::CODE_500, friendCheck->getErrorMessage());
        OATPP_ASSERT_HTTP(friendCheck->hasMoreToFetch(), Status::CODE_404, "好友不存在");
        auto fuser_id = friendCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        auto result = m_appClient->updateFriendRemark(user_id, fuser_id, request->remark);
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_400, "修改备注失败");
        return true;
    }

    oatpp::Boolean updateFriendGroup(const oatpp::String& currentUserIdHeader, const oatpp::String& friendUuid, const oatpp::Object<UpdateFriendGroupDTO>& request) {
        OATPP_ASSERT_HTTP(currentUserIdHeader && !currentUserIdHeader->empty(), Status::CODE_400, "用户ID不能为空");
        OATPP_ASSERT_HTTP(friendUuid && !friendUuid->empty(), Status::CODE_400, "好友ID不能为空");
        OATPP_ASSERT_HTTP(request, Status::CODE_400, "请求参数不能为空");
        OATPP_ASSERT_HTTP(request->group && !request->group->empty(), Status::CODE_400, "分组名称不能为空");
        auto userCheck = m_appClient->getUserIdByUuid(currentUserIdHeader);
        OATPP_ASSERT_HTTP(userCheck->isSuccess(), Status::CODE_500, userCheck->getErrorMessage());
        OATPP_ASSERT_HTTP(userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
        auto user_id = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        auto friendCheck = m_appClient->getUserIdByUuid(friendUuid);
        OATPP_ASSERT_HTTP(friendCheck->isSuccess(), Status::CODE_500, friendCheck->getErrorMessage());
        OATPP_ASSERT_HTTP(friendCheck->hasMoreToFetch(), Status::CODE_404, "好友不存在");
        auto fuser_id = friendCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        auto result = m_appClient->updateFriendGroup(user_id, fuser_id, request->group);
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_400, "移动分组失败");
        return true;
    }

    oatpp::Boolean deleteFriend(const oatpp::String& currentUserIdHeader, const oatpp::String& friendUuid) {
        OATPP_ASSERT_HTTP(currentUserIdHeader && !currentUserIdHeader->empty(), Status::CODE_400, "用户ID不能为空");
        OATPP_ASSERT_HTTP(friendUuid && !friendUuid->empty(), Status::CODE_400, "好友ID不能为空");
        auto userCheck = m_appClient->getUserIdByUuid(currentUserIdHeader);
        OATPP_ASSERT_HTTP(userCheck->isSuccess(), Status::CODE_500, userCheck->getErrorMessage());
        OATPP_ASSERT_HTTP(userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
        auto user_id = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        auto friendCheck = m_appClient->getUserIdByUuid(friendUuid);
        OATPP_ASSERT_HTTP(friendCheck->isSuccess(), Status::CODE_500, friendCheck->getErrorMessage());
        OATPP_ASSERT_HTTP(friendCheck->hasMoreToFetch(), Status::CODE_404, "好友不存在");
        auto fuser_id = friendCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        auto result = m_appClient->deleteFriend(user_id, fuser_id);
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_400, "删除好友失败");

        //auto convIdResult = m_appClient->getPrivateConversationId(user_id, fuser_id);
        //if (convIdResult->isSuccess() && convIdResult->hasMoreToFetch()) {
        //    auto convId = convIdResult->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;
        //    auto deleteConvResult = m_appClient->deleteConversation(convId);
        //    OATPP_ASSERT_HTTP(deleteConvResult->isSuccess(), Status::CODE_400, "删除会话失败");
        //}

        return true;
    }

    oatpp::Boolean blockUser(const oatpp::String& currentUserIdHeader, const oatpp::String& targetUserUuid) {
        OATPP_ASSERT_HTTP(currentUserIdHeader && !currentUserIdHeader->empty(), Status::CODE_400, "用户ID不能为空");
        OATPP_ASSERT_HTTP(targetUserUuid && !targetUserUuid->empty(), Status::CODE_400, "目标用户ID不能为空");
        auto userCheck = m_appClient->getUserIdByUuid(currentUserIdHeader);
        OATPP_ASSERT_HTTP(userCheck->isSuccess(), Status::CODE_500, userCheck->getErrorMessage());
        OATPP_ASSERT_HTTP(userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
        auto user_id = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        auto targetCheck = m_appClient->getUserIdByUuid(targetUserUuid);
        OATPP_ASSERT_HTTP(targetCheck->isSuccess(), Status::CODE_500, targetCheck->getErrorMessage());
        OATPP_ASSERT_HTTP(targetCheck->hasMoreToFetch(), Status::CODE_404, "目标用户不存在");
        auto target_id = targetCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        // 检查好友关系状态
        auto friendshipStatusResult = m_appClient->checkFriendshipStatus(user_id, target_id);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(friendshipStatusResult->isSuccess(), Status::CODE_500, friendshipStatusResult->getErrorMessage());
        #else
        OATPP_ASSERT_HTTP(friendshipStatusResult->isSuccess(), Status::CODE_500, "检查好友关系失败");
        #endif

        // 只有好友关系才能拉黑
        if (friendshipStatusResult->hasMoreToFetch()) {
            auto friendshipStatus = friendshipStatusResult->fetch<oatpp::Vector<oatpp::Object<StatusDTO>>>()[0]->status;
            if (friendshipStatus != "accepted") {
                OATPP_ASSERT_HTTP(false, Status::CODE_400, "只有好友关系才能拉黑");
            }
        } else {
            OATPP_ASSERT_HTTP(false, Status::CODE_400, "只有好友关系才能拉黑");
        }

        auto result = m_appClient->blockUser(user_id, target_id);
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_400, "拉黑用户失败");
        return true;
    }

    oatpp::Boolean unblockUser(const oatpp::String& currentUserIdHeader, const oatpp::String& targetUserUuid) {
        OATPP_ASSERT_HTTP(currentUserIdHeader && !currentUserIdHeader->empty(), Status::CODE_400, "用户ID不能为空");
        OATPP_ASSERT_HTTP(targetUserUuid && !targetUserUuid->empty(), Status::CODE_400, "目标用户ID不能为空");
        auto userCheck = m_appClient->getUserIdByUuid(currentUserIdHeader);
        OATPP_ASSERT_HTTP(userCheck->isSuccess(), Status::CODE_500, userCheck->getErrorMessage());
        OATPP_ASSERT_HTTP(userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
        auto user_id = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        auto targetCheck = m_appClient->getUserIdByUuid(targetUserUuid);
        OATPP_ASSERT_HTTP(targetCheck->isSuccess(), Status::CODE_500, targetCheck->getErrorMessage());
        OATPP_ASSERT_HTTP(targetCheck->hasMoreToFetch(), Status::CODE_404, "目标用户不存在");
        auto target_id = targetCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        // 检查好友关系状态
        auto friendshipStatusResult = m_appClient->checkFriendshipStatus(user_id, target_id);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(friendshipStatusResult->isSuccess(), Status::CODE_500, friendshipStatusResult->getErrorMessage());
        #else
        OATPP_ASSERT_HTTP(friendshipStatusResult->isSuccess(), Status::CODE_500, "检查好友关系失败");
        #endif

        // 只有被拉黑的关系才能取消拉黑
        if (friendshipStatusResult->hasMoreToFetch()) {
            auto friendshipStatus = friendshipStatusResult->fetch<oatpp::Vector<oatpp::Object<StatusDTO>>>()[0]->status;
            if (friendshipStatus != "blocked") {
                OATPP_ASSERT_HTTP(false, Status::CODE_400, "只有被拉黑的关系才能取消拉黑");
            }
        } else {
            OATPP_ASSERT_HTTP(false, Status::CODE_400, "只有被拉黑的关系才能取消拉黑");
        }

        auto result = m_appClient->unblockUser(user_id, target_id);
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_400, "取消拉黑用户失败");
        return true;
    }
};
