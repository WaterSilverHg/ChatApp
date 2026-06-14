#pragma once

#include "global.h"
#include "../dto/FriendDto.hpp"
#include "../dto/GeneralDto.hpp"
#include "../vo/FriendVo.hpp"
#include "../postgresql/AppPostgresql.hpp"
#include "../../tool/UuidIdCache.hpp"

class FriendService {
private:
    std::shared_ptr<AppPostgresql> m_appClient;
    std::shared_ptr<UuidIdCache> m_idCache;
    using Status = oatpp::web::protocol::http::Status;
public:
    FriendService(const std::shared_ptr<AppPostgresql>& appClient, const std::shared_ptr<UuidIdCache>& idCache) 
        : m_appClient(appClient), m_idCache(idCache) {}

    oatpp::Object<FriendResponseVO> sendFriendRequest(const oatpp::String& currentUserIdHeader, const oatpp::Object<SendFriendRequestDTO>& request) {
        ASYNC_THROW_IF(currentUserIdHeader && !currentUserIdHeader->empty(), "User ID cannot be empty");
        ASYNC_THROW_IF(request, "Request parameters cannot be empty");
        ASYNC_THROW_IF(request->toUserUuid && !request->toUserUuid->empty(), "Target user ID cannot be empty");
        ASYNC_THROW_IF(request->message && !request->message->empty(), "Request message cannot be empty");
        auto user_id = m_idCache->getUserId(currentUserIdHeader);
        ASYNC_THROW_IF(user_id > 0, "User does not exist or has been deactivated");

        auto tuser_id = m_idCache->getUserId(request->toUserUuid);
        ASYNC_THROW_IF(tuser_id > 0, "Target user does not exist");

        // 检查好友关系状态
        auto friendshipStatus1Result = m_appClient->checkFriendshipStatus(user_id, tuser_id);
        ASYNC_THROW_IF(friendshipStatus1Result->isSuccess(), "Failed to check friendship status");

        if (friendshipStatus1Result->hasMoreToFetch()) {
            auto friendshipStatus = friendshipStatus1Result->fetch<oatpp::Vector<oatpp::Object<StatusDTO>>>()[0]->status;
            if (friendshipStatus == "accepted") {
                ASYNC_THROW_IF(false, "Already friends, cannot send friend request");
            } else if (friendshipStatus == "block") {
                ASYNC_THROW_IF(false, "You have blocked this user, cannot send friend request");
            } else if (friendshipStatus == "blocked") {
                ASYNC_THROW_IF(false, "This user has blocked you, cannot send friend request");
            }
        }

        auto checkResult = m_appClient->checkFriendRequestExists(user_id, tuser_id);
        ASYNC_THROW_IF(checkResult->isSuccess(), checkResult->getErrorMessage());

        auto response = FriendResponseVO::createShared();
        oatpp::Object<UuidDTO> ruuid;

        if (checkResult->hasMoreToFetch()) {
            // 已存在请求：获取现有 UUID
            ruuid = checkResult->fetch<oatpp::Vector<oatpp::Object<UuidDTO>>>()[0];

            // 尝试重新激活（仅 rejected/canceled → pending，已 pending 的不动）
            auto updateResult = m_appClient->updateFriendRequestToPending(user_id, tuser_id, request->message);
            #ifdef SQLCHECK
            if (!updateResult->isSuccess()) {
                OATPP_LOGD("FriendService", "Error: %s", updateResult->getErrorMessage()->c_str());
                throw std::runtime_error("Failed to update friend request");
            }
            #else
            ASYNC_THROW_IF(updateResult->isSuccess(), "Failed to update friend request");
            #endif
            // updateResult 有行 → 之前是 rejected/canceled，现在是 re-send
            // updateResult 无行 → 之前是 pending，保持原样
            response->status = updateResult->hasMoreToFetch() ? "pending" : "already_pending";
        } else {
            auto result = m_appClient->sendFriendRequest(user_id, tuser_id, request->message);
            #ifdef SQLCHECK
            if (!result->isSuccess()) {
                OATPP_LOGD("FriendService", "Error: %s", result->getErrorMessage()->c_str());
                throw std::runtime_error("Failed to send friend request");
            }
            if (!result->hasMoreToFetch()) {
                OATPP_LOGD("FriendService", "Error: %s", "Failed to send friend request");
                throw std::runtime_error("Failed to send friend request");
            }
            #else
            ASYNC_THROW_IF(result->isSuccess() && result->hasMoreToFetch(), "Failed to send friend request");
            #endif
            ruuid = result->fetch<oatpp::Vector<oatpp::Object<UuidDTO>>>()[0];
            response->status = "pending";
        }

        response->uuid = ruuid->uuid;
        response->fromUserUuid = currentUserIdHeader;
        response->toUserUuid = request->toUserUuid;
        response->message = request->message;
        return response;
    }

