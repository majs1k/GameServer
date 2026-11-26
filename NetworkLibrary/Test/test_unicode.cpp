#include <iostream>
#include <string>
#include <Windows.h>

#include "MySQL.h"

std::wstring StringToWString(const std::string& str, UINT codePage = CP_UTF8)
{
	if (str.empty())
		return L"";

	int len = MultiByteToWideChar(codePage, 0, str.data(), static_cast<int>(str.size()), nullptr, 0);

	if (len <= 0)
		return L"";

	std::wstring wstr(len, L'\0');

	MultiByteToWideChar(codePage, 0, str.data(), static_cast<int>(str.size()), &wstr[0], len);
	return wstr;
}

std::string WStringToString(const std::wstring& wstr, UINT codePage = CP_UTF8)
{
	if (wstr.empty())
		return "";

	int len = WideCharToMultiByte(codePage, 0, wstr.data(), static_cast<int>(wstr.size()), nullptr, 0, nullptr, nullptr);

	if (len <= 0)
		return "";

	std::string str(len, '\0');

	WideCharToMultiByte(codePage, 0, wstr.data(), static_cast<int>(wstr.size()), str.data(), len, nullptr, nullptr);
	return str;
}

int main()
{
	setlocale(LC_ALL, "");

	MySQL mysql;

	// 1. mysql 연결
	mysql.Connect("tcp://127.0.0.1:3306", "root", "1234", "test");
	std::cout << "mysql Connected!" << std::endl;

	// 2. 테이블 생성
	//mysql.Execute(
	//	"CREATE TABLE IF NOT EXISTS TestTable ("
	//	"id INT AUTO_INCREMENT PRIMARY KEY,"
	//	"name VARCHAR(50),"
	//	"age INT)"
	//);

	mysql.Execute(
		"CREATE TABLE IF NOT EXISTS TestTable ("
		"id INT AUTO_INCREMENT PRIMARY KEY,"
		"name VARCHAR(50),"
		"age INT)"
		//" CHARACTER SET utf8mb4"
		//" COLLATE utf8mb4_unicode_ci"
	);

	/// 반드시 utf-8로 저장해야함
	std::string s = u8"한";

	//3. INSERT
	mysql.Execute(
		"INSERT INTO TestTable (name, age) "
		"VALUES ('" + s + "', 25)"
	);

	//mysql.Execute(
	//	"INSERT INTO TestTable (name, age) "
	//	"VALUES ('한', 25)"
	//);

	std::cout << "INSERT Success!" << std::endl;

	// 4. SELECT
	auto query = mysql.ExecuteQuery(
		"SELECT id, name, age FROM TestTable"
	);

	std::string str;

	if (query.result)
	{
		while (query.result->next())
		{
			str = query.result->getString("name");
		}
	}

	///utf-8 한글은 출력 불가. utf-16으로 변환해야 콘솔 출력 가능
	//std::cout << str;
	std::wstring wstr = StringToWString(str);
	std::wcout << wstr;

	//std::string s = WStringToString(wstr);
	//std::cout << s << std::endl;
}
