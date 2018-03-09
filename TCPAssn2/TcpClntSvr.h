#pragma once

#include <iostream>
#include <windows.h>
#include <fstream>

namespace tcpudp
{
	class ClientSide
	{
	public:
		ClientSide() = delete;
		static bool ClntConnect(clientInfo *clntInfo);
		static void ClntProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
	};

	class ServerSide
	{
	public:
		ServerSide() = delete;
		static bool SvrConnect(serverInfo *svrInfo);
		static BOOL CALLBACK SvrProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
		static bool WriteToFile(std::string filePath, char *buffer);
		static bool AppendToFile(std::string filePath, char *buffer);
		static void ResetStats();
	};

	long delay(SYSTEMTIME t1, SYSTEMTIME t2);
	void PrintChar(HWND hwnd, char* character, unsigned int row, int* X, int* Y);
	void ClearScreen(HWND hwnd);
}