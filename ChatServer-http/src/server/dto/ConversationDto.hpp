#pragma once

#include "global.h"

#include OATPP_CODEGEN_BEGIN(DTO) 


// 创建会话请求
class CreateConversationRequestDTO : public oatpp::DTO {
    DTO_INIT(CreateConversationRequestDTO, DTO)

        DTO_FIELD(String, type, "type");           // "private" 或 "group"
    DTO_FIELD(String, targetUserId, "targetuserid");     // 单聊时的目标用户ID
    DTO_FIELD(String, groupId, "groupid");               // 群聊时的群组ID
    DTO_FIELD(String, name, "name");                    // 可选：自定义会话名称
    DTO_FIELD(String, avatarUrl, "avatarurl");          // 可选：自定义会话头像
    DTO_FIELD(oatpp::Vector<Int64>, memberIds, "memberids"); // 创建群聊时的成员列表（可选）
};

// 更新会话请求
class UpdateConversationRequestDTO : public oatpp::DTO {
    DTO_INIT(UpdateConversationRequestDTO, DTO)
    DTO_FIELD(String, name, "name");
    DTO_FIELD(String, avatarUrl, "avatarurl");
};

// 添加会话成员请求
class AddConversationMemberRequestDTO : public oatpp::DTO {
    DTO_INIT(AddConversationMemberRequestDTO, DTO)
    DTO_FIELD(oatpp::Vector<oatpp::String>, userIds, "userids");
};

// 未读消息计数
class UnreadCountDTO : public oatpp::DTO {
    DTO_INIT(UnreadCountDTO, DTO)
    DTO_FIELD(Int32, total, "total");
    DTO_FIELD(Int32, privateMessages, "privatemessages");
    DTO_FIELD(Int32, groupMessages, "groupmessages");
};

class ConversationInfoDTO : public oatpp::DTO {
    DTO_INIT(ConversationInfoDTO, DTO)
        DTO_FIELD(String, uuid);
    DTO_FIELD(Int64, userId);
    DTO_FIELD(String, type);      // "private" 或 "group"
    DTO_FIELD(Int64, targetId);   // group_id 或 user_id
};

#include OATPP_CODEGEN_END(DTO) 
