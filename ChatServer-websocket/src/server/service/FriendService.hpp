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

    oatpp::Object<FriendResponseVO> sendFriendRequest(const oatpp::String& currentUserIdHeader, const oatpp::Object<SendFriendRequestDTO>& request) {
        ASYNC_THROW_IF(currentUserIdHeader && !currentUserIdHeader->empty(), "User ID cannot be empty");
        ASYNC_THROW_IF(request, "Request parameters cannot be empty");
        ASYNC_THROW_IF(request->toUserUuid && !request->toUserUuid->empty(), "Target user ID cannot be empty");
        ASYNC_THROW_IF(request->message && !request->message->empty(), "Request message cannot be empty");
        auto userCheck = m_appClient->getUserIdByUuid(currentUserIdHeader);
        #ifdef SQLCHECK
        ASYNC_THROW_IF(userCheck->isSuccess(), userCheck->getErrorMessage());
        ASYNC_THROW_IF(userCheck->hasMoreToFetch(), "User does not exist or has been deactivated");
        #else
        ASYNC_THROW_IF(userCheck->isSuccess() && userCheck->hasMoreToFetch(), "User does not exist or has been deactivated");
        #endif
        auto user_id = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        auto targetCheck = m_appClient->getUserIdByUuid(request->toUserUuid);
        #ifdef SQLCHECK
        ASYNC_THROW_IF(targetCheck->isSuccess(), targetCheck->getErrorMessage());
        ASYNC_THROW_IF(targetCheck->hasMoreToFetch(), "Target user does not exist");
        #else
        ASYNC_THROW_IF(targetCheck->isSuccess() && targetCheck->hasMoreToFetch(), "Target user does not exist");
        #endif
        auto tuser_id = targetCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        // 检查好友关系状态
        auto friendshipStatus1Result = m_appClient->checkFriendshipStatus(user_id, tuser_id);
        ASYNC_THROW_IF(friendshipStatus1Result->isSuccess(), "Failed to check friendship status");

        if (friendshipStatus1Result->hasMoreToFetch()) {
            auto friendshipStatus = friendshipStatus1Result->fetch<oatpp::Vector<oatpp::Object<StatusDTO>>>()[0]->status;
            if (friendshipStatus == "accepted") {
                ASYNC_THROW_IF(false, "Already friends, cannot send friend request");
            } else if (friendshipStatus == "blocked") {
                ASYNC_THROW_IF(false, "User has been blocked, cannot send friend request");
            }
            auto friendshipStatus2Result = m_appClient->checkFriendshipStatus(tuser_id, user_id);
            ASYNC_THROW_IF(friendshipStatus2Result->isSuccess(), friendshipStatus2Result->getErrorMessage());
            if (friendshipStatus2Result->hasMoreToFetch()) {
                friendshipStatus = friendshipStatus2Result->fetch<oatpp::Vector<oatpp::Object<StatusDTO>>>()[0]->status;
                if (friendshipStatus == "blocked") {
                    ASYNC_THROW_IF(false, "You have been blocked by this user, cannot send friend request");
                }
            }
        }

        auto checkResult = m_appClient->checkFriendRequestExists(user_id, tuser_id);
        ASYNC_THROW_IF(checkResult->isSuccess(), checkResult->getErrorMessage());

        oatpp::Object<UuidDTO> ruuid;
        if (checkResult->hasMoreToFetch()) {
            auto updateResult = m_appClient->updateFriendRequestToPending(user_id, tuser_id, request->message);
            #ifdef SQLCHECK
            ASYNC_THROW_IF(updateResult->isSuccess(), updateResult->getErrorMessage());
            ASYNC_THROW_IF(updateResult->hasMoreToFetch(), "Failed to update friend request");
            #else
            ASYNC_THROW_IF(updateResult->isSuccess() && updateResult->hasMoreToFetch(), "Failed to update friend request");
            #endif
            ruuid = updateResult->fetch<oatpp::Vector<oatpp::Object<UuidDTO>>>()[0];
        } else {
            auto result = m_appClient->sendFriendRequest(user_id, tuser_id, request->message);
            #ifdef SQLCHECK
            ASYNC_THROW_IF(result->isSuccess(), result->getErrorMessage());
            ASYNC_THROW_IF(result->hasMoreToFetch(), "Failed to send friend request");
            #else
            ASYNC_THROW_IF(result->isSuccess() && result->hasMoreToFetch(), "Failed to send friend request");
            #endif
            ruuid = result->fetch<oatpp::Vector<oatpp::Object<UuidDTO>>>()[0];
        }

        auto response = FriendResponseVO::createShared();
        response->uuid = ruuid->uuid;
        response->fromUserUuid = currentUserIdHeader;
        response->toUserUuid = request->toUserUuid;
        response->message = request->message;
        response->status = "pending";
        return response;
    }

    oatpp::Vector<oatpp::Object<FriendResponseVO>> getSentRequests(const oatpp::String& currentUserIdHeader) {
        ASYNC_THROW_IF(currentUserIdHeader && !currentUserIdHeader->empty(), "User ID cannot be empty");
        auto userCheck = m_appClient->getUserIdByUuid(currentUserIdHeader);
        #ifdef SQLCHECK
        ASYNC_THROW_IF(userCheck->isSuccess(), userCheck->getErrorMessage());
        ASYNC_THROW_IF(userCheck->hasMoreToFetch(), "User does not exist or has been deactivated");
        #else
        ASYNC_THROW_IF(userCheck->isSuccess() && userCheck->hasMoreToFetch(), "User does not exist or has been deactivated");
        #endif
        auto user_id = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        auto result = m_appClient->getSentFriendRequests(user_id);
        ASYNC_THROW_IF(result->isSuccess(), result->getErrorMessage());
        return result->fetch<oatpp::Vector<oatpp::Object<FriendResponseVO>>>();
    }

    oatpp::Vector<oatpp::Object<FriendResponseVO>> getReceivedRequests(const oatpp::String& currentUserIdHeader) {
        ASYNC_THROW_IF(currentUserIdHeader && !currentUserIdHeader->empty(), "User ID cannot be empty");
        auto userCheck = m_appClient->getUserIdByUuid(currentUserIdHeader);
        #ifdef SQLCHECK
        ASYNC_THROW_IF(userCheck->isSuccess(), userCheck->getErrorMessage());
        ASYNC_THROW_IF(userCheck->hasMoreToFetch(), "User does not exist or has been deactivated");
        #else
        ASYNC_THROW_IF(userCheck->isSuccess() && userCheck->hasMoreToFetch(), "User does not exist or has been deactivated");
        #endif
        auto user_id = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        auto result = m_appClient->getReceivedFriendRequests(user_id);
        ASYNC_THROW_IF(result->isSuccess(), result->getErrorMessage());
        return result->fetch<oatpp::Vector<oatpp::Object<FriendResponseVO>>>();
    }

    oatpp::Boolean acceptFriendRequest(const oatpp::String& currentUserIdHeader, const oatpp::String& requestUuid) {
        ASYNC_THROW_IF(currentUserIdHeader && !currentUserIdHeader->empty(), "User ID cannot be empty");
        ASYNC_THROW_IF(requestUuid && !requestUuid->empty(), "Request ID cannot be empty");

        auto userCheck = m_appClient->getUserIdByUuid(currentUserIdHeader);
        #ifdef SQLCHECK
        ASYNC_THROW_IF(userCheck->isSuccess(), userCheck->getErrorMessage());
        ASYNC_THROW_IF(userCheck->hasMoreToFetch(), "User does not exist or has been deactivated");
        #else
        ASYNC_THROW_IF(userCheck->isSuccess() && userCheck->hasMoreToFetch(), "User does not exist or has been deactivated");
        #endif
        oatpp::Int64 currentUserId = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        auto requestResult = m_appClient->getFriendRequestByUuid(requestUuid);
        #ifdef SQLCHECK
        ASYNC_THROW_IF(requestResult->isSuccess(), requestResult->getErrorMessage());
        ASYNC_THROW_IF(requestResult->hasMoreToFetch(), "Friend request does not exist");
        #else
        ASYNC_THROW_IF(requestResult->isSuccess() && requestResult->hasMoreToFetch(), "Friend request does not exist");
        #endif

        auto friendRequest = requestResult->fetch<oatpp::Vector<oatpp::Object<FriendResponseVO>>>()[0];

        ASYNC_THROW_IF(friendRequest->toUserUuid == currentUserIdHeader, "You do not have permission to process this request");

        auto fromUserResult = m_appClient->getUserIdByUuid(friendRequest->fromUserUuid);
        #ifdef SQLCHECK
        ASYNC_THROW_IF(fromUserResult->isSuccess(), fromUserResult->getErrorMessage());
        ASYNC_THROW_IF(fromUserResult->hasMoreToFetch(), "Sender user does not exist");
        #else
        ASYNC_THROW_IF(fromUserResult->isSuccess() && fromUserResult->hasMoreToFetch(), "Sender user does not exist");
        #endif
        oatpp::Int64 fromUserId = fromUserResult->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        auto toUserResult = m_appClient->getUserIdByUuid(friendRequest->toUserUuid);
        #ifdef SQLCHECK
        ASYNC_THROW_IF(toUserResult->isSuccess(), toUserResult->getErrorMessage());
        ASYNC_THROW_IF(toUserResult->hasMoreToFetch(), "Receiver user does not exist");
        #else
        ASYNC_THROW_IF(toUserResult->isSuccess() && toUserResult->hasMoreToFetch(), "Receiver user does not exist");
        #endif
        oatpp::Int64 toUserId = toUserResult->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        auto updateResult = m_appClient->updateFriendRequestStatus(requestUuid, "accepted");
        #ifdef SQLCHECK
        ASYNC_THROW_IF(updateResult->isSuccess(), updateResult->getErrorMessage());
        ASYNC_THROW_IF(updateResult->hasMoreToFetch(), "Friend request does not exist");
        #else
        ASYNC_THROW_IF(updateResult->isSuccess() && updateResult->hasMoreToFetch(), "Friend request does not exist");
        #endif

        auto checkFriendshipResult = m_appClient->checkFriendshipStatus(fromUserId, toUserId);
        ASYNC_THROW_IF(checkFriendshipResult->isSuccess(), checkFriendshipResult->getErrorMessage());

        if (checkFriendshipResult->hasMoreToFetch()) {
            auto updateFriendshipResult = m_appClient->acceptFriendshipStatus(fromUserId, toUserId);
            ASYNC_THROW_IF(updateFriendshipResult->isSuccess(), updateFriendshipResult->getErrorMessage());
        } else {
            auto createFriendshipResult = m_appClient->createFriendship(fromUserId, toUserId);
            ASYNC_THROW_IF(createFriendshipResult->isSuccess(), createFriendshipResult->getErrorMessage());
        }

        auto createConv1Result = m_appClient->createPrivateConversation(fromUserId, toUserId);
        #ifdef SQLCHECK
        if (!createConv1Result->isSuccess()) {
            OATPP_LOGD("WARRING", "%s", createConv1Result->getErrorMessage());
        }
        #endif

        auto createConv2Result = m_appClient->createPrivateConversation(toUserId, fromUserId);
        #ifdef SQLCHECK
        if (!createConv2Result->isSuccess()) {
            OATPP_LOGD("WARRING", "%s", createConv2Result->getErrorMessage());
        }
        #endif

        return true;
    }

    oatpp::Boolean rejectFriendRequest(const oatpp::String& requestUuid) {
        ASYNC_THROW_IF(requestUuid && !requestUuid->empty(), "Request ID cannot be empty");
        auto result = m_appClient->updateFriendRequestStatus(requestUuid, "reject");
        ASYNC_THROW_IF(result->isSuccess(), "Failed to reject friend request");
        return true;
    }

    oatpp::Boolean cancelFriendRequest(const oatpp::String& requestUuid) {
        ASYNC_THROW_IF(requestUuid && !requestUuid->empty(), "Request ID cannot be empty");
        auto result = m_appClient->cancelFriendRequest(requestUuid);
        ASYNC_THROW_IF(result->isSuccess(), "Failed to cancel friend request");
        return true;
    }

    oatpp::Vector<oatpp::Object<FriendInfoVO>> getFriends(const oatpp::String& currentUserIdHeader) {
        ASYNC_THROW_IF(currentUserIdHeader && !currentUserIdHeader->empty(), "User ID cannot be empty");
        auto userCheck = m_appClient->getUserIdByUuid(currentUserIdHeader);
        #ifdef SQLCHECK
        ASYNC_THROW_IF(userCheck->isSuccess(), userCheck->getErrorMessage());
        ASYNC_THROW_IF(userCheck->hasMoreToFetch(), "User does not exist or has been deactivated");
        #else
        ASYNC_THROW_IF(userCheck->isSuccess() && userCheck->hasMoreToFetch(), "User does not exist or has been deactivated");
        #endif
        auto user_id = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        auto result = m_appClient->getFriends(user_id);
        ASYNC_THROW_IF(result->isSuccess(), result->getErrorMessage());
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

    oatpp::Boolean updateFriendRemark(const oatpp::String& currentUserIdHeader, const oatpp::Object<UpdateFriendRemarkDTO>& request) {
        ASYNC_THROW_IF(currentUserIdHeader && !currentUserIdHeader->empty(), "User ID cannot be empty");
        ASYNC_THROW_IF(request->friendUuid && !request->friendUuid->empty(), "Friend ID cannot be empty");
        ASYNC_THROW_IF(request, "Request parameters cannot be empty");
        ASYNC_THROW_IF(request->remark && !request->remark->empty(), "Remark cannot be empty");
        auto userCheck = m_appClient->getUserIdByUuid(currentUserIdHeader);
        #ifdef SQLCHECK
        ASYNC_THROW_IF(userCheck->isSuccess(), userCheck->getErrorMessage());
        ASYNC_THROW_IF(userCheck->hasMoreToFetch(), "User does not exist or has been deactivated");
        #else
        ASYNC_THROW_IF(userCheck->isSuccess() && userCheck->hasMoreToFetch(), "User does not exist or has been deactivated");
        #endif
        auto user_id = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        auto friendCheck = m_appClient->getUserIdByUuid(request->friendUuid);
        #ifdef SQLCHECK
        ASYNC_THROW_IF(friendCheck->isSuccess(), friendCheck->getErrorMessage());
        ASYNC_THROW_IF(friendCheck->hasMoreToFetch(), "Friend does not exist");
        #else
        ASYNC_THROW_IF(friendCheck->isSuccess() && friendCheck->hasMoreToFetch(), "Friend does not exist");
        #endif
        auto fuser_id = friendCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        auto result = m_appClient->updateFriendRemark(user_id, fuser_id, request->remark);
        ASYNC_THROW_IF(result->isSuccess(), "Failed to update remark");
        return true;
    }

    oatpp::Boolean updateFriendGroup(const oatpp::String& currentUserIdHeader, const oatpp::Object<UpdateFriendGroupDTO>& request) {
        ASYNC_THROW_IF(currentUserIdHeader && !currentUserIdHeader->empty(), "User ID cannot be empty");
        ASYNC_THROW_IF(request->friendUuid && !request->friendUuid->empty(), "Friend ID cannot be empty");
        ASYNC_THROW_IF(request, "Request parameters cannot be empty");
        ASYNC_THROW_IF(request->groupUuid && !request->groupUuid->empty(), "Group name cannot be empty");
        auto userCheck = m_appClient->getUserIdByUuid(currentUserIdHeader);
        #ifdef SQLCHECK
        ASYNC_THROW_IF(userCheck->isSuccess(), userCheck->getErrorMessage());
        ASYNC_THROW_IF(userCheck->hasMoreToFetch(), "User does not exist or has been deactivated");
        #else
        ASYNC_THROW_IF(userCheck->isSuccess() && userCheck->hasMoreToFetch(), "User does not exist or has been deactivated");
        #endif
        auto user_id = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        auto friendCheck = m_appClient->getUserIdByUuid(request->friendUuid);
        #ifdef SQLCHECK
        ASYNC_THROW_IF(friendCheck->isSuccess(), friendCheck->getErrorMessage());
        ASYNC_THROW_IF(friendCheck->hasMoreToFetch(), "Friend does not exist");
        #else
        ASYNC_THROW_IF(friendCheck->isSuccess() && friendCheck->hasMoreToFetch(), "Friend does not exist");
        #endif
        auto fuser_id = friendCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        auto result = m_appClient->updateFriendGroup(user_id, fuser_id, request->groupUuid);
        ASYNC_THROW_IF(result->isSuccess(), "Failed to move friend to group");
        return true;
    }

    oatpp::Boolean deleteFriend(const oatpp::String& currentUserIdHeader, const oatpp::String& friendUuid) {
        ASYNC_THROW_IF(currentUserIdHeader && !currentUserIdHeader->empty(), "User ID cannot be empty");
        ASYNC_THROW_IF(friendUuid && !friendUuid->empty(), "Friend ID cannot be empty");
        auto userCheck = m_appClient->getUserIdByUuid(currentUserIdHeader);
        #ifdef SQLCHECK
        ASYNC_THROW_IF(userCheck->isSuccess(), userCheck->getErrorMessage());
        ASYNC_THROW_IF(userCheck->hasMoreToFetch(), "User does not exist or has been deactivated");
        #else
        ASYNC_THROW_IF(userCheck->isSuccess() && userCheck->hasMoreToFetch(), "User does not exist or has been deactivated");
        #endif
        auto user_id = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        auto friendCheck = m_appClient->getUserIdByUuid(friendUuid);
        #ifdef SQLCHECK
        ASYNC_THROW_IF(friendCheck->isSuccess(), friendCheck->getErrorMessage());
        ASYNC_THROW_IF(friendCheck->hasMoreToFetch(), "Friend does not exist");
        #else
        ASYNC_THROW_IF(friendCheck->isSuccess() && friendCheck->hasMoreToFetch(), "Friend does not exist");
        #endif
        auto fuser_id = friendCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        auto result = m_appClient->deleteFriend(user_id, fuser_id);
        ASYNC_THROW_IF(result->isSuccess(), "Failed to delete friend");

        //auto convIdResult = m_appClient->getPrivateConversationId(user_id, fuser_id);
        //if (convIdResult->isSuccess() && convIdResult->hasMoreToFetch()) {
        //    auto convId = convIdResult->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;
        //    auto deleteConvResult = m_appClient->deleteConversation(convId);
        //    OATPP_ASSERT_HTTP(deleteConvResult->isSuccess(), Status::CODE_400, "删除会话失败");
        //}

        return true;
    }

    oatpp::Boolean blockUser(const oatpp::String& currentUserIdHeader, const oatpp::String& targetUserUuid) {
        ASYNC_THROW_IF(currentUserIdHeader && !currentUserIdHeader->empty(), "User ID cannot be empty");
        ASYNC_THROW_IF(targetUserUuid && !targetUserUuid->empty(), "Target user ID cannot be empty");
        auto userCheck = m_appClient->getUserIdByUuid(currentUserIdHeader);
        #ifdef SQLCHECK
        ASYNC_THROW_IF(userCheck->isSuccess(), userCheck->getErrorMessage());
        ASYNC_THROW_IF(userCheck->hasMoreToFetch(), "User does not exist or has been deactivated");
        #else
        ASYNC_THROW_IF(userCheck->isSuccess() && userCheck->hasMoreToFetch(), "User does not exist or has been deactivated");
        #endif
        auto user_id = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        auto targetCheck = m_appClient->getUserIdByUuid(targetUserUuid);
        #ifdef SQLCHECK
        ASYNC_THROW_IF(targetCheck->isSuccess(), targetCheck->getErrorMessage());
        ASYNC_THROW_IF(targetCheck->hasMoreToFetch(), "Target user does not exist");
        #else
        ASYNC_THROW_IF(targetCheck->isSuccess() && targetCheck->hasMoreToFetch(), "Target user does not exist");
        #endif
        auto target_id = targetCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        auto friendshipStatusResult = m_appClient->checkFriendshipStatus(user_id, target_id);
        ASYNC_THROW_IF(friendshipStatusResult->isSuccess(), friendshipStatusResult->getErrorMessage());

        if (friendshipStatusResult->hasMoreToFetch()) {
            auto friendshipStatus = friendshipStatusResult->fetch<oatpp::Vector<oatpp::Object<StatusDTO>>>()[0]->status;
            if (friendshipStatus != "accepted") {
                ASYNC_THROW_IF(false, "Only friends can be blocked");
            }
        } else {
            ASYNC_THROW_IF(false, "Only friends can be blocked");
        }

        auto result = m_appClient->blockUser(user_id, target_id);
        ASYNC_THROW_IF(result->isSuccess(), "Failed to block user");
        return true;
    }

    oatpp::Boolean unblockUser(const oatpp::String& currentUserIdHeader, const oatpp::String& targetUserUuid) {
        ASYNC_THROW_IF(currentUserIdHeader && !currentUserIdHeader->empty(), "User ID cannot be empty");
        ASYNC_THROW_IF(targetUserUuid && !targetUserUuid->empty(), "Target user ID cannot be empty");
        auto userCheck = m_appClient->getUserIdByUuid(currentUserIdHeader);
        #ifdef SQLCHECK
        ASYNC_THROW_IF(userCheck->isSuccess(), userCheck->getErrorMessage());
        ASYNC_THROW_IF(userCheck->hasMoreToFetch(), "User does not exist or has been deactivated");
        #else
        ASYNC_THROW_IF(userCheck->isSuccess() && userCheck->hasMoreToFetch(), "User does not exist or has been deactivated");
        #endif
        auto user_id = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        auto targetCheck = m_appClient->getUserIdByUuid(targetUserUuid);
        #ifdef SQLCHECK
        ASYNC_THROW_IF(targetCheck->isSuccess(), targetCheck->getErrorMessage());
        ASYNC_THROW_IF(targetCheck->hasMoreToFetch(), "Target user does not exist");
        #else
        ASYNC_THROW_IF(targetCheck->isSuccess() && targetCheck->hasMoreToFetch(), "Target user does not exist");
        #endif
        auto target_id = targetCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

        auto friendshipStatusResult = m_appClient->checkFriendshipStatus(user_id, target_id);
        ASYNC_THROW_IF(friendshipStatusResult->isSuccess(), friendshipStatusResult->getErrorMessage());

        if (friendshipStatusResult->hasMoreToFetch()) {
            auto friendshipStatus = friendshipStatusResult->fetch<oatpp::Vector<oatpp::Object<StatusDTO>>>()[0]->status;
            if (friendshipStatus != "blocked") {
                ASYNC_THROW_IF(false, "Only blocked users can be unblocked");
            }
        } else {
            ASYNC_THROW_IF(false, "Only blocked users can be unblocked");
        }

        auto result = m_appClient->unblockUser(user_id, target_id);
        ASYNC_THROW_IF(result->isSuccess(), "Failed to unblock user");
        return true;
    }
};
