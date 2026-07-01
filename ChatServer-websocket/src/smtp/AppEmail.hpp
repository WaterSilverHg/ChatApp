#pragma once
#include"global.h"

class AppEmail{
    mailio::message msg_template;
    mailio::smtps conn;
    AppEmail() = delete;
public:
    AppEmail(const std::string& smtp_host,const unsigned smtp_port,const std::string &sender_name,const std::string& sender_email,const std::string& sender_password) :
        conn(smtp_host, smtp_port)
    {
        try{
            msg_template.from(mailio::mail_address(sender_name, sender_email));
            msg_template.subject("验证信息", mailio::codec::codec_t::UTF8);
            msg_template.content_type(mailio::mime::media_type_t::TEXT, "plain", "utf-8");
            msg_template.content_transfer_encoding(mailio::mime::content_transfer_encoding_t::BASE_64);
            conn.authenticate(sender_email, sender_password, mailio::smtps::auth_method_t::START_TLS);
            OATPP_LOGI("SMTP", "SMTP connection initialized successfully");
        }
        catch (const std::exception& ex) {
             OATPP_LOGE("SMTP", "Failed to initialize SMTP connection: %s", ex.what());
             std::cerr << "Error: Failed to initialize SMTP connection: " << ex.what() << std::endl;
             std::exit(1);
        }
	}
    inline bool sendVerificationCode(std::string verificationcode, std::string email);
};

bool AppEmail::sendVerificationCode(std::string verificationcode, std::string email)
{
    try {
        auto msg(msg_template);
        msg.add_recipient(mailio::mail_address("", email));
        msg.content("【聊天室】 验证码 ：" + verificationcode + " 用于聊天室身份验证，3分钟内有效，请勿泄露和转发。如非本人操作，请忽略此短信。");
        conn.submit(msg);
        return true;
    }
    catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
        return false;
    }
}