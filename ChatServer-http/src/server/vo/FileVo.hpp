#pragma once

#include "global.h"

#include OATPP_CODEGEN_BEGIN(DTO) 

// 文件信息
class FileInfoVO : public oatpp::DTO {
    DTO_INIT(FileInfoVO, DTO)
    DTO_FIELD(Int64, id, "id");
    DTO_FIELD(String, fileName, "fileName");
    DTO_FIELD(String, fileType, "fileType");
    DTO_FIELD(String, mimeType, "mimeType");
    DTO_FIELD(Int64, fileSize, "fileSize");
    DTO_FIELD(String, fileUrl, "fileUrl");
    DTO_FIELD(Int64, uploaderId, "uploaderId");
    DTO_FIELD(String, created_url, "created_url");
    DTO_FIELD(String, uploadTime, "uploadTime");
};


// 文件统计信息
class FileStatisticsVO : public oatpp::DTO {
    DTO_INIT(FileStatisticsVO, DTO)
    DTO_FIELD(Int64, totalFiles, "totalFiles");
    DTO_FIELD(Int64, totalSize, "totalSize");
    DTO_FIELD(oatpp::Fields<Int64>, typeCounts, "typeCounts");
};

#include OATPP_CODEGEN_END(DTO) 