    oatpp::Vector<oatpp::Object<FriendResponseVO>> getSentRequests(const oatpp::String& currentUserIdHeader) {
        ASYNC_THROW_IF(currentUserIdHeader && !currentUserIdHeader->empty(), "User ID cannot be empty");
        auto user_id = m_idCache->getUserId(currentUserIdHeader);
        ASYNC_THROW_IF(user_id > 0, "User does not exist or has been deactivated");

        auto result = m_appClient->getSentFriendRequests(user_id);
        ASYNC_THROW_IF(result->isSuccess(), result->getErrorMessage());
        return result->fetch<oatpp::Vector<oatpp::Object<FriendResponseVO>>>();
    }

    oatpp::Vector<oatpp::Object<FriendResponseVO>> getReceivedRequests(const oatpp::String& currentUserIdHeader) {
        ASYNC_THROW_IF(currentUserIdHeader && !currentUserIdHeader->empty(), "User ID cannot be empty");
        auto user_id = m_idCache->getUserId(currentUserIdHeader);
        ASYNC_THROW_IF(user_id > 0, "User does not exist or has been deactivated");

        auto result = m_appClient->getReceivedFriendRequests(user_id);
        ASYNC_THROW_IF(result->isSuccess(), result->getErrorMessage());
        return result->fetch<oatpp::Vector<oatpp::Object<FriendResponseVO>>>();
    }

    oatpp::Object<FriendRequestIdsDTO> handleFriendRequest(const oatpp::String& currentUserIdHeader, const oatpp::String& requestUuid, const oatpp::String& status) {
        ASYNC_THROW_IF(currentUserIdHeader && !currentUserIdHeader->empty(), "User ID cannot be empty");
        ASYNC_THROW_IF(requestUuid && !requestUuid->empty(), "Request ID cannot be empty");
        ASYNC_THROW_IF(status && !status->empty(), "Status cannot be empty");

        if (status != "accepted" && status != "rejected" && status != "canceled") {
            ASYNC_THROW_IF(false, "Invalid status: must be accepted, rejected, or canceled");
        }

        auto currentUserId = m_idCache->getUserId(currentUserIdHeader);
        ASYNC_THROW_IF(currentUserId > 0, "User does not exist or has been deactivated");

        auto transaction = m_appClient->beginTransaction();

        auto updateResult = m_appClient->handleFriendRequest(requestUuid, status, currentUserId);
        #ifdef SQLCHECK
        if (!updateResult->isSuccess()) {
            OATPP_LOGD("FriendService", "Error: %s", updateResult->getErrorMessage()->c_str());
            transaction.rollback();
            throw std::runtime_error("Friend request not found, already processed, or permission denied");
        }
        if (!updateResult->hasMoreToFetch()) {
            OATPP_LOGD("FriendService", "Error: %s", "Friend request not found, already processed, or permission denied");
            transaction.rollback();
            throw std::runtime_error("Friend request not found, already processed, or permission denied");
        }
        #else
        if (!updateResult->isSuccess() || !updateResult->hasMoreToFetch()) {
            transaction.rollback();
            throw std::runtime_error("Friend request not found, already processed, or permission denied");
        }
        #endif

        auto ids = updateResult->fetch<oatpp::Vector<oatpp::Object<FriendRequestIdsDTO>>>()[0];
        oatpp::Int64 fromUserId = ids->fromUserId;
        oatpp::Int64 toUserId = ids->toUserId;

        if (status == "accepted") {
            auto checkFriendshipResult = m_appClient->checkFriendshipStatus(fromUserId, toUserId);
            if (!checkFriendshipResult->isSuccess()) {
                transaction.rollback();
                throw std::runtime_error("Failed to check friendship status");
            }

            if (checkFriendshipResult->hasMoreToFetch()) {
                auto updateFriendshipResult = m_appClient->acceptFriendshipStatus(fromUserId, toUserId);
                if (!updateFriendshipResult->isSuccess()) {
                    transaction.rollback();
                    throw std::runtime_error("Failed to accept friendship status");
                }
            } else {
                auto createFriendshipResult = m_appClient->createFriendship(fromUserId, toUserId);
                if (!createFriendshipResult->isSuccess()) {
                    transaction.rollback();
                    throw std::runtime_error("Failed to create friendship");
                }
            }

            auto createConv1Result = m_appClient->createPrivateConversation(fromUserId, toUserId);
            if (!createConv1Result->isSuccess()) {
                transaction.rollback();
                throw std::runtime_error("Failed to create conversation for sender");
            }

            auto createConv2Result = m_appClient->createPrivateConversation(toUserId, fromUserId);
            if (!createConv2Result->isSuccess()) {
                transaction.rollback();
                throw std::runtime_error("Failed to create conversation for receiver");
            }
        }

        transaction.commit();
        return ids;
    }

