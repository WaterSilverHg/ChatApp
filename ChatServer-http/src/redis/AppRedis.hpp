#pragma once
#include"../global.h"

class AppRedis{
	sw::redis::Redis* handle;
public:
	AppRedis() = default;
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

	// 创建令牌
	bool createToken(const std::string& userUuid, const std::string& token, int expireSeconds = 3600) {
		try {
			std::string key = "token:" + userUuid;
			handle->set(key, token);
			handle->expire(key, std::chrono::seconds(expireSeconds));
			return true;
		} catch (const sw::redis::Error& e) {
			std::cerr << "Error creating token: " << e.what() << std::endl;
			return false;
		}
	}

	// 验证令牌
	bool validateToken(const std::string& userUuid, const std::string& token) {
		try {
			std::string key = "token:" + userUuid;
			auto storedToken = handle->get(key);
			return storedToken && *storedToken == token;
		} catch (const sw::redis::Error& e) {
			std::cerr << "Error validating token: " << e.what() << std::endl;
			return false;
		}
	}

	// 删除令牌
	bool deleteToken(const std::string& userUuid) {
		try {
			std::string key = "token:" + userUuid;
			handle->del(key);
			return true;
		} catch (const sw::redis::Error& e) {
			std::cerr << "Error deleting token: " << e.what() << std::endl;
			return false;
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