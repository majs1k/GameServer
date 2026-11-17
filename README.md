# IOCPGameServer

Tech     : C++17, MySQL

Platform : Windows / Visual Studio 2022

---

## Details

* IOCP 네트워크 라이브러리 구현
* IO / Logic / DB Thread
* Stateful Server

---

## DevLog
### 26-06-00
```text
문제 발생 -> 원인 완벽하게 파악 (디버깅, 로그, ...) -> 해결

지연삭제 -> 이터레이터로 인한 문제 방지

accept thread 분리, accept() 블로킹 호출

50byte Sleep(1) 기준 wsasend 두번 호출 vs wsasend 한번에 wsabuf 2개등록
83us / 7us
-> 당연히 시스템콜 호출이 적은 후자가 성능 좋음
```

### 26-06-05
```text
MemoryPool 클래스에서 Node를 malloc으로 할당하므로 생성자 호출 안됨 주의
ex) struct Node {int *data = nullptr;} -> malloc 시 nullptr 대입 안됨!

RinBuffer, Packet의 실제 버퍼는 반드시 buffer 메서드로 가져와야 함
RingBuffer에 Packet을 전달해서 디큐한 다음에는 Packet의 쓰기포지션 이동
```

### 26-06-06
```text
폴더명 변경시 vcxproj 파일이 업데이트가 안되어서 문제 발생 가능 -> 새 프로젝트 만들어서 옮기기

발생 가능한 이슈
1. SendPost() 내부에서 sendQueue 읽기에 락 필요? 다른스레드에서 sendQueue에 쓰기가 상황이 영향을 끼칠지??
2. 여러 스레드에서 세션맵 건들시 문제 발생 가능(세션 생성 삭제시 여러 스레드에서 접근 가능) -> 동기화 필요
3. sendPacket()은 동기화 필요. 다른 스레드에서 enqueue 가능 (게임서버니까)
```

### 26-06-07
```text
Logger에 날짜, 스레드, 로그타입 추가 (spdlog 참고)

스레드 생성하자마자 핸들 닫으면 핸들 무효화됨 주의
커널 오브젝트의 reference count와 signal 상태는 별개의 개념임

windows.h -> 반드시 winsock 밑에!! 헤더 include 하면서 순서 꼬일수 있음 주의
```

### 26-06-08
```text
SendPost()는 WSASend() 호출 전에 iocount 늘려야
다른 스레드에서 send 완료통지로 ioCount 내려서 0이 되는 상황 방지
```

### 26-06-11
```text
상대가 closesocket()시 (rst x) GQCS 성공 반환 후 numOfBytes 0 전달됨 -> 예외처리
```

### 26-06-16
```
프로파일러 단축키 입력이 씹힘 -> 여러 스레드에서 getch() 호출해서 생긴 문제
-> 키보드 입력은 메인스레드에서만 가능토록 수정
```

### 26-06-19
```
send 링버퍼를 Packet 포인터 저장하게 변경 -> 복사 생략 효과

WSASend() 호출시 wsabuf count 0인 상황 발생시 10022 에러 발생

링버퍼 enqueue시 프리사이즈 0으로 뜨는 상황...?
-> for문 탈출조건에 어떤 메서드 넣지 말고 스택변수로 넣기 & 링버퍼 크기 충분히 늘리기
```

### 26-08-18
```
Rpc 코드를 헤더파일로만 작동하도록 수정했다가 롤백 (RpcModule 코드를 사용할수 없게 되어서 불가능함)
```

### 26-08-29
```
db 정의파일 및 자동생성 코드
```

### 26-10-30
```
락프리 노드풀 전역으로 변경
```

### 26-10-31
```
isLogin, isInitSector 등 로직 확인용 변수는 따로 관리하는 것이 실수가 적음
if(sectorX < 0) 이런식으로 관리하는 방식에 비하여...

수신 버퍼 최적화 코드 수정
로그 flush 적용
rpc 인자 레퍼런스 적용
sessionId vector 오버로딩 적용
중복 로그인 처리
sessionId , Player* 로 해시맵 수정
```

### 26-11-09
```
lock_guard + {}

락프리큐 수정
```
