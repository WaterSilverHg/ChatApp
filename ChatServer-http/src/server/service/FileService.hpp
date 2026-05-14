#pragma once

#include "global.h"
#include"../dto/GeneralDto.hpp"
#include "../dto/FileDto.hpp"
#include "../vo/FileVo.hpp"
#include "../postgresql/AppClient.hpp"
#include "../../cos/AppCos.hpp"

class FileService {
private:
    std::shared_ptr<AppClient> m_appClient;
    std::shared_ptr<AppCos> m_cosClient;
    using Status = oatpp::web::protocol::http::Status;
    
    std::string generateCosObjectName(const oatpp::String& fileType,const oatpp::String& userUuid) {
        // 生成时间戳（毫秒级，避免同一秒内冲突）
        auto now = std::chrono::system_clock::now();
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        std::string timestamp = std::to_string(ms);

        // 生成随机数
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(1000, 9999);
        int randNum = dis(gen);

        // 组合最终对象名：uploads/{userUuid}/{timestamp}_{randNum}
        return fileType+"/" + userUuid + "/" + timestamp + "_" + std::to_string(randNum);
    }
public:
    FileService(const std::shared_ptr<AppClient>& appClient, 
                const std::shared_ptr<AppCos>& cosClient) 
        : m_appClient(appClient), m_cosClient(cosClient) {}

