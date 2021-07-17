// �÷���: VS2010

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

unsigned WINAPI SendMsg(void* arg);//������ �����Լ�
unsigned WINAPI RecvMsg(void* arg);//������ �����Լ�


BOOL CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM); // ��ȭ���� ���ν���

void DisplayText(char *fmt, ...); // ���� ��Ʈ�� ��� �Լ�
void err_quit(char *msg); // ���� ��� �Լ�
void err_display(char *msg); // ���� ��� �Լ�
int recvn(SOCKET s, char *buf, int len, int flags); // ����� ���� ������ ���� �Լ�
DWORD WINAPI ClientMain(LPVOID arg); // ���� ��� ������ �Լ�

char buf[BUFSIZE+1]; // ������ �ۼ��� ����
char name[NAME_SIZE]="[DEFAULT]"; // �̸� ����

HANDLE hReadEvent, hWriteEvent; // �̺�Ʈ
HWND hSendButton; // ������ ��ư
HWND hEdit1, hEdit2; // ���� ��Ʈ��

struct inf { // Ŭ���̾�Ʈ�� ������ ������ ����ü
	char inputName[20]; // �̸�
	char room[2]; // �� ��ȣ
};
struct inf inf;
int namecheck;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nCmdShow)
{
	// �̺�Ʈ ����
	hReadEvent = CreateEvent(NULL, FALSE, TRUE, NULL);
	if(hReadEvent == NULL) return 1;
	hWriteEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if(hWriteEvent == NULL) return 1;

	// ���� ��� ������ ����
	CreateThread(NULL, 0, ClientMain, NULL, 0, NULL);

	// ��ȭ���� ����
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DlgProc);

	// �̺�Ʈ ����
	CloseHandle(hReadEvent);
	CloseHandle(hWriteEvent);

	return 0;
}

// ��ȭ���� ���ν���
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
			EnableWindow(hSendButton, FALSE); // ������ ��ư ��Ȱ��ȭ
			WaitForSingleObject(hReadEvent, INFINITE); // �б� �Ϸ� ��ٸ���
			GetDlgItemText(hDlg, IDC_EDIT3, buf, BUFSIZE+1);
			SetEvent(hWriteEvent); // ���� �Ϸ� �˸���
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

// ���� ��Ʈ�� ��� �Լ�
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

// ���� �Լ� ���� ��� �� ����
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

// ���� �Լ� ���� ���
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

// ����� ���� ������ ���� �Լ�
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

// IP Ȯ���ϴ� �Լ�
BOOL IPcheck(char *str) {
	int cnt = 0;
	int num;
	int i;
	int temp[4];
	// '.' �� �������� �Է¹��� ���ڿ��� ������
	char *ptr = strtok(str, ".");
	
	while (ptr != NULL) {
		// ���� ���ڿ��� �������� Ȯ���Ѵ�
		if (atoi(ptr) || atoi(ptr) == 0) {
			if (cnt > 3)
				return 0;
			num = atoi(ptr);
			temp[cnt] = num;
			cnt++;
		}
		// ���ڰ� �ƴϸ� 0 ��ȯ
		else
			return 0;
		ptr = strtok(NULL, ".");
	}
	if (cnt != 4)
		return 0;
	// class D ������ Ȯ��
	else if (temp[0] >= 0 && temp[0] <= 255 && 
			 temp[1] >= 0 && temp[1] <= 255 && 
			 temp[2] >= 0 && temp[2] <= 255 && 
			 temp[3] >= 0 && temp[3] <= 255)
		return 1;
	else 
		return 0;
}

// PORT ��ȣ Ȯ���ϴ� �Լ�
BOOL PORTcheck(char *str) {
	if (atoi(str))
		if(atoi(str) >= 0 && atoi(str) <= 65535)
			return 1;
		else
			return 0;
	else
		return 0;
}


