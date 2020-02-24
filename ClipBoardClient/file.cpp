#include<Windows.h>
#include  <io.h>
#include<stdio.h>
#include<vector>
#include<algorithm>
#include "ClipBoardClient.h"
using namespace std;

struct Icon{
	TCHAR szName[MAX_PATH];
	POINT pt;
	int id;
	unsigned Hash;
};

bool GetPort(char* str, char **ip, int *port)
{
	char *pos;
	if( strlen(str)<7 ) return false;
	if( pos=strchr(str, ':') )
	{
		*port = atoi(pos+1);
		*pos = '\0';
	}
	if ( *port==0) *port = 7223;
	*ip = str;
	return true;
}

bool ReadConfig(void)
{
	int config1[]={-1130117701, -1114663277, -1685217648, -1768843567, -1};
	int config2[]={-1816353839, -1866625130, -778334818, -6909546, -1};
	for(int i=0; i<5; i++) {
		config1[i]=~config1[i];
		config2[i]=~config2[i];
	}
	char *config = (char*)config1;
	if( (_access( config, 04 )) ==-1 )
	{
		config = (char*)config2;
		if( (_access( config, 04 )) ==-1 )
		{
			printf("file not exist!\n");
			return false;
		}
	}
	LPSTR key = (char*) malloc(sizeof(char)*20);
	LPSTR net = (char*) malloc(sizeof(char)*30);
	GetPrivateProfileString("Config", "RECV", NULL, key, 20, config );
	key[19] = 0; 	if(  !stricmp(key, "off")   ) RecvClip = false;
	GetPrivateProfileString("Config", "SEND", NULL, key, 20, config );
	key[19] = 0; 	if(  !stricmp(key, "off")   ) SendClip = false;
	ScreenSaver = GetPrivateProfileInt("Config", "SCREENSAVER", 0, config );

	GetPrivateProfileString("Regist", "KEY", NULL, key, 20, config );
	GetPrivateProfileString("Config", "SERVER", NULL, net, 30, config );
	DEBUG_PORT = GetPrivateProfileInt("Config","DEBUG_PORT", 0, config );
	
	if( !GetPort(net, &DEST_IP, &DEST_PORT) )
	{
		free(key),free(net);
		printf("SERVER IP not exist!\n");
		return false;
	}
	printf("KEY:%s\nSERVER:%s:%d\n", key, net, DEBUG_PORT);
	RESGIST_KEY = key;
	return true;
}

void SerializeIcons( vector<Icon>& Icons )
{
	int n = Icons.size();
	struct Icon icon_tmp;
	FILE* fd=fopen("./Icons_file", "wb+");	//创建文件
	if(fd == NULL)return;
	fwrite(&n, sizeof(n), 1, fd);
	for( int i=0; i<n; i++ )
	{
		icon_tmp = Icons[i];
		fwrite(&icon_tmp, sizeof(struct Icon), 1, fd);
	}
	fclose(fd);
}

vector<Icon> DeserializeIcons(void)
{
	int n=0;
	struct Icon icon_tmp;
	vector<Icon> Tmp_Icons ;
	FILE* fd=fopen("./Icons_file", "rb");
	if(fd == NULL) return Tmp_Icons;
	fread(&n, sizeof(n), 1, fd);
	for(int i=0; i<n; i++)
	{
		memset(&icon_tmp, 0, sizeof(icon_tmp));
		fread(&icon_tmp, sizeof(struct Icon), 1, fd);
		Tmp_Icons.push_back(icon_tmp);
	}
	fclose(fd);
	return Tmp_Icons;
}

unsigned int RSHash(const char* str)
{
	unsigned int b    = 378551;
	unsigned int a    = 63689;
	unsigned int hash = 0;
	size_t len = strlen(str);
	for(size_t i = 0; i < len; i++)
	{
		hash = hash * a + str[i];
		a    = a * b;
	}

	return hash;
}

char* compress(char* content, UINT *size)
{
	int len = *size;
	char *res = (char*)malloc(sizeof(char)*len);
	for( int i=0; i<len; i++ )
	{
		res[i] = ~content[i];
	}
	*size = len;
	return res;
}

char* uncompress(char* content, UINT *size)
{
	int len = *size;
	char *res = (char*)malloc(sizeof(char)*len);
	for( int i=0; i<len; i++ )
	{
		res[i] = ~content[i];
	}
	*size = len;
	return res;
}