    // 创建文件上传记录（状态为 uploading），返回文件 uuid
    oatpp::Object<FileInfoVO> createUploadRecord(const oatpp::String& currentUserUuId, const oatpp::Object<UploadFileRequestDTO>& request) {
        OATPP_ASSERT_HTTP(currentUserUuId && !currentUserUuId->empty(), Status::CODE_400, "用户ID不能为空");
        
        auto userCheck = m_appClient->getUserIdByUuid(currentUserUuId);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(userCheck->isSuccess(), Status::CODE_500, userCheck->getErrorMessage());
        OATPP_ASSERT_HTTP(userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
        #else
        OATPP_ASSERT_HTTP(userCheck->isSuccess() && userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
        #endif
        auto user_id = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;
        
        oatpp::String cosObjectName = generateCosObjectName(request->fileType,currentUserUuId);
        
        auto result = m_appClient->createFileRecord(cosObjectName, request->fileSize, request->fileType, request->mimeType, user_id);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, result->getErrorMessage());
        OATPP_ASSERT_HTTP(result->hasMoreToFetch(), Status::CODE_400, "创建文件记录失败");
        #else
        OATPP_ASSERT_HTTP(result->isSuccess() && result->hasMoreToFetch(), Status::CODE_400, "创建文件记录失败");
        #endif
        
        return result->fetch<oatpp::Vector<oatpp::Object<FileInfoVO>>>()[0];
    }

    // 上传文件数据到 COS 并更新状态为 completed
    oatpp::Object<FileInfoVO> uploadFileData(const oatpp::String& sender,const oatpp::String& fileUuid, const std::string& fileData) {
        OATPP_ASSERT_HTTP(fileUuid && !fileUuid->empty(), Status::CODE_400, "文件ID不能为空");
        OATPP_ASSERT_HTTP(!fileData.empty(), Status::CODE_400, "文件数据不能为空");
        
        // 获取文件信息
        auto fileIdResult = m_appClient->getFileIdByUuid(fileUuid);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(fileIdResult->isSuccess(), Status::CODE_500, fileIdResult->getErrorMessage());
        OATPP_ASSERT_HTTP(fileIdResult->hasMoreToFetch(), Status::CODE_404, "文件不存在");
        #else
        OATPP_ASSERT_HTTP(fileIdResult->isSuccess() && fileIdResult->hasMoreToFetch(), Status::CODE_404, "文件不存在");
        #endif
        auto fileId = fileIdResult->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;
        
        // 获取文件详情（获取 cosObjectName）
        auto fileDetailResult = m_appClient->getFileDetailById(fileId);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(fileDetailResult->isSuccess(), Status::CODE_500, fileDetailResult->getErrorMessage());
        OATPP_ASSERT_HTTP(fileDetailResult->hasMoreToFetch(), Status::CODE_404, "文件不存在");
        #else
        OATPP_ASSERT_HTTP(fileDetailResult->isSuccess() && fileDetailResult->hasMoreToFetch(), Status::CODE_404, "文件不存在");
        #endif
        auto fileDetailInfo = fileDetailResult->fetch<oatpp::Vector<oatpp::Object<FileDetailInfoVO>>>()[0];
        
        // 上传到 COS
        std::string fileUrl;
        try {
            fileUrl = m_cosClient->uploadFromStream(fileData, fileDetailInfo->fileName, fileDetailInfo->mimeType);
        } catch (const std::exception& e) {
            auto updateResult = m_appClient->updateFileStatus("failed", oatpp::String(""), fileId);
#ifdef SQLCHECK
            OATPP_ASSERT_HTTP(false, Status::CODE_500, "上传至COS失败: " + oatpp::String(e.what()));
#else
            OATPP_ASSERT_HTTP(updateResult->isSuccess() && updateResult->hasMoreToFetch(), Status::CODE_400, "更新文件状态失败");
#endif
        }
        
        // 更新文件状态为 completed
        auto updateResult = m_appClient->updateFileStatus("completed", oatpp::String(fileUrl), fileId);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(updateResult->isSuccess(), Status::CODE_500, updateResult->getErrorMessage());
        OATPP_ASSERT_HTTP(updateResult->hasMoreToFetch(), Status::CODE_400, "更新文件状态失败");
        #else
        OATPP_ASSERT_HTTP(updateResult->isSuccess() && updateResult->hasMoreToFetch(), Status::CODE_400, "更新文件状态失败");
        #endif
        
        // 如果是头像类型，更新用户头像
        if (fileDetailInfo->fileType && fileDetailInfo->fileType == "avatar") {
            // 将 uploaderUuid 转换为 userId
            auto userIdResult = m_appClient->getUserIdByUuid(fileDetailInfo->uploaderUuid);
            #ifdef SQLCHECK
            OATPP_ASSERT_HTTP(userIdResult->isSuccess(), Status::CODE_500, userIdResult->getErrorMessage());
            OATPP_ASSERT_HTTP(userIdResult->hasMoreToFetch(), Status::CODE_404, "上传者不存在");
            #else
            OATPP_ASSERT_HTTP(userIdResult->isSuccess() && userIdResult->hasMoreToFetch(), Status::CODE_404, "上传者不存在");
            #endif
            auto userId = userIdResult->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;
            
            // 更新用户头像
            auto updateAvatarResult = m_appClient->updateUserAvatar(userId, oatpp::String(fileUrl));
            #ifdef SQLCHECK
            OATPP_ASSERT_HTTP(updateAvatarResult->isSuccess(), Status::CODE_500, updateAvatarResult->getErrorMessage());
            #else
            OATPP_ASSERT_HTTP(updateAvatarResult->isSuccess(), Status::CODE_500, "更新用户头像失败");
            #endif
        }
        
        return updateResult->fetch<oatpp::Vector<oatpp::Object<FileInfoVO>>>()[0];
    }

    // 完整的文件上传流程（一步完成）
    //oatpp::Object<FileInfoVO> uploadFile(const oatpp::String& currentUserUuId, const oatpp::String& fileName, 
    //                                     const std::string& fileData, const oatpp::String& mimeType, 
    //                                     const oatpp::String& fileType, v_int64 fileSize) {
    //    OATPP_ASSERT_HTTP(currentUserUuId && !currentUserUuId->empty(), Status::CODE_400, "用户ID不能为空");
    //    OATPP_ASSERT_HTTP(!fileData.empty(), Status::CODE_400, "文件数据不能为空");
    //    
    //    // 获取用户ID
    //    auto userCheck = m_appClient->getUserIdByUuid(currentUserUuId);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(userCheck->isSuccess(), Status::CODE_500, userCheck->getErrorMessage());
    //    OATPP_ASSERT_HTTP(userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
    //    #else
    //    OATPP_ASSERT_HTTP(userCheck->isSuccess() && userCheck->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
    //    #endif
    //    auto userId = userCheck->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;
    //    
    //    // 生成 COS 对象名
    //    std::string cosObjectName = generateCosObjectName(fileType, currentUserUuId);
    //    
    //    // 创建数据库记录（状态 uploading）
    //    auto createResult = m_appClient->createFileRecord(cosObjectName, fileSize, fileType, mimeType, userId);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(createResult->isSuccess(), Status::CODE_500, createResult->getErrorMessage());
    //    OATPP_ASSERT_HTTP(createResult->hasMoreToFetch(), Status::CODE_500, "创建文件记录失败");
    //    #else
    //    OATPP_ASSERT_HTTP(createResult->isSuccess() && createResult->hasMoreToFetch(), Status::CODE_500, "创建文件记录失败");
    //    #endif
    //    auto fileInfo = createResult->fetch<oatpp::Vector<oatpp::Object<FileInfoVO>>>()[0];
    //    auto fileId = fileInfo->uuid;
    //    
    //    // 上传到 COS
    //    std::string fileUrl;
    //    try {
    //        fileUrl = m_cosClient->uploadFromStream(fileData, cosObjectName, mimeType);
    //    } catch (const std::exception& e) {
    //        m_appClient->updateFileStatus("failed", oatpp::String(""), fileId);
    //        OATPP_ASSERT_HTTP(false, Status::CODE_500, "上传至COS失败: " + oatpp::String(e.what()));
    //    }
    //    
    //    // 更新文件状态为 completed
    //    auto updateResult = m_appClient->updateFileStatus("completed", oatpp::String(fileUrl), fileId);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(updateResult->isSuccess(), Status::CODE_500, updateResult->getErrorMessage());
    //    OATPP_ASSERT_HTTP(updateResult->hasMoreToFetch(), Status::CODE_500, "更新文件状态失败");
    //    #else
    //    OATPP_ASSERT_HTTP(updateResult->isSuccess() && updateResult->hasMoreToFetch(), Status::CODE_500, "更新文件状态失败");
    //    #endif
    //    
    //    return updateResult->fetch<oatpp::Vector<oatpp::Object<FileInfoVO>>>()[0];
    //}

    //oatpp::Object<FileInfoVO> uploadFileByPath(const oatpp::String& currentUserIdHeader, 
    //                                           const oatpp::String& fileName, 
    //                                           const oatpp::String& localFilePath,
    //                                           const oatpp::String& mimeType,
    //                                           const oatpp::String& fileType) {
    //    std::string cosObjectName = generateCosObjectName(currentUserIdHeader, fileName);
    //    std::string localPath = oatpp::utils::conversion::charSequenceToStdString(localFilePath);
    //    
    //    std::string fileUrl = m_cosClient->uploadFile(localPath, cosObjectName);
    //    
    //    auto fileSize = std::filesystem::file_size(localPath);
    //    
    //    auto result = m_appClient->uploadFile(fileName, (oatpp::Int64)fileSize, mimeType, 
    //        oatpp::String(fileUrl), fileType, std::stoll(currentUserIdHeader->c_str()));
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, result->getErrorMessage());
    //    OATPP_ASSERT_HTTP(result->hasMoreToFetch(), Status::CODE_400, "文件上传失败");
    //    #else
    //    OATPP_ASSERT_HTTP(result->isSuccess() && result->hasMoreToFetch(), Status::CODE_400, "文件上传失败");
    //    #endif

