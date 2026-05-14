#pragma once

#include "global.h"

#include OATPP_CODEGEN_BEGIN(DTO) 

class UuidDTO : public oatpp::DTO {
    DTO_INIT(UuidDTO, DTO)
        DTO_FIELD(String, uuid, "uuid");
};

class IdDTO : public oatpp::DTO {
    DTO_INIT(IdDTO, DTO)
        DTO_FIELD(Int64, id, "id");
};

class StatusDTO : public oatpp::DTO {
    DTO_INIT(StatusDTO, DTO)
        DTO_FIELD(String, status, "status");
};

// 分页请求
class PageRequestDTO : public oatpp::DTO {
    DTO_INIT(PageRequestDTO, DTO)
        DTO_FIELD(Int32, page, "page");
    DTO_FIELD(Int32, size, "size");
};

// 会话UUID请求
class ConvUuidDTO : public oatpp::DTO {
    DTO_INIT(ConvUuidDTO, DTO)
        DTO_FIELD(String, convUuid, "convuuid");
};

// 会话消息请求
class ConvMessagesRequestDTO : public oatpp::DTO {
    DTO_INIT(ConvMessagesRequestDTO, DTO)
        DTO_FIELD(String, convUuid, "convuuid");
    DTO_FIELD(Int32, page, "page");
    DTO_FIELD(Int32, size, "size");
};

// 请求UUID
class RequestUuidDTO : public oatpp::DTO {
    DTO_INIT(RequestUuidDTO, DTO)
        DTO_FIELD(String, reqUuid, "requuid");
};

// 好友UUID
class FriendUuidDTO : public oatpp::DTO {
    DTO_INIT(FriendUuidDTO, DTO)
        DTO_FIELD(String, friendUuid, "frienduuid");
};

// 目标用户UUID
class TargetUserUuidDTO : public oatpp::DTO {
    DTO_INIT(TargetUserUuidDTO, DTO)
        DTO_FIELD(String, targetUserUuid, "targetuseruuid");
};

// 群组UUID
class GroupUuidDTO : public oatpp::DTO {
    DTO_INIT(GroupUuidDTO, DTO)
        DTO_FIELD(String, groupUuid, "groupuuid");
};

// 移除群成员请求
class RemoveGroupMemberDTO : public oatpp::DTO {
    DTO_INIT(RemoveGroupMemberDTO, DTO)
        DTO_FIELD(String, groupUuid, "groupuuid");
    DTO_FIELD(String, userUuid, "useruuid");
};

// 用户UUID
class UserUuidDTO : public oatpp::DTO {
    DTO_INIT(UserUuidDTO, DTO)
        DTO_FIELD(String, userUuid, "useruuid");
};

// 私聊消息分页请求
class PrivateMessagesPageRequestDTO : public oatpp::DTO {
    DTO_INIT(PrivateMessagesPageRequestDTO, DTO)
        DTO_FIELD(String, userUuid, "useruuid");
    DTO_FIELD(Int32, page, "page");
    DTO_FIELD(Int32, size, "size");
};

// 消息UUID
class MessageUuidDTO : public oatpp::DTO {
    DTO_INIT(MessageUuidDTO, DTO)
        DTO_FIELD(String, messageUuid, "messageuuid");
};

// 群消息分页请求
class GroupMessagesPageRequestDTO : public oatpp::DTO {
    DTO_INIT(GroupMessagesPageRequestDTO, DTO)
    DTO_FIELD(String, groupUuid, "groupuuid");
    DTO_FIELD(Int32, page, "page");
    DTO_FIELD(Int32, size, "size");
};

// 好友请求用户ID对
class FriendRequestIdsDTO : public oatpp::DTO {
    DTO_INIT(FriendRequestIdsDTO, DTO)
    DTO_FIELD(Int64, fromUserId, "from_user_id");
    DTO_FIELD(Int64, toUserId, "to_user_id");
};

#include OATPP_CODEGEN_END(DTO)