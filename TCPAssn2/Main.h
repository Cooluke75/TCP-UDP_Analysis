#pragma once

#include <windows.h>
#include <stdio.h>
#include <strsafe.h>
#include <fstream>


namespace tcpudp
{
	// Global variables - for creating WinMain
	const TCHAR tchrProgramName[] = TEXT("TCP/UDP Analysis Terminal");
	HWND hwnd;
	//LPTSTR lpszCommPort;
	//COMMCONFIG ccfg;
	HINSTANCE hInst;

	int hostType = -1;

	// input handles
	HWND diaHwnd, butHwnd;
	//int menuID;
	int len;
	char* input;

	// Paint variables
	int X = 5, Y = 5; // current str coordinates
	char str[BUFSIZ];

	// Tx dialog variables
	char server_input[BUFSIZ];
	char port_str[BUFSIZ];
	char p_size_str[BUFSIZ];
	char send_time_str[BUFSIZ];
	int port = DEF_PORT;
	int p_size = DEF_P_SIZE;
	int send_times = DEF_SEND_TIMES;
	bool isIPAddress = true;
	bool isTCP = true;

	bool fileUpload = false;
	bool fileSave = false;
	bool clntConnected = false;
	bool svrConnected = false;
	bool ptclConnected = false;

	clientInfo *clntInfo;
	serverInfo *svrInfo;
	statsInfo *staInfo;

	// Winsock struct
	struct hostent* hp;
	struct servent* sv;

	// Functions
	LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
	BOOL CALLBACK DialogProc(HWND H, UINT M, WPARAM W, LPARAM L);
	bool InitializeWindows(HINSTANCE hInst, int nCmdShow);
	bool UploadFile();
	bool PacketizeFile(std::string filePath);
	bool SaveToFile();
	void PrintChar(HWND hwnd, char* character, unsigned int row, int* X, int* Y);
	void ClearScreen(HWND hwnd);
	void CleanUp();
}