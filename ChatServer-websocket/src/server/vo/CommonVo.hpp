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


//// 消息创建结果
//class MessageCreateVO : public oatpp::DTO {
//    DTO_INIT(MessageCreateVO, DTO)
//    DTO_FIELD(String, uuid, "uuid");
//    DTO_FIELD(String, createdAt, "createdat");
//};

#include OATPP_CODEGEN_END(DTO) 