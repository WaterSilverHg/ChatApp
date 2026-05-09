#pragma once

#include "global.h"

#include OATPP_CODEGEN_BEGIN(DTO) 

// 更新状态请求
class UpdateStatusRequestDTO : public oatpp::DTO {
    DTO_INIT(UpdateStatusRequestDTO, DTO)
    DTO_FIELD(String, status, "status");
};

// 批量获取状态请求
class BatchStatusRequestDTO : public oatpp::DTO {
    DTO_INIT(BatchStatusRequestDTO, DTO)
    DTO_FIELD(List<oatpp::String>, userUuids, "useruuids");
};

// 用户状态
class UserStatusDTO : public oatpp::DTO {
    DTO_INIT(UserStatusDTO, DTO)
    DTO_FIELD(Int64, userId, "userid");
    DTO_FIELD(String, status, "status");
    DTO_FIELD(String, lastSeen, "lastseen");
};

// 在线状态
class OnlineStatusDTO : public oatpp::DTO {
    DTO_INIT(OnlineStatusDTO, DTO)
    DTO_FIELD(Int64, userId, "userid");
    DTO_FIELD(Boolean, isOnline, "isonline");
    DTO_FIELD(String, lastSeen, "lastseen");
};

//class LastSeenDTO : public oatpp::DTO {
//    DTO_INIT(LastSeenDTO, DTO)
//    DTO_FIELD(String, lastSeen, "lastseen");
//};

// 更新用户资料请求
class UpdateProfileRequestDTO : public oatpp::DTO {
    DTO_INIT(UpdateProfileRequestDTO, DTO)
        DTO_FIELD(String, username, "username");
    DTO_FIELD(String, avatarUrl, "avatarUrl");
};

#include OATPP_CODEGEN_END(DTO) 
