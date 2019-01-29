#ifndef WORKERTHREAD_H
#define WORKERTHREAD_H

#include <QThread>
#include <windows.h>

#define STRLEN 20
typedef struct _mydata{
    // 4个不变函数的函数地址
    DWORD dwLoadLibrary;
    DWORD dwGetProcAddress;
    DWORD dwGetModuleFileName;
    DWORD dwGetModuleHandle;

    char user32dll[STRLEN]; // user32.dll 含有MessageBoxA函数
    char MessageBoxFun[STRLEN]; // "MessageBoxA" 字符串

    char msvcrtdll[STRLEN]; // MSVCRT.DLL 微软运行库 含有strcat函数，记住所有的 函数都需通过这种方式进行导出 然后才能使用
    char strcatFun[STRLEN]; // "strcat" 字符串

    char caption[STRLEN]; // 消息框标题 "Inject Code!"
    char content[MAX_PATH]; // 消息框内容
}MyData;



class WorkerThread : public QThread
{
    Q_OBJECT
public:
    explicit WorkerThread(int pid,QThread *parent = nullptr);
    void run();
signals:
    void doInjectFinish();

private:
    // 注入代码
    void injectCode();
public slots:

private:
    int m_pId;
};

#endif // WORKERTHREAD_H
