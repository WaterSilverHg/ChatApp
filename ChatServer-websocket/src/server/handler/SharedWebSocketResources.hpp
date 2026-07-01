// SharedWebSocketResources.hpp - 共享WebSocket资源
#pragma once

#include "global.h"
#include "server/dto/MessageDto.hpp"
#include "server/dto/ConversationDto.hpp"
#include "server/dto/FriendDto.hpp"
#include "server/dto/GroupDto.hpp"
#include "server/dto/UserStatusDto.hpp"
#include "server/dto/GeneralDto.hpp"
#include "server/vo/MessageVo.hpp"
#include "server/vo/GroupVo.hpp"
#include "server/vo/AuthVo.hpp"
#include "server/vo/CommonVo.hpp"
#include "server/vo/ConversationVO.hpp"
#include "server/vo/FriendVO.hpp"
#include "server/vo/NotificationVO.hpp"
#include "server/vo/UserStatusVO.hpp"
#include "server/service/ConversationService.hpp"
#include "server/service/FriendService.hpp"
#include "server/service/GroupService.hpp"
#include "server/service/MessageService.hpp"
#include "server/service/UserStatusService.hpp"
#include "server/postgresql/AppPostgresql.hpp"
#include "jwt/Appjwt.h"
#include "websocket/AppWebSocket.hpp"
#include "redis/AppRedis.hpp"
#include "tool/UuidIdCache.hpp"
#include "tool/HashUtils.hpp"
#include"server/dto/WebSocketMessageDto.hpp"

struct HeartbeatRecord {
    std::weak_ptr<oatpp::websocket::AsyncWebSocket> socket;
    std::atomic<std::chrono::steady_clock::time_point> lastActivityTime;
    std::atomic<std::chrono::steady_clock::time_point> lastPingSentTime;
    std::string userUuid;

    HeartbeatRecord(const std::shared_ptr<oatpp::websocket::AsyncWebSocket>& ws, const std::string& uuid)
        : socket(ws), userUuid(uuid) {
        auto now = std::chrono::steady_clock::now();
        lastActivityTime.store(now);
        lastPingSentTime.store(now);
    }
};

class SharedWebSocketResources {
public:
    std::shared_ptr<AppPostgresql> appClient;
    std::shared_ptr<Appjwt> jwt;
    std::shared_ptr<AppRedis> redis;
    std::shared_ptr<AppWebSocket> webSocket;
    std::shared_ptr<oatpp::data::mapping::ObjectMapper> objectMapper;

    std::shared_ptr<ConversationService> conversationService;
    std::shared_ptr<FriendService> friendService;
    std::shared_ptr<GroupService> groupService;
    std::shared_ptr<MessageService> messageService;
    std::shared_ptr<UserStatusService> statusService;
    std::shared_ptr<UuidIdCache> idCache;

    std::unordered_map<std::string, std::function<void(const oatpp::String&, const oatpp::String&)>> handlers;

    // 心跳相关
    std::unordered_map<std::string, std::shared_ptr<HeartbeatRecord>> heartbeatRecords;
    std::mutex heartbeatMutex;
    std::atomic<bool> heartbeatRunning;
    std::thread heartbeatThread;
    static constexpr std::chrono::seconds HEARTBEAT_INTERVAL{30};  // 检查间隔
    static constexpr std::chrono::seconds TIMEOUT_THRESHOLD{90};   // 超时阈值

    SharedWebSocketResources(
        const std::shared_ptr<AppPostgresql>& client,
        const std::shared_ptr<Appjwt>& jwtService,
        const std::shared_ptr<AppRedis>& redisService,
        const std::shared_ptr<AppWebSocket>& ws,
        const std::shared_ptr<oatpp::data::mapping::ObjectMapper>& mapper,
        const std::shared_ptr<UuidIdCache>& idCacheService)
        : appClient(client)
        , jwt(jwtService)
        , redis(redisService)
        , webSocket(ws)
        , objectMapper(mapper)
        , idCache(idCacheService)
        , conversationService(std::make_shared<ConversationService>(client, idCacheService))
        , friendService(std::make_shared<FriendService>(client, idCacheService))
        , groupService(std::make_shared<GroupService>(client, idCacheService))
        , messageService(std::make_shared<MessageService>(client, idCacheService))
        , statusService(std::make_shared<UserStatusService>(client, idCacheService))
    {
        initHandlers();
        startHeartbeatThread();
    }

