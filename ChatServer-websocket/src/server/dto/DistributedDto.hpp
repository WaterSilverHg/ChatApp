#pragma once

#include "global.h"

#include OATPP_CODEGEN_BEGIN(DTO)

// 分布式私聊消息 DTO
class DistributedMessageDTO : public oatpp::DTO {
    DTO_INIT(DistributedMessageDTO, DTO)
    
    DTO_FIELD(String, serverId, "serverid");           // 发送消息的服务器ID
    DTO_FIELD(String, targetUserUuid, "targetuseruuid");     // 目标用户UUID
    DTO_FIELD(String, payload, "payload");            // 消息内容
};

// 分布式群消息 DTO
class DistributedGroupMessageDTO : public oatpp::DTO {
    DTO_INIT(DistributedGroupMessageDTO, DTO)
    
    DTO_FIELD(String, serverId, "serverid");                  // 发送消息的服务器ID
    DTO_FIELD(Vector<String>, targetUserUuids, "targetuseruuids");   // 目标用户UUID列表
    DTO_FIELD(String, payload, "payload");                   // 消息内容
};

// 分布式状态 DTO
class DistributedStatusDTO : public oatpp::DTO {
    DTO_INIT(DistributedStatusDTO, DTO)
    
    DTO_FIELD(String, serverId, "serverid");     // 发送状态的服务器ID
    DTO_FIELD(String, userUuid, "useruuid");     // 用户UUID
    DTO_FIELD(String, status, "status");       // 状态（online/offline）
};

#include OATPP_CODEGEN_END(DTO)