    oatpp::Vector<oatpp::Object<FriendInfoVO>> getFriends(const oatpp::String& currentUserIdHeader) {
        ASYNC_THROW_IF(currentUserIdHeader && !currentUserIdHeader->empty(), "User ID cannot be empty");
        auto user_id = m_idCache->getUserId(currentUserIdHeader);
        ASYNC_THROW_IF(user_id > 0, "User does not exist or has been deactivated");

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
        
        auto user_id = m_idCache->getUserId(currentUserIdHeader);
        ASYNC_THROW_IF(user_id > 0, "User does not exist or has been deactivated");

        auto fuser_id = m_idCache->getUserId(request->friendUuid);
        ASYNC_THROW_IF(fuser_id > 0, "Friend does not exist");

        auto result = m_appClient->updateFriendRemark(user_id, fuser_id, request->remark);
        ASYNC_THROW_IF(result->isSuccess(), "Failed to update remark");
        return true;
    }

    oatpp::Boolean updateFriendGroup(const oatpp::String& currentUserIdHeader, const oatpp::Object<UpdateFriendGroupDTO>& request) {
        ASYNC_THROW_IF(currentUserIdHeader && !currentUserIdHeader->empty(), "User ID cannot be empty");
        ASYNC_THROW_IF(request->friendUuid && !request->friendUuid->empty(), "Friend ID cannot be empty");
        ASYNC_THROW_IF(request, "Request parameters cannot be empty");
        ASYNC_THROW_IF(request->groupUuid && !request->groupUuid->empty(), "Group name cannot be empty");
        
        auto user_id = m_idCache->getUserId(currentUserIdHeader);
        ASYNC_THROW_IF(user_id > 0, "User does not exist or has been deactivated");

        auto fuser_id = m_idCache->getUserId(request->friendUuid);
        ASYNC_THROW_IF(fuser_id > 0, "Friend does not exist");

        auto result = m_appClient->updateFriendGroup(user_id, fuser_id, request->groupUuid);
        ASYNC_THROW_IF(result->isSuccess(), "Failed to move friend to group");
        return true;
    }

    oatpp::Boolean deleteFriend(const oatpp::String& currentUserIdHeader, const oatpp::String& friendUuid) {
        ASYNC_THROW_IF(currentUserIdHeader && !currentUserIdHeader->empty(), "User ID cannot be empty");
        ASYNC_THROW_IF(friendUuid && !friendUuid->empty(), "Friend ID cannot be empty");
        
        auto user_id = m_idCache->getUserId(currentUserIdHeader);
        ASYNC_THROW_IF(user_id > 0, "User does not exist or has been deactivated");

        auto fuser_id = m_idCache->getUserId(friendUuid);
        ASYNC_THROW_IF(fuser_id > 0, "Friend does not exist");

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

        auto user_id = m_idCache->getUserId(currentUserIdHeader);
        ASYNC_THROW_IF(user_id > 0, "User does not exist or has been deactivated");

        auto target_id = m_idCache->getUserId(targetUserUuid);
        ASYNC_THROW_IF(target_id > 0, "Target user does not exist");

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

        auto user_id = m_idCache->getUserId(currentUserIdHeader);
        ASYNC_THROW_IF(user_id > 0, "User does not exist or has been deactivated");

        auto target_id = m_idCache->getUserId(targetUserUuid);
        ASYNC_THROW_IF(target_id > 0, "Target user does not exist");

        auto friendshipStatusResult = m_appClient->checkFriendshipStatus(user_id, target_id);
        ASYNC_THROW_IF(friendshipStatusResult->isSuccess(), friendshipStatusResult->getErrorMessage());

        if (friendshipStatusResult->hasMoreToFetch()) {
            auto friendshipStatus = friendshipStatusResult->fetch<oatpp::Vector<oatpp::Object<StatusDTO>>>()[0]->status;
            if (friendshipStatus != "block") {
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
