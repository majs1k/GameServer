// -------------------------------------------------------------------
//
// 데이터베이스
//
// mysql c++ connector 라이브러리
// mysqlcppconn .lib .dll 필요
// 서버에 mysql 설치시 MS visual 재배포판 설치 필수
// 
// -------------------------------------------------------------------
#pragma once
#pragma comment(lib, "mysqlcppconn.lib")

#include <memory>
#include <string>

#include <mysql_driver.h>
#include <mysql_connection.h>
#include <cppconn/connection.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/exception.h>

class MySQL
{
	struct QueryResult
	{
		// 선언 순서 중요!
		std::unique_ptr<sql::Statement> statement;
		std::unique_ptr<sql::ResultSet> result;
	};

private:
	sql::mysql::MySQL_Driver* driver_ = nullptr;
	std::unique_ptr<sql::Connection> connection_;

public:

	MySQL() = default;

	~MySQL()
	{
		if (connection_)
		{
			connection_->close();
		}
	}

	void Connect(const std::string& host, const std::string& user, const std::string& password, const std::string& schema)
	{
		try
		{
			/// 일반 모드
			//driver_ = sql::mysql::get_mysql_driver_instance();
			//connection_.reset(driver_->connect(host, user, password));
			//connection_->setSchema(schema);

			/// multi statement 모드
			driver_ = sql::mysql::get_mysql_driver_instance();
			sql::ConnectOptionsMap connectionProperties;

			connectionProperties["hostName"] = host;
			connectionProperties["userName"] = user;
			connectionProperties["password"] = password;
			connectionProperties["schema"] = schema;

			// 여러 SQL 문장을 하나의 문자열로 실행 허용
			connectionProperties["CLIENT_MULTI_STATEMENTS"] = true;

			connection_.reset(driver_->connect(connectionProperties));
		}
		catch (sql::SQLException& e)
		{
			std::cout << "DB Connect Error: " << e.what() << std::endl;
			__debugbreak();
		}
	}

	// INSERT, UPDATE, DELETE, CREATE 등의 쿼리 실행
	void Execute(const std::string& query)
	{
		try
		{
			std::unique_ptr<sql::Statement> statement(connection_->createStatement());

			statement->execute(query);
		}
		catch (sql::SQLException& e)
		{
			std::cout << "DB Execute Error: " << e.what() << std::endl;
			__debugbreak();
		}
	}

	void ExecuteMulti(const std::string& query)
	{
		try
		{
			std::unique_ptr<sql::Statement> statement(connection_->createStatement());

			bool hasResult = statement->execute(query);

			while (1)
			{
				if (hasResult)
				{
					std::unique_ptr<sql::ResultSet> result(statement->getResultSet());
				}
				else if (statement->getUpdateCount() == -1)
					break;

				hasResult = statement->getMoreResults();
			}
		}
		catch (sql::SQLException& e)
		{
			std::cout << "DB ExecuteMulti Error: " << e.what() << std::endl;
			__debugbreak();
		}
	}

	// SELECT 쿼리 실행
	QueryResult ExecuteQuery(const std::string& query)
	{
		QueryResult queryResult;

		try
		{
			queryResult.statement.reset(connection_->createStatement());

			queryResult.result.reset(queryResult.statement->executeQuery(query));

			return queryResult;
		}
		catch (sql::SQLException& e)
		{
			std::cout << e.what() << std::endl;
			__debugbreak();
			return {};
		}
	}

	bool IsConnected() const
	{
		return connection_ && connection_->isValid();
	}
};
