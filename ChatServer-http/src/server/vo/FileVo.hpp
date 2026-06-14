#pragma once

#include "global.h"

#include OATPP_CODEGEN_BEGIN(DTO) 

// 文件信息
class FileInfoVO : public oatpp::DTO {
    DTO_INIT(FileInfoVO, DTO)
    DTO_FIELD(String, uuid, "uuid");
    DTO_FIELD(String, fileName, "filename");
    DTO_FIELD(String, fileType, "filetype");
    DTO_FIELD(String, mimeType, "mimetype");
    DTO_FIELD(Int64, fileSize, "filesize");
    DTO_FIELD(String, fileUrl, "fileurl");
};

class FileDetailInfoVO : public oatpp::DTO {
    DTO_INIT(FileDetailInfoVO, DTO)
        DTO_FIELD(String, uuid, "uuid");
    DTO_FIELD(String, fileName, "filename");
    DTO_FIELD(String, fileType, "filetype");
    DTO_FIELD(String, mimeType, "mimetype");
    DTO_FIELD(Int64, fileSize, "filesize");
    DTO_FIELD(String, fileUrl, "fileurl");
    DTO_FIELD(String, uploaderUuid, "uploaderuuid");
    DTO_FIELD(String, uploadtime, "uploadtime");
    DTO_FIELD(String, targetUuid, "targetuuid");  // 关联目标UUID
};


// 文件统计信息
class FileStatisticsVO : public oatpp::DTO {
    DTO_INIT(FileStatisticsVO, DTO)
    DTO_FIELD(Int64, totalFiles, "totalFiles");
    DTO_FIELD(Int64, totalSize, "totalSize");
    DTO_FIELD(oatpp::Fields<Int64>, typeCounts, "typeCounts");
};

#include OATPP_CODEGEN_END(DTO) 
