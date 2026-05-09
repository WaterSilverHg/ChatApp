#pragma once

#include "global.h"

#include OATPP_CODEGEN_BEGIN(DTO)

//class WebSocketMessageDTO : public oatpp::DTO {
//    DTO_INIT(WebSocketMessageDTO, DTO)
//
//    DTO_FIELD(String, type, "type");
//    DTO_FIELD(String, fromUserId, "fromuseruuid");
//    DTO_FIELD(String, toUserId, "touseruuid");
//    DTO_FIELD(String, toGroupId, "togroupuuid");
//    DTO_FIELD(String, content, "content");
//    DTO_FIELD(String, messageType, "messagetype");
//    DTO_FIELD(String, messageId, "messageid");
//    DTO_FIELD(String, clientMsgId, "clientmsgid");
//    DTO_FIELD(String, timestamp, "timestamp");
//    DTO_FIELD(String, status, "status");
//
//    DTO_FIELD(String, fileUrl, "fileurl");
//    DTO_FIELD(Int64, fileSize, "filesize");
//    DTO_FIELD(String, fileName, "filename");
//    DTO_FIELD(String, mimeType, "mimetype");
//
//    DTO_FIELD(String, roomId, "roomid");
//    DTO_FIELD(String, userName, "username");
//    DTO_FIELD(String, avatarUrl, "avatarurl");
//    DTO_FIELD(Boolean, isOnline, "isonline");
//    DTO_FIELD(String, userStatus, "userstatus");
//
//    DTO_FIELD(Int32, unreadCount, "unreadcount");
//    DTO_FIELD(Int32, onlineCount, "onlinecount");
//};

class WebSocketResponseDTO : public oatpp::DTO {
    DTO_INIT(WebSocketResponseDTO, DTO)

    DTO_FIELD(String, type, "type");
    DTO_FIELD(String, content, "content");
    //DTO_FIELD(String, messageId, "messageid");
    //DTO_FIELD(String, fromUserId, "fromuserid");
    //DTO_FIELD(String, fromUserName, "fromusername");
    //DTO_FIELD(String, fromUserAvatar, "fromuseravatar");
    //DTO_FIELD(String, toUserId, "touserid");
    //DTO_FIELD(String, toGroupId, "togroupid");
    //DTO_FIELD(String, timestamp, "timestamp");
    DTO_FIELD(Boolean, success, "success");
    //DTO_FIELD(String, errorMessage, "errormessage");

    //DTO_FIELD(Object<WebSocketMessageDTO>, data, "data");
};

//class TypingStatusDTO : public oatpp::DTO {
//    DTO_INIT(TypingStatusDTO, DTO)
//    DTO_FIELD(String, fromUserId, "fromuserid");
//    DTO_FIELD(String, toUserId, "touserid");
//    DTO_FIELD(String, toGroupId, "togroupid");
//    DTO_FIELD(String, status, "status");
//};

//class ReadReceiptDTO : public oatpp::DTO {
//    DTO_INIT(ReadReceiptDTO, DTO)
//    DTO_FIELD(String, messageId, "messageid");
//    DTO_FIELD(String, fromUserId, "fromuserid");
//    DTO_FIELD(String, toUserId, "touserid");
//    DTO_FIELD(String, readAt, "readat");
//};
//
//class ResumeRequestDTO : public oatpp::DTO {
//    DTO_INIT(ResumeRequestDTO, DTO)
//    DTO_FIELD(String, lastMsgId, "lastmsgid");
//    DTO_FIELD(String, lastMessageTime, "lastmessagetime");
//};

// 批量删除通知请求
class BatchDeleteNotificationsRequest : public oatpp::DTO {
    DTO_INIT(BatchDeleteNotificationsRequest, DTO)
    DTO_FIELD(oatpp::Vector<Int64>, notificationIds, "notificationids");
};

#include OATPP_CODEGEN_END(DTO)