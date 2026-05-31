#pragma once

#include "global.h"

#include OATPP_CODEGEN_BEGIN(DTO) 

// 私聊消息
class PrivateMessageDTO : public oatpp::DTO {
    DTO_INIT(PrivateMessageDTO, DTO)

        DTO_FIELD(String, id, "id");                    // UUID
    DTO_FIELD(String, fromUserId, "fromuserid");    // 发送者 UUID
    DTO_FIELD(String, toUserId, "touserid");        // 接收者 UUID
    DTO_FIELD(String, content, "content");
    DTO_FIELD(String, messageType, "messagetype");  // text, image, file, audio, video, recalled
    DTO_FIELD(String, createdAt, "createdat");
    DTO_FIELD(String, deletedAt, "deletedat");      // 软删除时间
    DTO_FIELD(String, clientMsgId, "clientmsgid");  // 客户端消息 ID（用于去重）

    // 文件相关字段（可选，仅当 messageType 为 file/image/audio/video 时有值）
    DTO_FIELD(String, fileUrl, "fileurl");
    DTO_FIELD(Int64, fileSize, "filesize");
    DTO_FIELD(String, fileName, "filename");
    DTO_FIELD(String, mimeType, "mimetype");
};

// 群聊消息
class GroupMessageDTO : public oatpp::DTO {
    DTO_INIT(GroupMessageDTO, DTO)

        DTO_FIELD(String, id, "id");                    // UUID
    DTO_FIELD(String, fromUserId, "fromuserid");    // 发送者 UUID
    DTO_FIELD(String, groupId, "groupid");          // 群组 UUID
    DTO_FIELD(String, content, "content");
    DTO_FIELD(String, messageType, "messagetype");  // text, image, file, audio, video, recalled
    DTO_FIELD(String, createdAt, "createdat");
    DTO_FIELD(String, deletedAt, "deletedat");      // 软删除时间
    DTO_FIELD(String, clientMsgId, "clientmsgid");  // 客户端消息 ID（用于去重）

    // 文件相关字段（可选，仅当 messageType 为 file/image/audio/video 时有值）
    DTO_FIELD(String, fileUrl, "fileurl");
    DTO_FIELD(Int64, fileSize, "filesize");
    DTO_FIELD(String, fileName, "filename");
    DTO_FIELD(String, mimeType, "mimetype");
};

// 发送私聊消息请求
class SendPrivateMessageRequestDTO : public oatpp::DTO {
    DTO_INIT(SendPrivateMessageRequestDTO, DTO)

        DTO_FIELD(String, toUserUuid, "touseruuid");        // 接收者 UUID
    DTO_FIELD(String, content, "content");
    DTO_FIELD(String, messageType, "messagetype");  // text, image, file, audio, video
    //DTO_FIELD(String, clientMsgId, "clientmsgid");  // 客户端消息 ID（用于去重）

    // 文件相关字段（可选，仅当 messageType 为 file/image/audio/video 时需要）
    DTO_FIELD(String, fileUrl, "fileurl");
    DTO_FIELD(Int64, fileSize, "filesize");
    DTO_FIELD(String, fileName, "filename");
    DTO_FIELD(String, mimeType, "mimetype");
};

// 发送群消息请求
class SendGroupMessageRequestDTO : public oatpp::DTO {
    DTO_INIT(SendGroupMessageRequestDTO, DTO)

        DTO_FIELD(String, groupUuid, "groupuuid");          // 群组 UUID
    DTO_FIELD(String, content, "content");
    DTO_FIELD(String, messageType, "messagetype");  // text, image, file, audio, video

    // 文件相关字段（可选，仅当 messageType 为 file/image/audio/video 时需要）
    DTO_FIELD(String, fileUrl, "fileurl");
    DTO_FIELD(Int64, fileSize, "filesize");
    DTO_FIELD(String, fileName, "filename");
    DTO_FIELD(String, mimeType, "mimetype");
};

//// 消息信息结果
//class MessageInfoDTO : public oatpp::DTO {
//    DTO_INIT(MessageInfoDTO, DTO)
//
//        DTO_FIELD(String, msgUuid, "msgUuid");
//    DTO_FIELD(Int64, from_user_id, "from_user_id");
//    DTO_FIELD(Int64, to_user_id, "to_user_id");
//    DTO_FIELD(Int64, to_group_id, "to_group_id");
//    DTO_FIELD(String, message_type, "message_type");
//    DTO_FIELD(String, content, "content");
//    DTO_FIELD(String, file_url, "file_url");
//    DTO_FIELD(Int64, file_size, "file_size");
//    DTO_FIELD(String, file_name, "file_name");
//    DTO_FIELD(String, mime_type, "mime_type");
//    DTO_FIELD(String, status, "status");
//    DTO_FIELD(String, createdat, "createdat");
//};




#include OATPP_CODEGEN_END(DTO)