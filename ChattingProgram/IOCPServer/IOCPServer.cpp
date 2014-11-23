// IOCPServer.cpp : 콘솔 응용 프로그램에 대한 진입점을 정의합니다.

#include "stdafx.h"
#define BUF_SIZE 100
#define READ 3
#define WRITE 5
#define MAXUSER 500
typedef struct
{
	SOCKET hClntSock;
	SOCKADDR_IN clntAdr;

} PER_HANDLE_DATA, *LPPER_HANDLE_DATA;

typedef struct //buffer Info
{
	OVERLAPPED overlapped;
	WSABUF wsaBuf;
	char buffer[BUF_SIZE];
	int rwMode; // read & write

} PER_IO_DATA, *LPPER_IO_DATA;

unsigned int WINAPI EchoThreadMain(LPVOID CompletionPortIO);
void ErrorHandling(char *message);

SOCKET* hClientList;
int clntNum = 0;

int main(int argc, char* argv[]){

	WSADATA wsaData;
	HANDLE hComPort;
	SYSTEM_INFO SysInfo;
	LPPER_IO_DATA ioInfo;
	LPPER_HANDLE_DATA handleInfo;

	SOCKET hServSock;
	SOCKADDR_IN servAdr;
	DWORD recvBytes;  
	DWORD flags = 0;	
	
	hClientList = (SOCKET*)malloc(sizeof(SOCKET)*MAXUSER);

	if (argc != 2)
		ErrorHandling("[DEBUG] 초기 인자값 이상. port값 넣으세요 ");

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		ErrorHandling("[DEBUG] WSAStartup() error");

	hComPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	GetSystemInfo(&SysInfo);

	for (DWORD i = 0; i < SysInfo.dwNumberOfProcessors; ++i){
		_beginthreadex(NULL, 0, EchoThreadMain, (LPVOID)hComPort, 0, NULL);
	}
	
	hServSock = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (hServSock == INVALID_SOCKET)
		ErrorHandling("[DEBUG] WSASocket() Error");

	memset(&servAdr, 0, sizeof(servAdr));
	servAdr.sin_family = AF_INET;
	servAdr.sin_addr.s_addr = htonl(INADDR_ANY);
	servAdr.sin_port = htons(atoi(argv[1]));

	if (bind(hServSock, (SOCKADDR*)&servAdr, sizeof(servAdr)) == SOCKET_ERROR)
		ErrorHandling("[DEBUG] bind() Error");

	if (listen(hServSock, 5) == SOCKET_ERROR)
		ErrorHandling("[DEBUG] listen() Error");

	printf("ServerStarted!...\n");
	while (1){

		SOCKET hClntSock;
		SOCKADDR_IN clntAdr;
		int addrLen = sizeof(clntAdr);
		hClntSock = accept(hServSock, (SOCKADDR*)&clntAdr, &addrLen);
		printf("Client Access! Ip = %s", inet_ntoa(clntAdr.sin_addr));

		handleInfo = (LPPER_HANDLE_DATA)malloc(sizeof(PER_HANDLE_DATA));
		handleInfo->hClntSock = hClntSock;
		memcpy(&(handleInfo->clntAdr), &clntAdr, addrLen);

		if (CreateIoCompletionPort((HANDLE)hClntSock, hComPort, (DWORD)handleInfo, 0) != hComPort)
			ErrorHandling("[DEBUG] CreateIoCompletionPort() Error");

		ioInfo = (LPPER_IO_DATA)malloc(sizeof(PER_IO_DATA));
		memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
		ioInfo->wsaBuf.len = BUF_SIZE;
		ioInfo->wsaBuf.buf = ioInfo->buffer;
		ioInfo->rwMode = READ;
		WSARecv(handleInfo->hClntSock, &(ioInfo->wsaBuf), 1, &recvBytes, &flags, &(ioInfo->overlapped), NULL);
		
		hClientList[clntNum++] = hClntSock; //배열에 한명씩 추가
	}
	free(hClientList); // ClientList 해제
	CloseHandle(hComPort);
	closesocket(hServSock);
	WSACleanup();
	return 0;
}

unsigned int WINAPI EchoThreadMain(LPVOID pComport)
{
	HANDLE hComport = (HANDLE)pComport;
	SOCKET sock;
	DWORD bytesTrans;
	LPPER_HANDLE_DATA handleInfo;
	LPPER_IO_DATA ioInfo;
	DWORD flags = 0;

	while (1){

		int sendCount = 0;
		GetQueuedCompletionStatus(hComport, &bytesTrans, (LPDWORD)&handleInfo, (LPOVERLAPPED*)&ioInfo, INFINITE);
		sock = handleInfo->hClntSock;

		if (ioInfo->rwMode == READ){
			puts("[DEBUG] message received!");

			if (bytesTrans == 0){ //EOF전송시 
				printf("Client(IP = %s) Disconnected ! \n", inet_ntoa(handleInfo->clntAdr.sin_addr));
				
				for (int i = 0; i < clntNum; ++i){ //나간 유저 처리
					
					if (hClientList[i] == sock){ 
						closesocket(hClientList[i]); //나간 유저 삭제

						for (int idx = i; idx < MAXUSER; ++idx){ // ClientList 재배열
							hClientList[idx] = hClientList[idx + 1];
						}
						--clntNum;
					}
				}
		
				closesocket(sock);
				free(handleInfo);
				free(ioInfo);
				continue;
			}
			memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
			ioInfo->wsaBuf.len = bytesTrans;
			ioInfo->rwMode = WRITE;
			
			for (int i = 0; i < clntNum; ++i){
			WSASend(hClientList[i], &(ioInfo->wsaBuf), 1, NULL, 0, &(ioInfo->overlapped), NULL);
			++sendCount;
			}

			ioInfo = (LPPER_IO_DATA)malloc(sizeof(PER_IO_DATA));
			memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
			ioInfo->wsaBuf.len = BUF_SIZE;
			ioInfo->wsaBuf.buf = ioInfo->buffer;
			ioInfo->rwMode = READ;

			WSARecv(sock, &(ioInfo->wsaBuf), 1, NULL, &flags, &(ioInfo->overlapped), NULL);
		}

		else{
			puts("[DEBUG] message sent!");
			if (sendCount == clntNum){
				free(ioInfo);
			}
		}
	}
	return 0;
}

void ErrorHandling(char *message)
{
	fputs(message, stderr);
	fputs("\n", stderr);
	exit(1); //비 정상적 종료
}
