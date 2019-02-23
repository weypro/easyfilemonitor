/********************************************************************************************************
*文件名称：main.cpp
*说明：EasyMonitor源文件
*
*日期：2016.10
*版本：1.0
*备注：此程序以《C++黑客编程揭秘与防范第2版》（冀云编）中的Monitor程序为基础加以改进
*by wey
********************************************************************************************************/

#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <string.h>

//全局变量（大家别学我总用全局变量，我只是为了方便）
char szDirectory[MAX_PATH]={"0"};//输入的路径，懒得传函数参数
char tempDirectory[MAX_PATH]={"0"};//懒得传线程参数
//char f
CRITICAL_SECTION handle_lock;//临界区域
SYSTEMTIME systime;//系统时间


void GetDirectory();//得到要监控目录
void StartThread();//分配线程
DWORD WINAPI MonitorThread(LPVOID lpParam);//监控线程

void GetDirectory()
{
    printf("Notice: The longest length is 259.\nFormat: c:\\***;d:\\***\ninput: ");

    for(int i=0;i<MAX_PATH-1;i++)//数组最大[259]，不用<=
    {
        if((szDirectory[i]=getchar())=='\n')
        {
            szDirectory[i]='\0';
            return;//直接返回
        }
    }
    szDirectory[MAX_PATH-1]='\0';//字符串输入完成（防止溢出）
}

void StartThread()
{
    int i=0;

    for(int i=0,a=0;i<=strlen(szDirectory);i++,a++)//在第strlen(szDirectory)-1次是不会启动的
    {
        if(szDirectory[i]!='\0')
        {
            if(szDirectory[i]!=';')
            {
                tempDirectory[a]=szDirectory[i];
            }
            else
            {
                tempDirectory[a]='\0';
                //printf("传递：%s\n",tempDirectory);
                CloseHandle(CreateThread(NULL,0,MonitorThread,NULL,0,NULL));
                Sleep(100);
                ZeroMemory(tempDirectory,MAX_PATH);
                a=-1;
            }
        }
        else
        {
            tempDirectory[a]='\0';
            //printf("传递2：%s\n",tempDirectory);
            CloseHandle(CreateThread(NULL,0,MonitorThread,NULL,0,NULL));
            Sleep(100);
            return;
        }
        //printf("%s %s\n",tempDirectory,szDirectory);
    }
}

DWORD WINAPI MonitorThread(LPVOID lpParam)
{
    if(handle_lock.LockCount==-1)//防止多个监控线程抢一个临界区域，-1代表无线程进入
    {
        EnterCriticalSection(&handle_lock);
    }

    TCHAR temp[MAX_PATH];//为unicode而生
    char szLastFileName[MAX_PATH]={0};//防止多次记录产生刷屏
    WORD nLastFileTime=0;//顺便记一下上次的时间，防止只有同一文件在不同时间被修改不输出问题
    MultiByteToWideChar(CP_ACP, 0, tempDirectory, -1, temp, MAX_PATH);//ANSI字符转unicode
    BOOL bRet=FALSE;
    BYTE Buffer[1024]={0};
    FILE_NOTIFY_INFORMATION *pBuffer = (FILE_NOTIFY_INFORMATION *)Buffer;
    DWORD BytesReturned =0;
    HANDLE hFile=CreateFile(temp,FILE_LIST_DIRECTORY,
        FILE_SHARE_READ|FILE_SHARE_DELETE|FILE_SHARE_WRITE,
        NULL,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS,NULL);
    if(INVALID_HANDLE_VALUE==hFile)//找不到
    {
        printf("Fail to start. Press any key to continue.\n");
        return 1;
    }

    printf("Start to monitor %s.\n",tempDirectory);//走你

    while(1)
    {
        ZeroMemory(Buffer,1024);

        bRet=ReadDirectoryChangesW(hFile,&Buffer,sizeof(Buffer),
            TRUE,FILE_NOTIFY_CHANGE_FILE_NAME|//修改文件名
            FILE_NOTIFY_CHANGE_ATTRIBUTES|//修改文件属性
            FILE_NOTIFY_CHANGE_LAST_WRITE,//最后一次写入
            &BytesReturned,NULL,NULL);
        if(bRet==TRUE)
        {
            char szFileName[MAX_PATH]={0};
            GetLocalTime(&systime);//得到当前时间
            WideCharToMultiByte(CP_ACP,0,pBuffer->FileName,pBuffer->FileNameLength/2,
                szFileName,
                MAX_PATH,
                NULL,NULL);//unicode字符转ANSI
            if(strcmp(szFileName,szLastFileName)==0 && nLastFileTime==systime.wSecond)//已经有同一个文件记录了
            {
                continue;
            }
            strcpy(szLastFileName,szFileName);//存记录
            nLastFileTime=systime.wSecond;//存时间
            printf("%02d:%02d:%02d.%03d  ",systime.wHour,systime.wMinute,systime.wSecond,systime.wMilliseconds);
            switch(pBuffer->Action)
            {
            case FILE_ACTION_ADDED:
                printf("ADD : %s%s\n",tempDirectory,szFileName);
                break;
            case FILE_ACTION_MODIFIED:
                printf("MODIFY : %s%s\n",tempDirectory,szFileName);
                break;
            case FILE_ACTION_REMOVED:
                printf("REMOVE %s%s\n",tempDirectory,szFileName);
                break;
            case FILE_ACTION_RENAMED_OLD_NAME:
                printf("RENAMED : %s%s\n",tempDirectory,szFileName);
                if(pBuffer->NextEntryOffset!=0)
                {
                    FILE_NOTIFY_INFORMATION *tempBuffer=(FILE_NOTIFY_INFORMATION *)((DWORD)pBuffer+pBuffer->NextEntryOffset);
                    switch(tempBuffer->Action)
                    {
                    case FILE_ACTION_RENAMED_NEW_NAME:
                        ZeroMemory(szFileName, MAX_PATH);
                        WideCharToMultiByte(CP_ACP,0,tempBuffer->FileName,
                            tempBuffer->FileNameLength/2,
                            szFileName,MAX_PATH,NULL,NULL);//unicode字符转ANSI
                        printf(" -> %s \n",szFileName);
                        break;
                    }
                }
                break;
            case FILE_ACTION_RENAMED_NEW_NAME:
                printf("RENAME (new) : %s%s\n",tempDirectory,szFileName);
                break;
            }
        }
    }
    CloseHandle(hFile);
    printf("Close\n");
    return 0;
}

int main()
{
    SetConsoleTitle(TEXT("Easy monitor 1.0"));
    InitializeCriticalSection(&handle_lock);//初始化临界区域
    printf("Easy Monitor 1.0\n");
    GetDirectory();
    StartThread();
    Sleep(300);//不让主线程抢到
    EnterCriticalSection(&handle_lock);//起到永远暂停作用
    return 0;
}