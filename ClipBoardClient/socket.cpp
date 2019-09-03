#include<Windows.h>
#include<Wtsapi32.h>
#include<iostream>
#include "ClipBoardClient.h"
using namespace std;

extern char* getMAC( );

HWND hwnd=NULL;
SOCKET destSocket=NULL; 
static HANDLE hMutex_socket = CreateMutex(NULL,FALSE,NULL);

void myCloseRestart(SOCKET Socket)
{
	printf("myCloseRestart\n");
	if( Socket )closesocket(Socket); 
	WaitForSingleObject(hMutex_socket,INFINITE);
	if( destSocket==Socket )
	{
		if( destSocket ) PostMessage(hwnd, WM_RECONNECT, NULL, NULL);
		destSocket = 0;
	}
	ReleaseMutex(hMutex_socket);
}

DWORD WINAPI  init_server_socket( LPVOID lpParam  )
{
	SOCKADDR_IN serverSockAddr={0}; 
	SOCKADDR_IN clientSockAddr= {0} ; 
	int addrLen=sizeof(SOCKADDR_IN); 
	SOCKET serverSocket;  
	SOCKET clientSocket;
	int status;
	hwnd = (HWND)lpParam;

	if( DEBUG_PORT==0 ) return NULL;

	serverSockAddr.sin_port=htons(DEBUG_PORT); 
	serverSockAddr.sin_family=AF_INET; 
	serverSockAddr.sin_addr.s_addr=htonl(INADDR_ANY); 

	serverSocket = socket(AF_INET, SOCK_STREAM, 0);  
	status=bind(serverSocket, (LPSOCKADDR)&serverSockAddr, sizeof(serverSockAddr)); 
	if (status == SOCKET_ERROR)  {
		cout << "ERROR: bind unsuccessful" << endl;  
		return NULL;
	}
	status=listen(serverSocket, 1);  
	if (status == SOCKET_ERROR)  {
		cout << "ERROR: listen unsuccessful" << endl; 
		return NULL;
	}
	while(1)
	{
		clientSocket=accept(serverSocket, (LPSOCKADDR)&clientSockAddr, &addrLen);

		socket_recv( &clientSocket );
	}
	return NULL;
}

bool init_client_socket( HWND hWnd )
{
	int status; 
	SOCKADDR_IN destSockAddr={0}; 
	hwnd = hWnd;

	destSockAddr.sin_port = htons(DEST_PORT); 
	destSockAddr.sin_addr.s_addr = inet_addr(DEST_IP); 
	destSockAddr.sin_family=AF_INET;

	if( destSocket ) return true;
	destSocket=socket(AF_INET, SOCK_STREAM, 0);
	
	cout << "Trying to connect to IP Address: " << DEST_IP<<":"<< DEST_PORT<< endl; 

	status=connect(destSocket, (LPSOCKADDR)&destSockAddr, sizeof(destSockAddr));  
	if (status == SOCKET_ERROR)  
	{  
		cout << "ERROR: connect unsuccessful" << endl;  

		status=closesocket(destSocket);  
		destSocket = 0;
		if (status == SOCKET_ERROR)  
			cout << "ERROR: closesocket unsuccessful" << endl;  
		return false;  
	} 

	cout << "Connected..." << endl;
	DWORD TimeOut=1000;
	setsockopt(destSocket,SOL_SOCKET,SO_SNDTIMEO,(char *)&TimeOut,sizeof(TimeOut));
	CreateThread(NULL, 0 , socket_recv, &destSocket, 0, NULL); // ´´½¨Ïß³Ì
	return true;
}

void socket_send( char head, UINT send_format, char* content, UINT size, SOCKET Socket )
{
	char str[9]={0};
	int ret=0;
	unsigned int sended=0;
	str[0]=head;
	switch(head)
	{
	case 'C':
	case 'M':
	{
		unsigned int sendlen = size;
		char *sendbuf = compress(content, &sendlen);
		*((int*)(str+1)) = size+4;
		*((int*)(str+5)) = send_format;
		ret = send(destSocket, str, 9, NULL);
		if( ret<=0 )
		{
			myCloseRestart( destSocket );
			return ;
		}
		for(ret=0,sended=0; sended<sendlen; sended+=ret )
		{
			ret = send(destSocket, sendbuf+sended, sendlen-sended, NULL);
			if( ret<=0 )
			{
				free( sendbuf );
				myCloseRestart( destSocket );
				return ;
			}
		}
		free( sendbuf );
		break;
	}
	case 'P':
		*((int*)(str+1)) = send_format;
		ret = send(destSocket, str, 5, NULL);
		if( ret<=0 )
		{
			myCloseRestart( destSocket );
			return ;
		}
		break;
	default:
		return ;
	}

}

