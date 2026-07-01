cmake_minimum_required(VERSION 3.16)
project(ChatServer VERSION 1.0.0 LANGUAGES CXX)

# ==================== 全局编译选项 ====================
add_definitions(-DNOMINMAX)          # 解决 picojson.h 的报错问题

if(MSVC)
    add_compile_options(/bigobj /utf-8)
else()
    add_compile_options(-finput-charset=UTF-8)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ==================== 用户可配置的 vcpkg 路径 ====================
if(WIN32)
    set(VCPKG_ROOT_DEFAULT "C:/cppsoft/vcpkg")
    set(VCPKG_TRIPLET_DEFAULT "x64-windows")
else()
    set(VCPKG_ROOT_DEFAULT "/opt/vcpkg")
    set(VCPKG_TRIPLET_DEFAULT "x64-linux")
endif()

set(VCPKG_ROOT ${VCPKG_ROOT_DEFAULT} CACHE PATH "vcpkg installation root (e.g., C:/vcpkg or /opt/vcpkg)")
set(VCPKG_TARGET_TRIPLET ${VCPKG_TRIPLET_DEFAULT} CACHE STRING "vcpkg target triplet (e.g., x64-windows, x64-linux)")

# 构建 vcpkg 安装目录的完整路径（用于手动查找）
set(VCPKG_INSTALLED_DIR "${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}")

# ==================== 平台检测 ====================
if(WIN32)
    message(STATUS "检测到 Windows 平台")
    set(PLATFORM_WINDOWS TRUE)
elseif(UNIX AND NOT APPLE)
    message(STATUS "检测到 Linux 平台")
    set(PLATFORM_LINUX TRUE)
else()
    message(FATAL_ERROR "不支持的平台")
endif()

