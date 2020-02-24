#define WM_RECONNECT WM_USER+2
#define WM_ClipWrite WM_USER+1
#define TIME1 1000

#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"Wtsapi32.lib")

extern char* RESGIST_KEY;
extern int DEBUG_PORT;
extern int DEST_PORT;
extern char *DEST_IP;
extern SOCKET destSocket; 
extern char myVERSION[4];
extern bool SendClip;
extern bool RecvClip;
extern UINT ScreenSaver;

bool isNewAndUpdate( UINT format, char* content, DWORD lenth );
void socket_send( char head, UINT send_format, char* content, UINT size, SOCKET Socket=destSocket );
DWORD WINAPI  socket_recv( LPVOID lpParam  );
bool ReadConfig(void);
DWORD WINAPI  init_server_socket( LPVOID lpParam  );
bool init_client_socket( HWND hWnd );
char* compress(char* content, UINT *size);
char* uncompress(char* content, UINT *size);