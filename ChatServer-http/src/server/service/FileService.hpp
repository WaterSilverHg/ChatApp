#pragma once

#include "global.h"
#include "../dto/FileDto.hpp"
#include "../vo/FileVo.hpp"
#include "../postgresql/AppClient.hpp"

class FileService {
private:
    std::shared_ptr<AppClient> m_appClient;
    using Status = oatpp::web::protocol::http::Status;
public:
    FileService(const std::shared_ptr<AppClient>& appClient) : m_appClient(appClient) {}

    oatpp::Object<FileInfoVO> uploadFile(const oatpp::String& currentUserIdHeader, const oatpp::Object<UploadFileRequestDTO>& request) {
        auto result = m_appClient->uploadFile(request->fileName, request->fileSize, request->mimeType, request->fileUrl, request->fileType, std::stoll(currentUserIdHeader->c_str()));
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, result->getErrorMessage());
        OATPP_ASSERT_HTTP(result->hasMoreToFetch(), Status::CODE_400, "文件上传失败");
        #else
        OATPP_ASSERT_HTTP(result->isSuccess() && result->hasMoreToFetch(), Status::CODE_400, "文件上传失败");
        #endif

        auto file = result->fetch<oatpp::Vector<oatpp::Object<FileInfoVO>>>()[0];
        file->fileName = request->fileName;
        file->fileSize = request->fileSize;
        file->mimeType = request->mimeType;
        file->fileUrl = request->fileUrl;
        file->fileType = request->fileType;
        return file;
    }

    oatpp::Vector<oatpp::Object<FileInfoVO>> getFileList(const oatpp::String& currentUserIdHeader) {
        auto result = m_appClient->getFileList(std::stoll(currentUserIdHeader->c_str()));
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, result->getErrorMessage());
        #else
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, "获取文件列表失败");
        #endif
        return result->fetch<oatpp::Vector<oatpp::Object<FileInfoVO>>>();
    }

    oatpp::Object<FileInfoVO> getFileDetail(oatpp::Int64 fileId, const oatpp::String& currentUserIdHeader) {
        auto result = m_appClient->getFileDetail(fileId);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, result->getErrorMessage());
        OATPP_ASSERT_HTTP(result->hasMoreToFetch(), Status::CODE_404, "文件不存在");
        #else
        OATPP_ASSERT_HTTP(result->isSuccess() && result->hasMoreToFetch(), Status::CODE_404, "文件不存在");
        #endif
        return result->fetch<oatpp::Vector<oatpp::Object<FileInfoVO>>>()[0];
    }

    oatpp::Boolean deleteFile(oatpp::Int64 fileId, const oatpp::String& currentUserIdHeader) {
        auto result = m_appClient->deleteFile(fileId, std::stoll(currentUserIdHeader->c_str()));
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, result->getErrorMessage());
        #else
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_400, "删除文件失败");
        #endif
        return true;
    }

    oatpp::Boolean batchDeleteFiles(const oatpp::String& currentUserIdHeader, const oatpp::Object<BatchDeleteFilesRequestDTO>& request) {
        for (const auto& fileId : *request->fileIds) {
            auto result = m_appClient->deleteFile(fileId, std::stoll(currentUserIdHeader->c_str()));
            #ifdef SQLCHECK
            OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, result->getErrorMessage());
            #else
            OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_400, "删除文件失败");
            #endif
        }
        return true;
    }

    oatpp::Vector<oatpp::Object<FileInfoVO>> searchFiles(const oatpp::String& keyword, const oatpp::String& currentUserIdHeader) {
        auto result = m_appClient->searchFiles(keyword, std::stoll(currentUserIdHeader->c_str()));
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, result->getErrorMessage());
        #else
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, "搜索文件失败");
        #endif
        return result->fetch<oatpp::Vector<oatpp::Object<FileInfoVO>>>();
    }

    oatpp::Object<FileStatisticsVO> getFileStatistics(const oatpp::String& currentUserIdHeader) {
        auto result = m_appClient->getFileStatistics(std::stoll(currentUserIdHeader->c_str()));
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, result->getErrorMessage());
        OATPP_ASSERT_HTTP(result->hasMoreToFetch(), Status::CODE_404, "文件统计信息不存在");
        #else
        OATPP_ASSERT_HTTP(result->isSuccess() && result->hasMoreToFetch(), Status::CODE_404, "文件统计信息不存在");
        #endif
        return result->fetch<oatpp::Vector<oatpp::Object<FileStatisticsVO>>>()[0];
    }
};
