cmake_minimum_required(VERSION 3.16)
project(ChatServer VERSION 1.0.0 LANGUAGES CXX)

# 是否启用Swagger（如需启用，取消注释）
# add_definitions(-DSWAGGER)

add_definitions(-DNOMINMAX) # 解决 picojson.h 的报错问题

if(MSVC)
    add_compile_options(/bigobj)
endif()

if(MSVC)
    add_compile_options(/utf-8)
else()
    add_compile_options(-finput-charset=UTF-8)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(WIN32)
    message(STATUS "检测到 Windows 平台")
    set(PLATFORM_WINDOWS TRUE)
elseif(UNIX AND NOT APPLE)
    message(STATUS "检测到 Linux 平台")
    set(PLATFORM_LINUX TRUE)
endif()

# ==================== 依赖查找（均基于 vcpkg） ====================

# 1. Poco
find_package(Poco REQUIRED COMPONENTS Foundation JSON XML Util Net Crypto NetSSL CONFIG)
if(Poco_FOUND)
    message(STATUS "✅ Poco ${Poco_VERSION}")
    set(POCO_LIBRARIES Poco::Foundation Poco::JSON Poco::XML Poco::Util Poco::Net Poco::Crypto Poco::NetSSL)
else()
    message(FATAL_ERROR "❌ Poco 未找到")
endif()

# 2. OpenSSL
find_package(OpenSSL REQUIRED)
message(STATUS "✅ OpenSSL ${OPENSSL_VERSION}")

# 3. Oat++ 及模块
find_package(oatpp REQUIRED CONFIG)
message(STATUS "✅ oatpp")
find_package(oatpp-websocket CONFIG)
if(oatpp-websocket_FOUND)
    message(STATUS "✅ oatpp-websocket")
else()
    message(WARNING "⚠️ oatpp-websocket 未找到")
endif()
find_package(oatpp-swagger CONFIG)
if(oatpp-swagger_FOUND)
    message(STATUS "✅ oatpp-swagger")
else()
    message(WARNING "⚠️ oatpp-swagger 未找到")
endif()
find_package(oatpp-postgresql CONFIG)
if(oatpp-postgresql_FOUND)
    message(STATUS "✅ oatpp-postgresql")
else()
    message(WARNING "⚠️ oatpp-postgresql 未找到")
endif()

# 4. mailio
find_package(mailio CONFIG)
if(mailio_FOUND)
    message(STATUS "✅ mailio")
else()
    message(WARNING "⚠️ mailio 未找到")
endif()

# 5. redis++（手动设置，因为 vcpkg 的 cmake 目标不稳定）
set(REDIS_PP_ROOT /opt/vcpkg/installed/x64-linux)
set(REDIS_PP_INCLUDE_DIR ${REDIS_PP_ROOT}/include)
set(REDIS_PP_LIB_DIR ${REDIS_PP_ROOT}/lib)
find_library(REDIS_PP_LIBRARY NAMES redis++ libredis++.a PATHS ${REDIS_PP_LIB_DIR} NO_DEFAULT_PATH)
find_library(HIREDIS_LIBRARY NAMES hiredis libhiredis.a PATHS ${REDIS_PP_LIB_DIR} NO_DEFAULT_PATH)
if(REDIS_PP_LIBRARY AND HIREDIS_LIBRARY)
    message(STATUS "✅ redis++ 手动定位成功")
    set(REDIS_PP_LIBS ${REDIS_PP_LIBRARY} ${HIREDIS_LIBRARY})
    set(REDIS_PP_FOUND TRUE)
else()
    message(WARNING "⚠️ redis++ 未找到，Redis 功能将不可用")
    set(REDIS_PP_FOUND FALSE)
endif()

# 6. COS SDK
set(COS_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lib")
set(COS_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include/cos-cpp")
find_library(COS_LIBRARY NAMES cossdk libcossdk.so libcossdk.a PATHS ${COS_LIB_DIR} PATH_SUFFIXES linux linux-x64 x64 REQUIRED)
message(STATUS "✅ COS 库: ${COS_LIBRARY}")

# 7. bcrypt（仅 Windows）
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

# ==================== 包含目录（关键！） ====================
target_include_directories(ChatServer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${COS_INCLUDE_DIR}
    # 显式加入 vcpkg 所有头文件目录
    /opt/vcpkg/installed/x64-linux/include
    # redis++ 头文件目录（如果找到）
    ${REDIS_PP_INCLUDE_DIR}
    /opt/vcpkg/installed/x64-linux/include/oatpp-1.3.0
)

# ==================== 链接库 ====================
target_link_libraries(ChatServer PRIVATE
    # Oat++
    oatpp::oatpp
    $<$<BOOL:${oatpp-websocket_FOUND}>:oatpp::oatpp-websocket>
    $<$<BOOL:${oatpp-swagger_FOUND}>:oatpp::oatpp-swagger>
    $<$<BOOL:${oatpp-postgresql_FOUND}>:oatpp::oatpp-postgresql>
    # Poco + OpenSSL
    ${POCO_LIBRARIES}
    OpenSSL::SSL OpenSSL::Crypto
    # mailio
    $<$<BOOL:${mailio_FOUND}>:mailio::mailio>
    # redis++（手动定位的库文件）
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

# ==================== 平台设置 ====================
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
set_target_properties(ChatServer PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

# ==================== 宏定义 ====================
if(oatpp-swagger_FOUND)
    set(OATPP_SWAGGER_RESOURCES_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include/res")
    target_compile_definitions(ChatServer PRIVATE
        OATPP_SWAGGER_RES_PATH="${OATPP_SWAGGER_RESOURCES_DIR}"
        SQLCHECK
    )
endif()
target_compile_definitions(ChatServer PRIVATE CONF_PATH="${CMAKE_CURRENT_SOURCE_DIR}/config.json")

install(TARGETS ChatServer RUNTIME DESTINATION bin)

# ==================== 调试打印 ====================
get_target_property(CHAT_INCLUDE_DIRS ChatServer INCLUDE_DIRECTORIES)
message(STATUS "🔍 ChatServer 最终包含目录: ${CHAT_INCLUDE_DIRS}")
message(STATUS "========================================")