#ifndef WORKERTHREAD_H
#define WORKERTHREAD_H

#include <QThread>
#include <windows.h>

#define STRLEN 20
typedef struct _mydata{
    DWORD dwLoadLibrary;
    DWORD dwGetProcAddress;
    DWORD dwGetModuleFileName;

    char user32dll[STRLEN];
    char MessageBoxFun[STRLEN];
    char msg[STRLEN];
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
