#include<Windows.h>
#include<Wtsapi32.h>
#include<stdio.h>
#include<stdlib.h>
#include <time.h>
#include <locale.h>
#include "ClipBoardClient.h"

HINSTANCE g_hInst = NULL;
bool SendData=false;
HGLOBAL pWriteGlobal=NULL;
int DEBUG_PORT=0;
int DEST_PORT=0;
char *DEST_IP=NULL;
char* RESGIST_KEY=NULL;
char myVERSION[4]={"1.0"};
bool SendClip=true;
bool RecvClip=true;

static HANDLE hMutex = CreateMutex(NULL,FALSE,NULL);

void HotKey( HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam )
{
	switch( wParam )
	{
	case 1032:
		{
			MessageBox( hWnd, "³ÌÐòÍË³ö", "Õ³Ìù°å¿ØÖÆÆ÷", MB_OK|MB_TOPMOST |MB_ICONSTOP);
			DestroyWindow(hWnd);
			break;
		}
	case 1033:
		{
			SendClip = !SendClip;
			if(SendClip){
				MessageBox( hWnd, "Õ³Ìù°å·¢ËÍÒÑ¿ªÆô", "Õ³Ìù°å¿ØÖÆÆ÷", MB_OK|MB_TOPMOST |MB_ICONSTOP);
			}
			break;
		}
	case 1034:
		{
			RecvClip = !RecvClip;
			if(RecvClip){
				MessageBox( hWnd, "Õ³Ìù°å½ÓÊÕÒÑ¿ªÆô", "Õ³Ìù°å¿ØÖÆÆ÷", MB_OK|MB_TOPMOST |MB_ICONSTOP);
			}
			break;
		}
	default:
		break;
	}
}

static UINT auPriorityList[] = { 
	CF_UNICODETEXT,
	CF_LOCALE,
	CF_OEMTEXT,
	CF_TEXT, 
}; 

bool mystrcmp(char* src, char* dst, int len)
{
	for(int i=0; i<len; i++)
		if (src[i]!=dst[i]) return false;
	return true;
}

bool isNewAndUpdate( char* content, int lenth )
{
	static char* old_content = (char*)calloc(1, 4);
	static unsigned int old_lenth=0;
	while (lenth>0 && content[lenth-1]=='\0') lenth--;
	if( content==NULL || lenth==0 ) return false;
	WaitForSingleObject(hMutex,INFINITE);
	if( lenth==old_lenth && mystrcmp(old_content, content, lenth) ) 
	{
		ReleaseMutex(hMutex);
		return false;
	}
	free(old_content);
	old_content = (char*)malloc((lenth+1)*sizeof(char));
	memcpy(old_content, content, lenth);
	old_content[lenth] = '\0';
	old_lenth = lenth;
	ReleaseMutex(hMutex);
	return true;
}

