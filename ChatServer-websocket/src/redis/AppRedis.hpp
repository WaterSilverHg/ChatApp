#pragma once
#include"../global.h"
#include <functional>
#include <memory>

class AppRedis{
	sw::redis::Redis* handle;
	sw::redis::Subscriber* subscriber;
	std::atomic<bool> subscriberRunning;
	std::thread subscriberThread;
	std::function<void(const std::string&, const std::string&)> messageCallback;
	std::string redisUrl;
public:
	AppRedis() : handle(nullptr), subscriber(nullptr), subscriberRunning(false) {}
	~AppRedis() {
		stopSubscriber();
		if (subscriber != nullptr) {
			delete subscriber;
			subscriber = nullptr;
		}
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
			redisUrl = url;
			handle = new sw::redis::Redis(url);
			OATPP_LOGI("Redis", "Redis connection initialized successfully");
			return true;
		}
		catch (const sw::redis::Error& e) {
			OATPP_LOGE("Redis", "Failed to initialize Redis connection: %s", e.what());
			std::cerr << "Error: Failed to initialize Redis connection: " << e.what() << std::endl;
			std::exit(1);
		}
	}
	sw::redis::Redis* getHandle() {
		return handle;
	}

	// 创建会话（存储sessionId）
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

	// 验证会话（检查sessionId是否存在）
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

	// 短期去重 - 尝试创建去重键
	// 返回 true 表示是第一次请求，返回 false 表示重复请求
	bool tryAcquireDedupLock(const std::string& userUuid, const std::string& action, 
							  const std::string& targetUuid = "", const std::string& contentHash = "",
							  int ttlSeconds = 1) {
		try {
			std::string key = buildDedupKey(userUuid, action, targetUuid, contentHash);
			// SET NX EX: 只有键不存在时才设置，设置成功后自动过期
			auto result = handle->set(key, "1",
                    std::chrono::seconds(ttlSeconds),
                    sw::redis::UpdateType::NOT_EXIST);
			return result;
		} catch (const sw::redis::Error& e) {
			std::cerr << "Error acquiring dedup lock: " << e.what() << std::endl;
			return false;
		}
	}

	// 释放去重键（可选，通常让其自动过期）
	bool releaseDedupLock(const std::string& userUuid, const std::string& action, 
						  const std::string& targetUuid = "", const std::string& contentHash = "") {
		try {
			std::string key = buildDedupKey(userUuid, action, targetUuid, contentHash);
			handle->del(key);
			return true;
		} catch (const sw::redis::Error& e) {
			std::cerr << "Error releasing dedup lock: " << e.what() << std::endl;
			return false;
		}
	}

	// ===== Pub/Sub 方法 =====

	// 发布消息到指定频道
	bool publish(const std::string& channel, const std::string& message) {
		try {
			handle->publish(channel, message);
			return true;
		} catch (const sw::redis::Error& e) {
			OATPP_LOGE("Redis", "Failed to publish to channel %s: %s", channel.c_str(), e.what());
			return false;
		}
	}

	// 设置消息回调函数
	void setMessageCallback(std::function<void(const std::string&, const std::string&)> callback) {
		messageCallback = std::move(callback);
	}

	// 订阅指定频道
	bool subscribe(const std::vector<std::string>& channels) {
		try {
			if (!subscriber) {
				// 通过 Redis 实例的 subscriber() 方法创建
				subscriber = new sw::redis::Subscriber(handle->subscriber());
				
				// 设置消息回调函数
				subscriber->on_message([this](std::string channel, std::string msg) {
					if (messageCallback) {
						messageCallback(channel, msg);
					}
				});
			}
			
			for (const auto& channel : channels) {
				subscriber->subscribe(channel);
				OATPP_LOGI("Redis", "Subscribed to channel: %s", channel.c_str());
			}
			return true;
		} catch (const sw::redis::Error& e) {
			OATPP_LOGE("Redis", "Failed to subscribe: %s", e.what());
			return false;
		}
	}

	// 启动订阅线程
	void startSubscriber() {
		if (subscriberRunning.load()) {
			OATPP_LOGW("Redis", "Subscriber is already running");
			return;
		}
		
		if (!subscriber) {
			OATPP_LOGE("Redis", "Subscriber not initialized, call subscribe() first");
			return;
		}

		subscriberRunning.store(true);
		subscriberThread = std::thread([this]() {
			OATPP_LOGI("Redis", "Subscriber thread started");
			try {
				while (subscriberRunning.load()) {
					subscriber->consume();
				}
			} catch (const sw::redis::Error& e) {
				OATPP_LOGE("Redis", "Subscriber error: %s", e.what());
			} catch (const std::exception& e) {
				OATPP_LOGE("Redis", "Subscriber exception: %s", e.what());
			}
			OATPP_LOGI("Redis", "Subscriber thread stopped");
		});
	}

	// 停止订阅线程
	void stopSubscriber() {
		subscriberRunning.store(false);
		if (subscriber) {
			try {
				subscriber->unsubscribe();
			} catch (const sw::redis::Error& e) {
				OATPP_LOGW("Redis", "Error unsubscribing: %s", e.what());
			}
		}
		if (subscriberThread.joinable()) {
			subscriberThread.join();
		}
	}

private:
	// 构建去重键
	std::string buildDedupKey(const std::string& userUuid, const std::string& action, 
							  const std::string& targetUuid, const std::string& contentHash) {
		std::string key = "dedup:" + userUuid + ":" + action;
		if (!targetUuid.empty()) {
			key += ":" + targetUuid;
		} else {
			key += ":_";
		}
		if (!contentHash.empty()) {
			key += ":" + contentHash;
		}
		return key;
	}
};