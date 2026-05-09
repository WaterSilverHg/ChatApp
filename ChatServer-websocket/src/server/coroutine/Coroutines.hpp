#pragma once
#include"global.h"


class SendOneMessageCoroutine : public oatpp::async::Coroutine<SendOneMessageCoroutine> {
private:
    std::shared_ptr<oatpp::websocket::AsyncWebSocket> m_socket;
    oatpp::String m_message;
public:
    SendOneMessageCoroutine(
        const std::shared_ptr<oatpp::websocket::AsyncWebSocket>& socket,
        const oatpp::String& message
    )
        : m_socket(socket)
        , m_message(message)
    {}

    Action act() override {
        return m_socket->sendOneFrameTextAsync(m_message).next(finish());
    }
};


class SendCloseCoroutine : public oatpp::async::Coroutine<SendCloseCoroutine> {
private:
    std::shared_ptr<oatpp::websocket::AsyncWebSocket> m_socket;
    oatpp::String m_message;
public:
    SendCloseCoroutine(
        const std::shared_ptr<oatpp::websocket::AsyncWebSocket>& socket
    )
        : m_socket(socket)
    {}

    Action act() override {
        return m_socket->sendCloseAsync().next(finish());
    }
};


class SendPingCoroutine : public oatpp::async::Coroutine<SendPingCoroutine> {
private:
    std::shared_ptr<oatpp::websocket::AsyncWebSocket> m_socket;
    oatpp::String m_message;
public:
    SendPingCoroutine(
        const std::shared_ptr<oatpp::websocket::AsyncWebSocket>& socket
    )
        : m_socket(socket)
    {}

    Action act() override {
        return m_socket->sendPingAsync("heartbeat").next(finish());
    }
};


// ==================== 分发结果回调类型 ====================
// onOffline: 用户离线或推送失败时调用 → 应累加 unread_count
// onDelivered: 推送成功时调用 → 应更新 last_read_message_id
using OnMessageOffline  = std::function<void(const oatpp::String& userUuid)>;
using OnMessageDelivered = std::function<void(const oatpp::String& userUuid)>;


// ==================== 带回调的单帧发送协程 ====================
// 发送成功 → onDelivered(targetUuid)    更新 last_read_message_id
// 发送失败 → onOffline(targetUuid)      累加 unread_count
class SendOneFrameWithCallbackCoroutine : public oatpp::async::Coroutine<SendOneFrameWithCallbackCoroutine> {
private:
    std::shared_ptr<oatpp::websocket::AsyncWebSocket> m_socket;
    oatpp::String m_userUuid;
    oatpp::String m_message;
    OnMessageOffline  m_onOffline;
    OnMessageDelivered m_onDelivered;
    bool m_sent;
public:
    SendOneFrameWithCallbackCoroutine(
        const std::shared_ptr<oatpp::websocket::AsyncWebSocket>& socket,
        const oatpp::String& userUuid,
        const oatpp::String& message,
        OnMessageOffline  onOffline,
        OnMessageDelivered onDelivered
    )
        : m_socket(socket)
        , m_userUuid(userUuid)
        , m_message(message)
        , m_onOffline(onOffline)
        , m_onDelivered(onDelivered)
        , m_sent(false)
    {}

    Action act() override {
        if (!m_sent) {
            m_sent = true;
            // 发起异步发送，完成后回到 act() 执行成功回调
            return m_socket->sendOneFrameTextAsync(m_message).next(
                Action::createActionByType(Action::TYPE_REPEAT)
            );
        }
        // 发送成功
        if (m_onDelivered) {
            m_onDelivered(m_userUuid);
        }
        return finish();
    }

    Action handleError(oatpp::async::Error* error) override {
        // 发送失败 → 按离线处理
        if (m_onOffline) {
            m_onOffline(m_userUuid);
        }
        return finish();
    }
};


// ==================== 批量消息分发协程 ====================
// 离线用户 → onOffline (累加 unread_count)
// 在线用户 → 各自启动 SendOneFrameWithCallbackCoroutine 异步推送
// 所有操作 fire-and-forget，协程立即结束
class BatchSendMessageCoroutine : public oatpp::async::Coroutine<BatchSendMessageCoroutine> {
private:
    std::shared_ptr<oatpp::async::Executor> m_executor;
    std::vector<std::pair<oatpp::String, std::shared_ptr<oatpp::websocket::AsyncWebSocket>>> m_onlineTargets;
    std::vector<oatpp::String> m_offlineTargets;
    oatpp::String m_message;
    OnMessageOffline  m_onOffline;
    OnMessageDelivered m_onDelivered;
public:
    BatchSendMessageCoroutine(
        const std::shared_ptr<oatpp::async::Executor>& executor,
        std::vector<std::pair<oatpp::String, std::shared_ptr<oatpp::websocket::AsyncWebSocket>>>&& onlineTargets,
        std::vector<oatpp::String>&& offlineTargets,
        const oatpp::String& message,
        OnMessageOffline  onOffline,
        OnMessageDelivered onDelivered
    )
        : m_executor(executor)
        , m_onlineTargets(std::move(onlineTargets))
        , m_offlineTargets(std::move(offlineTargets))
        , m_message(message)
        , m_onOffline(onOffline)
        , m_onDelivered(onDelivered)
    {}

    Action act() override {
        for (auto& uuid : m_offlineTargets) {
            if (m_onOffline) {
                m_onOffline(uuid);
            }
        }

        for (auto& pair : m_onlineTargets) {
            m_executor->execute<SendOneFrameWithCallbackCoroutine>(
                pair.second, pair.first, m_message, m_onOffline, m_onDelivered
            );
        }

        return finish();
    }
};