void OnClipRead(HWND hWnd)
{
	LPCSTR clipboard_data=NULL;
	char *clipboard_buffer=NULL;
	UINT clipboard_format=0;
	HGLOBAL pGlobal=NULL;
	UINT data_size=0;
	bool flag=false;
	int i=5;
	Sleep(rand()%100+50); //ÈÃ±ðÈËÏÈÕ¼ÓÃclipboard£¬ÀýÈçwpsµÄÈõÖÇ
	while( i-- )
	if (OpenClipboard(hWnd)) {  
		clipboard_format = GetPriorityClipboardFormat(auPriorityList, 6); 
		if (clipboard_format==-1) {
			CloseClipboard();
			printf("Data unknown format: %s\n", GetLastError());
			return;
		}
		pGlobal = GetClipboardData(clipboard_format);
		if( pGlobal )
		{
			clipboard_data = (LPCSTR)GlobalLock(pGlobal);
			if( clipboard_data )
			{
				data_size = GlobalSize(pGlobal);
				clipboard_buffer=(char *)malloc(data_size*sizeof(char));
				memcpy(clipboard_buffer, clipboard_data, data_size);
				GlobalUnlock(pGlobal);
				flag=true;
			}
		}
		CloseClipboard();
		if( flag )break;
	}else{
		Sleep(100);
	}
	if (clipboard_format==CF_UNICODETEXT)
		wprintf(L"OnClipRead: %d, %s\n", data_size, (wchar_t*)clipboard_buffer);
	else
		printf("OnClipRead: %d, %s\n", data_size, clipboard_buffer);

	bool isnew=isNewAndUpdate( clipboard_buffer,  data_size);
	if( (isnew || SendData ) && SendClip && flag ){
		socket_send( 'M', clipboard_format, clipboard_buffer, data_size );
		printf("DataSended: %s %s\n", isnew?"isNew":"notNew", SendData?"reSend":"");
		SendData=false;
	}
	printf("\n");
	if( clipboard_buffer!=NULL ) free(clipboard_buffer);
}
void OnClipWrite(HWND hWnd, UINT format, char* clipboard_data, DWORD data_size)
{
	SendData = false;
	if( !isNewAndUpdate( clipboard_data, data_size ) ) return;
	if( !RecvClip ) return ;
	if (format==CF_UNICODETEXT)
		wprintf(L"RECV OnClipWrite: %s\n\n", (wchar_t*)clipboard_data);
	else
		printf("RECV OnClipWrite: %s\n\n", clipboard_data);

	HGLOBAL pWriteGlobal=NULL;
	int i=5;
	HANDLE ret=NULL;
	while(i--)
	if (OpenClipboard(hWnd)) {  
		EmptyClipboard(); 
		if( data_size==0 ) {
			CloseClipboard();
			printf("OnClipWrite: data_size is 0!\n");
			return ;
		}
		pWriteGlobal = GlobalAlloc(GMEM_MOVEABLE, data_size * sizeof(char));
		if (pWriteGlobal == NULL) { 
			CloseClipboard(); 
			printf("OnClipWrite: pWriteGlobal alloc failed!\n");
			Sleep(100);
			continue ; 
		}
		LPTSTR  pGlobal = (LPTSTR  )GlobalLock(pWriteGlobal); 
		if (pGlobal == NULL) { 
			CloseClipboard(); 
			printf("OnClipWrite: GlobalLock failed!\n");
			Sleep(100);
			continue ; 
		}
		memcpy(pGlobal, clipboard_data, data_size * sizeof(char)); 
		GlobalUnlock(pWriteGlobal); 

		ret = SetClipboardData(format, pWriteGlobal); 
		if (ret == NULL) { 
			CloseClipboard(); 
			printf("OnClipWrite: SetClipboardData failed!\n");
			Sleep(100);
			continue ; 
		}
		CloseClipboard();
		break;
	}
	else Sleep(100);
}

void init_socket( void )
{
	WSADATA Data; 
	int status=WSAStartup(MAKEWORD(2,2), &Data);  
	if (status != 0) {
		printf( "ERROR: WSAStartup unsuccessful\n" );
		return;
	}
}

void Create(HWND hWnd)
{
	AddClipboardFormatListener(hWnd);
	RegisterHotKey(hWnd,1032, MOD_CONTROL|MOD_ALT, 'Q');
	RegisterHotKey(hWnd,1033, MOD_CONTROL|MOD_ALT, 'O');
	RegisterHotKey(hWnd,1034, MOD_CONTROL|MOD_ALT, 'I');
	SetTimer(hWnd, 1000, 300*1000, NULL );
}
void Destory(HWND hWnd)
{
	WSACleanup(); 
	RemoveClipboardFormatListener(hWnd);
	KillTimer(hWnd, 1000);
	UnregisterHotKey(hWnd, 1032);
	UnregisterHotKey(hWnd, 1033);
}

LRESULT CALLBACK WndProc( HWND hWnd, UINT nMsg, WPARAM wParam, LPARAM lParam )
{
	switch( nMsg )
	{		
	case WM_RECONNECT:
		printf("WM_RECONNECT\n\n");
		if ( !init_client_socket(hWnd) )
		{
			Sleep(1000);
			PostMessage(hWnd, WM_RECONNECT, NULL, NULL);
		}else{
			SendData = true;
			OnClipRead(hWnd);
		}
		break;
	case WM_CLIPBOARDUPDATE:
		if (SendClip) {
			printf("WM_CLIPBOARDUPDATE\n");
			OnClipRead(hWnd);
		}
		break;
	case WM_HOTKEY:
		HotKey( hWnd, nMsg, wParam, lParam );
		break;
	case WM_TIMER:
		break;
	case WM_CREATE:
		break;
	case  WM_DESTROYCLIPBOARD:
		GlobalFree(pWriteGlobal);
		break;
	case WM_DESTROY:
		Destory(hWnd);
		PostQuitMessage( 0 );
		break;
	default: 
		return DefWindowProc(hWnd, nMsg, wParam, lParam); 
	}
	return (LRESULT) NULL; 
}

