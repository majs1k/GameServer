//#include <iostream>
//#include <memory>
//
//#include "Database/Database.h"
//
//int main()
//{
//	Database db;
//
//	// 1. DB 연결
//	if (!db.Connect("tcp://127.0.0.1:3306", "root", "1234", "test"))
//	{
//		std::cout << "DB Connection Failed!" << std::endl;
//		return 1;
//	}
//
//	std::cout << "DB Connected!" << std::endl;
//
//	// 2. 테이블 생성
//	db.Execute(
//		"CREATE TABLE IF NOT EXISTS TestTable ("
//		"id INT AUTO_INCREMENT PRIMARY KEY,"
//		"name VARCHAR(50),"
//		"age INT)"
//	);
//
//	// 3. INSERT
//	db.Execute(
//		"INSERT INTO TestTable (name, age) "
//		"VALUES ('Player1', 25)"
//	);
//
//	std::cout << "INSERT Success!" << std::endl;
//
//	// 4. SELECT
//	auto query = db.ExecuteQuery(
//		"SELECT id, name, age FROM TestTable"
//	);
//
//
//	if (query.result)
//	{
//		while (query.result->next())
//		{
//			std::cout
//				<< "ID: " << query.result->getInt("id")
//				<< ", Name: " << query.result->getString("name")
//				<< ", Age: " << query.result->getInt("age")
//				<< std::endl;
//		}
//	}
//
//	// 5. UPDATE
//	db.Execute(
//		"UPDATE TestTable SET age = 30 "
//		"WHERE name = 'Player1'"
//	);
//
//	// 6. DELETE
//	db.Execute(
//		"DELETE FROM TestTable WHERE name = 'Player1'"
//	);
//
//	std::cout << "DB Test Finished!" << std::endl;
//
//	return 0;
//}
