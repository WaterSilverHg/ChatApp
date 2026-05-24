#pragma once
#include"global.h"
#include"../postgresql/AppPostgresql.hpp"
#include"../../tool/UuidIdCache.hpp"
/**
 *  Class which creates and holds Application components and registers components in oatpp::base::Environment
 *  Order of components initialization is from top to bottom
 */
class SqlComponent {
public:

    OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::postgresql::ConnectionProvider>, dbConnectionProvider)([] {
        OATPP_COMPONENT(std::shared_ptr<SettingController>, config);
        return std::make_shared<oatpp::postgresql::ConnectionProvider>(
            config->getString("postgresql_connection_string")
        );
        }());

    OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::postgresql::ConnectionPool>, dbConnectionPool)([] {
        OATPP_COMPONENT(std::shared_ptr<oatpp::postgresql::ConnectionProvider>, dbConnectionProvider);
        return oatpp::postgresql::ConnectionPool::createShared(dbConnectionProvider, 10, std::chrono::seconds(5));
        }());

    OATPP_CREATE_COMPONENT(std::shared_ptr<oatpp::orm::Executor>, dbExecutor)([] {
        OATPP_COMPONENT(std::shared_ptr<oatpp::postgresql::ConnectionPool>, dbConnectionPool);
        return std::make_shared<oatpp::postgresql::Executor>(dbConnectionPool);
        }());

    OATPP_CREATE_COMPONENT(std::shared_ptr<AppPostgresql>, myDatabaseClient)([] {
        OATPP_COMPONENT(std::shared_ptr<oatpp::orm::Executor>, dbExecutor);
        return std::make_shared<AppPostgresql>(dbExecutor);
        }());


};