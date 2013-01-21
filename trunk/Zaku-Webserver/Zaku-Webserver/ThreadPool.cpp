#include "stdafx.h"
/*#include "ThreadPool.h"

void ThreadPool::Initialize(int nThread)
{
    m_nThreadCount = nThread;

    int nCounter = 0;
    int nThreadCount = m_nThreadCount - 1;
    
    while( nCounter <= nThreadCount )
    {
        // Create objects in heap
        m_ptrCThread[nCounter] = new Thread();

        m_ptrCThread[nCounter]->CreateWorkerThread();
        m_hThreadPool[nCounter] = m_ptrCThread[nCounter]->GetThreadHandle();
        // Increment the counter
        nCounter++;
    }    
}
//---------------------------------------
void ThreadPool::ShutDown()
{
    int Count = 0;

    while(Count <= (m_nThreadCount - 1))
    {
        m_ptrCThread[Count]->SignalShutDownEvent();
        
        Count++;
    }

    // Check if all threads ended successfully
    DWORD dwWaitResult = WaitForMultipleObjects( GetThreadCount(), m_hThreadPool, TRUE, INFINITE);
    
    switch(dwWaitResult)
    {
    case WAIT_OBJECT_0:
        {
            //cout << "All threads are ended.\n";
            // Close all handles 
            Count = 0;
            while( Count <= (m_nThreadCount - 1))
            {
                m_ptrCThread[Count]->ReleaseHandles();
                delete m_ptrCThread[Count];
                Count++;
            }
            
            break;
        }

    //default:
        //cout << "Wait Error: " << GetLastError() << "\n";
    }
    
}
//---------------------------------------
int ThreadPool::GetFreeThread()
{
    // Search which thread is free
    int count = 0;
    bool bIsAllBusy = true;
    while(count <= (m_nThreadCount - 1))
    {
        if(m_ptrCThread[count]->IsFree() == TRUE)
        {
            return count;
        }
        else
        {
            //cout << "Thread " << count << ": " << m_ptrCThread[count]->GetThreadID() << " is busy!!!\n";
        }
        
        count++;
    }
    if( bIsAllBusy )
    {
        //cout << "All thread are busy. Wait for thread to be free!!!\n" ;
        return -1;
    }
}
//---------------------------------------
void ThreadPool::AssignTask( char* strTask )
{
    int Count = GetFreeTherad();
    if( Count != -1)
    {
        m_ptrCThread[Count]->SetThreadBusy();
        
        // Set information to thread member so that thread can use it
        m_ptrCThread[Count]->SetMessage(strTask);
                
        // Signal Work event
        m_ptrCThread[Count]->SignalWorkEvent();
    }
}
//---------------------------------------
int ThreadPool::GetThreadCount()
{
    return m_nThreadCount;
}
*/