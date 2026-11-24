#include "LoginServer.h"
#include "RpcModule.h"
#include "RPC/RpcProxy.h"
#include "Utils/Logger.h"
#include "Utils/Profiler.h"


bool LoginServer::CS_ReqLogin(uint64_t sessionId, int64_t userId, SessionKey& sessionKey)
{
	/// sessionKey를 특정 플랫폼 API로 조회 -> 인증 성공

	std::string emailString;
	std::string nicknameString;

	/// userId로 테이블 조회
	auto query = db_.ExecuteQuery(
		"SELECT email, nickname, status FROM users WHERE user_id = "
		+ std::to_string(userId)
	);

	if (!query.result)
	{
		// 쿼리 실행 실패
		__debugbreak();
		return;
	}

	if (!query.result->next())
	{
		// 조회 결과 없음
		/// 조회 실패



		return;
	}

	emailString = query.result->getString("email");
	nicknameString = query.result->getString("nickname");

	/// email, nickname ->  utf-16으로 변환
	Email email = emailString;
	Nickname nickname = nicknameString;




	bool success = redis_.Set("user:1001", "player1", true, 60);

	if (success)
	{
		// Key가 없어서 새롭게 저장됨
		/// 세션키 redis로 저장

	}
	else
	{
		// Key가 이미 존재해서 저장되지 않음
		/// 중복 로그인

	}

	db_.Execute(
		"UPDATE users SET last_login_at = CURRENT_TIMESTAMP WHERE user_id = ?" + std::to_string(userId)
	);


	IpAddress gameServerIp = L"10.0.0.1";
	unsigned short gameServerPort = 1234;

	IpAddress chatServerIp = L"10.0.0.1";
	unsigned short chatServerPort = 1234;


	/// 로그인 응답 send
	SC_ResLogin(sessionId, userId, (unsigned char)status, email, nickname,
		sessionKey, gameServerIp, gameServerPort, chatServerIp, chatServerPort);
}