// TCP Ŭ���̾�Ʈ ���� �κ�
DWORD WINAPI ClientMain(LPVOID arg)
{
	WSADATA wsaData;
	SOCKET sock;
	SOCKADDR_IN serverAddr;
	HANDLE sendThread,recvThread;

	char myIp[100]; // Ip �ּ�
	char port[100]; // ���� port

	
	// IP �Է�
	while (1) {
		DisplayText("[TCP Ŭ���̾�Ʈ] ���� IP�� �Է����ּ���.\n");
		WaitForSingleObject(hWriteEvent, INFINITE);
		if(strlen(buf) == 0){
			EnableWindow(hSendButton, TRUE); // ������ ��ư Ȱ��ȭ
			SetEvent(hReadEvent); // �б� �Ϸ� �˸���
		}
		EnableWindow(hSendButton, TRUE); // ������ ��ư Ȱ��ȭ
		SetEvent(hReadEvent); // �б� �Ϸ� �˸���
		strcpy(myIp, buf);
		if (IPcheck(buf))
			break;
	}
	DisplayText("[TCP Ŭ���̾�Ʈ] ���� IP: %s.\n", myIp);

	// port �Է�
	while (1) {
		DisplayText("[TCP Ŭ���̾�Ʈ] ���� PORT�� �Է����ּ���.\n");
		WaitForSingleObject(hWriteEvent, INFINITE);
		if(strlen(buf) == 0){
				EnableWindow(hSendButton, TRUE); // ������ ��ư Ȱ��ȭ
				SetEvent(hReadEvent); // �б� �Ϸ� �˸���
		}
		EnableWindow(hSendButton, TRUE); // ������ ��ư Ȱ��ȭ
		SetEvent(hReadEvent); // �б� �Ϸ� �˸���
		if (PORTcheck(buf))
			break;
	}
	strcpy(port, buf);
	DisplayText("[TCP Ŭ���̾�Ʈ] port: %s.\n", port);
	
	// �̸� �Է�
	DisplayText("[TCP Ŭ���̾�Ʈ] �̸��� �Է����ּ���.\n");
	WaitForSingleObject(hWriteEvent, INFINITE);
	if(strlen(buf) == 0){
			EnableWindow(hSendButton, TRUE); // ������ ��ư Ȱ��ȭ
			SetEvent(hReadEvent); // �б� �Ϸ� �˸���
	}
	EnableWindow(hSendButton, TRUE); // ������ ��ư Ȱ��ȭ
	SetEvent(hReadEvent); // �б� �Ϸ� �˸���
	buf[strlen(buf)] ='\0';
	strcpy(inf.inputName, buf);
	DisplayText("[TCP Ŭ���̾�Ʈ] name: %s.\n", inf.inputName);

	// ���ȣ �Է�
	while (1) {
		DisplayText("[TCP Ŭ���̾�Ʈ] ���ȣ�� �Է����ּ���(1 or 2).\n");
		WaitForSingleObject(hWriteEvent, INFINITE);
		if(strlen(buf) == 0){
				EnableWindow(hSendButton, TRUE); // ������ ��ư Ȱ��ȭ
				SetEvent(hReadEvent); // �б� �Ϸ� �˸���
		}
		EnableWindow(hSendButton, TRUE); // ������ ��ư Ȱ��ȭ
		SetEvent(hReadEvent); // �б� �Ϸ� �˸���
		if (!strcmp(buf, "1") || !strcmp(buf, "2"))
			break;
	}
	strcpy(inf.room, buf);
	DisplayText("[TCP Ŭ���̾�Ʈ] room: %s.\n", inf.room);

	DisplayText("[TCP Ŭ���̾�Ʈ] ä�û����: Display\n");
	DisplayText("[TCP Ŭ���̾�Ʈ] �̸� ����: Change\n");
	DisplayText("[TCP Ŭ���̾�Ʈ] ����: quit\n");
	DisplayText("[TCP Ŭ���̾�Ʈ] ����� �ݵ�� quit���� �������ּ���\n");
	DisplayText("[TCP Ŭ���̾�Ʈ] %s�� ���Դϴ�.\n", inf.room);

	if(WSAStartup(MAKEWORD(2,2),&wsaData)!=0)// ������ ������ ����Ѵٰ� �ü���� �˸�
		ErrorHandling("WSAStartup() error!");

	sprintf(name,"[%s]",inf.inputName);
	sock=socket(PF_INET,SOCK_STREAM,0);//������ �ϳ� �����Ѵ�.

	memset(&serverAddr,0,sizeof(serverAddr));
	serverAddr.sin_family=AF_INET;
	serverAddr.sin_addr.s_addr=inet_addr(myIp);
	serverAddr.sin_port=htons(atoi(port));

	if(connect(sock,(SOCKADDR*)&serverAddr,sizeof(serverAddr))==SOCKET_ERROR)//������ �����Ѵ�.
		ErrorHandling("connect() error");

	send(sock, (char *)&inf, sizeof(inf), 0);


	sendThread=(HANDLE)_beginthreadex(NULL,0,SendMsg,(void*)&sock,0,NULL);//�޽��� ���ۿ� �����尡 ����ȴ�.
	recvThread=(HANDLE)_beginthreadex(NULL,0,RecvMsg,(void*)&sock,0,NULL);//�޽��� ���ſ� �����尡 ����ȴ�.

	WaitForSingleObject(sendThread,INFINITE);//���ۿ� �����尡 �����ɶ����� ��ٸ���./
	WaitForSingleObject(recvThread,INFINITE);//���ſ� �����尡 �����ɶ����� ��ٸ���.

	//Ŭ���̾�Ʈ�� ���Ḧ �õ��Ѵٸ� ���� �Ʒ��� ����ȴ�.
	closesocket(sock);//������ �����Ѵ�.
	WSACleanup();//������ ���� ��������� �ü���� �˸���.

	return 0;
}

