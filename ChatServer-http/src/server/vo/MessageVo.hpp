#pragma once

#include "global.h"
#include"UserStatusVo.hpp"

#include OATPP_CODEGEN_BEGIN(DTO) 

// 私聊消息
class PrivateMessageVO : public oatpp::DTO {
    DTO_INIT(PrivateMessageVO, DTO)
    DTO_FIELD(String, uuid, "uuid");
    DTO_FIELD(String, fromUserUuid, "fromuseruuid");
    DTO_FIELD(String, username, "username");
    DTO_FIELD(String, avatarUrl, "avatarurl");
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
        DTO_FIELD(String, uuid, "uuid");
    DTO_FIELD(String, fromUserUuid, "fromuseruuid");
    DTO_FIELD(String, username, "username");
    DTO_FIELD(String, avatarUrl, "avatarurl");
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

#include OATPP_CODEGEN_END(DTO) 
