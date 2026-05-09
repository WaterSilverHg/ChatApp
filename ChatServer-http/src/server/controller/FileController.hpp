#pragma once

#include "global.h"
#include "../dto/FileDto.hpp"
#include "../vo/FileVO.hpp"
#include "../service/FileService.hpp"

#include OATPP_CODEGEN_BEGIN(ApiController) 

class FileController : public oatpp::web::server::api::ApiController {
private:
    std::shared_ptr<FileService> m_fileService;

public:
    FileController(OATPP_COMPONENT(std::shared_ptr<ObjectMapper>, objectMapper),
                   OATPP_COMPONENT(std::shared_ptr<AppClient>, appClient))
        : oatpp::web::server::api::ApiController(objectMapper),
          m_fileService(std::make_shared<FileService>(appClient)) {}

    static std::shared_ptr<FileController> createShared(
        OATPP_COMPONENT(std::shared_ptr<ObjectMapper>, objectMapper),
        OATPP_COMPONENT(std::shared_ptr<AppClient>, appClient)
    ) {
        return std::make_shared<FileController>(objectMapper, appClient);
    }

    ENDPOINT_INFO(uploadFile) {
        info->summary = "上传文件";
        info->description = "上传文件并保存文件信息";
        info->addResponse<Object<FileInfoVO>>(Status::CODE_200, "application/json", "上传成功");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_401, "application/json", "用户不存在或已失效");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_400, "application/json", "文件上传失败");
        info->addConsumes<Object<UploadFileRequestDTO>>("application/json");
    }
    ENDPOINT("POST", "/api/files/upload", uploadFile, 
        BODY_DTO(Object<UploadFileRequestDTO>, request),
        HEADER(oatpp::String, userUuid, "userUuid")) {
        return createDtoResponse(Status::CODE_200, m_fileService->uploadFile(userUuid, request));
    }

    ENDPOINT_INFO(getFileList) {
        info->summary = "获取文件列表";
        info->description = "获取当前用户的文件列表";
        info->addResponse<oatpp::Vector<Object<FileInfoVO>>>(Status::CODE_200, "application/json", "获取成功");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_401, "application/json", "用户不存在或已失效");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_500, "application/json", "获取文件列表失败");
    }
    ENDPOINT("GET", "/api/files", getFileList,
        HEADER(String, userUuid, "userUuid")) {
        return createDtoResponse(Status::CODE_200, m_fileService->getFileList(userUuid));
    }

    ENDPOINT_INFO(getFileDetail) {
        info->summary = "获取文件详情";
        info->description = "获取指定文件的详细信息";
        info->addResponse<Object<FileInfoVO>>(Status::CODE_200, "application/json", "获取成功");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_401, "application/json", "用户不存在或已失效");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_404, "application/json", "文件不存在");
    }
    ENDPOINT("GET", "/api/files/{fileUuid}", getFileDetail, 
        PATH(String, fileUuid, "fileUuid"),
        HEADER(String, userUuid, "userUuid")) {
        return createDtoResponse(Status::CODE_200, m_fileService->getFileDetail(fileUuid, userUuid));
    }

    ENDPOINT_INFO(deleteFile) {
        info->summary = "删除文件";
        info->description = "删除指定的文件";
        info->addResponse<Boolean>(Status::CODE_200, "application/json", "删除成功");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_401, "application/json", "用户不存在或已失效");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_400, "application/json", "删除文件失败");
    }
    ENDPOINT("DELETE", "/api/files/{fileUuid}", deleteFile, 
        PATH(String, fileUuid, "fileUuid"),
        HEADER(String, userUuid, "userUuid")) {
        return createDtoResponse(Status::CODE_200, m_fileService->deleteFile(fileUuid, userUuid));
    }

    ENDPOINT_INFO(batchDeleteFiles) {
        info->summary = "批量删除文件";
        info->description = "批量删除多个文件";
        info->addResponse<Boolean>(Status::CODE_200, "application/json", "删除成功");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_401, "application/json", "用户不存在或已失效");
        info->addConsumes<Object<BatchDeleteFilesRequestDTO>>("application/json");
    }
    ENDPOINT("POST", "/api/files/batch-delete", batchDeleteFiles, 
        BODY_DTO(Object<BatchDeleteFilesRequestDTO>, request),
        HEADER(String, userUuid, "userUuid")) {
        return createDtoResponse(Status::CODE_200, m_fileService->batchDeleteFiles(userUuid, request));
    }

    ENDPOINT_INFO(searchFiles) {
        info->summary = "搜索文件";
        info->description = "根据关键词搜索文件";
        info->addResponse<oatpp::Vector<Object<FileInfoVO>>>(Status::CODE_200, "application/json", "搜索成功");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_401, "application/json", "用户不存在或已失效");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_500, "application/json", "搜索文件失败");
    }
    ENDPOINT("GET", "/api/files/search", searchFiles, 
        QUERY(String, keyword, "keyword"),
        HEADER(String, userUuid, "userUuid")) {
        return createDtoResponse(Status::CODE_200, m_fileService->searchFiles(keyword, userUuid));
    }

    ENDPOINT_INFO(getFileStatistics) {
        info->summary = "获取文件分类统计";
        info->description = "获取当前用户的文件分类统计信息";
        info->addResponse<Object<FileStatisticsVO>>(Status::CODE_200, "application/json", "获取成功");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_401, "application/json", "用户不存在或已失效");
        info->addResponse<Object<ErrorStatusDto>>(Status::CODE_500, "application/json", "获取文件统计失败");
    }
    ENDPOINT("GET", "/api/files/statistics", getFileStatistics,
        HEADER(String, userUuid, "userUuid")) {
        return createDtoResponse(Status::CODE_200, m_fileService->getFileStatistics(userUuid));
    }
};

#include OATPP_CODEGEN_END(ApiController)
