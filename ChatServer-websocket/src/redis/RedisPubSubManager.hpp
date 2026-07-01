#pragma once
#include "global.h"
#include "websocket/AppWebSocket.hpp"
#include "AppRedis.hpp"
#include "server/dto/WebSocketMessageDto.hpp"
#include "server/dto/DistributedDto.hpp"

class RedisPubSubManager {
public:
    // 频道名称常量
    static constexpr const char* CHANNEL_PRIVATE_MESSAGE = "chatapp:private_message";
    static constexpr const char* CHANNEL_GROUP_MESSAGE = "chatapp:group_message";
    static constexpr const char* CHANNEL_USER_STATUS = "chatapp:user_status";
    static constexpr const char* CHANNEL_FRIEND_REQUEST = "chatapp:friend_request";
    static constexpr const char* CHANNEL_GROUP_EVENT = "chatapp:group_event";

private:
    std::shared_ptr<AppRedis> m_redis;
    std::shared_ptr<AppWebSocket> m_webSocket;
    std::shared_ptr<oatpp::data::mapping::ObjectMapper> m_objectMapper;
    std::string m_serverId;  // 服务器唯一标识，用于消息去重

public:
    RedisPubSubManager(
        const std::shared_ptr<AppRedis>& redis,
        const std::shared_ptr<AppWebSocket>& webSocket,
        const std::shared_ptr<oatpp::data::mapping::ObjectMapper>& objectMapper
    ) : m_redis(redis), m_webSocket(webSocket), m_objectMapper(objectMapper) {
        // 生成唯一的服务器ID
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<uint64_t> dis;
        m_serverId = "server_" + std::to_string(dis(gen));
        OATPP_LOGI("RedisPubSubManager", "Server ID: %s", m_serverId.c_str());
    }

    ~RedisPubSubManager() {
        stop();
    }

    // 启动订阅
    void start() {
        std::vector<std::string> channels = {
            CHANNEL_PRIVATE_MESSAGE,
            CHANNEL_GROUP_MESSAGE,
            CHANNEL_USER_STATUS,
            CHANNEL_FRIEND_REQUEST,
            CHANNEL_GROUP_EVENT
        };

        m_redis->setMessageCallback([this](const std::string& channel, const std::string& message) {
            this->handleMessage(channel, message);
        });

        if (m_redis->subscribe(channels)) {
            m_redis->startSubscriber();
            OATPP_LOGI("RedisPubSubManager", "Started subscribing to all channels");
        }
    }

    // 停止订阅
    void stop() {
        m_redis->stopSubscriber();
    }

    // 发布私聊消息
    void publishPrivateMessage(const oatpp::String& targetUserUuid, const oatpp::String& message) {
        auto dto = oatpp::Object<DistributedMessageDTO>::createShared();
        dto->serverId = m_serverId;
        dto->targetUserUuid = targetUserUuid;
        dto->payload = message;
        
        oatpp::String json = m_objectMapper->writeToString(dto);
        m_redis->publish(CHANNEL_PRIVATE_MESSAGE, json);
    }

    // 发布群聊消息
    void publishGroupMessage(const std::vector<oatpp::String>& targetUserUuids, const oatpp::String& message) {
        auto dto = oatpp::Object<DistributedGroupMessageDTO>::createShared();
        dto->serverId = m_serverId;
        // 将 std::vector 转换为 oatpp::Vector
        dto->targetUserUuids = oatpp::Vector<oatpp::String>::createShared();
        for (const auto& uuid : targetUserUuids) {
            dto->targetUserUuids->push_back(uuid);
        }
        dto->payload = message;
        
        oatpp::String json = m_objectMapper->writeToString(dto);
        m_redis->publish(CHANNEL_GROUP_MESSAGE, json);
    }

    // 发布用户状态变更
    void publishUserStatus(const oatpp::String& userUuid, const oatpp::String& status) {
        auto dto = oatpp::Object<DistributedStatusDTO>::createShared();
        dto->serverId = m_serverId;
        dto->userUuid = userUuid;
        dto->status = status;
        
        oatpp::String json = m_objectMapper->writeToString(dto);
        m_redis->publish(CHANNEL_USER_STATUS, json);
    }

private:
    // 处理接收到的消息
    void handleMessage(const std::string& channel, const std::string& message) {
        try {
            if (channel == CHANNEL_PRIVATE_MESSAGE) {
                handlePrivateMessage(message);
            } else if (channel == CHANNEL_GROUP_MESSAGE) {
                handleGroupMessage(message);
            } else if (channel == CHANNEL_USER_STATUS) {
                handleUserStatus(message);
            } else if (channel == CHANNEL_FRIEND_REQUEST) {
                handleFriendRequest(message);
            } else if (channel == CHANNEL_GROUP_EVENT) {
                handleGroupEvent(message);
            }
        } catch (const std::exception& e) {
            OATPP_LOGE("RedisPubSubManager", "Error handling message on channel %s: %s", 
                       channel.c_str(), e.what());
        }
    }

    // 处理私聊消息
    void handlePrivateMessage(const std::string& message) {
        auto dto = m_objectMapper->readFromString<oatpp::Object<DistributedMessageDTO>>(oatpp::String(message));
        
        // 跳过自己发送的消息（去重）
        if (dto && dto->serverId == m_serverId) {
            return;
        }

        // 尝试发送给本地连接的用户
        if (dto && dto->targetUserUuid && dto->payload) {
            m_webSocket->sendMessageToUser(dto->targetUserUuid, dto->payload);
        }
    }

    // 处理群聊消息
    void handleGroupMessage(const std::string& message) {
        auto dto = m_objectMapper->readFromString<oatpp::Object<DistributedGroupMessageDTO>>(oatpp::String(message));
        
        // 跳过自己发送的消息（去重）
        if (dto && dto->serverId == m_serverId) {
            return;
        }

        // 发送给本地连接的群成员
        if (dto && dto->targetUserUuids && dto->payload) {
            m_webSocket->batchPushMessage(*dto->targetUserUuids, dto->payload);
        }
    }

    // 处理用户状态变更
    void handleUserStatus(const std::string& message) {
        auto dto = m_objectMapper->readFromString<oatpp::Object<DistributedStatusDTO>>(oatpp::String(message));
        
        // 跳过自己发送的状态
        if (dto && dto->serverId == m_serverId) {
            return;
        }

        // 可以在这里添加状态同步逻辑，比如通知好友等
        if (dto && dto->userUuid && dto->status) {
            OATPP_LOGD("RedisPubSubManager", "User %s status changed to %s (from remote)", 
                       dto->userUuid->c_str(), dto->status->c_str());
        }
    }

    // 处理好友请求事件
    void handleFriendRequest(const std::string& message) {
        auto dto = m_objectMapper->readFromString<oatpp::Object<DistributedMessageDTO>>(oatpp::String(message));
        
        if (dto && dto->serverId == m_serverId) {
            return;
        }

        if (dto && dto->targetUserUuid && dto->payload) {
            m_webSocket->sendMessageToUser(dto->targetUserUuid, dto->payload);
        }
    }

    // 处理群组事件
    void handleGroupEvent(const std::string& message) {
        auto dto = m_objectMapper->readFromString<oatpp::Object<DistributedGroupMessageDTO>>(oatpp::String(message));
        
        if (dto && dto->serverId == m_serverId) {
            return;
        }

        if (dto && dto->targetUserUuids && dto->payload) {
            m_webSocket->batchPushMessage(*dto->targetUserUuids, dto->payload);
        }
    }
};