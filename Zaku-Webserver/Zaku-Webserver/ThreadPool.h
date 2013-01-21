//TheadPool.h
/*
#pragma once
#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include "Thread.h"

class ThreadPool
{
public:
	ThreadPool(void);
public:
	~ThreadPool(void);

private:
	Thread*	m_ptrCThread[5];
	HANDLE	m_hThreadPool[5]; // Handle will be used in the end of Pool MGR for waiting on all thread to end
	int		m_nThreadCount;
public:

	void	Initialize(int nThread);    
	void	AssignTask( char* strTask );
	void	ShutDown(void);
	int		GetFreeThread(void);
	char*	GetTaskMessage(void);
	int		GetThreadCount(void);
	HANDLE	GetMutex(void);
};

#endif // THREAD_POOL_H*/