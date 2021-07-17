// 플랫폼: VS2010

#pragma comment(lib, "ws2_32")
#include <winsock2.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <Windows.h>
#include <process.h>
#include "resource.h"

#define SERVERIP   "127.0.0.1"
#define SERVERPORT 9000
#define BUFSIZE    512
#define NAME_SIZE	20

unsigned WINAPI SendMsg(void* arg);//쓰레드 전송함수
unsigned WINAPI RecvMsg(void* arg);//쓰레드 수신함수


BOOL CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM); // 대화상자 프로시저

void DisplayText(char *fmt, ...); // 편집 컨트롤 출력 함수
void err_quit(char *msg); // 오류 출력 함수
void err_display(char *msg); // 오류 출력 함수
int recvn(SOCKET s, char *buf, int len, int flags); // 사용자 정의 데이터 수신 함수
DWORD WINAPI ClientMain(LPVOID arg); // 소켓 통신 스레드 함수

char buf[BUFSIZE+1]; // 데이터 송수신 버퍼
char name[NAME_SIZE]="[DEFAULT]"; // 이름 저장

HANDLE hReadEvent, hWriteEvent; // 이벤트
HWND hSendButton; // 보내기 버튼
HWND hEdit1, hEdit2; // 편집 컨트롤

struct inf { // 클라이언트의 정보를 저장할 구조체
	char inputName[20]; // 이름
	char room[2]; // 방 번호
};
struct inf inf;
int namecheck;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow)
{
	// 이벤트 생성
	hReadEvent = CreateEvent(NULL, FALSE, TRUE, NULL);
	if(hReadEvent == NULL) return 1;
	hWriteEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if(hWriteEvent == NULL) return 1;

	// 소켓 통신 스레드 생성
	CreateThread(NULL, 0, ClientMain, NULL, 0, NULL);

	// 대화상자 생성
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DlgProc);

	// 이벤트 제거
	CloseHandle(hReadEvent);
	CloseHandle(hWriteEvent);

	return 0;
}

// 대화상자 프로시저
BOOL CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg){
	case WM_INITDIALOG:
		hEdit1 = GetDlgItem(hDlg, IDC_EDIT3);
		hEdit2 = GetDlgItem(hDlg, IDC_EDIT4);
		hSendButton = GetDlgItem(hDlg, IDOK);
		SendMessage(hEdit1, EM_SETLIMITTEXT, BUFSIZE, 0);
		return TRUE;
	case WM_COMMAND:
		switch(LOWORD(wParam)){
		case IDOK:
			EnableWindow(hSendButton, FALSE); // 보내기 버튼 비활성화
			WaitForSingleObject(hReadEvent, INFINITE); // 읽기 완료 기다리기
			GetDlgItemText(hDlg, IDC_EDIT3, buf, BUFSIZE+1);
			SetEvent(hWriteEvent); // 쓰기 완료 알리기
			SetFocus(hEdit1);
			SendMessage(hEdit1, EM_SETSEL, 0, -1);
			return TRUE;
		case IDCANCEL:
			EndDialog(hDlg, IDCANCEL);
			return TRUE;
		}
		return FALSE;
	}
	return FALSE;
}

// 편집 컨트롤 출력 함수
void DisplayText(char *fmt, ...)
{
	va_list arg;
	va_start(arg, fmt);

	char cbuf[BUFSIZE+256];
	vsprintf(cbuf, fmt, arg);

	int nLength = GetWindowTextLength(hEdit2);
	SendMessage(hEdit2, EM_SETSEL, nLength, nLength);
	SendMessage(hEdit2, EM_REPLACESEL, FALSE, (LPARAM)cbuf);

	va_end(arg);
}

// 소켓 함수 오류 출력 후 종료
void err_quit(char *msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	MessageBox(NULL, (LPCTSTR)lpMsgBuf, msg, MB_ICONERROR);
	LocalFree(lpMsgBuf);
	exit(1);
}

// 소켓 함수 오류 출력
void err_display(char *msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	DisplayText("[%s] %s", msg, (char *)lpMsgBuf);
	LocalFree(lpMsgBuf);
}

// 사용자 정의 데이터 수신 함수
int recvn(SOCKET s, char *buf, int len, int flags)
{
	int received;
	char *ptr = buf;
	int left = len;

	while(left > 0){
		received = recv(s, ptr, left, flags);
		if(received == SOCKET_ERROR)
			return SOCKET_ERROR;
		else if(received == 0)
			break;
		left -= received;
		ptr += received;
	}

	return (len - left);
}

void ErrorHandling(char* msg){
	fputs(msg,stderr);
	fputc('\n',stderr);
	exit(1);
}

