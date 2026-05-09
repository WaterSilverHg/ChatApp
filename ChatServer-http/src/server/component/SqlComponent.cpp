#pragma once
#include"global.h"
#include"../postgresql/AppClient.hpp"
/**
 *  Class which creates and holds Application components and registers components in oatpp::base::Environment
 *  Order of components initialization is from top to bottom
 */
class SqlComponent {
public:

 // 1. ×¢²á ConnectionProvider
    OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::postgresql::ConnectionProvider>, dbConnectionProvider)([] {
        OATPP_COMPONENT(std::shared_ptr<SettingController>, config);
        return std::make_shared<oatpp::postgresql::ConnectionProvider>(
            config->getString("postgresql_connection_string")
        );
        }());

    // 2. ×¢²á ConnectionPool
    OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::postgresql::ConnectionPool>, dbConnectionPool)([] {
        OATPP_COMPONENT(std::shared_ptr<oatpp::postgresql::ConnectionProvider>, dbConnectionProvider);
        return oatpp::postgresql::ConnectionPool::createShared(dbConnectionProvider, 10, std::chrono::seconds(5));
        }());

    // 3. ×¢²á Executor
    OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::orm::Executor>, dbExecutor)([] {
        OATPP_COMPONENT(std::shared_ptr<oatpp::postgresql::ConnectionPool>, dbConnectionPool);
        return std::make_shared<oatpp::postgresql::Executor>(dbConnectionPool);
        }());

    // 4. ×¢²á AppClient£¨ÒÀÀµ Executor£©
    OATPP_CREATE_COMPONENT(std::shared_ptr<AppClient>, myDatabaseClient)([] {
        OATPP_COMPONENT(std::shared_ptr<oatpp::orm::Executor>, dbExecutor);
        return std::make_shared<AppClient>(dbExecutor);
        }());
};