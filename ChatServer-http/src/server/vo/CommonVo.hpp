#pragma once

#include "global.h"

#include OATPP_CODEGEN_BEGIN(DTO) 

// 基本ID结果
class IdResultVO : public oatpp::DTO {
    DTO_INIT(IdResultVO, DTO)
    DTO_FIELD(Int64, id, "id");
};

// 基本UUID结果
class UuidResultVO : public oatpp::DTO {
    DTO_INIT(UuidResultVO, DTO)
    DTO_FIELD(String, uuid, "uuid");
};

// 消息信息结果
class MessageInfoVO : public oatpp::DTO {
    DTO_INIT(MessageInfoVO, DTO)
    DTO_FIELD(Int64, from_user_id, "from_user_id");
    DTO_FIELD(Int64, to_user_id, "to_user_id");
    DTO_FIELD(Int64, to_group_id, "to_group_id");
    DTO_FIELD(String, message_type, "message_type");
    DTO_FIELD(String, content, "content");
    DTO_FIELD(String, file_url, "file_url");
    DTO_FIELD(Int64, file_size, "file_size");
    DTO_FIELD(String, file_name, "file_name");
    DTO_FIELD(String, mime_type, "mime_type");
    DTO_FIELD(String, status, "status");
    DTO_FIELD(String, createdat, "createdat");
};

// 消息创建结果
class MessageCreateVO : public oatpp::DTO {
    DTO_INIT(MessageCreateVO, DTO)
    DTO_FIELD(String, uuid, "uuid");
    DTO_FIELD(String, createdAt, "createdat");
};

#include OATPP_CODEGEN_END(DTO) 