// 头文件包含
#include <WinSock2.h>
#include <Iphlpapi.h>
#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <algorithm>  

#pragma comment(lib,"iphlpapi.lib")

#define MALLOC(x) HeapAlloc(GetProcessHeap(), 0, (x))
#define FREE(x) HeapFree(GetProcessHeap(), 0, (x))

typedef struct _MyAdpterInfo
{
	std::vector<std::string> Ip;
	std::string MacAddress;
	std::string Description;
	std::string Name;
	UINT Type;
}MyAdpterInfo;

int MyGetAdptersInfo(std::vector<MyAdpterInfo>& adpterInfo)
{
	PIP_ADAPTER_INFO pAdapterInfo;
	PIP_ADAPTER_INFO pAdapter = NULL;
	DWORD dwRetVal = 0;

	ULONG ulOutBufLen = sizeof(IP_ADAPTER_INFO);
	pAdapterInfo = (IP_ADAPTER_INFO *)MALLOC(sizeof(IP_ADAPTER_INFO));
	if (pAdapterInfo == NULL)
	{
		printf("Error allocating memory needed to call GetAdaptersinfo\n");
		return -1;
	}
	// Make an initial call to GetAdaptersInfo to get
	// the necessary size into the ulOutBufLen variable
	if (GetAdaptersInfo(pAdapterInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW)
	{
		FREE(pAdapterInfo);
		pAdapterInfo = (IP_ADAPTER_INFO *)MALLOC(ulOutBufLen);
		if (pAdapterInfo == NULL)
		{
			printf("Error allocating memory needed to call GetAdaptersinfo\n");
			return -1;    //    error data return
		}
	}

	if ((dwRetVal = GetAdaptersInfo(pAdapterInfo, &ulOutBufLen)) == NO_ERROR)
	{
		pAdapter = pAdapterInfo;
		while (pAdapter)
		{
			MyAdpterInfo info;
			info.Name = std::string(pAdapter->AdapterName);
			info.Description = std::string(pAdapter->Description);
			std::transform(info.Description.begin(), info.Description.end(), info.Description.begin(), ::tolower); 
			info.Type = pAdapter->Type;
			char buffer[20];
			sprintf_s(buffer, "%.2X%.2X%.2X%.2X%.2X%.2X", pAdapter->Address[0],
				pAdapter->Address[1], pAdapter->Address[2], pAdapter->Address[3], 
				pAdapter->Address[4], pAdapter->Address[5]);
			info.MacAddress = std::string(buffer);
			IP_ADDR_STRING *pIpAddrString = &(pAdapter->IpAddressList);
			do 
			{
				info.Ip.push_back(std::string(pIpAddrString->IpAddress.String));
				pIpAddrString = pIpAddrString->Next;
			} while (pIpAddrString);
			pAdapter = pAdapter->Next;
			adpterInfo.push_back(info);
		}
		if (pAdapterInfo)
			FREE(pAdapterInfo);
		return 0;    // normal return
	}
	else
	{
		if (pAdapterInfo)
			FREE(pAdapterInfo);
		printf("GetAdaptersInfo failed with error: %d\n", dwRetVal);
		return 1;    //    null data return
	}
}

char* getMAC( )
{
	int i;
	char* res=NULL;
	std::vector<MyAdpterInfo> AdptersInfo;
	int ret = MyGetAdptersInfo(AdptersInfo);
	for (i = 0; i < AdptersInfo.size(); ++i)
	{
		if( strstr( AdptersInfo[i].Description.c_str(), "virtual" ) ) continue;
		if( strstr( AdptersInfo[i].Description.c_str(), "vpn" ) ) continue;
		break;
	}
	if( i<AdptersInfo.size() )
	{
		res = strdup( AdptersInfo[i].MacAddress.c_str() );
		/*std::cout << "Adpter name: " << AdptersInfo[i].Name << std::endl;
		std::cout << "Adpter description: " << AdptersInfo[i].Description << std::endl;
		std::cout << "Adpter MAC: " << AdptersInfo[i].MacAddress << std::endl;
		std::cout << "Adpter IP: ";
		int num =  AdptersInfo[i].Ip.size();
		for (int j = 0; j <num; ++j)
		{
			if (j != 0)
			{
				std::cout << ", ";
			}
			std::cout << AdptersInfo[i].Ip[j];
		}
		std::cout << std::endl << std::endl << std::endl;*/
	}
	return res;
}