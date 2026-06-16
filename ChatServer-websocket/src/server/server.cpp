#include"global.h"
#include"server/component/AppComponent.cpp"
#include"server/component/SqlComponent.cpp"
#include"server/component/OtherComponent.cpp"
//#include"controller/AuthController.hpp"
//#include"controller/FriendController.hpp"
//#include "controller/MessageController.hpp"
//#include "controller/GroupController.hpp"
//#include "controller/ConversationController.hpp"
//#include "controller/UserStatusController.hpp"
#include "controller/WebSocketController.hpp"
#include "../redis/RedisPubSubManager.hpp"

static void run() {

	OtherComponent othercomponent;
	SqlComponent sqlcomponent;
	AppComponent appcomponent;

	OATPP_COMPONENT(std::shared_ptr<oatpp::web::server::HttpRouter>, router);

	OATPP_COMPONENT(std::shared_ptr<oatpp::data::mapping::ObjectMapper>, objectMapper);

	//router->addController(AuthController::createShared());
	//router->addController(FriendController::createShared());
	//router->addController(MessageController::createShared());
	//router->addController(GroupController::createShared());
	//router->addController(ConversationController::createShared());
	//router->addController(StatusController::createShared());
	router->addController(WebSocketController::createShared(objectMapper));

#ifdef SWAGGER
	oatpp::web::server::api::Endpoints docEndpoints;
	docEndpoints.append(router->addController(AuthController::createShared())->getEndpoints());
	//docEndpoints.append(router->addController(FriendController::createShared())->getEndpoints());
	//docEndpoints.append(router->addController(MessageController::createShared())->getEndpoints());
	//docEndpoints.append(router->addController(GroupController::createShared())->getEndpoints());
	//docEndpoints.append(router->addController(ConversationController::createShared())->getEndpoints());
	//docEndpoints.append(router->addController(StatusController::createShared())->getEndpoints());
	auto swaggerController = oatpp::swagger::Controller::createShared(docEndpoints);
	router->addController(swaggerController);
#endif

	// 启动 Redis Pub/Sub 订阅者
	OATPP_COMPONENT(std::shared_ptr<RedisPubSubManager>, pubSubManager);
	pubSubManager->start();
	OATPP_LOGI("MyApp", "Redis Pub/Sub subscriber started");

	OATPP_COMPONENT(std::shared_ptr<oatpp::network::ConnectionHandler>, httpConnectionHandler, "http");
	//OATPP_COMPONENT(std::shared_ptr<oatpp::websocket::AsyncConnectionHandler>, httpConnectionHandler, "ws-server-handler");
	OATPP_COMPONENT(std::shared_ptr<oatpp::network::ServerConnectionProvider>, connectionProvider);

	oatpp::network::Server server(connectionProvider, httpConnectionHandler);

	OATPP_LOGI("MyApp", "Server running on port %s", connectionProvider->getProperty("port").getData());

	server.run();

}