// IP 확인하는 함수
BOOL IPcheck(char *str) {
	int cnt = 0;
	int num;
	int i;
	int temp[4];
	// '.' 를 기준으로 입력받은 문자열을 나눈다
	char *ptr = strtok(str, ".");
	
	while (ptr != NULL) {
		// 나눈 문자열이 숫자인지 확인한다
		if (atoi(ptr) || atoi(ptr) == 0) {
			if (cnt > 3)
				return 0;
			num = atoi(ptr);
			temp[cnt] = num;
			cnt++;
		}
		// 숫자가 아니면 0 반환
		else
			return 0;
		ptr = strtok(NULL, ".");
	}
	if (cnt != 4)
		return 0;
	// class D 범위를 확인
	else if (temp[0] >= 0 && temp[0] <= 255 && 
			 temp[1] >= 0 && temp[1] <= 255 && 
			 temp[2] >= 0 && temp[2] <= 255 && 
			 temp[3] >= 0 && temp[3] <= 255)
		return 1;
	else 
		return 0;
}

// PORT 번호 확인하는 함수
BOOL PORTcheck(char *str) {
	if (atoi(str))
		if(atoi(str) >= 0 && atoi(str) <= 65535)
			return 1;
		else
			return 0;
	else
		return 0;
}


// TCP 클라이언트 시작 부분
DWORD WINAPI ClientMain(LPVOID arg)
{
	WSADATA wsaData;
	SOCKET sock;
	SOCKADDR_IN serverAddr;
	HANDLE sendThread,recvThread;

	char myIp[100]; // Ip 주소
	char port[100]; // 서버 port

	
	// IP 입력
	while (1) {
		DisplayText("[TCP 클라이언트] 서버 IP를 입력해주세요.\n");
		WaitForSingleObject(hWriteEvent, INFINITE);
		if(strlen(buf) == 0){
			EnableWindow(hSendButton, TRUE); // 보내기 버튼 활성화
			SetEvent(hReadEvent); // 읽기 완료 알리기
		}
		EnableWindow(hSendButton, TRUE); // 보내기 버튼 활성화
		SetEvent(hReadEvent); // 읽기 완료 알리기
		strcpy(myIp, buf);
		if (IPcheck(buf))
			break;
	}
	DisplayText("[TCP 클라이언트] 서버 IP: %s.\n", myIp);

	// port 입력
	while (1) {
		DisplayText("[TCP 클라이언트] 서버 PORT를 입력해주세요.\n");
		WaitForSingleObject(hWriteEvent, INFINITE);
		if(strlen(buf) == 0){
				EnableWindow(hSendButton, TRUE); // 보내기 버튼 활성화
				SetEvent(hReadEvent); // 읽기 완료 알리기
		}
		EnableWindow(hSendButton, TRUE); // 보내기 버튼 활성화
		SetEvent(hReadEvent); // 읽기 완료 알리기
		if (PORTcheck(buf))
			break;
	}
	strcpy(port, buf);
	DisplayText("[TCP 클라이언트] port: %s.\n", port);
	
	// 이름 입력
	DisplayText("[TCP 클라이언트] 이름를 입력해주세요.\n");
	WaitForSingleObject(hWriteEvent, INFINITE);
	if(strlen(buf) == 0){
			EnableWindow(hSendButton, TRUE); // 보내기 버튼 활성화
			SetEvent(hReadEvent); // 읽기 완료 알리기
	}
	EnableWindow(hSendButton, TRUE); // 보내기 버튼 활성화
	SetEvent(hReadEvent); // 읽기 완료 알리기
	buf[strlen(buf)] ='\0';
	strcpy(inf.inputName, buf);
	DisplayText("[TCP 클라이언트] name: %s.\n", inf.inputName);

	// 방번호 입력
	while (1) {
		DisplayText("[TCP 클라이언트] 방번호를 입력해주세요(1 or 2).\n");
		WaitForSingleObject(hWriteEvent, INFINITE);
		if(strlen(buf) == 0){
				EnableWindow(hSendButton, TRUE); // 보내기 버튼 활성화
				SetEvent(hReadEvent); // 읽기 완료 알리기
		}
		EnableWindow(hSendButton, TRUE); // 보내기 버튼 활성화
		SetEvent(hReadEvent); // 읽기 완료 알리기
		if (!strcmp(buf, "1") || !strcmp(buf, "2"))
			break;
	}
	strcpy(inf.room, buf);
	DisplayText("[TCP 클라이언트] room: %s.\n", inf.room);

	DisplayText("[TCP 클라이언트] 채팅사용자: Display\n");
	DisplayText("[TCP 클라이언트] 이름 변경: Change\n");
	DisplayText("[TCP 클라이언트] 종료: quit\n");
	DisplayText("[TCP 클라이언트] 종료시 반드시 quit으로 종료해주세요\n");
	DisplayText("[TCP 클라이언트] %s번 방입니다.\n", inf.room);

	if(WSAStartup(MAKEWORD(2,2),&wsaData)!=0)// 윈도우 소켓을 사용한다고 운영체제에 알림
		ErrorHandling("WSAStartup() error!");

	sprintf(name,"[%s]",inf.inputName);
	sock=socket(PF_INET,SOCK_STREAM,0);//소켓을 하나 생성한다.

	memset(&serverAddr,0,sizeof(serverAddr));
	serverAddr.sin_family=AF_INET;
	serverAddr.sin_addr.s_addr=inet_addr(myIp);
	serverAddr.sin_port=htons(atoi(port));

	if(connect(sock,(SOCKADDR*)&serverAddr,sizeof(serverAddr))==SOCKET_ERROR)//서버에 접속한다.
		ErrorHandling("connect() error");

	send(sock, (char *)&inf, sizeof(inf), 0);


	sendThread=(HANDLE)_beginthreadex(NULL,0,SendMsg,(void*)&sock,0,NULL);//메시지 전송용 쓰레드가 실행된다.
	recvThread=(HANDLE)_beginthreadex(NULL,0,RecvMsg,(void*)&sock,0,NULL);//메시지 수신용 쓰레드가 실행된다.

	WaitForSingleObject(sendThread,INFINITE);//전송용 쓰레드가 중지될때까지 기다린다./
	WaitForSingleObject(recvThread,INFINITE);//수신용 쓰레드가 중지될때까지 기다린다.

	//클라이언트가 종료를 시도한다면 이줄 아래가 실행된다.
	closesocket(sock);//소켓을 종료한다.
	WSACleanup();//윈도우 소켓 사용중지를 운영체제에 알린다.

	return 0;
}