unsigned WINAPI SendMsg(void* arg){//���ۿ� �������Լ�
	SOCKET sock=*((SOCKET*)arg);//������ ������ �����Ѵ�.
	char nameMsg[NAME_SIZE+BUFSIZE];
	int len;
	while(1){//�ݺ�
		WaitForSingleObject(hWriteEvent, INFINITE); // ���� �Ϸ� ��ٸ���

		// ���ڿ� ���̰� 0�̸� ������ ����
		if(strlen(buf) == 0){
			EnableWindow(hSendButton, TRUE); // ������ ��ư Ȱ��ȭ
			SetEvent(hReadEvent); // �б� �Ϸ� �˸���
			continue;
		}
		len = strlen(buf);
		buf[len]='\0';
		if(!strcmp(buf,"quit")){//quit�� �Է��ϸ� �����Ѵ�.
			send(sock,"q",1,0);//q�� �������� �����Ѵ�.
		}
		else if(!strcmp(buf, "Display")) { // Display �Է½� ����� ���� ���
			send(sock, "D", 1, 0); // D�� �������� ����
		}
		else if(!strcmp(buf, "Change")) { // Change �Է½� ����� �̸� ����
BACK:
			namecheck = 2;
			send(sock, "C", 1, 0); // C�� �������� ����

			EnableWindow(hSendButton, TRUE); // ������ ��ư Ȱ��ȭ
			SetEvent(hReadEvent); // �б� �Ϸ� �˸���

			DisplayText("[TCP Ŭ���̾�Ʈ] �̸��� �Է����ּ���.\n");
			WaitForSingleObject(hWriteEvent, INFINITE); // ���� �Ϸ� ��ٸ���

			// ���ڿ� ���̰� 0�̸� ������ ����
			if(strlen(buf) == 0){
				EnableWindow(hSendButton, TRUE); // ������ ��ư Ȱ��ȭ
				SetEvent(hReadEvent); // �б� �Ϸ� �˸���
				continue;
			}
			EnableWindow(hSendButton, TRUE); // ������ ��ư Ȱ��ȭ
			SetEvent(hReadEvent); // �б� �Ϸ� �˸���
			buf[strlen(buf)] ='\0';
			strcpy(inf.inputName, buf);
			DisplayText("[TCP Ŭ���̾�Ʈ] name: %s.\n", inf.inputName);
			sprintf(name,"[%s]",inf.inputName);

			send(sock, inf.inputName, strlen(inf.inputName), 0); // ����� �̸��� �������� ����
			while(1){
				if (namecheck == 1)
					break;
				else if (namecheck == 0) {
					DisplayText("[TCP Ŭ���̾�Ʈ] �̹� �����ϴ� �̸��Դϴ�\n");
					goto BACK;
				}
			}
		}
		else {
			sprintf(nameMsg,"%s %s",name,buf);
			send(sock,nameMsg,strlen(nameMsg),0);//nameMsg�� �������� �����Ѵ�.
		}
		EnableWindow(hSendButton, TRUE); // ������ ��ư Ȱ��ȭ
		SetEvent(hReadEvent); // �б� �Ϸ� �˸���
	}
	return 0;
}

unsigned WINAPI RecvMsg(void* arg){
	SOCKET sock=*((SOCKET*)arg);//������ ������ �����Ѵ�.
	char nameMsg[NAME_SIZE+BUFSIZE];
	int strLen;

	while(1){//�ݺ�
		strLen=recv(sock,nameMsg,NAME_SIZE+BUFSIZE-1,0);//�����κ��� �޽����� �����Ѵ�.
		if(strLen==-1)
			return -1;
		nameMsg[strLen]=0;//���ڿ��� ���� �˸��� ���� ����
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
