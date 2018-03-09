#pragma once

#ifndef _SocketStruct_H
#define DATA_BUFSIZE	1000000000
#include <winsock2.h>
#include <windows.h>

namespace tcpudp
{
	typedef struct _SOCKET_INFORMATION {
		//BOOL RecvPosted;
		OVERLAPPED Overlapped;
		CHAR *ReadBuffer;
		WSABUF DataBuf;
		SOCKET Socket;
		DWORD BytesSEND;
		DWORD BytesRECV;
		_SOCKET_INFORMATION *Next;
	} SOCKET_INFORMATION, *LPSOCKET_INFORMATION;

	void CreateSocketInfo(SOCKET *s);
	LPSOCKET_INFORMATION GetSocketInfo(SOCKET *s);
	void FreeSocketInfo(SOCKET *s);
}
#endif // !_SocketStruct_H