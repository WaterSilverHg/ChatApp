#pragma once

#include "global.h"

#include OATPP_CODEGEN_BEGIN(DTO) 

class UuidDTO : public oatpp::DTO {
    DTO_INIT(UuidDTO, DTO)
        DTO_FIELD(String, uuid, "uuid");
};

class IdDTO : public oatpp::DTO {
    DTO_INIT(IdDTO, DTO)
        DTO_FIELD(Int64, id, "id");
};

class StatusDTO : public oatpp::DTO {
    DTO_INIT(StatusDTO, DTO)
        DTO_FIELD(String, status, "status");
};

#include OATPP_CODEGEN_END(DTO)