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

# ==================== vcpkg 依赖（统一 find_package，跨平台） ====================
find_package(OpenSSL REQUIRED)
message(STATUS "✅ OpenSSL ${OPENSSL_VERSION}")

# Oat++ 及其模块
find_package(oatpp CONFIG REQUIRED)
message(STATUS "✅ oatpp")
find_package(oatpp-websocket CONFIG REQUIRED)
message(STATUS "✅ oatpp-websocket")
find_package(oatpp-swagger CONFIG REQUIRED)
message(STATUS "✅ oatpp-swagger")
find_package(oatpp-postgresql CONFIG REQUIRED)
message(STATUS "✅ oatpp-postgresql")

# Poco
find_package(Poco REQUIRED COMPONENTS Foundation JSON XML Util Net Crypto NetSSL CONFIG)
message(STATUS "✅ Poco ${Poco_VERSION}")

# redis++
find_package(redis++ CONFIG REQUIRED)
if(redis++_FOUND)
    # 按优先级检测常见目标名（跨平台兼容）
    if(TARGET redis++::redis++_static)
        set(REDIS_TARGET redis++::redis++_static)
    elseif(TARGET redis++::redis++)
        set(REDIS_TARGET redis++::redis++)
    elseif(TARGET redis++::redis_plus_plus)
        set(REDIS_TARGET redis++::redis_plus_plus)
    else()
        message(FATAL_ERROR "无法识别 redis++ 提供的 CMake 目标")
    endif()
    message(STATUS "✅ redis++ 目标: ${REDIS_TARGET}")
endif()

# mailio
find_package(mailio CONFIG REQUIRED)
message(STATUS "✅ mailio")

# ==================== COS SDK（不在 vcpkg 中，手动查找） ====================
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

# ==================== bcrypt（不在 vcpkg 中，手动查找） ====================
set(BCRYPT_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib")
set(BCRYPT_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include/bcrypt")

if(PLATFORM_LINUX)
    find_library(BCRYPT_LIBRARY
        NAMES bcrypt libbcrypt.a
        PATHS ${BCRYPT_LIB_DIR}
        PATH_SUFFIXES linux linux-x64 x64
        REQUIRED
    )
    message(STATUS "✅ bcrypt (Linux): ${BCRYPT_LIBRARY}")
elseif(PLATFORM_WINDOWS)
    # 用户编译的 bcrypt_hash.lib（用于 bcrypt_checkpw 等哈希函数）
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        find_library(BCRYPT_LIBRARY
            NAMES bcrypt_hash.lib
            PATHS ${BCRYPT_LIB_DIR}
            PATH_SUFFIXES Debug windows/x64/Debug
            NO_DEFAULT_PATH
            REQUIRED
        )
    else()
        find_library(BCRYPT_LIBRARY
            NAMES bcrypt_hash.lib
            PATHS ${BCRYPT_LIB_DIR}
            PATH_SUFFIXES Release windows/x64/Release
            NO_DEFAULT_PATH
            REQUIRED
        )
    endif()
    # 系统 bcrypt.lib（Windows API，用于加密功能）
    set(WINDOWS_BCRYPT_LIB "bcrypt")
    message(STATUS "✅ bcrypt (Windows): ${BCRYPT_LIBRARY} + 系统 bcrypt.lib")
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
    #${BCRYPT_INCLUDE_DIR}
)

# ==================== 链接库 ====================
target_link_libraries(ChatServer PRIVATE
    # COS SDK（依赖 Poco、OpenSSL，不在 vcpkg 中）
    ${COS_LIBRARY}
    # bcrypt（不在 vcpkg 中）
    ${BCRYPT_LIBRARY}
    $<$<BOOL:${PLATFORM_WINDOWS}>:${WINDOWS_BCRYPT_LIB}>
    # vcpkg 库（通过 find_package 提供的 imported target，自动处理传递依赖和链接顺序）
    mailio::mailio
    ${REDIS_TARGET}
    Poco::Foundation Poco::JSON Poco::XML Poco::Util Poco::Net Poco::Crypto Poco::NetSSL
    OpenSSL::SSL OpenSSL::Crypto
    oatpp::oatpp
    oatpp::oatpp-websocket
    oatpp::oatpp-swagger
    oatpp::oatpp-postgresql
    # 系统库
    $<$<PLATFORM_ID:Linux>:pthread>
    $<$<PLATFORM_ID:Linux>:dl>
    $<$<PLATFORM_ID:Linux>:crypt>
)

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
message(STATUS "  Poco: ${Poco_VERSION}")
message(STATUS "  COS: ${COS_LIBRARY}")
message(STATUS "========================================")
