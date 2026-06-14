#pragma once

#include "global.h"

#include OATPP_CODEGEN_BEGIN(DTO) 

// 文件信息
class FileInfoDTO : public oatpp::DTO {
    DTO_INIT(FileInfoDTO, DTO)
    DTO_FIELD(Int64, id, "id");
    DTO_FIELD(String, fileName, "filename");
    DTO_FIELD(String, fileType, "filetype");
    DTO_FIELD(String, mimeType, "mimetype");
    DTO_FIELD(Int64, fileSize, "filesize");
    DTO_FIELD(String, fileUrl, "fileurl");
    DTO_FIELD(Int64, uploaderId, "uploaderid");
    DTO_FIELD(String, createdAt, "createdat");
    DTO_FIELD(String, uploadTime, "uploadtime");
};

// 上传文件请求（创建文件记录）
class UploadFileRequestDTO : public oatpp::DTO {
    DTO_INIT(UploadFileRequestDTO, DTO)
        DTO_FIELD(String, fileName, "filename");
    DTO_FIELD(String, fileType, "filetype");
    DTO_FIELD(String, mimeType, "mimetype");
    DTO_FIELD(Int64, fileSize, "filesize");
    DTO_FIELD(String, targetUuid, "targetuuid");  // 关联目标UUID（如群聊UUID）
};

// 批量删除文件请求
class BatchDeleteFilesRequestDTO : public oatpp::DTO {
    DTO_INIT(BatchDeleteFilesRequestDTO, DTO)
    DTO_FIELD(oatpp::Vector<Int64>, fileIds, "fileids");
};

// 文件统计信息
class FileStatisticsDTO : public oatpp::DTO {
    DTO_INIT(FileStatisticsDTO, DTO)
    DTO_FIELD(Int64, totalFiles, "totalfiles");
    DTO_FIELD(Int64, totalSize, "totalsize");
    DTO_FIELD(oatpp::Fields<Int64>, typeCounts, "typecounts");
};

#include OATPP_CODEGEN_END(DTO) 