# ==================== Linux 逻辑 ====================
if(PLATFORM_LINUX)
    # ----- Poco 手动查找（Linux） -----
    set(POCO_INCLUDE_DIR "${VCPKG_INSTALLED_DIR}/include")
    set(POCO_LIBRARY_DIR "${VCPKG_INSTALLED_DIR}/lib")
    find_library(POCO_FOUNDATION_LIB NAMES PocoFoundation PATHS ${POCO_LIBRARY_DIR} REQUIRED)
    find_library(POCO_JSON_LIB      NAMES PocoJSON      PATHS ${POCO_LIBRARY_DIR} REQUIRED)
    find_library(POCO_XML_LIB       NAMES PocoXML       PATHS ${POCO_LIBRARY_DIR} REQUIRED)
    find_library(POCO_UTIL_LIB      NAMES PocoUtil      PATHS ${POCO_LIBRARY_DIR} REQUIRED)
    find_library(POCO_NET_LIB       NAMES PocoNet       PATHS ${POCO_LIBRARY_DIR} REQUIRED)
    find_library(POCO_CRYPTO_LIB    NAMES PocoCrypto    PATHS ${POCO_LIBRARY_DIR} REQUIRED)
    find_library(POCO_NETSSL_LIB    NAMES PocoNetSSL    PATHS ${POCO_LIBRARY_DIR} REQUIRED)

    set(POCO_LIBRARIES
        ${POCO_FOUNDATION_LIB}
        ${POCO_JSON_LIB}
        ${POCO_XML_LIB}
        ${POCO_UTIL_LIB}
        ${POCO_NET_LIB}
        ${POCO_CRYPTO_LIB}
        ${POCO_NETSSL_LIB}
    )
    set(POCO_INCLUDE_DIRS ${POCO_INCLUDE_DIR})
    message(STATUS "✅ Poco (Linux) - ${POCO_LIBRARIES}")

    # ----- redis++ 手动查找（Linux） -----
    set(REDIS_PP_INCLUDE_DIR "${VCPKG_INSTALLED_DIR}/include")
    set(REDIS_PP_LIB_DIR "${VCPKG_INSTALLED_DIR}/lib")
    find_library(REDIS_PP_LIBRARY NAMES redis++ libredis++.a PATHS ${REDIS_PP_LIB_DIR} NO_DEFAULT_PATH)
    find_library(HIREDIS_LIBRARY NAMES hiredis libhiredis.a PATHS ${REDIS_PP_LIB_DIR} NO_DEFAULT_PATH)
    if(REDIS_PP_LIBRARY AND HIREDIS_LIBRARY)
        set(REDIS_PP_LIBS ${REDIS_PP_LIBRARY} ${HIREDIS_LIBRARY})
        set(REDIS_PP_FOUND TRUE)
        message(STATUS "✅ redis++ (Linux)")
    else()
        message(WARNING "⚠️ redis++ 未找到，Redis 功能将不可用")
        set(REDIS_PP_FOUND FALSE)
    endif()

    # ----- mailio 手动查找（Linux） -----
    set(MAILIO_INCLUDE_DIR "${VCPKG_INSTALLED_DIR}/include")
    set(MAILIO_LIBRARY_DIR "${VCPKG_INSTALLED_DIR}/lib")
    find_library(MAILIO_LIBRARY NAMES mailio libmailio.so libmailio.a PATHS ${MAILIO_LIBRARY_DIR} REQUIRED)
    message(STATUS "✅ mailio (Linux) - ${MAILIO_LIBRARY}")

    # ----- Oat++ 手动查找（Linux） -----
    set(OATPP_INCLUDE_DIR "${VCPKG_INSTALLED_DIR}/include")
    set(OATPP_LIBRARY_DIR "${VCPKG_INSTALLED_DIR}/lib")
    find_library(OATPP_CORE_LIBRARY NAMES oatpp liboatpp.so liboatpp.a PATHS ${OATPP_LIBRARY_DIR} REQUIRED)
    find_library(OATPP_WEBSOCKET_LIBRARY NAMES oatpp-websocket liboatpp-websocket.so liboatpp-websocket.a PATHS ${OATPP_LIBRARY_DIR})
    find_library(OATPP_JSON_LIBRARY NAMES oatpp-parser-json liboatpp-parser-json.so liboatpp-parser-json.a PATHS ${OATPP_LIBRARY_DIR})
    find_library(OATPP_SWAGGER_LIBRARY NAMES oatpp-swagger liboatpp-swagger.so liboatpp-swagger.a PATHS ${OATPP_LIBRARY_DIR})
    find_library(OATPP_POSTGRESQL_LIBRARY NAMES oatpp-postgresql liboatpp-postgresql.so liboatpp-postgresql.a PATHS ${OATPP_LIBRARY_DIR})

    set(OATPP_SWAGGER_RESOURCES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include/res")

    message(STATUS "✅ Oat++ (Linux)")
    message(STATUS "  核心库: ${OATPP_CORE_LIBRARY}")
    if(OATPP_WEBSOCKET_LIBRARY)
        message(STATUS "  WebSocket: ${OATPP_WEBSOCKET_LIBRARY}")
    endif()
    if(OATPP_JSON_LIBRARY)
        message(STATUS "  JSON: ${OATPP_JSON_LIBRARY}")
    endif()
    if(OATPP_SWAGGER_LIBRARY)
        message(STATUS "  Swagger: ${OATPP_SWAGGER_LIBRARY}")
    endif()
    if(OATPP_POSTGRESQL_LIBRARY)
        message(STATUS "  PostgreSQL: ${OATPP_POSTGRESQL_LIBRARY}")
    endif()

    # 导入目标
    add_library(oatpp::oatpp UNKNOWN IMPORTED)
    set_target_properties(oatpp::oatpp PROPERTIES
        IMPORTED_LOCATION "${OATPP_CORE_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${OATPP_INCLUDE_DIR}"
    )
    if(OATPP_WEBSOCKET_LIBRARY)
        add_library(oatpp::oatpp-websocket UNKNOWN IMPORTED)
        set_target_properties(oatpp::oatpp-websocket PROPERTIES
            IMPORTED_LOCATION "${OATPP_WEBSOCKET_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${OATPP_INCLUDE_DIR}"
            INTERFACE_LINK_LIBRARIES "oatpp::oatpp"
        )
    endif()
    if(OATPP_JSON_LIBRARY)
        add_library(oatpp::oatpp-parser-json UNKNOWN IMPORTED)
        set_target_properties(oatpp::oatpp-parser-json PROPERTIES
            IMPORTED_LOCATION "${OATPP_JSON_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${OATPP_INCLUDE_DIR}"
            INTERFACE_LINK_LIBRARIES "oatpp::oatpp"
        )
    endif()
    if(OATPP_SWAGGER_LIBRARY)
        add_library(oatpp::oatpp-swagger UNKNOWN IMPORTED)
        set_target_properties(oatpp::oatpp-swagger PROPERTIES
            IMPORTED_LOCATION "${OATPP_SWAGGER_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${OATPP_INCLUDE_DIR}"
            INTERFACE_LINK_LIBRARIES "oatpp::oatpp"
        )
    endif()
    if(OATPP_POSTGRESQL_LIBRARY)
        add_library(oatpp::oatpp-postgresql UNKNOWN IMPORTED)
        set_target_properties(oatpp::oatpp-postgresql PROPERTIES
            IMPORTED_LOCATION "${OATPP_POSTGRESQL_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${OATPP_INCLUDE_DIR}"
            INTERFACE_LINK_LIBRARIES "oatpp::oatpp"
        )
    endif()
endif()

# ==================== Windows 逻辑 ====================
if(PLATFORM_WINDOWS)
    # ----- Poco -----
    find_package(Poco REQUIRED COMPONENTS Foundation JSON XML Util Net Crypto NetSSL CONFIG)
    if(Poco_FOUND)
        set(POCO_LIBRARIES Poco::Foundation Poco::JSON Poco::XML Poco::Util Poco::Net Poco::Crypto Poco::NetSSL)
        set(POCO_INCLUDE_DIRS ${Poco_INCLUDE_DIRS})
        message(STATUS "✅ Poco ${Poco_VERSION}")
    else()
        message(FATAL_ERROR "❌ Poco 未找到，请检查 vcpkg 安装")
    endif()

    # ----- redis++ -----
    find_package(redis++ CONFIG)
    if(redis++_FOUND)
        set(REDIS_PP_LIBS redis++::redis++)
        set(REDIS_PP_FOUND TRUE)
        set(REDIS_PP_INCLUDE_DIR "")  # 目标自动传递
        message(STATUS "✅ redis++ (via find_package)")
    else()
        # 手动查找（备用）
        set(REDIS_PP_INCLUDE_DIR "${VCPKG_INSTALLED_DIR}/include")
        set(REDIS_PP_LIB_DIR "${VCPKG_INSTALLED_DIR}/lib")
        find_library(REDIS_PP_LIBRARY NAMES redis++ PATHS ${REDIS_PP_LIB_DIR} NO_DEFAULT_PATH)
        find_library(HIREDIS_LIBRARY NAMES hiredis PATHS ${REDIS_PP_LIB_DIR} NO_DEFAULT_PATH)
        if(REDIS_PP_LIBRARY AND HIREDIS_LIBRARY)
            set(REDIS_PP_LIBS ${REDIS_PP_LIBRARY} ${HIREDIS_LIBRARY})
            set(REDIS_PP_FOUND TRUE)
            message(STATUS "✅ redis++ (手动定位 Windows)")
        else()
            message(WARNING "⚠️ redis++ 未找到，Redis 功能将不可用")
            set(REDIS_PP_FOUND FALSE)
        endif()
    endif()

    # ----- mailio -----
    find_package(mailio CONFIG)
    if(mailio_FOUND)
        message(STATUS "✅ mailio")
    else()
        message(WARNING "⚠️ mailio 未找到")
    endif()

    # ----- Oat++ (Windows) -----
    find_package(oatpp CONFIG REQUIRED)
    message(STATUS "✅ oatpp")
    find_package(oatpp-websocket CONFIG)
    if(oatpp-websocket_FOUND)
        message(STATUS "✅ oatpp-websocket")
    endif()
    find_package(oatpp-swagger CONFIG)
    if(oatpp-swagger_FOUND)
        message(STATUS "✅ oatpp-swagger")
    endif()
    find_package(oatpp-postgresql CONFIG)
    if(oatpp-postgresql_FOUND)
        message(STATUS "✅ oatpp-postgresql")
    endif()
endif()

# ==================== 公共依赖（跨平台） ====================
find_package(OpenSSL REQUIRED)
message(STATUS "✅ OpenSSL ${OPENSSL_VERSION}")

# ==================== bcrypt（仅 Windows） ====================
if(WIN32)
    set(BCRYPT_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib")
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(BCRYPT_LIB "${BCRYPT_LIB_DIR}/Debug/bcrypt.lib")
    else()
        set(BCRYPT_LIB "${BCRYPT_LIB_DIR}/Release/bcrypt.lib")
    endif()
endif()

# ==================== 源文件 ====================
file(GLOB_RECURSE SRC_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp"
)
add_executable(ChatServer ${SRC_FILES})

# ==================== 包含目录 ====================
target_include_directories(ChatServer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    # Linux 下手动添加 Poco 和 redis++ 头文件
    $<$<BOOL:${PLATFORM_LINUX}>:${POCO_INCLUDE_DIRS}>
    $<$<BOOL:${PLATFORM_LINUX}>:${REDIS_PP_INCLUDE_DIR}>
    $<$<BOOL:${PLATFORM_LINUX}>:${MAILIO_INCLUDE_DIR}>
    # Windows 下若 redis++ 为手动模式，则添加头文件
    $<$<AND:$<BOOL:${PLATFORM_WINDOWS}>,$<NOT:$<BOOL:${redis++_FOUND}>>>:${REDIS_PP_INCLUDE_DIR}>
)

# ==================== 链接库 ====================
if(PLATFORM_LINUX)
    target_link_libraries(ChatServer PRIVATE
        ${OATPP_CORE_LIBRARY}
        $<$<BOOL:${OATPP_WEBSOCKET_LIBRARY}>:${OATPP_WEBSOCKET_LIBRARY}>
        $<$<BOOL:${OATPP_JSON_LIBRARY}>:${OATPP_JSON_LIBRARY}>
        $<$<BOOL:${OATPP_SWAGGER_LIBRARY}>:${OATPP_SWAGGER_LIBRARY}>
        $<$<BOOL:${OATPP_POSTGRESQL_LIBRARY}>:${OATPP_POSTGRESQL_LIBRARY}>
        ${POCO_LIBRARIES}
        ${OPENSSL_LIBRARIES}
        ${MAILIO_LIBRARY}
        ${REDIS_PP_LIBS}
        pthread
        dl
    )
else()
    target_link_libraries(ChatServer PRIVATE
        oatpp::oatpp
        $<$<BOOL:${oatpp-websocket_FOUND}>:oatpp::oatpp-websocket>
        $<$<BOOL:${oatpp-swagger_FOUND}>:oatpp::oatpp-swagger>
        $<$<BOOL:${oatpp-postgresql_FOUND}>:oatpp::oatpp-postgresql>
        ${POCO_LIBRARIES}
        OpenSSL::SSL OpenSSL::Crypto
        $<$<BOOL:${mailio_FOUND}>:mailio::mailio>
        $<$<BOOL:${REDIS_PP_FOUND}>:${REDIS_PP_LIBS}>
    )
    if(WIN32 AND BCRYPT_LIB)
        target_link_libraries(ChatServer PRIVATE ${BCRYPT_LIB})
    endif()
endif()

# ==================== 平台特定设置 ====================
if(WIN32)
    target_compile_definitions(ChatServer PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()
if(PLATFORM_LINUX)
    target_compile_options(ChatServer PRIVATE -Wall -Wextra)
    set_target_properties(ChatServer PROPERTIES
        INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib"
        INSTALL_RPATH_USE_LINK_PATH TRUE
    )
endif()
set_target_properties(ChatServer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# ==================== 宏定义 ====================
if(PLATFORM_LINUX AND OATPP_SWAGGER_LIBRARY OR (PLATFORM_WINDOWS AND oatpp-swagger_FOUND))
    set(OATPP_SWAGGER_RESOURCES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include/res")
    target_compile_definitions(ChatServer PRIVATE
        OATPP_SWAGGER_RES_PATH="${OATPP_SWAGGER_RESOURCES_DIR}"
        SQLCHECK
    )
endif()
target_compile_definitions(ChatServer PRIVATE
    CONF_PATH="${CMAKE_CURRENT_SOURCE_DIR}/config.json"
)

# ==================== 安装 ====================
install(TARGETS ChatServer RUNTIME DESTINATION bin)

# ==================== 调试信息 ====================
message(STATUS "========================================")
message(STATUS "配置摘要:")
message(STATUS "  平台: ${CMAKE_SYSTEM_NAME}")
message(STATUS "  构建类型: ${CMAKE_BUILD_TYPE}")
message(STATUS "  VCPKG_ROOT: ${VCPKG_ROOT}")
message(STATUS "  VCPKG_TARGET_TRIPLET: ${VCPKG_TARGET_TRIPLET}")
message(STATUS "  VCPKG_INSTALLED_DIR: ${VCPKG_INSTALLED_DIR}")
if(PLATFORM_LINUX)
    message(STATUS "  Poco: ${POCO_LIBRARIES}")
    if(REDIS_PP_FOUND)
        message(STATUS "  redis++: ${REDIS_PP_LIBRARY}")
        message(STATUS "  hiredis: ${HIREDIS_LIBRARY}")
    endif()
    message(STATUS "  mailio: ${MAILIO_LIBRARY}")
    message(STATUS "  Oat++ 核心: ${OATPP_CORE_LIBRARY}")
endif()
if(PLATFORM_WINDOWS)
    if(Poco_FOUND)
        message(STATUS "  Poco: ${Poco_VERSION}")
    endif()
    if(redis++_FOUND)
        message(STATUS "  redis++: 已找到")
    endif()
    if(mailio_FOUND)
        message(STATUS "  mailio: 已找到")
    endif()
endif()
message(STATUS "========================================")