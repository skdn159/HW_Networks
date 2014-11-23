// ChattingClient.cpp : 콘솔 응용 프로그램에 대한 진입점을 정의합니다.

#include "stdafx.h"

#define  BUF_SIZE 100
#define  NAME_SIZE 20

unsigned WINAPI SendMsg(void* arg);
unsigned WINAPI RecvMsg(void* arg);
void ErrorHandling(char *message);

char name[NAME_SIZE] = "[default]";
char msg[BUF_SIZE];

int main(int argc, char* argv[])
{
	WSADATA wsaData;
	SOCKET hSock;
	SOCKADDR_IN servAdr;
	HANDLE hSndThread, hRcvThread;
	if (argc != 4)
		ErrorHandling("[DEBUG] 인자값 Error IP, Port, Name \n");
		
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		ErrorHandling("[DEBUG] WSAStartup() Error\n");
	
	sprintf_s(name, NAME_SIZE, "[%s]", argv[3]);
	
	hSock = socket(PF_INET, SOCK_STREAM, 0);
	if (hSock == INVALID_SOCKET)
		ErrorHandling("[DEBUG] socket() Error");
	
	memset(&servAdr, 0, sizeof(servAdr));
	servAdr.sin_family = AF_INET;
	servAdr.sin_addr.s_addr = inet_addr(argv[1]);
	servAdr.sin_port = htons(atoi(argv[2]));

	if (connect(hSock, (SOCKADDR*)&servAdr, sizeof(servAdr)) == SOCKET_ERROR)
		ErrorHandling("[DEBUG] connect() Error!\n");
	
	printf_s("Connected to Server!........\n");
	printf_s("(q or Q to QUIT !)\n");

	hSndThread = (HANDLE)_beginthreadex(NULL, 0, SendMsg, (void*)&hSock, 0, NULL);
	hRcvThread = (HANDLE)_beginthreadex(NULL, 0, RecvMsg, (void*)&hSock, 0, NULL);

	WaitForSingleObject(hSndThread, INFINITE);
	WaitForSingleObject(hRcvThread, INFINITE);

	closesocket(hSock);
	WSACleanup();
	return 0;
}

unsigned WINAPI SendMsg(void* arg)
{
	SOCKET hSock = *((SOCKET*)arg);
	char nameMsg[NAME_SIZE + BUF_SIZE];

	while (1){
		fgets(msg, BUF_SIZE, stdin);
		if (!strcmp(msg, "q\n") || !strcmp(msg, "Q\n")){
			closesocket(hSock);
			exit(0); //정상적인 종료
		}
		sprintf_s(nameMsg, NAME_SIZE + BUF_SIZE, "%s  %s", name, msg);
		send(hSock, nameMsg, strlen(nameMsg), 0);
	}
	return 0;
}
unsigned WINAPI RecvMsg(void* arg)
{
	int hSock = *((SOCKET*)arg);
	char nameMsg[NAME_SIZE + BUF_SIZE];
	int strLen;
	while (1){
		strLen = recv(hSock, nameMsg, NAME_SIZE + BUF_SIZE - 1, 0);
		if (strLen == -1){
			return -1;
		}
		nameMsg[strLen] = 0;
		fputs(nameMsg, stdout);
	}
	return 0;
}
void ErrorHandling(char *message)
{
	fputs(message, stderr);
	fputs("\n", stderr);
	exit(1); // 비정상적 종료
}