unsigned WINAPI SendMsg(void* arg){//전송용 쓰레드함수
	SOCKET sock=*((SOCKET*)arg);//서버용 소켓을 전달한다.
	char nameMsg[NAME_SIZE+BUFSIZE];
	int len;
	while(1){//반복
		WaitForSingleObject(hWriteEvent, INFINITE); // 쓰기 완료 기다리기

		// 문자열 길이가 0이면 보내지 않음
		if(strlen(buf) == 0){
			EnableWindow(hSendButton, TRUE); // 보내기 버튼 활성화
			SetEvent(hReadEvent); // 읽기 완료 알리기
			continue;
		}
		len = strlen(buf);
		buf[len]='\0';
		if(!strcmp(buf,"quit")){//quit를 입력하면 종료한다.
			send(sock,"q",1,0);//q를 서버에게 전송한다.
		}
		else if(!strcmp(buf, "Display")) { // Display 입력시 사용자 정보 출력
			send(sock, "D", 1, 0); // D를 서버에게 전송
		}
		else if(!strcmp(buf, "Change")) { // Change 입력시 사용자 이름 변경
BACK:
			namecheck = 2;
			send(sock, "C", 1, 0); // C를 서버에게 전송

			EnableWindow(hSendButton, TRUE); // 보내기 버튼 활성화
			SetEvent(hReadEvent); // 읽기 완료 알리기

			DisplayText("[TCP 클라이언트] 이름를 입력해주세요.\n");
			WaitForSingleObject(hWriteEvent, INFINITE); // 쓰기 완료 기다리기

			// 문자열 길이가 0이면 보내지 않음
			if(strlen(buf) == 0){
				EnableWindow(hSendButton, TRUE); // 보내기 버튼 활성화
				SetEvent(hReadEvent); // 읽기 완료 알리기
				continue;
			}
			EnableWindow(hSendButton, TRUE); // 보내기 버튼 활성화
			SetEvent(hReadEvent); // 읽기 완료 알리기
			buf[strlen(buf)] ='\0';
			strcpy(inf.inputName, buf);
			DisplayText("[TCP 클라이언트] name: %s.\n", inf.inputName);
			sprintf(name,"[%s]",inf.inputName);

			send(sock, inf.inputName, strlen(inf.inputName), 0); // 변경된 이름을 서버에게 전송
			while(1){
				if (namecheck == 1)
					break;
				else if (namecheck == 0) {
					DisplayText("[TCP 클라이언트] 이미 존재하는 이름입니다\n");
					goto BACK;
				}
			}
		}
		else {
			sprintf(nameMsg,"%s %s",name,buf);
			send(sock,nameMsg,strlen(nameMsg),0);//nameMsg를 서버에게 전송한다.
		}
		EnableWindow(hSendButton, TRUE); // 보내기 버튼 활성화
		SetEvent(hReadEvent); // 읽기 완료 알리기
	}
	return 0;
}

unsigned WINAPI RecvMsg(void* arg){
	SOCKET sock=*((SOCKET*)arg);//서버용 소켓을 전달한다.
	char nameMsg[NAME_SIZE+BUFSIZE];
	int strLen;

	while(1){//반복
		strLen=recv(sock,nameMsg,NAME_SIZE+BUFSIZE-1,0);//서버로부터 메시지를 수신한다.
		if(strLen==-1)
			return -1;
		nameMsg[strLen]=0;//문자열의 끝을 알리기 위해 설정
		if(!strcmp(nameMsg,"q")){
			DisplayText("left the chat\n");
			closesocket(sock);
			exit(0);
		}
		if (!strcmp(nameMsg, "NCF"))
			namecheck = 0;
		else if (!strcmp(nameMsg, "NCT"))
			namecheck = 1;
		else
			DisplayText("%s\n", nameMsg);
	}
	return 0;
}
