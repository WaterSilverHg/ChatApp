#pragma once
#include"global.h"
#include"server/postgresql/AppPostgresql.hpp"
/**
 *  Class which creates and holds Application components and registers components in oatpp::base::Environment
 *  Order of components initialization is from top to bottom
 */
class SqlComponent {
public:

 // 1. ע�� ConnectionProvider
    OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::postgresql::ConnectionProvider>, dbConnectionProvider)([] {
        OATPP_COMPONENT(std::shared_ptr<SettingController>, config);
        return std::make_shared<oatpp::postgresql::ConnectionProvider>(
            config->getString("postgresql_connection_string")
        );
        }());

    // 2. ע�� ConnectionPool
    OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::postgresql::ConnectionPool>, dbConnectionPool)([] {
        OATPP_COMPONENT(std::shared_ptr<oatpp::postgresql::ConnectionProvider>, dbConnectionProvider);
        return oatpp::postgresql::ConnectionPool::createShared(dbConnectionProvider, 10, std::chrono::seconds(5));
        }());

    // 3. ע�� Executor
    OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::orm::Executor>, dbExecutor)([] {
        OATPP_COMPONENT(std::shared_ptr<oatpp::postgresql::ConnectionPool>, dbConnectionPool);
        return std::make_shared<oatpp::postgresql::Executor>(dbConnectionPool);
        }());

    // 4. Register AppPostgresql component with Executor
    OATPP_CREATE_COMPONENT(std::shared_ptr<AppPostgresql>, myDatabaseClient)([] {
        OATPP_COMPONENT(std::shared_ptr<oatpp::orm::Executor>, dbExecutor);
        return std::make_shared<AppPostgresql>(dbExecutor);
        }());
};