    ~SharedWebSocketResources() {
        stopHeartbeatThread();
        if (heartbeatThread.joinable()) {
            heartbeatThread.join();
        }
    }

    void addHeartbeatRecord(const std::string& connId, const std::shared_ptr<oatpp::websocket::AsyncWebSocket>& socket, const std::string& userUuid) {
        std::lock_guard<std::mutex> lock(heartbeatMutex);
        heartbeatRecords[connId] = std::make_shared<HeartbeatRecord>(socket, userUuid);
    }

    void updateActivityTime(const std::string& connId) {
        std::lock_guard<std::mutex> lock(heartbeatMutex);
        auto it = heartbeatRecords.find(connId);
        if (it != heartbeatRecords.end()) {
            it->second->lastActivityTime.store(std::chrono::steady_clock::now());
        }
    }

    void removeHeartbeatRecord(const std::string& connId) {
        std::lock_guard<std::mutex> lock(heartbeatMutex);
        heartbeatRecords.erase(connId);
    }

public:
    void startHeartbeatThread() {
        heartbeatRunning.store(true);
        heartbeatThread = std::thread([this]() {
            while (heartbeatRunning.load()) {
                std::this_thread::sleep_for(HEARTBEAT_INTERVAL);
                checkHeartbeat();
            }
        });
        // 不调用 detach()，在析构函数中 join()
    }

    void stopHeartbeatThread() {
        heartbeatRunning.store(false);
    }

