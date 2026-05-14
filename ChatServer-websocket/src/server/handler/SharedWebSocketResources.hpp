// SharedWebSocketResources.hpp - 共享WebSocket资源
#pragma once

#include "global.h"
#include "../dto/MessageDto.hpp"
#include "../dto/ConversationDto.hpp"
#include "../dto/FriendDto.hpp"
#include "../dto/GroupDto.hpp"
#include "../dto/UserStatusDto.hpp"
#include "../dto/GeneralDto.hpp"
#include "../vo/MessageVo.hpp"
#include "../vo/GroupVo.hpp"
#include "../vo/AuthVo.hpp"
#include "../vo/CommonVo.hpp"
#include "../vo/ConversationVO.hpp"
#include "../vo/FriendVO.hpp"
#include "../vo/NotificationVO.hpp"
#include "../vo/UserStatusVO.hpp"
#include "../service/ConversationService.hpp"
#include "../service/FriendService.hpp"
#include "../service/GroupService.hpp"
#include "../service/MessageService.hpp"
#include "../service/UserStatusService.hpp"
#include "../postgresql/AppClient.hpp"
#include "../../jwt/Appjwt.h"
#include "../../websocket/AppWebSocket.hpp"
#include "../../redis/AppRedis.hpp"

#include"../dto/WebSocketMessageDto.hpp"


class SharedWebSocketResources {
public:
    std::shared_ptr<AppClient> appClient;
    std::shared_ptr<Appjwt> jwt;
    std::shared_ptr<AppRedis> redis;
    std::shared_ptr<AppWebSocket> webSocket;
    std::shared_ptr<oatpp::data::mapping::ObjectMapper> objectMapper;

    std::shared_ptr<ConversationService> conversationService;
    std::shared_ptr<FriendService> friendService;
    std::shared_ptr<GroupService> groupService;
    std::shared_ptr<MessageService> messageService;
    std::shared_ptr<UserStatusService> statusService;

    std::unordered_map<std::string, std::function<void(const oatpp::String&, const oatpp::String&)>> handlers;

    SharedWebSocketResources(
        const std::shared_ptr<AppClient>& client,
        const std::shared_ptr<Appjwt>& jwtService,
        const std::shared_ptr<AppRedis>& redisService,
        const std::shared_ptr<AppWebSocket>& ws,
        const std::shared_ptr<oatpp::data::mapping::ObjectMapper>& mapper)
        : appClient(client)
        , jwt(jwtService)
        , redis(redisService)
        , webSocket(ws)
        , objectMapper(mapper)
        , conversationService(std::make_shared<ConversationService>(client))
        , friendService(std::make_shared<FriendService>(client))
        , groupService(std::make_shared<GroupService>(client))
        , messageService(std::make_shared<MessageService>(client))
        , statusService(std::make_shared<UserStatusService>(client))
    {
        initHandlers();
    }

    void sendResponse(const oatpp::String& userUuid, const char* type, const oatpp::String& content) {
        auto response = oatpp::Object<WebSocketResponseDTO>::createShared();
        response->type = type;
        response->content = content;
        response->success = true;
        webSocket->sendMessageToUser(userUuid, objectMapper->writeToString(response));
    }

    void sendError(const oatpp::String& userUuid, const oatpp::String& errorMessage) {
        auto response = oatpp::Object<WebSocketResponseDTO>::createShared();
        response->type = "error";
        response->content = errorMessage;
        response->success = false;
        webSocket->sendMessageToUser(userUuid, objectMapper->writeToString(response));
    }

private:
    // UUID → BIGINT ID 转换
    oatpp::Int64 resolveUserId(const oatpp::String& uuid) {
        auto r = appClient->getUserIdByUuid(uuid);
        if (!r->isSuccess() || !r->hasMoreToFetch()) {
            throw std::runtime_error("User not found: " + *uuid);
        }
        return r->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;
    }

    oatpp::Int64 resolveMessageId(const oatpp::String& uuid) {
        auto r = appClient->getMessageIdByUuid(uuid);
        if (!r->isSuccess() || !r->hasMoreToFetch()) {
            throw std::runtime_error("Message not found: " + *uuid);
        }
        return r->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;
    }

