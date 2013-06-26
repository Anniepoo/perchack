/*******************************************************************************

INTEL CORPORATION PROPRIETARY INFORMATION
This software is supplied under the terms of a license agreement or nondisclosure
agreement with Intel Corporation and may not be copied or disclosed except in
accordance with the terms of that agreement
Copyright(c) 2012 Intel Corporation. All Rights Reserved.

*******************************************************************************/
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <stdio.h>

#pragma comment(lib, "Ws2_32.lib")


#include "util_pipeline.h"
#include "gesture_render.h"

WSADATA wsaData;
#define DEFAULT_PORT "7015"

SOCKET ListenSocket = INVALID_SOCKET;
SOCKET ClientSocket;
#define DEFAULT_BUFLEN 512

class MyPipeline: public UtilPipeline {
public:
	MyPipeline(void):UtilPipeline(),m_render(L"Gesture Viewer") { 
		EnableGesture();
	}
	virtual void PXCAPI OnGesture(PXCGesture::Gesture *data) {
		if (data->active) m_gdata=(*data); 
	}
	virtual bool OnNewFrame(void) {
		return m_render.RenderFrame(ClientSocket, QueryImage(PXCImage::IMAGE_TYPE_DEPTH), QueryGesture(), &m_gdata);
	}
protected:
	GestureRender		m_render;
	PXCGesture::Gesture m_gdata;
};



char recvbuf[DEFAULT_BUFLEN];
int iResult, iSendResult;
int recvbuflen = DEFAULT_BUFLEN;

int wmain(int argc, WCHAR* argv[]) {
	struct addrinfo *result = NULL, *ptr = NULL, hints;

	MyPipeline pipeline;

	int iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed: %d\n", iResult);
		return 1;
	}

    
	ZeroMemory(&hints, sizeof (hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the local address and port to be used by the server
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

	if (ListenSocket == INVALID_SOCKET) {
		printf("Error at socket(): %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}

	    // Setup the TCP listening socket
    iResult = bind( ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

	freeaddrinfo(result);

	// listen on the socket
	if ( listen( ListenSocket, SOMAXCONN ) == SOCKET_ERROR ) {
		printf( "Listen failed with error: %ld\n", WSAGetLastError() );
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	ClientSocket = INVALID_SOCKET;

	// Accept a client socket
	ClientSocket = accept(ListenSocket, NULL, NULL);
	if (ClientSocket == INVALID_SOCKET) {
		printf("accept failed: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	if (!pipeline.LoopFrames()) wprintf_s(L"Failed to initialize or stream data"); 
    return 0;
}
