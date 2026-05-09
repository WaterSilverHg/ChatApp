#pragma once

#include "global.h"

#include OATPP_CODEGEN_BEGIN(DTO) 

// 群组信息
class GroupInfoVO : public oatpp::DTO {
    DTO_INIT(GroupInfoVO, DTO)
    DTO_FIELD(String, uuid, "uuid");
    DTO_FIELD(String, name, "name");
    DTO_FIELD(String, description, "description");
    DTO_FIELD(String, avatarUrl, "avatarurl");
    DTO_FIELD(String, ownerUuid, "owneruuid");
    DTO_FIELD(Int32, memberCount, "membercount");
    DTO_FIELD(String, createdAt, "createdat");
};

class GroupDetailInfoVO : public oatpp::DTO {
    DTO_INIT(GroupDetailInfoVO, DTO)

        DTO_FIELD(String, uuid, "uuid");                    
    DTO_FIELD(String, name, "name");                    
    DTO_FIELD(String, description, "description");     
    DTO_FIELD(String, avatarUrl, "avatarurl");         
    DTO_FIELD(Int32, memberCount, "membercount");      
    DTO_FIELD(Int32, maxMembers, "maxmembers");        
    DTO_FIELD(String, role, "role");               
    DTO_FIELD(Int32, unreadCount, "unreadcount");      
    DTO_FIELD(String, lastMessage, "lastmessage");     
    DTO_FIELD(String, lastMessageTime, "lastmessagetime"); 
    DTO_FIELD(Boolean, isMuted, "ismuted");          
    DTO_FIELD(String, createdAt, "createdat");       
};


// 群成员信息
class GroupMemberVO : public oatpp::DTO {
    DTO_INIT(GroupMemberVO, DTO)
    DTO_FIELD(String, userUuid, "useruuid");
    DTO_FIELD(String, username, "username");
    DTO_FIELD(String, avatarUrl, "avatarurl");
    DTO_FIELD(String, role, "role"); // owner, admin, member
    DTO_FIELD(String, joinedAt, "joinedat");
};

#include OATPP_CODEGEN_END(DTO) 
