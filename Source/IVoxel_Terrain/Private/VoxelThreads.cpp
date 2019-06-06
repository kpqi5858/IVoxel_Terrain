#include "VoxelThreads.h"
#include "VoxelWorld.h"

/**
* The real thread entry point. It waits for work events to be queued. Once
* an event is queued, it executes it and goes back to waiting.
*/

uint32 FMyQueuedThread::Run()
{
	while (!TimeToDie.Load(EMemoryOrder::Relaxed))
	{
		// This will force sending the stats packet from the previous frame.
		SET_DWORD_STAT(STAT_MyThreadPoolDummyCounter, 0);
		// We need to wait for shorter amount of time
		bool bContinueWaiting = true;
		while (bContinueWaiting)
		{
			DECLARE_SCOPE_CYCLE_COUNTER(TEXT("FMyQueuedThread::Run.WaitForWork"), STAT_FMyQueuedThread_Run_WaitForWork, STATGROUP_ThreadPoolAsyncTasks);

			// Wait for some work to do
			bContinueWaiting = !DoWorkEvent->Wait(10);
		}

		IMyQueuedWork* LocalQueuedWork = QueuedWork;
		QueuedWork = nullptr;
		FPlatformMisc::MemoryBarrier();
		check(LocalQueuedWork || TimeToDie.Load(EMemoryOrder::Relaxed)); // well you woke me up, where is the job or termination request?
		while (LocalQueuedWork)
		{
			// Tell the object to do the work
			LocalQueuedWork->DoThreadedWork();
			// Let the object cleanup before we remove our ref to it
			LocalQueuedWork = OwningThreadPool->ReturnToPoolOrGetNextJob(this);
		}
	}
	return 0;
}

/**
* Creates the thread with the specified stack size and creates the various
* events to be able to communicate with it.
*
* @param InPool The thread pool interface used to place this thread back into the pool of available threads when its work is done
* @param InStackSize The size of the stack to create. 0 means use the current thread's stack size
* @param ThreadPriority priority of new thread
* @return True if the thread and all of its initialization was successful, false otherwise
*/

bool FMyQueuedThread::Create(FMyQueuedThreadPool * InPool, uint32 InStackSize, EThreadPriority ThreadPriority)
{
	static int32 PoolThreadIndex = 0;
	const FString PoolThreadName = FString::Printf(TEXT("MyPoolThread %d"), PoolThreadIndex);
	PoolThreadIndex++;

	OwningThreadPool = InPool;
	DoWorkEvent = FPlatformProcess::GetSynchEventFromPool();
	Thread = FRunnableThread::Create(this, *PoolThreadName, InStackSize, ThreadPriority, FPlatformAffinity::GetPoolThreadMask());
	check(Thread);
	return true;
}

/**
* Tells the thread to exit. If the caller needs to know when the thread
* has exited, it should use the bShouldWait value and tell it how long
* to wait before deciding that it is deadlocked and needs to be destroyed.
* NOTE: having a thread forcibly destroyed can cause leaks in TLS, etc.
*
* @return True if the thread exited graceful, false otherwise
*/

bool FMyQueuedThread::KillThread()
{
	bool bDidExitOK = true;
	// Tell the thread it needs to die
	TimeToDie = true;
	// Trigger the thread so that it will come out of the wait state if
	// it isn't actively doing work
	DoWorkEvent->Trigger();
	// If waiting was specified, wait the amount of time. If that fails,
	// brute force kill that thread. Very bad as that might leak.
	Thread->WaitForCompletion();
	// Clean up the event
	FPlatformProcess::ReturnSynchEventToPool(DoWorkEvent);
	DoWorkEvent = nullptr;
	delete Thread;
	return bDidExitOK;
}

/**
* Tells the thread there is work to be done. Upon completion, the thread
* is responsible for adding itself back into the available pool.
*
* @param InQueuedWork The queued work to perform
*/

void FMyQueuedThread::DoWork(IMyQueuedWork * InQueuedWork)
{
	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("FMyQueuedThread::DoWork"), STAT_FMyQueuedThread_DoWork, STATGROUP_ThreadPoolAsyncTasks);

	check(QueuedWork == nullptr && "Can't do more than one task at a time");
	// Tell the thread the work to be done
	QueuedWork = InQueuedWork;
	FPlatformMisc::MemoryBarrier();
	// Tell the thread to wake up and do its job
	DoWorkEvent->Trigger();
}

bool FMyQueuedThreadPool::Create(uint32 InNumQueuedThreads, uint32 StackSize, EThreadPriority ThreadPriority)
{
	// Make sure we have synch objects
	bool bWasSuccessful = true;
	check(SynchQueue == nullptr);
	SynchQueue = new FCriticalSection();
	FScopeLock Lock(SynchQueue);
	// Presize the array so there is no extra memory allocated
	check(QueuedThreads.Num() == 0);
	QueuedThreads.Empty(InNumQueuedThreads);

	// Check for stack size override.
	if (FQueuedThreadPool::OverrideStackSize > StackSize)
	{
		StackSize = FQueuedThreadPool::OverrideStackSize;
	}

	// Now create each thread and add it to the array
	for (uint32 Count = 0; Count < InNumQueuedThreads && bWasSuccessful == true; Count++)
	{
		// Create a new queued thread
		FMyQueuedThread* pThread = new FMyQueuedThread();
		// Now create the thread and add it if ok
		if (pThread->Create(this, StackSize, ThreadPriority) == true)
		{
			QueuedThreads.Add(pThread);
			AllThreads.Add(pThread);
		}
		else
		{
			// Failed to fully create so clean up
			bWasSuccessful = false;
			delete pThread;
		}
	}
	// Destroy any created threads if the full set was not successful
	if (bWasSuccessful == false)
	{
		Destroy();
	}
	return bWasSuccessful;
}

