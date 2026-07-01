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

# ==================== Linux 逻辑（仅保留 Poco 和 redis++ 手动查找） ====================
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
endif()

# ==================== Windows 逻辑 ====================
if(PLATFORM_WINDOWS)
    # ----- Poco 使用 find_package（Windows） -----
    find_package(Poco REQUIRED COMPONENTS Foundation JSON XML Util Net Crypto NetSSL CONFIG)
    if(Poco_FOUND)
        set(POCO_LIBRARIES Poco::Foundation Poco::JSON Poco::XML Poco::Util Poco::Net Poco::Crypto Poco::NetSSL)
        set(POCO_INCLUDE_DIRS ${Poco_INCLUDE_DIRS})
        message(STATUS "✅ Poco ${Poco_VERSION}")
    else()
        message(FATAL_ERROR "❌ Poco 未找到，请检查 vcpkg 安装")
    endif()

    # ----- redis++ 优先 find_package，失败则手动（Windows） -----
    find_package(redis++ CONFIG)
    if(redis++_FOUND)
        set(REDIS_PP_LIBS redis++::redis++)
        set(REDIS_PP_FOUND TRUE)
        set(REDIS_PP_INCLUDE_DIR "")  # 目标自动传递头文件
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
endif()

# ==================== 公共依赖（跨平台，使用 find_package） ====================
find_package(OpenSSL REQUIRED)
message(STATUS "✅ OpenSSL ${OPENSSL_VERSION}")

# Oat++ 及其模块（所有平台统一使用 find_package，由 vcpkg 提供配置）
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

find_package(mailio CONFIG)
if(mailio_FOUND)
    message(STATUS "✅ mailio")
else()
    message(WARNING "⚠️ mailio 未找到")
endif()

# ==================== COS SDK（平台相关） ====================
set(COS_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib")
set(COS_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include/cos-cpp")

if(PLATFORM_LINUX)
    find_library(COS_LIBRARY
        NAMES cossdk libcossdk.so libcossdk.a
        PATHS ${COS_LIB_DIR}
        PATH_SUFFIXES linux linux-x64 x64
        REQUIRED
    )
elseif(PLATFORM_WINDOWS)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        find_library(COS_LIBRARY
            NAMES cossdkd.lib
            PATHS ${COS_LIB_DIR}
            PATH_SUFFIXES Debug windows/x64/Debug
            REQUIRED
        )
    else()
        find_library(COS_LIBRARY
            NAMES cossdk.lib
            PATHS ${COS_LIB_DIR}
            PATH_SUFFIXES Release windows/x64/Release
            REQUIRED
        )
    endif()
endif()
message(STATUS "✅ COS: ${COS_LIBRARY}")

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
    ${COS_INCLUDE_DIR}
    # Linux 下手动添加 Poco 和 redis++ 头文件（Windows 下由 find_package 目标自动传递）
    $<$<BOOL:${PLATFORM_LINUX}>:${POCO_INCLUDE_DIRS}>
    $<$<BOOL:${PLATFORM_LINUX}>:${REDIS_PP_INCLUDE_DIR}>
    # Windows 下若 redis++ 为手动模式，则添加头文件
    $<$<AND:$<BOOL:${PLATFORM_WINDOWS}>,$<NOT:$<BOOL:${redis++_FOUND}>>>:${REDIS_PP_INCLUDE_DIR}>
)

# ==================== 链接库 ====================
target_link_libraries(ChatServer PRIVATE
    # Oat++ 及其模块（由 find_package 提供目标）
    oatpp::oatpp
    $<$<BOOL:${oatpp-websocket_FOUND}>:oatpp::oatpp-websocket>
    $<$<BOOL:${oatpp-swagger_FOUND}>:oatpp::oatpp-swagger>
    $<$<BOOL:${oatpp-postgresql_FOUND}>:oatpp::oatpp-postgresql>
    # Poco
    ${POCO_LIBRARIES}
    # OpenSSL
    OpenSSL::SSL OpenSSL::Crypto
    # mailio
    $<$<BOOL:${mailio_FOUND}>:mailio::mailio>
    # redis++
    $<$<BOOL:${REDIS_PP_FOUND}>:${REDIS_PP_LIBS}>
    # COS
    ${COS_LIBRARY}
    # 系统库
    $<$<PLATFORM_ID:Linux>:pthread>
    $<$<PLATFORM_ID:Linux>:dl>
)

if(WIN32 AND BCRYPT_LIB)
    target_link_libraries(ChatServer PRIVATE ${BCRYPT_LIB})
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
if(oatpp-swagger_FOUND)
    set(OATPP_SWAGGER_RESOURCES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include/res")
    target_compile_definitions(ChatServer PRIVATE
        OATPP_SWAGGER_RES_PATH="${OATPP_SWAGGER_RESOURCES_DIR}"
        SQLCHECK
    )
endif()
target_compile_definitions(ChatServer PRIVATE
    CONF_PATH="${CMAKE_CURRENT_SOURCE_DIR}/config.json"
)

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
endif()
if(PLATFORM_WINDOWS)
    if(Poco_FOUND)
        message(STATUS "  Poco: ${Poco_VERSION}")
    endif()
    if(redis++_FOUND)
        message(STATUS "  redis++: 已找到")
    endif()
endif()
message(STATUS "========================================")