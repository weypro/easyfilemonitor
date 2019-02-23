/********************************************************************************************************
*�ļ����ƣ�main.cpp
*˵����EasyMonitorԴ�ļ�
*
*���ڣ�2016.10
*�汾��1.0
*��ע���˳����ԡ�C++�ڿͱ�̽����������2�桷�����Ʊࣩ�е�Monitor����Ϊ�������ԸĽ�
*by wey
********************************************************************************************************/

#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <string.h>

//ȫ�ֱ�������ұ�ѧ������ȫ�ֱ�������ֻ��Ϊ�˷��㣩
char szDirectory[MAX_PATH]={"0"};//�����·�������ô���������
char tempDirectory[MAX_PATH]={"0"};//���ô��̲߳���
//char f
CRITICAL_SECTION handle_lock;//�ٽ�����
SYSTEMTIME systime;//ϵͳʱ��


void GetDirectory();//�õ�Ҫ���Ŀ¼
void StartThread();//�����߳�
DWORD WINAPI MonitorThread(LPVOID lpParam);//����߳�

void GetDirectory()
{
    printf("Notice: The longest length is 259.\nFormat: c:\\***;d:\\***\ninput: ");

    for(int i=0;i<MAX_PATH-1;i++)//�������[259]������<=
    {
        if((szDirectory[i]=getchar())=='\n')
        {
            szDirectory[i]='\0';
            return;//ֱ�ӷ���
        }
    }
    szDirectory[MAX_PATH-1]='\0';//�ַ���������ɣ���ֹ�����
}

void StartThread()
{
    int i=0;

    for(int i=0,a=0;i<=strlen(szDirectory);i++,a++)//�ڵ�strlen(szDirectory)-1���ǲ���������
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
                //printf("���ݣ�%s\n",tempDirectory);
                CloseHandle(CreateThread(NULL,0,MonitorThread,NULL,0,NULL));
                Sleep(100);
                ZeroMemory(tempDirectory,MAX_PATH);
                a=-1;
            }
        }
        else
        {
            tempDirectory[a]='\0';
            //printf("����2��%s\n",tempDirectory);
            CloseHandle(CreateThread(NULL,0,MonitorThread,NULL,0,NULL));
            Sleep(100);
            return;
        }
        //printf("%s %s\n",tempDirectory,szDirectory);
    }
}

DWORD WINAPI MonitorThread(LPVOID lpParam)
{
    if(handle_lock.LockCount==-1)//��ֹ�������߳���һ���ٽ�����-1�������߳̽���
    {
        EnterCriticalSection(&handle_lock);
    }

    TCHAR temp[MAX_PATH];//Ϊunicode����
    char szLastFileName[MAX_PATH]={0};//��ֹ��μ�¼����ˢ��
    WORD nLastFileTime=0;//˳���һ���ϴε�ʱ�䣬��ֹֻ��ͬһ�ļ��ڲ�ͬʱ�䱻�޸Ĳ��������
    MultiByteToWideChar(CP_ACP, 0, tempDirectory, -1, temp, MAX_PATH);//ANSI�ַ�תunicode
    BOOL bRet=FALSE;
    BYTE Buffer[1024]={0};
    FILE_NOTIFY_INFORMATION *pBuffer = (FILE_NOTIFY_INFORMATION *)Buffer;
    DWORD BytesReturned =0;
    HANDLE hFile=CreateFile(temp,FILE_LIST_DIRECTORY,
        FILE_SHARE_READ|FILE_SHARE_DELETE|FILE_SHARE_WRITE,
        NULL,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS,NULL);
    if(INVALID_HANDLE_VALUE==hFile)//�Ҳ���
    {
        printf("Fail to start. Press any key to continue.\n");
        return 1;
    }

    printf("Start to monitor %s.\n",tempDirectory);//����

    while(1)
    {
        ZeroMemory(Buffer,1024);

        bRet=ReadDirectoryChangesW(hFile,&Buffer,sizeof(Buffer),
            TRUE,FILE_NOTIFY_CHANGE_FILE_NAME|//�޸��ļ���
            FILE_NOTIFY_CHANGE_ATTRIBUTES|//�޸��ļ�����
            FILE_NOTIFY_CHANGE_LAST_WRITE,//���һ��д��
            &BytesReturned,NULL,NULL);
        if(bRet==TRUE)
        {
            char szFileName[MAX_PATH]={0};
            GetLocalTime(&systime);//�õ���ǰʱ��
            WideCharToMultiByte(CP_ACP,0,pBuffer->FileName,pBuffer->FileNameLength/2,
                szFileName,
                MAX_PATH,
                NULL,NULL);//unicode�ַ�תANSI
            if(strcmp(szFileName,szLastFileName)==0 && nLastFileTime==systime.wSecond)//�Ѿ���ͬһ���ļ���¼��
            {
                continue;
            }
            strcpy(szLastFileName,szFileName);//���¼
            nLastFileTime=systime.wSecond;//��ʱ��
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
                            szFileName,MAX_PATH,NULL,NULL);//unicode�ַ�תANSI
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
    InitializeCriticalSection(&handle_lock);//��ʼ���ٽ�����
    printf("Easy Monitor 1.0\n");
    GetDirectory();
    StartThread();
    Sleep(300);//�������߳�����
    EnterCriticalSection(&handle_lock);//����Զ��ͣ����
    return 0;
}