void FMyQueuedThreadPool::Destroy()
{
	if (SynchQueue)
	{
		{
			FScopeLock Lock(SynchQueue);
			TimeToDie = 1;
			FPlatformMisc::MemoryBarrier();
			// Clean up all queued objects

			while (!QueuedWork.empty())
			{
				QueuedWork.top()->Abandon();
				QueuedWork.pop();
			}
		}
		// wait for all threads to finish up
		while (1)
		{
			{
				FScopeLock Lock(SynchQueue);
				if (AllThreads.Num() == QueuedThreads.Num())
				{
					break;
				}
			}
			FPlatformProcess::Sleep(0.0f);
		}
		// Delete all threads
		{
			FScopeLock Lock(SynchQueue);
			// Now tell each thread to die and delete those
			for (int32 Index = 0; Index < AllThreads.Num(); Index++)
			{
				AllThreads[Index]->KillThread();
				delete AllThreads[Index];
			}
			QueuedThreads.Empty();
			AllThreads.Empty();
		}
		delete SynchQueue;
		SynchQueue = nullptr;
	}
}

int32 FMyQueuedThreadPool::GetNumQueuedJobs() const
{
	// this is a estimate of the number of queued jobs. 
	// no need for thread safe lock as the queuedWork array isn't moved around in memory so unless this class is being destroyed then we don't need to wrory about it
	return QueuedWork.size();
}

int32 FMyQueuedThreadPool::GetNumThreads() const
{
	return AllThreads.Num();
}

void FMyQueuedThreadPool::AddQueuedWork(IMyQueuedWork * InQueuedWork)
{
	check(InQueuedWork != nullptr);

	if (TimeToDie)
	{
		InQueuedWork->Abandon();
		return;
	}

	// Check to see if a thread is available. Make sure no other threads
	// can manipulate the thread pool while we do this.
	//
	// We pick a thread from the back of the array since this will be the
	// most recently used thread and therefore the most likely to have
	// a 'hot' cache for the stack etc (similar to Windows IOCP scheduling
	// strategy). Picking from the back also happens to be cheaper since
	// no memory movement is necessary.

	check(SynchQueue);

	FMyQueuedThread* Thread = nullptr;

	{
		FScopeLock sl(SynchQueue);
		const int32 AvailableThreadCount = QueuedThreads.Num();
		if (AvailableThreadCount == 0)
		{
			// No thread available, queue the work to be done
			// as soon as one does become available
			QueuedWork.push(InQueuedWork);

			return;
		}

		const int32 ThreadIndex = AvailableThreadCount - 1;

		Thread = QueuedThreads[ThreadIndex];
		// Remove it from the list so no one else grabs it
		QueuedThreads.RemoveAt(ThreadIndex, 1, /* do not allow shrinking */ false);
	}

	// Tell our chosen thread to do the work
	Thread->DoWork(InQueuedWork);
}

IMyQueuedWork* FMyQueuedThreadPool::ReturnToPoolOrGetNextJob(FMyQueuedThread* InQueuedThread)
{
	check(InQueuedThread != nullptr);
	IMyQueuedWork* Work = nullptr;
	// Check to see if there is any work to be done
	FScopeLock sl(SynchQueue);
	if (TimeToDie)
	{
		check(!QueuedWork.size());  // we better not have anything if we are dying
	}
	if (QueuedWork.size() > 0)
	{
		// Grab the oldest work in the queue. This is slower than
		// getting the most recent but prevents work from being
		// queued and never done
		Work = QueuedWork.top();
		QueuedWork.pop();
	}
	if (!Work)
	{
		// There was no work to be done, so add the thread to the pool
		QueuedThreads.Add(InQueuedThread);
	}
	return Work;
}


FVoxelPolygonizerThread::FVoxelPolygonizerThread(AVoxelChunkRender* Render)
	: Chunk(Render)
{
}

void FVoxelPolygonizerThread::DoThreadedWork()
{
	Chunk->Polygonize();
	delete this;
}

void FVoxelPolygonizerThread::Abandon()
{
	delete this;
}

FWorldGeneratorThread::FWorldGeneratorThread(UVoxelChunk* Chunk)
	: Chunk(Chunk)
{
}

void FWorldGeneratorThread::DoThreadedWork()
{
	Chunk->GenerateWorld();
	delete this;
}

void FWorldGeneratorThread::Abandon()
{
	delete this;
}

FPostWorldGeneratorThread::FPostWorldGeneratorThread(UVoxelChunk* Chunk)
	: Chunk(Chunk)
{
}

void FPostWorldGeneratorThread::DoThreadedWork()
{
	Chunk->PostGenerateWorld();
	delete this;
}

void FPostWorldGeneratorThread::Abandon()
{
	delete this;
}

FUpdateVisiblityThread::FUpdateVisiblityThread(UVoxelChunk* Chunk)
	: Chunk(Chunk)
{
}

void FUpdateVisiblityThread::DoThreadedWork()
{
	Chunk->UpdateFaceVisiblityAll();
	delete this;
}

void FUpdateVisiblityThread::Abandon()
{
	delete this;
}

FVoxelChunkLoaderThread::FVoxelChunkLoaderThread(AVoxelWorld* World)
	: World(World)
{
}

void FVoxelChunkLoaderThread::DoThreadedWork()
{
	World->InitChunkAroundInvoker();
	IsDone = true;
}

void FVoxelChunkLoaderThread::Abandon()
{
}
