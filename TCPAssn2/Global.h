#pragma once
#pragma comment(lib, "WS2_32.lib")

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#define CLIENT		0
#define SERVER		1
#define INITIAL_X	5
#define INITIAL_Y	5
#define DEF_PORT		7000
#define DEF_P_SIZE		1024
#define DEF_SEND_TIMES	10

#define WM_SOCKET (WM_USER + 1)

#include <winsock2.h>	// winsock2.h needs to be include before windows.h, otherwise will have redefinition problem
#include <windows.h>
#include <string>
#include <vector>
#include <chrono>
#include <ctime>

namespace tcpudp
{
	struct clientInfo
	{
		HWND *hwnd;
		std::string server_input;
		int port = DEF_PORT;
		int send_times = DEF_SEND_TIMES;
		bool isTCP = true;
		bool isIPAddress = true;
		char *sendBuffer;
		int packetSize = DEF_P_SIZE;
		bool *connected;
		SOCKET *sendSocket;
	};

	struct serverInfo
	{
		HWND *hwnd;
		int port = DEF_PORT;
		int packetSize = DEF_P_SIZE;
		int send_times = DEF_SEND_TIMES;
		bool isTCP = true;
		std::string svFilePath;
		bool *connected;
		SOCKET *recvSocket;
	};

	struct statsInfo
	{
		long transferredTime = 0;
		int totalBytesRECV = 0;
		int packetRECV = 0;
		int packetLOST = 0;
		bool isTCP;
	};
}