#include "AppErrorHandler.h"

AppErrorHandler::AppErrorHandler(const std::shared_ptr<oatpp::data::mapping::ObjectMapper>& objectMapper)
  : m_objectMapper(objectMapper)
{}

std::shared_ptr<AppErrorHandler::OutgoingResponse>
AppErrorHandler::handleError(const Status& status, const oatpp::String& message, const Headers& headers) {

  auto error = ErrorStatusDto::createShared();
  error->status = "ERROR";
  error->code = status.code;
  error->message = message;

  auto response = ResponseFactory::createResponse(status, error, m_objectMapper);

  for(const auto& pair : headers.getAll()) {
    response->putHeader(pair.first.toString(), pair.second.toString());
  }

  return response;

}