BOOL RegisterWnd( LPSTR pszClassName )
{
	WNDCLASSEX wce = { 0 };
	wce.cbSize = sizeof( wce );
	wce.cbWndExtra = 0;
	wce.cbClsExtra = 0;
	wce.hCursor = NULL;
	wce.hIcon = NULL;
	wce.hIconSm = NULL;
	wce.hInstance = g_hInst;
	wce.lpfnWndProc = WndProc;
	wce.lpszClassName = pszClassName;
	wce.lpszMenuName = NULL;

	return RegisterClassEx( &wce );
}

HWND CreateWnd( LPSTR pszClassName )
{
	HWND hWnd = CreateWindowEx( 0,
		pszClassName, "MyWnd", NULL,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		NULL, NULL, g_hInst, NULL);
	return hWnd;
}

void Message( HWND hWnd )
{
	MSG msg = {0};
	while( GetMessage( &msg, NULL, 0, 0 ) )
	{
		DispatchMessage( &msg );
	}
}

void initConsole(void)
{
	AllocConsole();                     // ´ò¿ª¿ØÖÆÌ¨×ÊÔ´
	freopen( "CONOUT$", "w+t", stdout );// ÉêÇëÐ´
	freopen( "CONIN$", "r+t", stdin );  // ÉêÇë¶Á
	printf("Hello World£¡\n");          // Ð´Êý¾Ý	
}

extern int __argc;
extern char**__argv;
bool parseCmdline( char *cmdline, bool* debug )
{
	char *pos=NULL, *pos2=NULL;
	int level=0;
	int keychar[]={-1969382725, -6638952, -1};
	for(int i=0; i< 3; i++)
	{
		keychar[i] = ~keychar[i];
	}
	if( pos = strstr(cmdline, (char*)keychar) )
	{
		if( strlen(pos)>=strlen((char*)keychar)+2 )
		{
			pos+=strlen((char*)keychar)+1;
			for( pos2=pos; *pos2!='\0'&&*pos2!=' ';pos2++ );
			*pos2 = '\0';
			level = atoi(pos);
			if( level==2 ) *debug=true;
		}
		return true;
	}
	return false;
}

int APIENTRY WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	bool debug=false;
	DWORD nResponse=0;

	setlocale(LC_ALL,"chs");
	srand (time(NULL));
	if( parseCmdline( lpCmdLine, &debug ) )initConsole();
	if ( !ReadConfig() ) {
		WTSSendMessage(WTS_CURRENT_SERVER_HANDLE, WTS_CURRENT_SESSION, "Õ³Ìù°å¿ØÖÆÆ÷", strlen("Õ³Ìù°å¿ØÖÆÆ÷"), "Çë×¢²á£¡", strlen("Çë×¢²á£¡"), MB_OK, 0, &nResponse  , false);
		return 0;
	}
	init_socket();

	g_hInst = hInstance;
	RegisterWnd( "MYWND" );
	HWND hWnd = CreateWnd( "MYWND" );
	ShowWindow(hWnd, SW_HIDE);
	if(debug)
	{
		CreateThread(NULL, 0 , init_server_socket, (void*)hWnd, 0, NULL);
	}

	if( !init_client_socket( hWnd ) ) {
		MessageBox( hWnd, "socket init failed!", "Õ³Ìù°å¿ØÖÆÆ÷", MB_OK|MB_TOPMOST |MB_ICONINFORMATION|MB_SYSTEMMODAL|MB_SETFOREGROUND);
		WSACleanup(); 
		PostQuitMessage( 0 );
		return 0;
	}

	Create(hWnd);
	if(!debug)MessageBox( hWnd, "Õ³Ìù°åÒÑ¿ªÆô", "Õ³Ìù°å¿ØÖÆÆ÷", MB_OK|MB_TOPMOST |MB_ICONINFORMATION|MB_SYSTEMMODAL|MB_SETFOREGROUND);
	else MessageBox( hWnd, "Õ³Ìù°åÒÑ¿ªÆô-SERVER mode", "Õ³Ìù°å¿ØÖÆÆ÷", MB_OK|MB_TOPMOST |MB_ICONINFORMATION|MB_SYSTEMMODAL|MB_SETFOREGROUND);
	Message( hWnd );

	return 0;
}

