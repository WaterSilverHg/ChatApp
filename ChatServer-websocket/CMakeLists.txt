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
message(STATUS "✅ redis++")

# mailio
find_package(mailio CONFIG REQUIRED)
message(STATUS "✅ mailio")

# ==================== bcrypt（仅 Windows，不在 vcpkg 中） ====================
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
)

# ==================== 链接库 ====================
target_link_libraries(ChatServer PRIVATE
    # vcpkg 库（通过 find_package 提供的 imported target，自动处理传递依赖和链接顺序）
    mailio::mailio
    redis++::redis++
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
message(STATUS "  Poco: ${Poco_VERSION}")
message(STATUS "========================================")
