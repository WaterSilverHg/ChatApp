#pragma once

#include"global.h"
#include "server/dto/ErrorStatusDto.hpp"

class AppErrorHandler : public oatpp::web::server::handler::ErrorHandler {
private:
  typedef oatpp::web::protocol::http::outgoing::Response OutgoingResponse;
  typedef oatpp::web::protocol::http::Status Status;
  typedef oatpp::web::protocol::http::outgoing::ResponseFactory ResponseFactory;
private:
  std::shared_ptr<oatpp::data::mapping::ObjectMapper> m_objectMapper;
public:

	AppErrorHandler(const std::shared_ptr<oatpp::data::mapping::ObjectMapper>& objectMapper);

  std::shared_ptr<OutgoingResponse>
  handleError(const Status& status, const oatpp::String& message, const Headers& headers) override;

};
