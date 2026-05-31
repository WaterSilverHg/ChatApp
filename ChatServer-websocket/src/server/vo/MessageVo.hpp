#pragma once

#include "global.h"

#include OATPP_CODEGEN_BEGIN(DTO) 

// 私聊消息
class PrivateMessageVO : public oatpp::DTO {
    DTO_INIT(PrivateMessageVO, DTO)
    DTO_FIELD(String, msguuid, "msguuid");
    DTO_FIELD(String, fromUserUuid, "fromuseruuid");
    DTO_FIELD(String, toUserUuid, "touseruuid");
    DTO_FIELD(String, messageType, "messagetype");
    DTO_FIELD(String, content, "content");
    DTO_FIELD(String, fileUrl, "fileurl");
    DTO_FIELD(Int64, fileSize, "filesize");
    DTO_FIELD(String, fileName, "filename");
    DTO_FIELD(String, mimeType, "mimetype");
    DTO_FIELD(String, createdAt, "createdat");
};


// 群聊消息
class GroupMessageVO : public oatpp::DTO {
    DTO_INIT(GroupMessageVO, DTO)
        DTO_FIELD(String, uuid, "msguuid");
    DTO_FIELD(String, fromUserUuid, "fromuseruuid");
    DTO_FIELD(String, groupUuid, "groupuuid");
    DTO_FIELD(String, messageType, "messagetype"); // text, image, file
    DTO_FIELD(String, content, "content");
    DTO_FIELD(String, fileUrl, "fileurl");
    DTO_FIELD(Int64, fileSize, "filesize");
    DTO_FIELD(String, fileName, "filename");
    DTO_FIELD(String, mimeType, "mimetype");
    DTO_FIELD(String, createdAt, "createdat");
};

// 离线消息
class OfflineMessageVO : public oatpp::DTO {
    DTO_INIT(OfflineMessageVO, DTO)

        DTO_FIELD(String, id, "id");                    // UUID
    DTO_FIELD(String, fromUserId, "fromuserid");    // 发送者 UUID
    DTO_FIELD(String, toUserId, "touserid");        // 接收者 UUID
    DTO_FIELD(String, content, "content");
    DTO_FIELD(String, messageType, "messagetype");  // text, image, file, audio, video
    DTO_FIELD(String, createdAt, "createdat");
    DTO_FIELD(Boolean, isRead, "isread");

    // 文件相关字段（可选，仅当 messageType 为 file/image/audio/video 时有值）
    DTO_FIELD(String, fileUrl, "fileurl");
    DTO_FIELD(Int64, fileSize, "filesize");
    DTO_FIELD(String, fileName, "filename");
    DTO_FIELD(String, mimeType, "mimetype");
};

// 消息详情（用于撤回广播）
class MessageInfoVO : public oatpp::DTO {
    DTO_INIT(MessageInfoVO, DTO)
    DTO_FIELD(Int64, id, "id");
    DTO_FIELD(String, msgUuid, "msguuid");
    DTO_FIELD(Int64, fromUserId, "fromuserid");
    DTO_FIELD(String, fromUserUuid, "fromuseruuid");
    DTO_FIELD(Int64, toUserId, "touserid");
    DTO_FIELD(Int64, toGroupId, "togroupid");
    DTO_FIELD(String, messageType, "messagetype");
};

#include OATPP_CODEGEN_END(DTO) 
