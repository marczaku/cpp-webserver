// Thread.cpp

#include "stdafx.h"
#include "Thread.h"

Thread::Thread(void)
{
    // Initialize members
    m_ThreadID    = 0;
    m_hThread    = NULL;
    m_bIsFree    = TRUE;
    m_strMessage = "";

    m_hWorkEvent[0] = CreateEvent(NULL, TRUE, FALSE, NULL);
    m_hWorkEvent[1] = CreateEvent(NULL, TRUE, FALSE, NULL);
}
//---------------------------------------
Thread::~Thread(void)
{

}
//---------------------------------------
DWORD WINAPI Thread::ThreadProc(LPVOID Param)
{
    // Create object of ThreadPoolMgr
    Thread* ptrThread = (Thread*)Param;
    
    BOOL bShutDown = FALSE;

    while( !bShutDown )
    {
        DWORD dwWaitResult = WaitForMultipleObjects( 2, ptrThread->m_hWorkEvent , FALSE, INFINITE);
        switch(dwWaitResult)
        {
        case WAIT_OBJECT_0 :
            // Work event signaled Call the Run function    
            ptrThread->Run();
            break;

        case WAIT_OBJECT_0 + 1 :
            bShutDown = TRUE;
            break;
        }
    }

    return 0;
}
//---------------------------------------
void Thread::Run(void)
{    
	/*
    ofstream myfile;
    myfile.open(m_strMessage.c_str());
    
    myfile << "Thread " << GetCurrentThreadId() <<" is Running.\n";
    myfile << "Thread Message: "<< m_strMessage.c_str() << "\n";
    myfile.close();*/
    /*cout << "Thread " << GetCurrentThreadId() <<" is Running.\n";
    cout << "Thread Message: "<< m_strMessage.c_str() << "\n";*/
    
    //Sleep(3000);

    // work is done. Now 
    // 1. m_bIsFree flag to true
    // 2. make work event to nonsiganled and 
    // 3. set message string to empty
    m_bIsFree = TRUE;
    m_strMessage="";
    ResetEvent(m_hWorkEvent[0]);
}
//---------------------------------------
BOOL Thread::IsFree()
{    
    return m_bIsFree;
}
//---------------------------------------
void Thread::CreateWorkerThread()
{
    m_hThread = CreateThread(NULL, NULL, ThreadProc, (LPVOID)this, NULL, &m_ThreadID);
    
    //if( m_hThread == NULL )
        //cout << "Thread could not be created: " << GetLastError() << "\n";
    //else
        //cout << "Successfully created thread: " << m_ThreadID << "\n";
}
//---------------------------------------
HANDLE Thread::GetThreadHandle()
{
    return m_hThread;
}
//---------------------------------------
DWORD Thread::GetThreadID()
{
    return m_ThreadID;
}
//---------------------------------------
void Thread::SetMessage(char* strMessage)
{
    m_strMessage = strMessage;
}
//---------------------------------------
void Thread::SignalWorkEvent()
{
    SetEvent( m_hWorkEvent[0] );
}
//---------------------------------------
void Thread::SignalShutDownEvent()
{
    SetEvent( m_hWorkEvent[1] );
}
//---------------------------------------
void Thread::SetThreadBusy()
{
    m_bIsFree = FALSE;
}
//---------------------------------------
void Thread::ReleaseHandles()
{
    // Close all handles 
    CloseHandle(m_hThread);
    CloseHandle(m_hWorkEvent[0]);
    CloseHandle(m_hWorkEvent[1]);
}