#include "LoginServer.h"
#include "RpcModule.h"
#include "RPC/RpcProxy.h"
#include "Utils/Logger.h"
#include "Utils/Profiler.h"
#include "Utils/CommonUtility.h"


bool LoginServer::CS_ReqLogin(uint64_t sessionId, int64_t userId, SessionKey& sessionKey)
{
	/// + 특정 플랫폼 API로 조회 -> 인증 성공 -> 세션키 발급(sessionKey)

	Email email;
	Nickname nickname;
	IpAddress gameServerIpAddress = (IpAddress)u8"1.2.3.4";
	unsigned short gameServerPort = 0;
	IpAddress chatServerIpAddress;
	unsigned short chatServerPort = 0;

	/// userId로 테이블 조회
	auto query = mysql_.ExecuteQuery(
		"SELECT email, nickname FROM users WHERE user_id = "
		+ std::to_string(userId)
	);

	/// 조회 결과 없음
	if (!query.result->next())
	{
		proxy_.SC_ResLogin(sessionId, 0, en_PACKET_CS_LOGIN_RES_LOGIN::dfLOGIN_STATUS_FAIL, email, nickname,
			gameServerIpAddress, gameServerPort, chatServerIpAddress, chatServerPort);
		return true;
	}

	//bool success = redis_.Set("session:" + std::to_string(userId), "1", true, 60 * 60);

	///// Key가 이미 존재해서 저장 x -> 중복 로그인
	//if (!success)
	//{
	//	/// 접속 세션 삭제
	//	//redis->Del("session:" + std::to_string(userId));

	//	proxy_.SC_ResLogin(sessionId, 0, en_PACKET_CS_LOGIN_RES_LOGIN::dfLOGIN_STATUS_GAME, email, nickname,
	//		gameServerIpAddress, gameServerPort, chatServerIpAddress, chatServerPort);

	//	/// + 게임 서버로 알림

	//	return true;
	//}

	/// 유효기간 60초
	redis_.Set("token:" + std::to_string(userId), sessionKey.sessionKey_, false, 60);

	/// email, nickname -> std::string -> utf-16 (복사 생성자)
	email = query.result->getString("email").asStdString();
	nickname = query.result->getString("nickname").asStdString();

	mysql_.Execute(
		"UPDATE users SET last_login_at = CURRENT_TIMESTAMP WHERE user_id = " + std::to_string(userId)
	);

	std::string chatServerIp;
	std::string remoteIp = GetSessionIp(sessionId);

	if (remoteIp == "127.0.0.1")
	{
		chatServerIp = u8"127.0.0.1";
	}
	else if (remoteIp == "10.0.1.1")
	{
		chatServerIp = u8"10.0.1.2";
	}
	else if (remoteIp == "10.0.2.1")
	{
		chatServerIp = u8"10.0.2.2";
	}

	chatServerIpAddress = chatServerIp;
	chatServerPort = 12001;

	/// lvalue가 와야함
	/// 로그인 응답
	proxy_.SC_ResLogin(sessionId, userId, (unsigned char)en_PACKET_CS_LOGIN_RES_LOGIN::dfLOGIN_STATUS_OK, email, nickname,
		gameServerIpAddress, gameServerPort, chatServerIpAddress, chatServerPort);

	return true;
}
