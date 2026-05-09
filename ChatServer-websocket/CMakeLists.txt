cmake_minimum_required(VERSION 3.16)
project(ChatServer VERSION 1.0.0 LANGUAGES CXX)

#是否启用Swagger
#add_definitions(-DSWAGGER)


add_definitions(-DNOMINMAX) #解决picojson.h的报错问题


if(MSVC)
    add_compile_options(/bigobj)
endif()

if(MSVC)
    # 强制 MSVC 将输入和输出的字符集都解释为 UTF-8
    add_compile_options(/utf-8)
else()
    # 为 GCC/Clang 明确指定输入字符集为 UTF-8
    add_compile_options(-finput-charset=UTF-8)
endif()
# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)  # Oat++ 1.3.0 使用 C++17
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 判断操作系统
if(WIN32)
    message(STATUS "检测到 Windows 平台")
    set(PLATFORM_WINDOWS TRUE)
elseif(UNIX AND NOT APPLE)
    message(STATUS "检测到 Linux 平台")
    set(PLATFORM_LINUX TRUE)
    
    # 设置 Oat++ 版本（可根据需要修改）
    set(OATPP_VERSION "1.3.0")
    # set(OATPP_VERSION "1.4.0")  # 如果要用 1.4.0，取消注释
endif()

# ==================== 查找 OpenSSL ====================
message(STATUS "查找 OpenSSL...")
find_package(OpenSSL REQUIRED)

if(OPENSSL_FOUND)
    message(STATUS "✅ 找到 OpenSSL")
    message(STATUS "   OpenSSL 包含目录: ${OPENSSL_INCLUDE_DIR}")
    message(STATUS "   OpenSSL 库目录: ${OPENSSL_LIBRARY_DIRS}")
    message(STATUS "   OpenSSL 版本: ${OPENSSL_VERSION}")
else()
    if(PLATFORM_LINUX)
        message(FATAL_ERROR "未找到 OpenSSL，请安装: sudo apt install libssl-dev")
    elseif(PLATFORM_WINDOWS)
        message(FATAL_ERROR "未找到 OpenSSL，请通过 vcpkg 安装: vcpkg install openssl:x64-windows")
    endif()
endif()

# ==================== 查找 mailio 库 ====================
message(STATUS "查找 mailio...")
if(PLATFORM_LINUX)
    find_path(MAILIO_INCLUDE_DIR 
        NAMES mailio/mailio.hpp
        PATHS /usr/local/include /usr/include
        PATH_SUFFIXES mailio
    )
    
    find_library(MAILIO_LIBRARY 
        NAMES mailio libmailio.so libmailio.a
        PATHS /usr/local/lib64 /usr/local/lib /usr/lib64 /usr/lib
    )
    
    if(MAILIO_INCLUDE_DIR AND MAILIO_LIBRARY)
        message(STATUS "✅ 找到 mailio")
        message(STATUS "   mailio 包含目录: ${MAILIO_INCLUDE_DIR}")
        message(STATUS "   mailio 库: ${MAILIO_LIBRARY}")
    else()
        message(WARNING "⚠️ 未找到 mailio，邮件功能将不可用。请安装: sudo apt install libmailio-dev")
    endif()
    
elseif(PLATFORM_WINDOWS)
    if(NOT VCPKG_ROOT)
        set(VCPKG_ROOT "C:/cppsoft/vcpkg")
    endif()
    
    set(MAILIO_CONFIG_DIR "${VCPKG_ROOT}/installed/x64-windows/share/mailio")
    
    if(EXISTS "${MAILIO_CONFIG_DIR}/mailio-Config.cmake")
        message(STATUS "在 vcpkg 路径找到 mailio: ${MAILIO_CONFIG_DIR}")
        find_package(mailio CONFIG REQUIRED PATHS ${MAILIO_CONFIG_DIR} NO_DEFAULT_PATH)
        message(STATUS "✅ 找到 mailio")
    else()
        message(WARNING "⚠️ 未找到 mailio，邮件功能将不可用。请安装: vcpkg install mailio:x64-windows")
    endif()
endif()