    //    auto file = result->fetch<oatpp::Vector<oatpp::Object<FileInfoVO>>>()[0];
    //    file->fileName = fileName;
    //    file->fileSize = (oatpp::Int64)fileSize;
    //    file->mimeType = mimeType;
    //    file->fileUrl = oatpp::String(fileUrl);
    //    file->fileType = fileType;
    //    return file;
    //}

    //oatpp::Vector<oatpp::Object<FileInfoVO>> getFileList(const oatpp::String& currentUserIdHeader) {
    //    auto result = m_appClient->getFileList(std::stoll(currentUserIdHeader->c_str()));
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, result->getErrorMessage());
    //    #else
    //    OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, "获取文件列表失败");
    //    #endif
    //    return result->fetch<oatpp::Vector<oatpp::Object<FileInfoVO>>>();
    //}

    oatpp::Object<FileDetailInfoVO> getFileDetail(oatpp::String fileUuid) {

        OATPP_ASSERT_HTTP(fileUuid && !fileUuid->empty(), Status::CODE_400, "用户ID不能为空");
        auto result = m_appClient->getFileIdByUuid(fileUuid);
#ifdef SQLCHECK
        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, result->getErrorMessage());
        OATPP_ASSERT_HTTP(result->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
#else
        OATPP_ASSERT_HTTP(result->isSuccess() && result->hasMoreToFetch(), Status::CODE_401, "用户不存在或已失效");
#endif
        auto fileId = result->fetch<oatpp::Vector<oatpp::Object<IdDTO>>>()[0]->id;
        auto detailResult = m_appClient->getFileDetailById(fileId);
        #ifdef SQLCHECK
        OATPP_ASSERT_HTTP(detailResult->isSuccess(), Status::CODE_500, detailResult->getErrorMessage());
        OATPP_ASSERT_HTTP(detailResult->hasMoreToFetch(), Status::CODE_404, "文件不存在");
        #else
        OATPP_ASSERT_HTTP(detailResult->isSuccess() && detailResult->hasMoreToFetch(), Status::CODE_404, "文件不存在");
        #endif
        return detailResult->fetch<oatpp::Vector<oatpp::Object<FileDetailInfoVO>>>()[0];
    }

    //oatpp::Boolean deleteFile(oatpp::Int64 fileId, const oatpp::String& currentUserIdHeader) {
    //    auto result = m_appClient->getFileDetail(fileId);
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, result->getErrorMessage());
    //    OATPP_ASSERT_HTTP(result->hasMoreToFetch(), Status::CODE_404, "文件不存在");
    //    #else
    //    OATPP_ASSERT_HTTP(result->isSuccess() && result->hasMoreToFetch(), Status::CODE_404, "文件不存在");
    //    #endif
    //    
    //    auto fileInfo = result->fetch<oatpp::Vector<oatpp::Object<FileInfoVO>>>()[0];
    //    std::string fileUrl = oatpp::utils::conversion::charSequenceToStdString(fileInfo->fileUrl);
    //    
    //    size_t pos = fileUrl.find("/" + m_cosClient->getBucketName() + ".cos.");
    //    if (pos != std::string::npos) {
    //        std::string cosObjectName = fileUrl.substr(pos + m_cosClient->getBucketName().size() + 6);
    //        m_cosClient->deleteFile(cosObjectName);
    //    }

    //    result = m_appClient->deleteFile(fileId, std::stoll(currentUserIdHeader->c_str()));
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, result->getErrorMessage());
    //    #else
    //    OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_400, "删除文件失败");
    //    #endif
    //    return true;
    //}

    //oatpp::Boolean batchDeleteFiles(const oatpp::String& currentUserIdHeader, const oatpp::Object<BatchDeleteFilesRequestDTO>& request) {
    //    for (const auto& fileId : *request->fileIds) {
    //        auto result = m_appClient->getFileDetail(fileId);
    //        #ifdef SQLCHECK
    //        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, result->getErrorMessage());
    //        OATPP_ASSERT_HTTP(result->hasMoreToFetch(), Status::CODE_404, "文件不存在");
    //        #else
    //        OATPP_ASSERT_HTTP(result->isSuccess() && result->hasMoreToFetch(), Status::CODE_404, "文件不存在");
    //        #endif
    //        
    //        auto fileInfo = result->fetch<oatpp::Vector<oatpp::Object<FileInfoVO>>>()[0];
    //        std::string fileUrl = oatpp::utils::conversion::charSequenceToStdString(fileInfo->fileUrl);
    //        
    //        size_t pos = fileUrl.find("/" + m_cosClient->getBucketName() + ".cos.");
    //        if (pos != std::string::npos) {
    //            std::string cosObjectName = fileUrl.substr(pos + m_cosClient->getBucketName().size() + 6);
    //            m_cosClient->deleteFile(cosObjectName);
    //        }

    //        result = m_appClient->deleteFile(fileId, std::stoll(currentUserIdHeader->c_str()));
    //        #ifdef SQLCHECK
    //        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, result->getErrorMessage());
    //        #else
    //        OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_400, "删除文件失败");
    //        #endif
    //    }
    //    return true;
    //}

    //oatpp::Vector<oatpp::Object<FileInfoVO>> searchFiles(const oatpp::String& keyword, const oatpp::String& currentUserIdHeader) {
    //    auto result = m_appClient->searchFiles(keyword, std::stoll(currentUserIdHeader->c_str()));
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, result->getErrorMessage());
    //    #else
    //    OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, "搜索文件失败");
    //    #endif
    //    return result->fetch<oatpp::Vector<oatpp::Object<FileInfoVO>>>();
    //}

    //oatpp::Object<FileStatisticsVO> getFileStatistics(const oatpp::String& currentUserIdHeader) {
    //    auto result = m_appClient->getFileStatistics(std::stoll(currentUserIdHeader->c_str()));
    //    #ifdef SQLCHECK
    //    OATPP_ASSERT_HTTP(result->isSuccess(), Status::CODE_500, result->getErrorMessage());
    //    OATPP_ASSERT_HTTP(result->hasMoreToFetch(), Status::CODE_404, "文件统计信息不存在");
    //    #else
    //    OATPP_ASSERT_HTTP(result->isSuccess() && result->hasMoreToFetch(), Status::CODE_404, "文件统计信息不存在");
    //    #endif
    //    return result->fetch<oatpp::Vector<oatpp::Object<FileStatisticsVO>>>()[0];
    //}
};