    void checkHeartbeat() {
        auto now = std::chrono::steady_clock::now();
        
        // 需要发送 Ping 的连接
        std::vector<std::pair<std::string, std::weak_ptr<oatpp::websocket::AsyncWebSocket>>> toPing;
        
        {
            std::lock_guard<std::mutex> lock(heartbeatMutex);
            for (auto it = heartbeatRecords.begin(); it != heartbeatRecords.end(); ) {
                auto& record = it->second;
                auto lastTime = record->lastActivityTime.load();
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastTime);

                if (elapsed >= TIMEOUT_THRESHOLD) {
                    // 超时，关闭连接并删除记录
                    OATPP_LOGW("Heartbeat", "Connection timeout for user %s, closing connection", record->userUuid.c_str());
                    if (auto socket = record->socket.lock()) {
                        socket->sendCloseAsync();
                    }
                    it = heartbeatRecords.erase(it);
                } else {
                    // 检查是否需要发送 Ping（避免 Ping 风暴）
                    auto lastPing = record->lastPingSentTime.load();
                    auto pingElapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastPing);
                    
                    if (pingElapsed >= HEARTBEAT_INTERVAL) {
                        // 记录需要发送 Ping 的连接（复制 weak_ptr）
                        toPing.emplace_back(it->first, record->socket);
                        record->lastPingSentTime.store(now);
                    }
                    ++it;
                }
            }
        }

        // 在锁外发送 Ping
        for (auto& [connId, weakSocket] : toPing) {
            if (auto socket = weakSocket.lock()) {
                socket->sendPingAsync(oatpp::String("ping"));
            }
        }
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
    // UUID → BIGINT ID 转换（走 Redis 缓存，防重复 DB 查询）
    oatpp::Int64 resolveUserId(const oatpp::String& uuid) {
        auto id = idCache->getUserId(uuid);
        if (id <= 0) throw std::runtime_error("User not found: " + *uuid);
        return id;
    }

    oatpp::Int64 resolveMessageId(const oatpp::String& uuid) {
        auto id = idCache->getMessageId(uuid);
        if (id <= 0) throw std::runtime_error("Message not found: " + *uuid);
        return id;
    }

    oatpp::Int64 resolveGroupId(const oatpp::String& uuid) {
        auto id = idCache->getGroupId(uuid);
        if (id <= 0) throw std::runtime_error("Group not found: " + *uuid);
        return id;
    }

    // 获取群成员 UUID 列表（排除发送者）
    std::vector<oatpp::String> getGroupMemberUuids(oatpp::Int64 groupId, const oatpp::String& excludeUserUuid) {
        std::vector<oatpp::String> uuids;
        try {
            if (groupId <= 0) return uuids;

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
            // 防重：同一用户对同一目标 3 秒内重复请求
            if (!redis->tryAcquireDedupLock(*userUuid, "send_friend_req",
                    *request->toUserUuid, HashUtils::hashContent(request->message), 3)) {
                sendError(userUuid, "Duplicate request, please wait");
                return;
            }
            auto result = friendService->sendFriendRequest(userUuid, request);
            sendResponse(userUuid, "send_friend_request_response", objectMapper->writeToString(result));
            //可以考虑给推送方发一个提示，表示有新的请求
        };

        handlers["handle_friend_request"] = [this](const oatpp::String& userUuid, const oatpp::String& msg) {
            auto request = objectMapper->readFromString<oatpp::Object<HandleFriendRequestDTO>>(msg);
            if (!request || !request->reqUuid || !request->status) {
                sendError(userUuid, "Request UUID and status are required");
                return;
            }
            if (!redis->tryAcquireDedupLock(*userUuid, "handle_friend_req",
                    *request->reqUuid, "", 3)) {
                sendError(userUuid, "Duplicate request, please wait");
                return;
            }
            auto result = friendService->handleFriendRequest(userUuid, request->reqUuid, request->status);
            sendResponse(userUuid, "handle_friend_request_response", objectMapper->writeToString(result));

            if (request->status == "accepted") {
                oatpp::Int64 fromUserId = result->fromUserId;
                oatpp::Int64 toUserId = result->toUserId;

                auto fromUserUuid = idCache->getUserUuid(fromUserId);
                //auto toUserUuid = idCache->getUserUuid(toUserId);

                auto toUserInfoResult = appClient->getUserById(toUserId);
                if (toUserInfoResult->isSuccess() && toUserInfoResult->hasMoreToFetch()) {
                    auto toUserInfo = toUserInfoResult->fetch<oatpp::Vector<oatpp::Object<UserInfoVO>>>()[0];

                    auto broadcast = oatpp::Object<WebSocketResponseDTO>::createShared();
                    broadcast->type = "friend_request_accepted";
                    broadcast->content = objectMapper->writeToString(toUserInfo);
                    broadcast->success = true;

                    webSocket->sendMessageToUser(fromUserUuid, objectMapper->writeToString(broadcast));
                }
            }
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
            if (!redis->tryAcquireDedupLock(*userUuid, "create_group",
                    "", HashUtils::hashContent(request->name), 5)) {
                sendError(userUuid, "Duplicate request, please wait");
                return;
            }
            auto result = groupService->createGroup(userUuid, request);
            sendResponse(userUuid, "create_group_response", objectMapper->writeToString(result));

            if (request->memberUuids && !request->memberUuids->empty()) {
                auto broadcast = oatpp::Object<WebSocketResponseDTO>::createShared();
                broadcast->type = "group_created";
                broadcast->content = objectMapper->writeToString(result);
                broadcast->success = true;
                auto broadcastJson = objectMapper->writeToString(broadcast);

                std::vector<oatpp::String> memberUuids;
                for (const auto& memberUuid : *request->memberUuids) {
                    if (memberUuid && !memberUuid->empty()) {
                        memberUuids.push_back(memberUuid);
                    }
                }

                if (!memberUuids.empty()) {
                    webSocket->batchPushMessage(memberUuids, broadcastJson);
                }
            }
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
            // 防重：同一群组 5 秒内重复解散
            if (!redis->tryAcquireDedupLock(*userUuid, "dissolve_group",
                    *request->groupUuid, "", 5)) {
                sendError(userUuid, "Duplicate request, please wait");
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

            auto targetUserUuid = request->userUuid;

            auto broadcast = oatpp::Object<WebSocketResponseDTO>::createShared();
            broadcast->type = "group_member_removed";
            broadcast->success = true;

            oatpp::Object<GroupUuidDTO> eventDto = oatpp::Object<GroupUuidDTO>::createShared();
            eventDto->groupUuid = request->groupUuid;
            broadcast->content = objectMapper->writeToString(eventDto);

            webSocket->sendMessageToUser(targetUserUuid, objectMapper->writeToString(broadcast));
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
            // 防重：同一用户对同一目标 2 秒内重复发送相同内容
            if (!redis->tryAcquireDedupLock(*userUuid, "send_private_msg",
                    *request->toUserUuid, HashUtils::hashContent(request->content), 2)) {
                sendError(userUuid, "Duplicate message, please wait");
                return;
            }

            // 开始事务
            auto transaction = appClient->beginTransaction();
            oatpp::Object<PrivateMessageVO> result;

            try {
                result = messageService->sendPrivateMessage(userUuid, request);

                auto senderId = resolveUserId(userUuid);
                auto receiverId = resolveUserId(request->toUserUuid);
                auto messageId = resolveMessageId(result->msguuid);

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

            oatpp::Object<MessageInfoVO> messageInfo;
            try {
                messageInfo = messageService->getMessageInfo(request->messageUuid);
            } catch (const std::exception& e) {
                sendError(userUuid, "Failed to get message info");
                return;
            }

            auto result = messageService->recallMessage(userUuid, request->messageUuid);
            sendResponse(userUuid, "recall_message_response", objectMapper->writeToString(result));

            // 构建撤回广播消息
            auto recallBroadcast = oatpp::Object<WebSocketResponseDTO>::createShared();
            recallBroadcast->type = "message_recalled";
            recallBroadcast->content = request->messageUuid;
            recallBroadcast->success = true;
            auto recallJson = objectMapper->writeToString(recallBroadcast);

            // 通知相关用户
            if (messageInfo->toGroupId > 0) {
                // 群聊消息，通知所有群成员（包括发送者自己）
                auto memberUuids = getGroupMemberUuids(messageInfo->toGroupId, userUuid);
                memberUuids.push_back(userUuid);  // 包括发送者
                webSocket->batchPushMessage(memberUuids, recallJson);
            } else if (messageInfo->toUserId > 0) {
                // 私聊消息，通知发送者和接收者
                auto senderUuid = messageInfo->fromUserUuid;
                //为了接口复用，用户卡一下应该是能接受的
                auto receiverUuidObj = idCache->getUserUuid(messageInfo->toUserId);
                if (receiverUuidObj) {
                    webSocket->batchPushMessage({senderUuid, receiverUuidObj}, recallJson);
                }
            }
        };

        handlers["send_group_message"] = [this](const oatpp::String& userUuid, const oatpp::String& msg) {
            auto request = objectMapper->readFromString<oatpp::Object<SendGroupMessageRequestDTO>>(msg);
            if (!request) {
                sendError(userUuid, "Invalid message data");
                return;
            }
            // 防重：同一用户在同一群 2 秒内重复发送相同内容
            if (!redis->tryAcquireDedupLock(*userUuid, "send_group_msg",
                    *request->groupUuid, HashUtils::hashContent(request->content), 2)) {
                sendError(userUuid, "Duplicate message, please wait");
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
                memberUuids = getGroupMemberUuids(groupId, userUuid);
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
            // 防重：同一用户对同一群 3 秒内重复申请
            if (!redis->tryAcquireDedupLock(*userUuid, "send_group_req",
                    *request->groupUuid, HashUtils::hashContent(request->message), 3)) {
                sendError(userUuid, "Duplicate request, please wait");
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
            if (!redis->tryAcquireDedupLock(*userUuid, "handle_group_req",
                    *request->grUuid, "", 3)) {
                sendError(userUuid, "Duplicate request, please wait");
                return;
            }
            auto result = groupService->handleGroupRequest(userUuid, request->grUuid, request);
            sendResponse(userUuid, "handle_group_request_response", objectMapper->writeToString(result));

            if (request->status == "accepted") {
                oatpp::Int64 groupId = result->groupId;
                oatpp::Int64 requesterId = result->requesterId;

                auto requesterUuid = idCache->getUserUuid(requesterId);
                //auto groupUuid = idCache->getGroupUuid(groupId);

                auto groupInfoResult = appClient->getGroupDetail(groupId);
                if (groupInfoResult->isSuccess() && groupInfoResult->hasMoreToFetch()) {
                    auto groupInfo = groupInfoResult->fetch<oatpp::Vector<oatpp::Object<GroupDetailInfoVO>>>()[0];

                    auto broadcast = oatpp::Object<WebSocketResponseDTO>::createShared();
                    broadcast->type = "group_request_accepted";
                    broadcast->content = objectMapper->writeToString(groupInfo);
                    broadcast->success = true;

                    webSocket->sendMessageToUser(requesterUuid, objectMapper->writeToString(broadcast));
                }
            }
        };
    }
};