    oatpp::Int64 resolveGroupId(const oatpp::String& uuid) {
        auto r = appClient->getGroupIdByUuid(uuid);
        if (!r->isSuccess() || !r->hasMoreToFetch()) {
            throw std::runtime_error("Group not found: " + *uuid);
        }
        return r->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;
    }

    // 获取群成员 UUID 列表（排除发送者）
    std::vector<oatpp::String> getGroupMemberUuids(const oatpp::String& groupUuid, const oatpp::String& excludeUserUuid) {
        std::vector<oatpp::String> uuids;
        try {
            auto gidRes = appClient->getGroupIdByUuid(groupUuid);
            if (!gidRes->isSuccess() || !gidRes->hasMoreToFetch()) return uuids;
            auto groupId = gidRes->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;

            auto members = appClient->getGroupMembers(groupId);
            if (!members->isSuccess()) return uuids;
            auto list = members->fetch<oatpp::Vector<oatpp::Object<GroupMemberVO>>>();
            for (auto& m : *list) {
                if (m->userUuid != excludeUserUuid) {
                    uuids.push_back(m->userUuid);
                }
            }
        } catch (const std::exception& e) {
            OATPP_LOGE("SharedWebSocketResources", "getGroupMemberUuids failed: %s", e.what());
        }
        return uuids;
    }

