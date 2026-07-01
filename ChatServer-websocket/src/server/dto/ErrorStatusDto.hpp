#pragma once
#include"global.h"


#include OATPP_CODEGEN_BEGIN(DTO)

class ErrorStatusDto : public oatpp::DTO {

  DTO_INIT(ErrorStatusDto, DTO)

  DTO_FIELD_INFO(status) {
    info->description = "Short status text";
  }
  DTO_FIELD(String, status);

  DTO_FIELD_INFO(code) {
    info->description = "Status code";
  }
  DTO_FIELD(Int32, code);

  DTO_FIELD_INFO(message) {
    info->description = "Verbose message";
  }
  DTO_FIELD(String, message);

};

#include OATPP_CODEGEN_END(DTO)

