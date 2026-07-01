#pragma once

#include "global.h"
#include"server/vo/UserStatusVo.hpp"

#include OATPP_CODEGEN_BEGIN(DTO) 

// 好友请求
//class FriendRequestDTO : public oatpp::DTO {
//    DTO_INIT(FriendRequestDTO, DTO)
//    DTO_FIELD(String, uuid, "uuid");
//    DTO_FIELD(String, fromUserId, "fromuserid");
//    DTO_FIELD(String, toUserId, "touserid");
//    DTO_FIELD(String, status, "status"); // pending, accepted, rejected
//    DTO_FIELD(String, createdAt, "createdat");
//    DTO_FIELD(String, message, "message");
//};
// 好友请求
class FriendRequestResponseDTO : public oatpp::DTO {
    DTO_INIT(FriendRequestResponseDTO, DTO)
        DTO_FIELD(String, uuid, "uuid");
    DTO_FIELD(String, fromUserUuid, "fromuseruuid");
    DTO_FIELD(String, toUserUuid, "touseruuid");
    DTO_FIELD(String, status, "status"); // pending, accepted, rejected
    DTO_FIELD(String, createdAt, "createdat");
    DTO_FIELD(String, message, "message");
};

// 好友请求（包含请求方用户信息）
class ReceivedFriendRequestDetailDTO : public oatpp::DTO {
    DTO_INIT(ReceivedFriendRequestDetailDTO, DTO)
    DTO_FIELD(String, uuid, "uuid");
    DTO_FIELD(String, fromUserUuid, "fromuseruuid");
    DTO_FIELD(String, fromUsername, "fromusername");
    DTO_FIELD(String, fromAvatarUrl, "fromavatarurl");
    DTO_FIELD(String, toUserUuid, "touseruuid");
    DTO_FIELD(String, status, "status");
    DTO_FIELD(String, createdAt, "createdat");
    DTO_FIELD(String, message, "message");
};

// 好友请求（包含接收方用户信息）
class SentFriendRequestDetailDTO : public oatpp::DTO {
    DTO_INIT(SentFriendRequestDetailDTO, DTO)
    DTO_FIELD(String, uuid, "uuid");
    DTO_FIELD(String, fromUserUuid, "fromuseruuid");
    DTO_FIELD(String, toUserUuid, "touseruuid");
    DTO_FIELD(String, toUsername, "tousername");
    DTO_FIELD(String, toAvatarUrl, "toavatarurl");
    DTO_FIELD(String, status, "status");
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
    DTO_FIELD(String, remark, "remark");
};

// 移动好友到分组
class UpdateFriendGroupDTO : public oatpp::DTO {
    DTO_INIT(UpdateFriendGroupDTO, DTO)
    DTO_FIELD(String, group, "group");
};

// 创建/管理好友分组
class CreateFriendGroupDTO : public oatpp::DTO {
    DTO_INIT(CreateFriendGroupDTO, DTO)
    DTO_FIELD(String, name, "name");
    DTO_FIELD(Vector<String>, memberUuids, "memberuuids");
};

class RenameFriendGroupDTO : public oatpp::DTO {
    DTO_INIT(RenameFriendGroupDTO, DTO)
    DTO_FIELD(String, newName, "newname");
};

// 分组名列表项
class GroupNameVO : public oatpp::DTO {
    DTO_INIT(GroupNameVO, DTO)
    DTO_FIELD(String, groupName, "groupname");
};

#include OATPP_CODEGEN_END(DTO)
