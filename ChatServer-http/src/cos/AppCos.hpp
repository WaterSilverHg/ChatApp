#pragma once

#include "global.h"

class AppCos {
private:
    std::shared_ptr<qcloud_cos::CosAPI> m_cosApi;
    std::string m_bucketName;
    std::string m_region;

public:
    AppCos(const std::string& appId, const std::string& secretId, 
           const std::string& secretKey, const std::string& region, 
           const std::string& bucketName) 
        : m_bucketName(bucketName), m_region(region) {
        qcloud_cos::CosConfig config(std::stoull(appId), secretId, secretKey, region);
        //(5000);  // ���ӳ�ʱ 5 ��
        m_cosApi = std::make_shared<qcloud_cos::CosAPI>(config);
    }

    ~AppCos() = default;

    //std::string uploadFile(const std::string& localFilePath, const std::string& cosObjectName) {
    //    qcloud_cos::PutObjectByFileReq req(m_bucketName, cosObjectName, localFilePath);
    //    qcloud_cos::PutObjectByFileResp resp;
    //    
    //    auto result = m_cosApi->PutObject(req, &resp);
    //    if (result.IsSucc()) {
    //        return m_cosApi->GetObjectUrl(m_bucketName, cosObjectName, true, m_region);
    //    }
    //    throw std::runtime_error("COS upload failed: " + result.GetErrorMsg());
    //}

    std::string uploadFromStream(const std::string& data, const std::string& cosObjectName, 
                                 const std::string& contentType = "application/octet-stream") {
        std::istringstream is(data);
        std::string tmp;
        qcloud_cos::PutObjectByStreamReq req(m_bucketName, cosObjectName, is);
        req.SetContentType(contentType);
        //std::string picOpsStr = R"({"is_pic_info": 1, "rules": [{"fileid": "path/to/converted.webp", "rule": "imageMogr2/format/webp"}]})";
        std::string picOpsJson = R"({"is_pic_info":1,"rules":[{"fileid":")" + ("webp/" + cosObjectName + ".webp") + R"(", "rule":"imageMogr2/format/webp"}]})";
        req.AddHeader("Pic-Operations", picOpsJson);
        qcloud_cos::PutObjectByStreamResp resp;
        auto result = m_cosApi->PutObject(req, &resp);
        if (result.IsSucc()) {
            return m_cosApi->GetObjectUrl(m_bucketName, cosObjectName, true/*, m_region*/);
        }
        throw oatpp::web::protocol::http::HttpError(oatpp::web::protocol::http::Status::CODE_400, "COS stream upload failed: " + result.GetErrorMsg());
        //throw std::runtime_error("COS stream upload failed: " + result.GetErrorMsg());
    }

    bool downloadFile(const std::string& cosObjectName, const std::string& localFilePath) {
        qcloud_cos::GetObjectByFileReq req(m_bucketName, cosObjectName, localFilePath);
        //req.SetSendTimeoutInms(60000);
        req.SetRecvTimeoutInms(60000);
        qcloud_cos::GetObjectByFileResp resp;
        
        auto result = m_cosApi->GetObject(req, &resp);
        return result.IsSucc();
    }

    bool deleteFile(const std::string& cosObjectName) {
        qcloud_cos::DeleteObjectReq req(m_bucketName, cosObjectName);
        qcloud_cos::DeleteObjectResp resp;
        
        auto result = m_cosApi->DeleteObject(req, &resp);
        return result.IsSucc();
    }

    bool isObjectExist(const std::string& cosObjectName) {
        return m_cosApi->IsObjectExist(m_bucketName, cosObjectName);
    }

    std::string generatePresignedUrl(const std::string& cosObjectName, uint64_t expireSeconds = 3600) {
        uint64_t currentTime = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        
        return m_cosApi->GeneratePresignedUrl(m_bucketName, cosObjectName, 
                                              currentTime, currentTime + expireSeconds);
    }

    std::string getObjectUrl(const std::string& cosObjectName) {
        return m_cosApi->GetObjectUrl(m_bucketName, cosObjectName, true, m_region);
    }

    std::string getBucketName() const {
        return m_bucketName;
    }

    std::string getRegion() const {
        return m_region;
    }
};