# ==================== 查找 redis++ 库 ====================
message(STATUS "查找 redis++...")
if(PLATFORM_LINUX)
    find_path(REDIS_PLUS_PLUS_INCLUDE_DIR 
        NAMES sw/redis++/redis++.h
        PATHS /usr/local/include /usr/include
    )
    
    find_library(REDIS_PLUS_PLUS_LIBRARY 
        NAMES redis++ libredis++.so libredis++.a
        PATHS /usr/local/lib64 /usr/local/lib /usr/lib64 /usr/lib
    )
    
    # redis++ 依赖 hiredis
    find_path(HIREDIS_INCLUDE_DIR 
        NAMES hiredis/hiredis.h
        PATHS /usr/local/include /usr/include
    )
    
    find_library(HIREDIS_LIBRARY 
        NAMES hiredis libhiredis.so libhiredis.a
        PATHS /usr/local/lib64 /usr/local/lib /usr/lib64 /usr/lib
    )
    
    if(REDIS_PLUS_PLUS_INCLUDE_DIR AND REDIS_PLUS_PLUS_LIBRARY AND HIREDIS_INCLUDE_DIR AND HIREDIS_LIBRARY)
        message(STATUS "✅ 找到 redis++")
        message(STATUS "   redis++ 包含目录: ${REDIS_PLUS_PLUS_INCLUDE_DIR}")
        message(STATUS "   redis++ 库: ${REDIS_PLUS_PLUS_LIBRARY}")
        message(STATUS "   hiredis 包含目录: ${HIREDIS_INCLUDE_DIR}")
        message(STATUS "   hiredis 库: ${HIREDIS_LIBRARY}")
    else()
        message(WARNING "⚠️ 未找到 redis++，Redis功能将不可用。请安装: sudo apt install libhiredis-dev")
        message(WARNING "   然后编译安装 redis++: https://github.com/sewenew/redis++")
    endif()
    
elseif(PLATFORM_WINDOWS)
    if(NOT VCPKG_ROOT)
        set(VCPKG_ROOT "C:/cppsoft/vcpkg")
    endif()
    
    set(REDIS_PLUS_PLUS_CONFIG_DIR "${VCPKG_ROOT}/installed/x64-windows/share/redis++")
    
    if(EXISTS "${REDIS_PLUS_PLUS_CONFIG_DIR}/redis++-Config.cmake")
        message(STATUS "在 vcpkg 路径找到 redis++: ${REDIS_PLUS_PLUS_CONFIG_DIR}")
        find_package(redis++ CONFIG REQUIRED PATHS ${REDIS_PLUS_PLUS_CONFIG_DIR} NO_DEFAULT_PATH)
        message(STATUS "✅ 找到 redis++")
    else()
        message(WARNING "⚠️ 未找到 redis++，Redis功能将不可用。请安装: vcpkg install redis++:x64-windows")
    endif()
endif()

# ==================== 收集所有源文件 ====================

# 自动收集 src 目录下的所有源文件
file(GLOB_RECURSE SRC_FILES 
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    # "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp"
)
set(PROJECT_SOURCES ${SRC_FILES})
# message(STATUS "当前的文件有 ${PROJECT_SOURCES}")

