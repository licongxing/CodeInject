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
    // 定义函数指针类型 方便下面的类型转换
    typedef HMODULE (__stdcall *MyLoadLibrary)(LPCSTR);
    typedef FARPROC (__stdcall *MyGetProcAddress)(HMODULE,LPCSTR);
    typedef DWORD (__stdcall *MyGetModuleFileName)(HMODULE,LPSTR,DWORD);
    typedef HMODULE (*MyGetModuleHandle)(LPCSTR);
    typedef int (__stdcall *MyMessageBox)(HWND,LPCSTR,LPCSTR,UINT);
    typedef char*(*Mystrcat)(char *dest, const char *src);


    // 获取函数地址
    MyLoadLibrary myLoadLibrary = (MyLoadLibrary)data->dwLoadLibrary;
    MyGetProcAddress myGetProcAddress = (MyGetProcAddress)data->dwGetProcAddress;
    MyGetModuleFileName myGetModuleFileName = (MyGetModuleFileName)data->dwGetModuleFileName;
    MyGetModuleHandle myGetModuleHandle = (MyGetModuleHandle)data->dwGetModuleHandle;
    // 所有用的函数，都需像下面一样 导出函数地址然后再使用，包括标准库函数也一样
    // 笔者刚开始就犯了一个错，直接使用sprintf、strcat 等函数，这样是不行！因为我们自己注入的代码，函数地址需要我们自己确定！
    HMODULE user32dll = myLoadLibrary(data->user32dll);
    HMODULE msvcrtdll = myGetModuleHandle(data->msvcrtdll);// msvcrt.dll 微软运行库，可直接获取
    MyMessageBox myMessageBox = (MyMessageBox)myGetProcAddress(user32dll,data->MessageBoxFun);
    Mystrcat mystrcat = (Mystrcat)myGetProcAddress(msvcrtdll,data->strcatFun);


    // 调用获取到的函数
    char curFile[MAX_PATH] = {0};
    myGetModuleFileName(NULL,curFile,MAX_PATH);
    mystrcat(data->content,curFile);
    myMessageBox(NULL, data->content ,data->caption, MB_OK);
    return 0;
}
// 注入Code操作
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
    strcpy(data.msvcrtdll,"msvcrt.dll");
    strcpy(data.MessageBoxFun,"MessageBoxA");
    strcpy(data.strcatFun,"strcat");
    strcpy(data.caption,"Inject Code!");
    strcpy(data.content,"current process:");// 这里使用中文会乱码

    HMODULE module = GetModuleHandleA("kernel32.dll");
    data.dwLoadLibrary = (DWORD)GetProcAddress(module,"LoadLibraryA");
    data.dwGetProcAddress = (DWORD)GetProcAddress(module,"GetProcAddress");
    data.dwGetModuleFileName = (DWORD)GetProcAddress(module,"GetModuleFileNameA");
    data.dwGetModuleHandle = (DWORD)GetProcAddress(module,"GetModuleHandleA");
    int dataLen = sizeof(MyData);

    // 1.目标进程申请空间 存放data数据，也就是注入的 可执行代码函数 的参数
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
    int codeLen = 0x4000; // 4 * 16^3 = 16KB足够了
    // 第5个参数，数据具有可执行权限
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
