#include "Database/MySQL.h"
#include "Utils/ConfigLoader.h"

#define CONFIG_FILENAME			L"LoginConfig.json"

int main()
{
	ConfigLoader::Instance().Load(CONFIG_FILENAME);

	MySQL mysql;

	mysql.Connect(config.dbHost, config.dbUser, config.dbPassword, config.dbSchema);

	std::string query;

	query =
		"INSERT INTO users (user_id, email, nickname) VALUES ";

	for (int idx = 1; idx <= 100'000; idx++)
	{
		if (idx > 1)
			query += ", ";

		query += "("
			+ std::to_string(idx)
			+ ", 'ID_" + std::to_string(idx)
			+ "', 'NICK_" + std::to_string(idx)
			+ "')";
	}

	mysql.Execute(query);
}
