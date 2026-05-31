#pragma once

#include "global.h"

#include OATPP_CODEGEN_BEGIN(DTO) 

// 好友请求
class FriendRequestDTO : public oatpp::DTO {
    DTO_INIT(FriendRequestDTO, DTO)
    DTO_FIELD(String, uuid, "uuid");
    DTO_FIELD(String, fromUserId, "fromuserid");
    DTO_FIELD(String, toUserId, "touserid");
    DTO_FIELD(String, status, "status"); // pending, accepted, rejected
    DTO_FIELD(String, createdAt, "createdat");
    DTO_FIELD(String, message, "message");
};

 //发送好友请求
class SendFriendRequestDTO : public oatpp::DTO {
    DTO_INIT(SendFriendRequestDTO, DTO)
    DTO_FIELD(String, toUserUuid, "touseruuid");
    DTO_FIELD(String, message, "message");
};

// 好友信息
class FriendInfoDTO : public oatpp::DTO {
    DTO_INIT(FriendInfoDTO, DTO)
    DTO_FIELD(Int64, id, "id");
    DTO_FIELD(Int64, friendId, "friendid");
    DTO_FIELD(String, username, "username");
    DTO_FIELD(String, avatar, "avatar");
    DTO_FIELD(String, remark, "remark");
    DTO_FIELD(String, group, "group");
    DTO_FIELD(String, status, "status"); // online, offline
    DTO_FIELD(String, createdAt, "createdat");
};

// 修改好友备注
class UpdateFriendRemarkDTO : public oatpp::DTO {
    DTO_INIT(UpdateFriendRemarkDTO, DTO)
        DTO_FIELD(String, friendUuid, "frienduuid");
    DTO_FIELD(String, remark, "remark");
};

// 移动好友到分组
class UpdateFriendGroupDTO : public oatpp::DTO {
    DTO_INIT(UpdateFriendGroupDTO, DTO)
        DTO_FIELD(String, friendUuid, "frienduuid");
    DTO_FIELD(String, groupUuid, "groupuuid");
};

// 处理好友请求 (accept/reject/cancel)
class HandleFriendRequestDTO : public oatpp::DTO {
    DTO_INIT(HandleFriendRequestDTO, DTO)
    DTO_FIELD(String, reqUuid, "requuid");  // 好友请求UUID
    DTO_FIELD(String, status, "status");    // accepted/rejected/canceled
};

#include OATPP_CODEGEN_END(DTO) 
