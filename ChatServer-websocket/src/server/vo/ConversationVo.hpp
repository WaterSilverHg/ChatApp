#pragma once

#include "global.h"

#include OATPP_CODEGEN_BEGIN(DTO) 

// 会话信息
class ConversationVO : public oatpp::DTO {
    DTO_INIT(ConversationVO, DTO)
    DTO_FIELD(String, type, "type"); // private, group
    DTO_FIELD(String, targetId, "targetid"); // userId or groupId
    DTO_FIELD(String, targetName, "targetname");
    DTO_FIELD(String, targetAvatar, "targetavatar");
    DTO_FIELD(String, lastMessage, "lastmessage");
    DTO_FIELD(String, lastMessageType, "lastmessagetype");
    DTO_FIELD(String, lastMessageTime, "lastmessagetime");
    DTO_FIELD(Int32, unreadCount, "unreadcount");
    DTO_FIELD(Boolean, isMuted, "ismuted");
};

//// 会话成员
//class ConversationMemberVO : public oatpp::DTO {
//    DTO_INIT(ConversationMemberVO, DTO)
//        DTO_FIELD(Int64, id, "id");
//    DTO_FIELD(Int64, userId, "userId");
//    DTO_FIELD(String, username, "username");
//    DTO_FIELD(String, avatarUrl, "avatarUrl");
//    DTO_FIELD(String, role, "role");
//    DTO_FIELD(String, joinedAt, "joinedAt");
//};

// 会话详情
//class ConversationDetailVO : public oatpp::DTO {
//    DTO_INIT(ConversationDetailVO, DTO)
//    DTO_FIELD(String, uuid, "uuid");
//    DTO_FIELD(String, type, "type");
//    DTO_FIELD(String, name, "name");
//    DTO_FIELD(String, avatarUrl, "avatarUrl");
//    DTO_FIELD(String, createdAt, "createdAt");
//    DTO_FIELD(oatpp::Vector<oatpp::Object<ConversationMemberVO>>, members, "members");
//};



#include OATPP_CODEGEN_END(DTO) 