    void initHandlers() {
        handlers["send_friend_request"] = [this](const oatpp::String& userUuid, const oatpp::String& msg) {
            auto request = objectMapper->readFromString<oatpp::Object<SendFriendRequestDTO>>(msg);
            if (!request) {
                sendError(userUuid, "Invalid friend request data");
                return;
            }
            auto result = friendService->sendFriendRequest(userUuid, request);
            sendResponse(userUuid, "send_friend_request_response", objectMapper->writeToString(result));
        };

        handlers["handle_friend_request"] = [this](const oatpp::String& userUuid, const oatpp::String& msg) {
            auto request = objectMapper->readFromString<oatpp::Object<HandleFriendRequestDTO>>(msg);
            if (!request || !request->reqUuid || !request->status) {
                sendError(userUuid, "Request UUID and status are required");
                return;
            }
            auto result = friendService->handleFriendRequest(userUuid, request->reqUuid, request->status);
            sendResponse(userUuid, "handle_friend_request_response", objectMapper->writeToString(result));
        };


        handlers["delete_friend"] = [this](const oatpp::String& userUuid, const oatpp::String& msg) {
            auto request = objectMapper->readFromString<oatpp::Object<FriendUuidDTO>>(msg);
            if (!request || !request->friendUuid) {
                sendError(userUuid, "Friend UUID is required");
                return;
            }
            auto result = friendService->deleteFriend(userUuid, request->friendUuid);
            sendResponse(userUuid, "delete_friend_response", objectMapper->writeToString(result));
        };

        handlers["block_user"] = [this](const oatpp::String& userUuid, const oatpp::String& msg) {
            auto request = objectMapper->readFromString<oatpp::Object<TargetUserUuidDTO>>(msg);
            if (!request || !request->targetUserUuid) {
                sendError(userUuid, "Target user UUID is required");
                return;
            }
            auto result = friendService->blockUser(userUuid, request->targetUserUuid);
            sendResponse(userUuid, "block_user_response", objectMapper->writeToString(result));
        };

        handlers["unblock_user"] = [this](const oatpp::String& userUuid, const oatpp::String& msg) {
            auto request = objectMapper->readFromString<oatpp::Object<TargetUserUuidDTO>>(msg);
            if (!request || !request->targetUserUuid) {
                sendError(userUuid, "Target user UUID is required");
                return;
            }
            auto result = friendService->unblockUser(userUuid, request->targetUserUuid);
            sendResponse(userUuid, "unblock_user_response", objectMapper->writeToString(result));
        };

        handlers["create_group"] = [this](const oatpp::String& userUuid, const oatpp::String& msg) {
            auto request = objectMapper->readFromString<oatpp::Object<CreateGroupRequestDTO>>(msg);
            if (!request) {
                sendError(userUuid, "Invalid group data");
                return;
            }
            auto result = groupService->createGroup(userUuid, request);
            sendResponse(userUuid, "create_group_response", objectMapper->writeToString(result));
        };

        handlers["update_group"] = [this](const oatpp::String& userUuid, const oatpp::String& msg) {
            auto request = objectMapper->readFromString<oatpp::Object<UpdateGroupRequestDTO>>(msg);
            auto result = groupService->updateGroup(userUuid, request);
            sendResponse(userUuid, "update_group_response", objectMapper->writeToString(result));
        };

        handlers["dissolve_group"] = [this](const oatpp::String& userUuid, const oatpp::String& msg) {
            auto request = objectMapper->readFromString<oatpp::Object<GroupUuidDTO>>(msg);
            if (!request || !request->groupUuid) {
                sendError(userUuid, "Group UUID is required");
                return;
            }
            auto result = groupService->dissolveGroup(userUuid, request->groupUuid);
            sendResponse(userUuid, "dissolve_group_response", objectMapper->writeToString(result));
        };

        handlers["add_group_members"] = [this](const oatpp::String& userUuid, const oatpp::String& msg) {
            auto request = objectMapper->readFromString<oatpp::Object<AddGroupMemberRequestDTO>>(msg);
            if (!request || !request->groupUuid) {
                sendError(userUuid, "Group UUID is required");
                return;
            }
            auto result = groupService->addGroupMembers(userUuid, request->groupUuid, request);
            sendResponse(userUuid, "add_group_members_response", objectMapper->writeToString(result));
        };

        handlers["remove_group_member"] = [this](const oatpp::String& userUuid, const oatpp::String& msg) {
            auto request = objectMapper->readFromString<oatpp::Object<RemoveGroupMemberDTO>>(msg);
            if (!request || !request->groupUuid || !request->userUuid) {
                sendError(userUuid, "Group UUID and User UUID are required");
                return;
            }
            auto result = groupService->removeGroupMember(userUuid, request->groupUuid, request->userUuid);
            sendResponse(userUuid, "remove_group_member_response", objectMapper->writeToString(result));
        };

        handlers["set_member_role"] = [this](const oatpp::String& userUuid, const oatpp::String& msg) {
            auto request = objectMapper->readFromString<oatpp::Object<SetMemberRoleRequestDTO>>(msg);
            auto result = groupService->setMemberRole(userUuid,request);
            sendResponse(userUuid, "set_member_role_response", objectMapper->writeToString(result));
        };

        handlers["join_group"] = [this](const oatpp::String& userUuid, const oatpp::String& msg) {
            auto request = objectMapper->readFromString<oatpp::Object<GroupUuidDTO>>(msg);
            if (!request || !request->groupUuid) {
                sendError(userUuid, "Group UUID is required");
                return;
            }
            auto result = groupService->joinGroup(userUuid, request->groupUuid);
            sendResponse(userUuid, "join_group_response", objectMapper->writeToString(result));
        };

        handlers["leave_group"] = [this](const oatpp::String& userUuid, const oatpp::String& msg) {
            auto request = objectMapper->readFromString<oatpp::Object<GroupUuidDTO>>(msg);
            if (!request || !request->groupUuid) {
                sendError(userUuid, "Group UUID is required");
                return;
            }
            auto result = groupService->leaveGroup(userUuid, request->groupUuid);
            sendResponse(userUuid, "leave_group_response", objectMapper->writeToString(result));
        };

        handlers["send_private_message"] = [this](const oatpp::String& userUuid, const oatpp::String& msg) {
            auto request = objectMapper->readFromString<oatpp::Object<SendPrivateMessageRequestDTO>>(msg);
            if (!request) {
                sendError(userUuid, "Invalid message data");
                return;
            }

            // 开始事务
            auto transaction = appClient->beginTransaction();
            oatpp::Object<PrivateMessageVO> result;

            try {
                result = messageService->sendPrivateMessage(userUuid, request);

                auto senderId = resolveUserId(userUuid);
                auto receiverId = resolveUserId(request->toUserUuid);
                auto messageId = resolveMessageId(result->uuid);

                // 发送方会话：unread = 0（自己发出的消息无需未读计数）
                appClient->upsertConversationSenderPrivate(senderId, receiverId, messageId);
                // 接收方会话：无论在线与否，unread + 1（拉取消息时 CTE 归零）
                appClient->incrementConversationUnreadPrivate(receiverId, senderId, messageId);

                // 提交事务
                transaction.commit();
            } catch (const std::exception& e) {
                // 回滚事务
                transaction.rollback();
                OATPP_LOGE("SharedWebSocketResources", "send_private_message failed: %s", e.what());
                sendError(userUuid, "Failed to send message: " + oatpp::String(e.what()));
                return;
            }

            // 构造转发消息
            auto forwardMsg = oatpp::Object<WebSocketResponseDTO>::createShared();
            forwardMsg->type = "new_private_message";
            forwardMsg->content = objectMapper->writeToString(result);
            forwardMsg->success = true;
            auto forwardJson = objectMapper->writeToString(forwardMsg);

            // 仅 WebSocket 推送，会话已在上面更新完毕
            webSocket->batchPushMessage({request->toUserUuid}, forwardJson);

            sendResponse(userUuid, "send_private_message_response", objectMapper->writeToString(result));
        };

        handlers["recall_message"] = [this](const oatpp::String& userUuid, const oatpp::String& msg) {
            auto request = objectMapper->readFromString<oatpp::Object<MessageUuidDTO>>(msg);
            if (!request || !request->messageUuid) {
                sendError(userUuid, "Message UUID is required");
                return;
            }
            auto result = messageService->recallMessage(userUuid, request->messageUuid);
            sendResponse(userUuid, "recall_message_response", objectMapper->writeToString(result));
        };

        handlers["send_group_message"] = [this](const oatpp::String& userUuid, const oatpp::String& msg) {
            auto request = objectMapper->readFromString<oatpp::Object<SendGroupMessageRequestDTO>>(msg);
            if (!request) {
                sendError(userUuid, "Invalid message data");
                return;
            }

            // 开始事务
            auto transaction = appClient->beginTransaction();
            oatpp::Object<GroupMessageVO> result;
            std::vector<oatpp::String> memberUuids;

            try {
                result = messageService->sendGroupMessage(userUuid, request);

                auto senderId = resolveUserId(userUuid);
                auto groupId = resolveGroupId(request->groupUuid);
                auto messageId = resolveMessageId(result->uuid);

                // 发送方自身的群会话：unread = 0
                appClient->upsertConversationSenderGroup(senderId, groupId, messageId);

                // 获取群成员（排除发送者），每个成员的会话 unread + 1
                memberUuids = getGroupMemberUuids(request->groupUuid, userUuid);
                for (auto& uuid : memberUuids) {
                    auto memberId = resolveUserId(uuid);
                    appClient->incrementConversationUnreadGroup(memberId, groupId, messageId);
                }

                // 提交事务
                transaction.commit();
            } catch (const std::exception& e) {
                // 回滚事务
                transaction.rollback();
                OATPP_LOGE("SharedWebSocketResources", "send_group_message failed: %s", e.what());
                sendError(userUuid, "Failed to send group message: " + oatpp::String(e.what()));
                return;
            }

            // 构造转发消息
            auto forwardMsg = oatpp::Object<WebSocketResponseDTO>::createShared();
            forwardMsg->type = "new_group_message";
            forwardMsg->content = objectMapper->writeToString(result);
            forwardMsg->success = true;
            auto forwardJson = objectMapper->writeToString(forwardMsg);

            // 仅 WebSocket 推送，会话已在上面更新完毕
            webSocket->batchPushMessage(memberUuids, forwardJson);

            sendResponse(userUuid, "send_group_message_response", objectMapper->writeToString(result));
        };

        handlers["send_group_request"] = [this](const oatpp::String& userUuid, const oatpp::String& msg) {
            auto request = objectMapper->readFromString<oatpp::Object<SendGroupRequestDTO>>(msg);
            if (!request || !request->groupUuid) {
                sendError(userUuid, "Group UUID is required");
                return;
            }
            auto result = groupService->sendGroupRequest(userUuid, request->groupUuid, request);
            sendResponse(userUuid, "send_group_request_response", objectMapper->writeToString(result));
        };

        handlers["handle_group_request"] = [this](const oatpp::String& userUuid, const oatpp::String& msg) {
            auto request = objectMapper->readFromString<oatpp::Object<HandleGroupRequestDTO>>(msg);
            if (!request || !request->grUuid) {
                sendError(userUuid, "Request UUID is required");
                return;
            }
            auto result = groupService->handleGroupRequest(userUuid, request->grUuid, request);
            sendResponse(userUuid, "handle_group_request_response", objectMapper->writeToString(result));
        };
    }
};