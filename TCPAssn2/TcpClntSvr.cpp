#include "Global.h"
#include "SocketStruct.h"
#include "TcpClntSvr.h"
#include "resource.h"

namespace tcpudp
{
	LPSOCKET_INFORMATION SocketInfo;
	HWND *hwnd;
	struct sockaddr_in server;
	struct sockaddr_in client;

	clientInfo *cInfo;
	SOCKET sdSocket;
	int totalSize;
	int udpSendCount = 0;

	serverInfo *sInfo;
	SOCKET listenSocket;
	statsInfo *stats;
	int totalExpectedSize;
	int udpRecvCount = 0;

	SYSTEMTIME stStartTime, stEndTime;
	//struct tm *startTime;
	//struct tm *endTime;
	//time_t stClock, endClock;

	SOCKET acceptSocket;

	// Paint variables
	int mX = 5, mY = 5; // current str coordinates
	char mStr[BUFSIZ];

	/*----------------------------------------------------------------------
	-- FUNCTION:	ClntConnect
	--
	-- DATE:		February 14, 2018
	--
	-- DESIGNER:	Luke Lee
	--
	-- PROGRAMMER:	Luke Lee
	--
	-- INTERFACE:	bool ClntConnect(clientInfo *clntInfo)
	--
	-- ARGUMENT:	clntInfo		- A pointer to a client info struct
	--								  which contains all the info
	--								  required from client
	--
	-- RETURNS:		bool			- returns true if client connected
	--								  successfully
	--
	-- NOTES:
	-- This function is responsible for connecting the client to the server
	-- with specified address and port.
	----------------------------------------------------------------------*/
	bool ClientSide::ClntConnect(clientInfo *clntInfo)
	{
		WSADATA stWSAData;
		WORD wVersionRequested = MAKEWORD(2, 2);
		DWORD Ret;
		struct hostent	*hp;

		cInfo = clntInfo;
		hwnd = clntInfo->hwnd;

		const char* host = clntInfo->server_input.c_str();
		totalSize = clntInfo->packetSize * clntInfo->send_times;

		// Initialize the DLL with version Winsock 2.2
		WSAStartup(wVersionRequested, &stWSAData);

		// Create the socket
		if (cInfo->isTCP)
		{
			if ((sdSocket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
			{
				sprintf_s(mStr, "Cannot create socket.");
				MessageBox(*hwnd, mStr, "Error", MB_OK);
				FreeSocketInfo(&sdSocket);
				return false;
			}
		}
		else // is UDP protocol
		{
			if ((sdSocket = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET)
			{
				sprintf_s(mStr, "Cannot create socket.");
				MessageBox(*hwnd, mStr, "Error", MB_OK);
				FreeSocketInfo(&sdSocket);
				return false;
			}
		}

		clntInfo->sendSocket = &sdSocket;

		// Initialize and set up the address structure
		memset((char *)&server, 0, sizeof(struct sockaddr_in));
		server.sin_family = AF_INET;
		server.sin_port = htons
		(clntInfo->port);
		if (clntInfo->isIPAddress)
		{
			struct in_addr* addr_p;
			addr_p = (struct in_addr*)malloc(sizeof(struct in_addr));
			addr_p->s_addr = inet_addr(host);
			if ((hp = gethostbyaddr((char *)addr_p, PF_INET, sizeof(&addr_p))) == NULL)
			{
				sprintf_s(mStr, "Unknown server address.");
				MessageBox(*hwnd, mStr, "Error", MB_OK);
				FreeSocketInfo(&sdSocket);
				return false;
			}
		}
		else
		{
			if ((hp = gethostbyname(host)) == NULL)
			{
				sprintf_s(mStr, "Unknown server name.");
				MessageBox(*hwnd, mStr, "Error", MB_OK);
				FreeSocketInfo(&sdSocket);
				return false;
			}
		}

		// Copy the server address
		memcpy((char *)&server.sin_addr, hp->h_addr, hp->h_length);

		WSAAsyncSelect(sdSocket, *clntInfo->hwnd, WM_SOCKET, FD_CONNECT | FD_WRITE | FD_CLOSE);

		if (cInfo->isTCP)
		{
			// Connecting to the server
			if ((Ret = WSAConnect(sdSocket, (struct sockaddr *)&server, sizeof(server), 0, 0, 0, NULL)) != 0)
			{
				if ((Ret = WSAGetLastError()) != WSAEWOULDBLOCK)
				{
					sprintf_s(mStr, "Can't connect to server: %d", Ret);
					MessageBox(*hwnd, mStr, "Error", MB_OK);
					FreeSocketInfo(&sdSocket);
					return false;
				}
				// else, connection is still in progress
			}
			else
			{
				fprintf(stdout, "Connecting to server %s on port %d", server, clntInfo->port);
				return true;
			}
		}
		else // is UDP prorocol
		{
			// Bind local address to the socket
			memset((char *)&client, 0, sizeof(client));
			client.sin_family = AF_INET;
			client.sin_port = htons(0);  // bind to any available port
			client.sin_addr.s_addr = htonl(INADDR_ANY);
			if (bind(sdSocket, (struct sockaddr *)&client, sizeof(client)) == SOCKET_ERROR)
			{
				sprintf_s(mStr, "Can't bind name to UDP client socket.");
				MessageBox(*hwnd, mStr, "Error", MB_OK);
				return false;
			}
			CreateSocketInfo(&sdSocket);
		}
		return true;
	}


	/*----------------------------------------------------------------------
	-- FUNCTION:	ClntProc
	--
	-- DATE:		February 14, 2018
	--
	-- DESIGNER:	Luke Lee
	--
	-- PROGRAMMER:	Luke Lee
	--
	-- INTERFACE:	void CALLBACK ClntProc(HWND hwnd, UINT Message,
	--										 WPARAM wParam, LPARAM lParam)
	--
	-- ARGUMENT:	hwnd			- A handle to the Window.
	--				message			- A message to process.
	--				wParam			- Additional Message information
	--								  (Socket)
	--				lParam			- Additional Message information
	--								  (Winsock event message)
	--
	-- RETURNS:		void
	--
	-- NOTES:
	-- This function is responsible for handling the winsock events
	-- asynchronously. It handles the connect, write, and close events.
	----------------------------------------------------------------------*/
	void ClientSide::ClntProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		DWORD iRc;
		DWORD sentBytes;

		if (WSAGETSELECTERROR(lParam))
		{
			printf("Socket failed with error %d\n", WSAGETSELECTERROR(lParam));
			FreeSocketInfo(&wParam);
		}
		else
		{
			switch (WSAGETSELECTEVENT(lParam))
			{
			case FD_CONNECT:
				// connection has completed. Check for errors.
				if ((iRc = WSAGETSELECTERROR(lParam)) == 0)
				{
					// successful connection
					CreateSocketInfo(&wParam);
				}
				else
				{
					// Report error on the window
					return;
				}
				break;
			case FD_WRITE:
				SocketInfo = GetSocketInfo(&wParam);
				if (cInfo->isTCP)
				{
					while (SocketInfo->BytesSEND < totalSize)
					{
						SocketInfo->DataBuf.buf = cInfo->sendBuffer;
						SocketInfo->DataBuf.len = cInfo->packetSize;

						if (WSASend(SocketInfo->Socket, &(SocketInfo->DataBuf), 1, &sentBytes, 0, NULL, NULL) == SOCKET_ERROR)
						{
							if (WSAGetLastError() != WSAEWOULDBLOCK)
							{
								sprintf_s(mStr, "WSASend() failed with error %d", WSAGetLastError());
								MessageBox(hwnd, mStr, "Error", MB_OK);
							}
						}
						else
						{
							// update sent bytes count
							SocketInfo->BytesSEND += sentBytes;
						}
					}
				}
				else // is UDP protocol
				{
					SocketInfo->DataBuf.buf = cInfo->sendBuffer;
					SocketInfo->DataBuf.len = cInfo->packetSize;

					if (udpSendCount < cInfo->send_times)
					{
						int server_len = sizeof(server);
						//if ((WSASendTo(SocketInfo->Socket, &(SocketInfo->DataBuf), cInfo->packetSize, &sentBytes, 0,
						//	(struct sockaddr *)&server, server_len, NULL, NULL)) == SOCKET_ERROR)
						if ((sentBytes = sendto(SocketInfo->Socket, SocketInfo->DataBuf.buf, cInfo->packetSize, 0,
							(struct sockaddr *)&server, server_len)) < 0)
						{
							if (WSAGetLastError() != WSAEWOULDBLOCK)
							{
								sprintf_s(mStr, "WSASendTo() failed with error %d", WSAGetLastError());
								//MessageBox(hwnd, mStr, "Error", MB_OK);
								printf("");
							}
						}
						else
						{
							// update sent bytes count
							SocketInfo->BytesSEND += sentBytes;
						}
						udpSendCount++;
						if (udpSendCount < cInfo->send_times)
						{
							PostMessage(*cInfo->hwnd, WM_SOCKET, wParam, FD_WRITE);
						}
					}
				}
				break;

			case FD_CLOSE:
				*cInfo->connected = false;
				EnableMenuItem(GetMenu(*cInfo->hwnd), IDM_START, MF_ENABLED);
				EnableMenuItem(GetMenu(*cInfo->hwnd), IDM_STOP, MF_DISABLED);
				
				//printf("Closing socket %d\n", wParam);
				FreeSocketInfo(&wParam);
			}
		}
	}

	/*----------------------------------------------------------------------
	-- FUNCTION:	SvrConnect
	--
	-- DATE:		February 14, 2018
	--
	-- DESIGNER:	Luke Lee
	--
	-- PROGRAMMER:	Luke Lee
	--
	-- INTERFACE:	bool SvrConnect(clientInfo *svrInfo)
	--
	-- ARGUMENT:	svrInfo			- A pointer to a server info struct
	--								  which contains all the info
	--								  required from server
	--
	-- RETURNS:		bool			- returns true if client connected
	--								  successfully
	--
	-- NOTES:
	-- This function is responsible for binding the server to the client
	-- socket and listening for connection.
	----------------------------------------------------------------------*/
	bool ServerSide::SvrConnect(serverInfo *svrInfo)
	{
		WSADATA stWSAData;
		WORD wVersionRequested = MAKEWORD(2, 2);
		DWORD Ret;
		struct hostent	*hp;
		struct sockaddr_in internetAddr;

		sInfo = svrInfo;
		hwnd = svrInfo->hwnd;

		totalExpectedSize = svrInfo->packetSize * svrInfo->send_times;

		// Initialize the DLL with version Winsock 2.2
		if ((Ret = WSAStartup(wVersionRequested, &stWSAData)) != 0)
		{
			sprintf_s(mStr, "WSAStartup() failed with error %d", Ret);
			MessageBox(*hwnd, mStr, "Error", MB_OK);
			return false;
		}

		if (sInfo->isTCP)
		{
			if ((listenSocket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET)
			{
				sprintf_s(mStr, "Failed to get a socket %d\n", WSAGetLastError());
				MessageBox(*hwnd, mStr, "Error", MB_OK);
				return false;
			}
			WSAAsyncSelect(listenSocket, *svrInfo->hwnd, WM_SOCKET, FD_ACCEPT | FD_CLOSE);
		}
		else
		{
			if ((listenSocket = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET)
			{
				sprintf_s(mStr, "Failed to get a socket %d\n", WSAGetLastError());
				MessageBox(*hwnd, mStr, "Error", MB_OK);
				return false;
			}
			WSAAsyncSelect(listenSocket, *svrInfo->hwnd, WM_SOCKET, FD_ACCEPT | FD_READ | FD_CLOSE);
		}

		memset((char *)&internetAddr, 0, sizeof(struct sockaddr_in));
		internetAddr.sin_family = AF_INET;
		internetAddr.sin_addr.s_addr = htonl(INADDR_ANY);
		internetAddr.sin_port = htons(svrInfo->port);

		if (bind(listenSocket, (PSOCKADDR)&internetAddr, sizeof(internetAddr)) == SOCKET_ERROR)
		{
			sprintf_s(mStr, "bind() failed with error %d\n", WSAGetLastError());
			MessageBox(*hwnd, mStr, "Error", MB_OK);
			return false;
		}

		if (sInfo->isTCP)
		{
			if (listen(listenSocket, 5))
			{
				sprintf_s(mStr, "listen() failed with error %d\n", WSAGetLastError());
				MessageBox(*hwnd, mStr, "Error", MB_OK);
				return false;
			}
		}
		else
		{
			CreateSocketInfo(&listenSocket);
		}
		sInfo->recvSocket = &listenSocket;
		stats = new statsInfo();

		return true;
	}

	/*----------------------------------------------------------------------
	-- FUNCTION:	SvrProc
	--
	-- DATE:		February 14, 2018
	--
	-- DESIGNER:	Luke Lee
	--
	-- PROGRAMMER:	Luke Lee
	--
	-- INTERFACE:	void CALLBACK SvrProc(HWND hwnd, UINT Message,
	--										 WPARAM wParam, LPARAM lParam)
	--
	-- ARGUMENT:	hwnd			- A handle to the Window.
	--				message			- A message to process.
	--				wParam			- Additional Message information
	--								  (Socket)
	--				lParam			- Additional Message information
	--								  (Winsock event message)
	--
	-- RETURNS:		void
	--
	-- NOTES:
	-- This function is responsible for handling the winsock events
	-- asynchronously. It handles the connect, write, and close events.
	----------------------------------------------------------------------*/
	BOOL CALLBACK ServerSide::SvrProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		SOCKET acceptSocket;
		LPSOCKET_INFORMATION SocketInfo;
		DWORD recvBytes;
		DWORD sendBytes;
		DWORD flags;

		if (WSAGETSELECTERROR(lParam))
		{
			sprintf_s(mStr, "Socket failed with error %d\n", WSAGETSELECTERROR(lParam));
			MessageBox(hwnd, mStr, "Error", MB_OK);
			FreeSocketInfo(&wParam);
		}
		else
		{
			switch (WSAGETSELECTEVENT(lParam))
			{
			case FD_ACCEPT:
				if ((acceptSocket = accept(wParam, NULL, NULL)) == INVALID_SOCKET)
				{
					sprintf_s(mStr, "accept() failed with error %d\n", WSAGetLastError());
					MessageBox(hwnd, mStr, "Error", MB_OK);
					break;
				}

				// Create a socket information structure to associate with the
				// socket for processing I/O.
				CreateSocketInfo(&acceptSocket);

				//printf("Socket number %d connected\n", acceptSocket);

				// start timer
				GetSystemTime(&stStartTime);
				//time(&stClock);
				//startTime = localtime(&stClock);

				WSAAsyncSelect(acceptSocket, *sInfo->hwnd, WM_SOCKET, FD_READ | FD_CLOSE);
				break;

			case FD_READ:
				SocketInfo = GetSocketInfo(&wParam);

				SocketInfo->ReadBuffer = new char[totalExpectedSize];
				SocketInfo->DataBuf.buf = SocketInfo->ReadBuffer;
				SocketInfo->DataBuf.len = totalExpectedSize;
				flags = 0;

				if (sInfo->isTCP) // TCP protocol 
				{
					if (WSARecv(SocketInfo->Socket, &(SocketInfo->DataBuf), 1, &recvBytes, &flags, NULL, NULL) == SOCKET_ERROR)
					{
						if (WSAGetLastError() != WSAEWOULDBLOCK)
						{
							sprintf_s(mStr, "WSARecv() failed with error %d\n", WSAGetLastError());
							MessageBox(hwnd, mStr, "Error", MB_OK);
							FreeSocketInfo(&wParam);
							return 0;
						}
					}
					else // no error so update the byte count
					{
						SocketInfo->BytesRECV += recvBytes;
						stats->totalBytesRECV = SocketInfo->BytesRECV;

						WriteToFile(sInfo->svFilePath, SocketInfo->DataBuf.buf);

						// stop timer
						GetSystemTime(&stEndTime);
						//time(&endClock);
						//endTime = localtime(&endClock);

						// free up buffer after reading
						//ZeroMemory(&(SocketInfo->ReadBuffer), sizeof(SocketInfo));
						delete(SocketInfo->ReadBuffer);
						SocketInfo->DataBuf.buf = 0;
						SocketInfo->DataBuf.len = 0;
					}
				}
				else // is UDP protocol
				{
					struct sockaddr_in client;
					int client_len = sizeof(client);
					//if ((WSARecvFrom(SocketInfo->Socket, &(SocketInfo->DataBuf), sInfo->packetSize, &recvBytes, &flags,
					//	(struct sockaddr *)&client, &client_len, NULL, NULL)) == SOCKET_ERROR)
					if ((recvBytes = recvfrom(SocketInfo->Socket, SocketInfo->DataBuf.buf, sInfo->packetSize, 0,
						(struct sockaddr *)&client, &client_len)) < 0)
					{
						if (WSAGetLastError() != WSAEWOULDBLOCK)
						{
							sprintf_s(mStr, "recvfrom() failed with error %d", WSAGetLastError());
							MessageBox(hwnd, mStr, "Error", MB_OK);
						}
					}
					else
					{
						if (recvBytes > 0)
						{
							if (udpRecvCount == 0)
							{
								// start timer
								GetSystemTime(&stStartTime);
								//time(&stClock);
								//startTime = localtime(&stClock);
								WriteToFile(sInfo->svFilePath, SocketInfo->DataBuf.buf);
							}
							else
							{
								// stop timer (every successful receive)
								GetSystemTime(&stEndTime);
								//time(&endClock);
								//endTime = localtime(&endClock);
								AppendToFile(sInfo->svFilePath, SocketInfo->DataBuf.buf);
							}
						}
						udpRecvCount++;
						SocketInfo->BytesRECV += recvBytes;
						stats->totalBytesRECV += recvBytes;
					}
				}
				break;
			case FD_CLOSE:
				char str[16];
				if (sInfo->isTCP)
				{
					// update stats struct
					stats->transferredTime = tcpudp::delay(stStartTime, stEndTime);
					stats->packetRECV = stats->totalBytesRECV / sInfo->packetSize;
					stats->packetLOST = sInfo->send_times - stats->packetRECV;
					strcpy_s(str, "TCP");
				}
				else
				{
					if (udpRecvCount == 0)
						stats->transferredTime = 0;
					else
						stats->transferredTime = tcpudp::delay(stStartTime, stEndTime);
					stats->packetRECV = udpRecvCount;
					stats->packetLOST = sInfo->send_times - stats->packetRECV;
					strcpy_s(str, "UDP");
				}

				*sInfo->connected = false;
				EnableMenuItem(GetMenu(*sInfo->hwnd), IDM_START, MF_ENABLED);
				EnableMenuItem(GetMenu(*sInfo->hwnd), IDM_STOP, MF_DISABLED);

				sprintf_s(mStr, "Protocol: %s\nTransfer time: %ld\nPacket received: %d\nPacket lost: %d\nTotal bytes received: %d",
					str, stats->transferredTime, stats->packetRECV, stats->packetLOST, stats->totalBytesRECV);
				MessageBox(hwnd, mStr, "Result", MB_OK);

				sprintf_s(mStr, "Receive complete!");
				ClearScreen(hwnd);
				for (unsigned int i = 0; i < strlen(mStr); i++)
				{
					PrintChar(hwnd, &mStr[i], 0, &mX, &mY);
					mY = INITIAL_Y;
				}
				mX = INITIAL_X;

				//printf("Closing socket %d\n", wParam);
				ResetStats();
				FreeSocketInfo(&wParam);
				break;
			}
		}
		return 0;
	}

	/*----------------------------------------------------------------------
	-- FUNCTION:	delay
	--
	-- DATE:		February 14, 2018
	--
	-- DESIGNER:	Luke Lee
	--
	-- PROGRAMMER:	Luke Lee
	--
	-- INTERFACE:	long delay(SYSTEMTIME t1, SYSTEMTIME t2);
	--
	-- ARGUMENT:	t1				- first SYSTEMTIME input
	--				t2				- second SYSTEMTIME input
	--
	-- RETURNS:		long			- long in terms of differences between
	--								  two times
	--
	-- NOTES:
	-- This function is used to get the time differences in ms between time 1
	-- and time 2 as SYSTENTIME structure
	----------------------------------------------------------------------*/
	long delay(SYSTEMTIME t1, SYSTEMTIME t2)
	{
		long d;

		d = (t2.wSecond - t1.wSecond) * 1000;
		d += (t2.wMilliseconds - t1.wMilliseconds);
		return(d);
	}

	/*----------------------------------------------------------------------
	-- FUNCTION:	WriteToFile
	--
	-- DATE:		February 14, 2018
	--
	-- DESIGNER:	Luke Lee
	--
	-- PROGRAMMER:	Luke Lee
	--
	-- INTERFACE:	bool WriteToFile(std::string filePath, char *buffer)
	--
	-- ARGUMENT:	filePath		- a string path to file
	--				buffer			- array of char
	--
	-- RETURNS:		bool			- returns true if succeffully write to
	--								  file
	--
	-- NOTES:
	-- This function writes the received buffer into a file specified by
	-- user.
	----------------------------------------------------------------------*/
	bool ServerSide::WriteToFile(std::string filePath, char *buffer)
	{
		std::fstream fileWrite(filePath, std::fstream::in | std::fstream::out | std::fstream::app);
		if (fileWrite.is_open())
		{
			fileWrite << buffer;
			fileWrite.close();
			return true;
		}
		return false;
	}

	bool ServerSide::AppendToFile(std::string filePath, char *buffer)
	{
		std::ofstream outfile;

		outfile.open(filePath, std::ios_base::app);
		if (outfile.is_open())
		{
			outfile << buffer;
			outfile.close();
			return true;
		}
		return false;
	}

	/*----------------------------------------------------------------------
	-- FUNCTION:	ResetStats
	--
	-- DATE:		February 14, 2018
	--
	-- DESIGNER:	Luke Lee
	--
	-- PROGRAMMER:	Luke Lee
	--
	-- INTERFACE:	void ResetStats()
	--
	-- ARGUMENT:	void
	--
	-- RETURNS:		void
	--
	-- NOTES:
	-- This function resets the values in stats info struct.
	----------------------------------------------------------------------*/
	void ServerSide::ResetStats()
	{
		stats->transferredTime = 0;
		stats->packetRECV = 0;
		stats->totalBytesRECV = 0;
		stats->packetLOST = 0;
	}

	/*----------------------------------------------------------------------
	-- FUNCTION:	CreateSocketInfo
	--
	-- DATE:		February 14, 2018
	--
	-- DESIGNER:	Luke Lee
	--
	-- PROGRAMMER:	Luke Lee
	--
	-- INTERFACE:	void CreateSocketInfo(SOCEKT *s)
	--
	-- ARGUMENT:	*s				- pointer to a socket
	--
	-- RETURNS:		void
	--
	-- NOTES:
	-- This function creates a Socket Info struct corresponding to *s.
	----------------------------------------------------------------------*/
	void CreateSocketInfo(SOCKET *s)
	{
		LPSOCKET_INFORMATION SI;

		if ((SI = (LPSOCKET_INFORMATION)malloc(sizeof(SOCKET_INFORMATION))) == NULL)
		{
			sprintf_s(mStr, "GlobalAlloc() failed with error %d\n", GetLastError());
			MessageBox(*hwnd, mStr, "Error", MB_OK);
			return;
		}

		// Prepare SocketInfo structure for use.
		SI->Socket = *s;
		SI->BytesSEND = 0;
		SI->BytesRECV = 0;

		SocketInfo = SI;
	}

	/*----------------------------------------------------------------------
	-- FUNCTION:	GetSocketInfo
	--
	-- DATE:		February 14, 2018
	--
	-- DESIGNER:	Luke Lee
	--
	-- PROGRAMMER:	Luke Lee
	--
	-- INTERFACE:	void GetSocketInfo(SOCEKT *s)
	--
	-- ARGUMENT:	*s				- pointer to a socket
	--
	-- RETURNS:		LPSOCKET_INFORMATION
	--
	-- NOTES:
	-- This function gets and returns the Socket Info struct corresponding
	-- to *s.
	----------------------------------------------------------------------*/
	LPSOCKET_INFORMATION GetSocketInfo(SOCKET *s)
	{
		if (SocketInfo->Socket == *s)
		{
			return SocketInfo;
		}
		return nullptr;
	}

	/*----------------------------------------------------------------------
	-- FUNCTION:	FreeSocketInfo
	--
	-- DATE:		February 14, 2018
	--
	-- DESIGNER:	Luke Lee
	--
	-- PROGRAMMER:	Luke Lee
	--
	-- INTERFACE:	void FreeSocketInfo(SOCEKT *s)
	--
	-- ARGUMENT:	*s				- pointer to a socket
	--
	-- RETURNS:		LPSOCKET_INFORMATION
	--
	-- NOTES:
	-- This function takes in a pointer to a socket and free up the memory
	-- for the Socket Info struct associated with that socket.
	----------------------------------------------------------------------*/
	void FreeSocketInfo(SOCKET *s)
	{
		closesocket(*s);
		if (SocketInfo != nullptr)
		{
			free(SocketInfo);
		}
		WSACleanup();
	}
}
