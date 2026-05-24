#pragma once

//oatpp
#include "oatpp/core/Types.hpp"
#include "oatpp/core/macro/codegen.hpp"
#include <oatpp/core/data/mapping/type/Object.hpp>
#include <oatpp/core/base/Environment.hpp>
#include "oatpp/network/Server.hpp"
#include "oatpp-swagger/Controller.hpp"
#include "oatpp-websocket/ConnectionHandler.hpp"
#include "oatpp/web/server/HttpConnectionHandler.hpp"
#include "oatpp/web/server/HttpRouter.hpp"
#include "oatpp/network/tcp/server/ConnectionProvider.hpp"
#include "oatpp/parser/json/mapping/ObjectMapper.hpp"
#include "oatpp/core/macro/component.hpp"
#include "oatpp-swagger/Controller.hpp"
#include "oatpp-websocket/Handshaker.hpp"
#include "oatpp/web/server/api/ApiController.hpp"
#include "oatpp/network/ConnectionHandler.hpp"
#include "oatpp-websocket/WebSocket.hpp"
#include "oatpp/web/server/handler/AuthorizationHandler.hpp"
#include "oatpp/web/server/interceptor/AllowCorsGlobal.hpp"
#include"oatpp/web/mime/multipart/PartList.hpp"
#include"oatpp/web/mime/multipart/Reader.hpp"
//#include"oatpp/web/mime/multipart/PartReader.hpp"
#include "oatpp/web/mime/multipart/InMemoryDataProvider.hpp"
#include"oatpp/encoding/Url.hpp"
//#include "oatpp/web/server/AsyncHttpConnectionHandler.hpp"

//oatpp-postgresql
#include <oatpp-postgresql/orm.hpp>

//Swagger
#include "oatpp-swagger/Controller.hpp"
#include "oatpp-swagger/Resources.hpp"

//websocket
//#include <oatpp-websocket/Handshaker.hpp>
//#include <oatpp-websocket/WebSocket.hpp>

//hiredis
////#include"hiredis/hiredis.h"
#include"sw/redis++/redis++.h"
//jwt
#include"jwt-cpp/jwt.h"
#include"jwt-cpp/traits/kazuho-picojson/traits.h"
//邮件
#include"mailio/smtp.hpp"

//bcrypt
#include"bcrypt/BCrypt.hpp"

//腾讯云cos
#include"Poco/JSON/Object.h"//不加报错，神了Object找不到说是
#include "cos-cpp/cos_api.h"
#include "cos-cpp/cos_config.h"

#include<fstream>
#include <memory>
#include <iostream>
#include<stdio.h>
#include<vector>
#include<queue>
#include<random>
#include<thread>
#include<algorithm>
#include <chrono>
#include <fstream>
#include <filesystem>

#include"config/SettingController.h"

#include"Windows.h"


inline constexpr int EXPIRESIN = 3600 * 24 * 3;

//#include"fastdfs/fdfs_client.h"


//template <class T>
//class Singleton {
//public:
//	static std::shared_ptr<T> getSingleton() {
//		//std::call_once(initFlag, &Singleton::__init);
//		//return instance;
//		static std::once_flag s_flag;
//		std::call_once(s_flag, [&]() {
//			_instance = std::shared_ptr<T>(new T);
//			});
//		return _instance;
//	}
//
//
//	virtual ~Singleton() = default;
//protected:
//	Singleton() = default;
//	Singleton(const T&) = delete;
//	Singleton& operator=(const Singleton&) = delete;
//	static inline std::shared_ptr<T> _instance = nullptr;
//};