# ==================== 查找 Oat++ 库 ====================
if(PLATFORM_LINUX)
    message(STATUS "使用 Linux 特定路径查找 Oat++ ${OATPP_VERSION}")
    # Oat++ 1.3.0 的路径
    set(OATPP_INCLUDE_DIR "/usr/local/include/oatpp-${OATPP_VERSION}/oatpp" "/usr/local/include/oatpp-${OATPP_VERSION}")
    set(OATPP_LIBRARY_DIR "/usr/local/lib64/oatpp-${OATPP_VERSION}")
    
    message(STATUS "查找 Oat++ 头文件路径: ${OATPP_INCLUDE_DIR}")
    message(STATUS "查找 Oat++ 库文件路径: ${OATPP_LIBRARY_DIR}")
    
    
    # 如果上面的 find_library 失败，尝试搜索其他路径
    if(NOT OATPP_CORE_LIBRARY)
        message(STATUS "在指定路径未找到库，尝试系统搜索...")
        find_library(OATPP_CORE_LIBRARY 
            NAMES oatpp liboatpp.so liboatpp.a
            PATHS /usr/local/lib64 /usr/local/lib /usr/lib64 /usr/lib
            PATH_SUFFIXES oatpp-${OATPP_VERSION} oatpp-1.3.0 oatpp-1.4.0 oatpp
            REQUIRED
        )
    endif()
    
    # 查找可选的 WebSocket 库
    find_library(OATPP_WEBSOCKET_LIBRARY 
        NAMES oatpp-websocket liboatpp-websocket.so liboatpp-websocket.a
        PATHS ${OATPP_LIBRARY_DIR} /usr/local/lib64 /usr/local/lib
        PATH_SUFFIXES oatpp-${OATPP_VERSION} oatpp-1.3.0 oatpp-1.4.0
    )
    
    # 查找可选的 JSON 解析库
    find_library(OATPP_JSON_LIBRARY 
        NAMES oatpp-parser-json liboatpp-parser-json.so liboatpp-parser-json.a
        PATHS ${OATPP_LIBRARY_DIR} /usr/local/lib64 /usr/local/lib
        PATH_SUFFIXES oatpp-${OATPP_VERSION} oatpp-1.3.0 oatpp-1.4.0
    )
    
    # ========== 新增：查找 Swagger 库 ==========
    find_library(OATPP_SWAGGER_LIBRARY 
        NAMES oatpp-swagger liboatpp-swagger.so liboatpp-swagger.a
        PATHS ${OATPP_LIBRARY_DIR} /usr/local/lib64 /usr/local/lib
        PATH_SUFFIXES oatpp-${OATPP_VERSION} oatpp-1.3.0 oatpp-1.4.0
    )
    
    # ========== 新增：查找 PostgreSQL 库 ==========
    find_library(OATPP_POSTGRESQL_LIBRARY 
        NAMES oatpp-postgresql liboatpp-postgresql.so liboatpp-postgresql.a
        PATHS ${OATPP_LIBRARY_DIR} /usr/local/lib64 /usr/local/lib
        PATH_SUFFIXES oatpp-${OATPP_VERSION} oatpp-1.3.0 oatpp-1.4.0
    )
    
    # ========== 新增：查找 Swagger 资源目录 ==========
    # 用于存放 Swagger UI 的静态资源文件
    set(OATPP_SWAGGER_RESOURCES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include/res")
    
    # 检查是否找到核心库
    if(OATPP_INCLUDE_DIR AND OATPP_CORE_LIBRARY)
        message(STATUS "✅ 找到 Oat++ 头文件: ${OATPP_INCLUDE_DIR}")
        message(STATUS "✅ 找到 Oat++ 核心库: ${OATPP_CORE_LIBRARY}")
        if(OATPP_WEBSOCKET_LIBRARY)
            message(STATUS "✅ 找到 Oat++ WebSocket 库: ${OATPP_WEBSOCKET_LIBRARY}")
        endif()
        if(OATPP_JSON_LIBRARY)
            message(STATUS "✅ 找到 Oat++ JSON 库: ${OATPP_JSON_LIBRARY}")
        endif()
        if(OATPP_SWAGGER_LIBRARY)
            message(STATUS "✅ 找到 Oat++ Swagger 库: ${OATPP_SWAGGER_LIBRARY}")
            message(STATUS "✅ Swagger 资源目录: ${OATPP_SWAGGER_RESOURCES_DIR}")
        else()
            message(WARNING "⚠️ 未找到 Oat++ Swagger 库，Swagger 功能将不可用")
        endif()
        
        if(OATPP_POSTGRESQL_LIBRARY)
            message(STATUS "✅ 找到 Oat++ PostgreSQL 库: ${OATPP_POSTGRESQL_LIBRARY}")
        else()
            message(WARNING "⚠️ 未找到 Oat++ PostgreSQL 库，PostgreSQL 功能将不可用")
        endif()
        
        # 创建导入的目标
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
        
        # ========== 新增：创建 Swagger 导入目标 ==========
        if(OATPP_SWAGGER_LIBRARY)
            add_library(oatpp::oatpp-swagger UNKNOWN IMPORTED)
            set_target_properties(oatpp::oatpp-swagger PROPERTIES
                IMPORTED_LOCATION "${OATPP_SWAGGER_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${OATPP_INCLUDE_DIR}"
                INTERFACE_LINK_LIBRARIES "oatpp::oatpp"
            )
        endif()
        
        # ========== 新增：创建 PostgreSQL 导入目标 ==========
        if(OATPP_POSTGRESQL_LIBRARY)
            add_library(oatpp::oatpp-postgresql UNKNOWN IMPORTED)
            set_target_properties(oatpp::oatpp-postgresql PROPERTIES
                IMPORTED_LOCATION "${OATPP_POSTGRESQL_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${OATPP_INCLUDE_DIR}"
                INTERFACE_LINK_LIBRARIES "oatpp::oatpp"
            )
        endif()
    else()
        message(FATAL_ERROR "无法找到 Oat++ ${OATPP_VERSION} 库，请确保已正确安装")
    endif()
    
elseif(PLATFORM_WINDOWS)
    message(STATUS "使用 Windows 平台查找 Oat++")
    
    # Windows 平台使用 vcpkg
    if(NOT VCPKG_ROOT)
        set(VCPKG_ROOT "C:/cppsoft/vcpkg")
    endif()
    
    set(OATPP_CONFIG_DIR "${VCPKG_ROOT}/installed/x64-windows/share/oatpp")
    set(OATPP_WEBSOCKET_CONFIG_DIR "${VCPKG_ROOT}/installed/x64-windows/share/oatpp-websocket")
    set(OATPP_SWAGGER_CONFIG_DIR "${VCPKG_ROOT}/installed/x64-windows/share/oatpp-swagger")
    set(OATPP_POSTGRESQL_CONFIG_DIR "${VCPKG_ROOT}/installed/x64-windows/share/oatpp-postgresql")
    
    if(EXISTS "${OATPP_CONFIG_DIR}/oatppConfig.cmake")
        message(STATUS "在 vcpkg 路径找到 Oat++: ${OATPP_CONFIG_DIR}")
        find_package(oatpp CONFIG REQUIRED PATHS ${OATPP_CONFIG_DIR} NO_DEFAULT_PATH)
        
        # 查找可选组件
        if(EXISTS "${OATPP_WEBSOCKET_CONFIG_DIR}/oatpp-websocketConfig.cmake")
            find_package(oatpp-websocket CONFIG REQUIRED PATHS ${OATPP_WEBSOCKET_CONFIG_DIR} NO_DEFAULT_PATH)
        endif()
        
        # ========== 新增：查找 Swagger 组件 ==========
        if(EXISTS "${OATPP_SWAGGER_CONFIG_DIR}/oatpp-swaggerConfig.cmake")
            message(STATUS "找到 Oat++ Swagger 配置: ${OATPP_SWAGGER_CONFIG_DIR}")
            set(OATPP_SWAGGER_RESOURCES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include/res")
            find_package(oatpp-swagger CONFIG REQUIRED PATHS ${OATPP_SWAGGER_CONFIG_DIR} NO_DEFAULT_PATH)
        else()
            message(WARNING "未找到 Oat++ Swagger，如需使用请安装: vcpkg install oatpp-swagger:x64-windows")
        endif()
        
        # ========== 新增：查找 PostgreSQL 组件 ==========
        if(EXISTS "${OATPP_POSTGRESQL_CONFIG_DIR}/oatpp-postgresqlConfig.cmake")
            message(STATUS "找到 Oat++ PostgreSQL 配置: ${OATPP_POSTGRESQL_CONFIG_DIR}")
            find_package(oatpp-postgresql CONFIG REQUIRED PATHS ${OATPP_POSTGRESQL_CONFIG_DIR} NO_DEFAULT_PATH)
        else()
            message(WARNING "未找到 Oat++ PostgreSQL，如需使用请安装: vcpkg install oatpp-postgresql:x64-windows")
        endif()
    else()
        message(FATAL_ERROR "无法找到 Oat++，请确保已通过 vcpkg 安装: vcpkg install oatpp:x64-windows")
    endif()
endif()

# ==================== 添加可执行文件 ====================
add_executable(ChatServer ${PROJECT_SOURCES})

# ==================== 设置包含目录 ====================
target_include_directories(ChatServer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(PLATFORM_LINUX)
    target_include_directories(ChatServer PRIVATE
        ${OATPP_INCLUDE_DIR}
    )
endif()

# ========== 添加 OpenSSL 包含目录 ==========
target_include_directories(ChatServer PRIVATE
    ${OPENSSL_INCLUDE_DIR}
)

# ========== 添加 mailio 包含目录 ==========
if(PLATFORM_LINUX)
    if(MAILIO_INCLUDE_DIR)
        target_include_directories(ChatServer PRIVATE
            ${MAILIO_INCLUDE_DIR}
        )
    endif()
endif()

# ========== 添加 redis++ 包含目录 ==========
if(PLATFORM_LINUX)
    if(REDIS_PLUS_PLUS_INCLUDE_DIR)
        target_include_directories(ChatServer PRIVATE
            ${REDIS_PLUS_PLUS_INCLUDE_DIR}
            ${HIREDIS_INCLUDE_DIR}
        )
    endif()
endif()

# ==================== 设置 bcrypt 库路径 ====================
set(BCRYPT_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib")

# ==================== 链接库 ====================
if(PLATFORM_LINUX AND NOT TARGET oatpp::oatpp)
    target_link_libraries(ChatServer PRIVATE
        ${OATPP_CORE_LIBRARY}
        pthread
        dl
    )
    if(OATPP_WEBSOCKET_LIBRARY)
        target_link_libraries(ChatServer PRIVATE ${OATPP_WEBSOCKET_LIBRARY})
    endif()
    if(OATPP_JSON_LIBRARY)
        target_link_libraries(ChatServer PRIVATE ${OATPP_JSON_LIBRARY})
    endif()
    # ========== 新增：链接 Swagger 库 ==========
    if(OATPP_SWAGGER_LIBRARY)
        target_link_libraries(ChatServer PRIVATE ${OATPP_SWAGGER_LIBRARY})
    endif()
    # ========== 新增：链接 PostgreSQL 库 ==========
    if(OATPP_POSTGRESQL_LIBRARY)
        target_link_libraries(ChatServer PRIVATE ${OATPP_POSTGRESQL_LIBRARY})
    endif()
    # ========== 链接 OpenSSL ==========
    target_link_libraries(ChatServer PRIVATE
        ${OPENSSL_LIBRARIES}
    )
    
    # ========== 链接 mailio ==========
    if(MAILIO_LIBRARY)
        target_link_libraries(ChatServer PRIVATE ${MAILIO_LIBRARY})
    endif()
    
    # ========== 链接 redis++ ==========
    if(REDIS_PLUS_PLUS_LIBRARY)
        target_link_libraries(ChatServer PRIVATE 
            ${REDIS_PLUS_PLUS_LIBRARY}
            ${HIREDIS_LIBRARY}
        )
    endif()
else()
    target_link_libraries(ChatServer PRIVATE
        oatpp::oatpp
    )
    if(TARGET oatpp::oatpp-websocket)
        target_link_libraries(ChatServer PRIVATE oatpp::oatpp-websocket)
    endif()
    if(TARGET oatpp::oatpp-parser-json)
        target_link_libraries(ChatServer PRIVATE oatpp::oatpp-parser-json)
    endif()
    # ========== 新增：链接 Swagger 库 ==========
    if(TARGET oatpp::oatpp-swagger)
        target_link_libraries(ChatServer PRIVATE oatpp::oatpp-swagger)
    endif()
    # ========== 新增：链接 PostgreSQL 库 ==========
    if(TARGET oatpp::oatpp-postgresql)
        target_link_libraries(ChatServer PRIVATE oatpp::oatpp-postgresql)
    endif()
    # ========== 链接 OpenSSL ==========
    target_link_libraries(ChatServer PRIVATE
        ${OPENSSL_LIBRARIES}
    )
    
    # ========== 链接 mailio ==========
    if(TARGET mailio::mailio)
        target_link_libraries(ChatServer PRIVATE mailio::mailio)
    endif()
    
    # ========== 链接 redis++ ==========
    if(TARGET redis++::redis++ OR TARGET redis++::redis_plus_plus)
        if(TARGET redis++::redis++)
            target_link_libraries(ChatServer PRIVATE redis++::redis++)
        else()
            target_link_libraries(ChatServer PRIVATE redis++::redis_plus_plus)
        endif()
    endif()
endif()

# ========== 链接 bcrypt 库（根据 Debug/Release 配置） ==========
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_link_libraries(ChatServer PRIVATE "${BCRYPT_LIB_DIR}/Debug/bcrypt.lib")
    message(STATUS "链接 bcrypt 库 (Debug): ${BCRYPT_LIB_DIR}/Debug/bcrypt.lib")
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_link_libraries(ChatServer PRIVATE "${BCRYPT_LIB_DIR}/Release/bcrypt.lib")
    message(STATUS "链接 bcrypt 库 (Release): ${BCRYPT_LIB_DIR}/Release/bcrypt.lib")
else()
    # 默认使用 Release 版本
    target_link_libraries(ChatServer PRIVATE "${BCRYPT_LIB_DIR}/Release/bcrypt.lib")
    message(STATUS "链接 bcrypt 库 (默认 Release): ${BCRYPT_LIB_DIR}/Release/bcrypt.lib")
endif()

# ==================== 平台特定设置 ====================
if(WIN32)
    target_compile_definitions(ChatServer PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

if(PLATFORM_LINUX)
    target_compile_options(ChatServer PRIVATE -Wall -Wextra)
    set_target_properties(ChatServer PROPERTIES
        INSTALL_RPATH "${OATPP_LIBRARY_DIR}"
        INSTALL_RPATH_USE_LINK_PATH TRUE
    )
endif()

set_target_properties(ChatServer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# 定义宏
target_compile_definitions(ChatServer PRIVATE
    OATPP_SWAGGER_RES_PATH="${OATPP_SWAGGER_RESOURCES_DIR}"
    SQLCHECK
)#Swagger

target_compile_definitions(ChatServer PRIVATE
    CONF_PATH="${CMAKE_CURRENT_SOURCE_DIR}/config.json"
)#Config

# ==================== 安装规则 ====================
install(TARGETS ChatServer
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# ==================== 调试信息 ====================
message(STATUS "========================================")
message(STATUS "配置摘要:")
message(STATUS "  平台: ${CMAKE_SYSTEM_NAME}")
message(STATUS "  构建类型: ${CMAKE_BUILD_TYPE}")
message(STATUS "  源文件数量: ${PROJECT_SOURCES}")
message(STATUS "  OpenSSL 版本: ${OPENSSL_VERSION}")
if(PLATFORM_LINUX)
    message(STATUS "  Oat++ 版本: ${OATPP_VERSION}")
    message(STATUS "  Oat++ 头文件: ${OATPP_INCLUDE_DIR}")
    message(STATUS "  Oat++ 核心库: ${OATPP_CORE_LIBRARY}")
    if(OATPP_SWAGGER_LIBRARY)
        message(STATUS "  Oat++ Swagger 库: ${OATPP_SWAGGER_LIBRARY}")
        message(STATUS "  Swagger 资源目录: ${OATPP_SWAGGER_RESOURCES_DIR}")
    endif()
    if(OATPP_POSTGRESQL_LIBRARY)
        message(STATUS "  Oat++ PostgreSQL 库: ${OATPP_POSTGRESQL_LIBRARY}")
    endif()
    if(MAILIO_LIBRARY)
        message(STATUS "  mailio 库: ${MAILIO_LIBRARY}")
    endif()
    if(REDIS_PLUS_PLUS_LIBRARY)
        message(STATUS "  redis++ 库: ${REDIS_PLUS_PLUS_LIBRARY}")
        message(STATUS "  hiredis 库: ${HIREDIS_LIBRARY}")
    endif()
elseif(PLATFORM_WINDOWS)
    if(TARGET mailio::mailio)
        message(STATUS "  mailio: 已找到")
    else()
        message(STATUS "  mailio: 未找到")
    endif()
    if(TARGET redis++::redis++)
        message(STATUS "  redis++: 已找到")
    else()
        message(STATUS "  redis++: 未找到")
    endif()
endif()
message(STATUS "========================================")