bool setClip( SOCKET Socket, int size )
{
	int ret=0, recvd=0;
	char *buf = (char*)malloc(sizeof(char)*size);
	for( ret=0,recvd=0; recvd<size; recvd+=ret )
	{
		ret = recv(Socket, buf+recvd, size-recvd, NULL);
		if( ret<0 )
		{
			free(buf);
			myCloseRestart( Socket );
			return false;
		}
	}

	if (RecvClip){
		unsigned int sendlen = size-4;
		char *recvbuf = uncompress(  buf+4, &sendlen);
		OnClipWrite(hwnd, *(int*)buf, recvbuf, sendlen);
		free(recvbuf);
	}
	free(buf);
	return true;
}

bool parseCmd( SOCKET Socket, int flag )
{
	DWORD nResponse;
	switch(flag)
	{
	case 'U':
		WTSSendMessage(WTS_CURRENT_SERVER_HANDLE, WTS_CURRENT_SESSION, "Õ³Ìù°å¿ØÖÆÆ÷", strlen("Õ³Ìù°å¿ØÖÆÆ÷"), "ÇëÉý¼¶µ½×îÐÂ°æ£¡", strlen("ÇëÉý¼¶µ½×îÐÂ°æ£¡"), MB_OK, 0, &nResponse  , false);
		Sleep(500); //»æ»­´°¿Ú
		DestroyWindow(hwnd);
		if( Socket )closesocket(Socket); 
		if( destSocket==Socket )
			destSocket = 0;
		PostQuitMessage( 0 );
		ExitProcess(0);
		return false;
	case 'S':
		WTSSendMessage(WTS_CURRENT_SERVER_HANDLE, WTS_CURRENT_SESSION, "Õ³Ìù°å¿ØÖÆÆ÷", strlen("Õ³Ìù°å¿ØÖÆÆ÷"), "×¢²áÂëÎÞÐ§£¡", strlen("×¢²áÂëÎÞÐ§£¡"), MB_OK, 0, &nResponse  , false);
		Sleep(500); //»æ»­´°¿Ú
		DestroyWindow(hwnd);
		if( Socket )closesocket(Socket); 
		if( destSocket==Socket )
			destSocket = 0;
		PostQuitMessage( 0 );
		ExitProcess(0);
		return false;
	case 'V':
	{
		int version = *(int*)myVERSION;
		char *MAC = getMAC( );
		int lenMAC=0, lenRESGIST_KEY=0;
		if( MAC!=NULL ){
			lenMAC = strlen( MAC );
		}
		if( RESGIST_KEY!=NULL ){
			lenRESGIST_KEY = strlen( RESGIST_KEY );
		}
		char *mykey = (char*)malloc( sizeof(char)*( lenMAC+lenRESGIST_KEY+3 ) );
		mykey[0] = 0;
		if( lenMAC>0 ) sprintf( mykey, "%s",  MAC);
		if( lenRESGIST_KEY>0 ) sprintf( mykey+strlen(mykey), "--%s",  RESGIST_KEY);
		socket_send('C', version, mykey, (UINT)strlen(mykey)+1, Socket);
		return true;
	}
	default:
		if( Socket )closesocket(Socket); 
		if( destSocket==Socket )
			destSocket = 0;
		return false;
	}
	return true;
}

bool parsePing( SOCKET Socket, int flag )
{
	if( flag!=200 ) 
		socket_send('P', 200, NULL, 0, Socket);
	return true;
}

DWORD WINAPI  socket_recv( LPVOID lpParam  )
{
	SOCKET Socket = *(SOCKET*)lpParam;
	int ret=0, flag;
	char head[5];
	while( 1 ){
	ret = recv(Socket, head, 5, NULL);
	if( ret<=0 )
	{
		printf("errno=%d, %d\n", errno, WSAGetLastError() );
		myCloseRestart( Socket );
		return NULL;
	}

	flag = *((int*)(head+1));
	switch( head[0] )
	{
		case 'M': 
			if( !setClip( Socket, flag ) ) return NULL;
			break;
		case 'C':
			if( !parseCmd( Socket, flag ) ) return NULL;
			break;
		case 'P':
			if( !parsePing( Socket, flag ) )return NULL;
			break;
		default:
			if( Socket )closesocket(Socket); 
			if( destSocket==Socket )
				destSocket = 0;
			return NULL;
	}

	}
	return NULL;
}
