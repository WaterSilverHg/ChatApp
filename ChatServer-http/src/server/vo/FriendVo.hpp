#pragma once

#include "global.h"

#include OATPP_CODEGEN_BEGIN(DTO) 



//已发送好友请求的 VO
class SentFriendRequestVO : public oatpp::DTO {
    DTO_INIT(SentFriendRequestVO, DTO)
        DTO_FIELD(String, uuid, "uuid");
    DTO_FIELD(Object<UserInfoVO>, receiver, "receiver");   // 接收方用户信息
    DTO_FIELD(String, status, "status");
    DTO_FIELD(String, createdAt, "createdat");
    DTO_FIELD(String, message, "message");
};
//已收到好友请求的 VO
class ReceivedFriendRequestVO : public oatpp::DTO {
    DTO_INIT(ReceivedFriendRequestVO, DTO)
        DTO_FIELD(String, uuid, "uuid");
    DTO_FIELD(Object<UserInfoVO>, requester, "requester"); // 请求方用户信息
    DTO_FIELD(String, status, "status");
    DTO_FIELD(String, createdAt, "createdat");
    DTO_FIELD(String, message, "message");
};

// 好友信息
class FriendInfoVO : public oatpp::DTO {
    DTO_INIT(FriendInfoVO, DTO)
    DTO_FIELD(String, friendUuid, "frienduuid");
    DTO_FIELD(String, username, "username");
    DTO_FIELD(String, avatarUrl, "avatarurl");
    DTO_FIELD(String, remark, "remark");
    DTO_FIELD(String, groupName, "groupname");
    DTO_FIELD(String, status, "status"); // online, offline
    DTO_FIELD(String, createdAt, "createdat");
};

// 好友详情信息（用于查看好友详情弹窗）
class UserDetailInfoVO : public oatpp::DTO {
    DTO_INIT(UserDetailInfoVO, DTO)
    DTO_FIELD(String, userUuid, "useruuid");
    DTO_FIELD(String, username, "username");
    DTO_FIELD(String, email, "email");
    DTO_FIELD(String, avatarUrl, "avatarurl");
    DTO_FIELD(String, remark, "remark");
    DTO_FIELD(String, groupName, "groupname");
    DTO_FIELD(String, status, "status");
    DTO_FIELD(String, lastSeen, "lastseen");
    DTO_FIELD(Boolean, isMuted, "ismuted");
    DTO_FIELD(String, createdAt, "createdat");
};



#include OATPP_CODEGEN_END(DTO) 
