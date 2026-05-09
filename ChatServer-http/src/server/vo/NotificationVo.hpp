#pragma once

#include "global.h"

#include OATPP_CODEGEN_BEGIN(DTO) 

// 通知信息
class NotificationVO : public oatpp::DTO {
    DTO_INIT(NotificationVO, DTO)
    DTO_FIELD(Int64, id, "id");
    DTO_FIELD(String, type, "type"); // friend_request, group_invite, system
    DTO_FIELD(String, title, "title");
    DTO_FIELD(String, content, "content");
    DTO_FIELD(String, targetUrl, "targetUrl");
    DTO_FIELD(Boolean, isRead, "isRead");
    DTO_FIELD(String, createdAt, "createdAt");
};

#include OATPP_CODEGEN_END(DTO) 
