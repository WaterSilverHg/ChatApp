#include"global.h"
#include"server/component/AppComponent.cpp"
#include"server/component/SqlComponent.cpp"
#include"server/component/OtherComponent.cpp"
#include"controller/AuthController.hpp"
#include"controller/FriendController.hpp"
#include "controller/MessageController.hpp"
#include "controller/GroupController.hpp"
#include "controller/FileController.hpp"
#include "controller/ConversationController.hpp"
//#include "controller/NotificationController.hpp"
#include "controller/UserStatusController.hpp"
//#include "controller/WebSocketController.hpp"

static void run() {

	/* Register Components in scope of run() method */
	OtherComponent othercomponent;
	AppComponent appcomponent;
	SqlComponent sqlcomponent;

	//components.addSecurityScheme("api_key",
	//	oatpp::swagger::SecuritySchemeInfo{ "apiKey", "query", "api_key" });

	/* Get router component */
	OATPP_COMPONENT(std::shared_ptr<oatpp::web::server::HttpRouter>, router);

	/* Create MyController and add all of its endpoints to router */


	//router->addController(std::make_shared<testController>());
	OATPP_COMPONENT(std::shared_ptr<oatpp::data::mapping::ObjectMapper>, objectMapper);
	OATPP_COMPONENT(std::shared_ptr<oatpp::orm::Executor>, executor);
	OATPP_COMPONENT(std::shared_ptr<AppRedis>, redis);
	// OATPP_COMPONENT(std::shared_ptr<AppEmail>, email);
	//OATPP_COMPONENT(std::shared_ptr<AppWebSocket>, websocket);



#ifdef SWAGGER
	oatpp::web::server::api::Endpoints docEndpoints;
	docEndpoints.append(router->addController(AuthController::createShared())->getEndpoints());
	docEndpoints.append(router->addController(FriendController::createShared())->getEndpoints());
	docEndpoints.append(router->addController(MessageController::createShared())->getEndpoints());
	docEndpoints.append(router->addController(GroupController::createShared())->getEndpoints());
	docEndpoints.append(router->addController(FileController::createShared())->getEndpoints());
	docEndpoints.append(router->addController(ConversationController::createShared())->getEndpoints());
	//docEndpoints.append(router->addController(NotificationController::createShared())->getEndpoints());
	docEndpoints.append(router->addController(UserStatusController::createShared())->getEndpoints());
	//docEndpoints.append(NotificationController::createShared()->getEndpoints());
	docEndpoints.append(UserStatusController::createShared()->getEndpoints());



	auto swaggerController = oatpp::swagger::Controller::createShared(docEndpoints);
	router->addController(swaggerController);
#else
	router->addController(AuthController::createShared());
	router->addController(FriendController::createShared());
	router->addController(MessageController::createShared());
	router->addController(GroupController::createShared());
	router->addController(FileController::createShared());
	router->addController(ConversationController::createShared());
	//router->addController(NotificationController::createShared());
	router->addController(UserStatusController::createShared());
#endif

	/* Get connection handler component */
	OATPP_COMPONENT(std::shared_ptr<oatpp::network::ConnectionHandler>, connectionHandler, "http");

	/* Get connection provider component */
	OATPP_COMPONENT(std::shared_ptr<oatpp::network::ServerConnectionProvider>, connectionProvider);

	/* Create server which takes provided TCP connections and passes them to HTTP connection handler */
	oatpp::network::Server server(connectionProvider, connectionHandler);

	/* Priny info about server port */
	OATPP_LOGI("MyApp", "Server running on port %s", connectionProvider->getProperty("port").getData());

	/* Run server */
	server.run();

}