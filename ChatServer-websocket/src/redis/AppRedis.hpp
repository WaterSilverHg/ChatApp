#pragma once
#include"../global.h"

class AppRedis{
	sw::redis::Redis* handle;
public:
	AppRedis() : handle(nullptr) {}
	~AppRedis() {
		if (handle != nullptr) {
			delete handle;
			handle = nullptr;
		}
	}
	
	// 禁止拷贝构造和赋值
	AppRedis(const AppRedis&) = delete;
	AppRedis& operator=(const AppRedis&) = delete;
	
	// 允许移动构造和赋值
	AppRedis(AppRedis&& other) noexcept : handle(other.handle) {
		other.handle = nullptr;
	}
	AppRedis& operator=(AppRedis&& other) noexcept {
		if (this != &other) {
			delete handle;
			handle = other.handle;
			other.handle = nullptr;
		}
		return *this;
	}
	
	bool initRedis(const char* url) {
		try {
			handle = new sw::redis::Redis(url);
			return true;
		}
		catch (const sw::redis::Error& e) {
			std::cerr << "Error: " << e.what() << std::endl;
			return false;
		}
	}
	sw::redis::Redis* getHandle() {
		return handle;
	}

	// 创建会话（存储 sessionId）
	bool createSession(const std::string& userUuid, const std::string& sessionId, int expireSeconds = 3600) {
		try {
			std::string key = "session:" + userUuid;
			handle->set(key, sessionId);
			handle->expire(key, std::chrono::seconds(expireSeconds));
			return true;
		} catch (const sw::redis::Error& e) {
			std::cerr << "Error creating session: " << e.what() << std::endl;
			return false;
		}
	}

	// 验证会话（检查 sessionId 是否存在）
	bool validateSession(const std::string& userUuid, const std::string& sessionId) {
		try {
			std::string key = "session:" + userUuid;
			auto storedSessionId = handle->get(key);
			return storedSessionId && *storedSessionId == sessionId;
		} catch (const sw::redis::Error& e) {
			std::cerr << "Error validating session: " << e.what() << std::endl;
			return false;
		}
	}

	// 删除会话
	bool deleteSession(const std::string& userUuid) {
		try {
			std::string key = "session:" + userUuid;
			handle->del(key);
			return true;
		} catch (const sw::redis::Error& e) {
			std::cerr << "Error deleting session: " << e.what() << std::endl;
			return false;
		}
	}

	// 获取当前 sessionId
	std::string getSession(const std::string& userUuid) {
		try {
			std::string key = "session:" + userUuid;
			auto sessionId = handle->get(key);
			if (sessionId) {
				return *sessionId;
			}
			return "";
		} catch (const sw::redis::Error& e) {
			std::cerr << "Error getting session: " << e.what() << std::endl;
			return "";
		}
	}

	// 设置验证码
	bool setVerificationCode(const std::string& email, const std::string& code, int expireSeconds = 300) {
		try {
			std::string key = "verification:" + email;
			handle->set(key, code);
			handle->expire(key, std::chrono::seconds(expireSeconds));
			return true;
		} catch (const sw::redis::Error& e) {
			std::cerr << "Error setting verification code: " << e.what() << std::endl;
			return false;
		}
	}

	// 获取验证码
	std::string getVerificationCode(const std::string& email) {
		try {
			std::string key = "verification:" + email;
			auto code = handle->get(key);
			if (code) {
				return *code;
			}
			return "";
		} catch (const sw::redis::Error& e) {
			std::cerr << "Error getting verification code: " << e.what() << std::endl;
			return "";
		}
	}

	// 获取验证码剩余TTL（秒）
	long long getVerificationCodeTTL(const std::string& email) {
		try {
			std::string key = "verification:" + email;
			auto ttl = handle->ttl(key);
			return ttl;
		} catch (const sw::redis::Error& e) {
			std::cerr << "Error getting verification code TTL: " << e.what() << std::endl;
			return -1;
		}
	}

	// 检查验证码是否存在
	bool existsVerificationCode(const std::string& email) {
		try {
			std::string key = "verification:" + email;
			return handle->exists(key);
		} catch (const sw::redis::Error& e) {
			std::cerr << "Error checking verification code existence: " << e.what() << std::endl;
			return false;
		}
	}
};