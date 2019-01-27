#include "workerthread.h"
#include <QDebug>
#include <windows.h>
#include <TlHelp32.h>

WorkerThread::WorkerThread(int pid,QThread *parent) : QThread(parent)
{
    m_pId = pid;
}
void WorkerThread::run()
{
    injectCode();
}
// 注入的可执行代码，执行弹框
DWORD WINAPI CodeFunc(LPVOID param)
{
    MyData* data = (MyData*)param;
    typedef HMODULE (__stdcall *MyLoadLibrary)(LPCSTR);
    typedef FARPROC (__stdcall *MyGetProcAddress)(HMODULE,LPCSTR);
    typedef DWORD (__stdcall *MyGetModuleFileName)(HMODULE,LPSTR,DWORD);
    typedef int (__stdcall *MyMessageBox)(HWND,LPCSTR,LPCSTR,UINT);
    MyLoadLibrary myLoadLibrary = (MyLoadLibrary)data->dwLoadLibrary;
    MyGetProcAddress myGetProcAddress = (MyGetProcAddress)data->dwGetProcAddress;
    MyGetModuleFileName myGetModuleFileName = (MyGetModuleFileName)data->dwGetModuleFileName;

    HMODULE user32dll = myLoadLibrary(data->user32dll);
    MyMessageBox myMessageBox = (MyMessageBox)myGetProcAddress(user32dll,data->MessageBoxFun);
    char curFile[MAX_PATH] = {0};
    myGetModuleFileName(NULL,curFile,MAX_PATH);
    char tmp[MAX_PATH] = {0};
    sprintf_s(tmp,"当前进程：%s",curFile);
    myMessageBox(NULL,"aaa","bbb",MB_OK);
    //myMessageBox(NULL,data->msg,tmp,MB_OK);
    return 0;
}
// 注入Code
void WorkerThread::injectCode()
{
    HANDLE targetProc = OpenProcess(PROCESS_ALL_ACCESS,FALSE,m_pId);
    if( targetProc == NULL )
    {
        qDebug() << "OpenProcess error";
        return;
    }
    MyData data = {0};
    strcpy(data.user32dll,"user32.dll");
    strcpy(data.MessageBoxFun,"MessageBoxA");
    strcpy(data.msg,"Inject Code!");
    HMODULE module = LoadLibraryA("kernel32.dll");
    data.dwLoadLibrary = (DWORD)GetProcAddress(module,"LoadLibraryA");
    data.dwGetProcAddress = (DWORD)GetProcAddress(module,"GetProcAddress");
    data.dwGetModuleFileName = (DWORD)GetProcAddress(module,"GetModuleFileNameA");
    int dataLen = sizeof(MyData);

    // 1.目标进程申请空间 存放data数据，也就是可执行代码函数的参数
    LPVOID pData = VirtualAllocEx(targetProc,NULL,dataLen,MEM_COMMIT | MEM_RESERVE,PAGE_READWRITE );
    if( pData == NULL )
    {
        qDebug() << "VirtualAllocEx error 1";
        return;
    }
    int ret = WriteProcessMemory(targetProc,pData,&data,dataLen,NULL);
    if( ret == 0 )
    {
        qDebug() << "WriteProcessMemory error 1";
        return;
    }

    // 2.目标进程申请空间 存放CodeFunc可执行代码
    int codeLen = 0x4000;
    LPVOID pCode = VirtualAllocEx(targetProc,NULL,codeLen,MEM_COMMIT,PAGE_EXECUTE_READWRITE );
    if( pCode == NULL )
    {
        qDebug() << "VirtualAllocEx error 2";
        return;
    }
    ret = WriteProcessMemory(targetProc,pCode,(LPCVOID)&CodeFunc,codeLen,NULL);
    if( ret == 0 )
    {
        qDebug() << "WriteProcessMemory error 2";
        return;
    }

    // 3.创建远程线程 执行可执行代码
    HANDLE tHandle = CreateRemoteThread(targetProc,NULL,NULL,
                       (LPTHREAD_START_ROUTINE)pCode,pData,NULL,NULL);
    if(tHandle == NULL)
    {
        qDebug() << "CreateRemoteThread error";
        return ;
    }
    qDebug() << "注入，wait ..." ;
    WaitForSingleObject(tHandle,INFINITY);
    CloseHandle(tHandle);
    CloseHandle(targetProc);
    qDebug() << "注入，finish ...";
    emit doInjectFinish();
}
