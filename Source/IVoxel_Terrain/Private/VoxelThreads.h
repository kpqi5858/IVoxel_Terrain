#pragma once

#include "VoxelChunkRender.h"
#include "HAL/ThreadingBase.h"
#include <queue>

DECLARE_DWORD_COUNTER_STAT(TEXT("MyThreadPoolDummyCounter"), STAT_MyThreadPoolDummyCounter, STATGROUP_ThreadPoolAsyncTasks);

class IMyQueuedWork
{
public:
	virtual void DoThreadedWork() = 0;

	virtual void Abandon() = 0;

	virtual int GetPriority() const = 0;

public:
	virtual ~IMyQueuedWork() { }
};

class FMyQueuedThread
	: public FRunnable
{
protected:

	/** The event that tells the thread there is work to do. */
	FEvent* DoWorkEvent;

	/** If true, the thread should exit. */
	TAtomic<bool> TimeToDie;

	/** The work this thread is doing. */
	IMyQueuedWork* volatile QueuedWork;

	/** The pool this thread belongs to. */
	class FMyQueuedThreadPool* OwningThreadPool;

	/** My Thread  */
	FRunnableThread* Thread;

	/**
	 * The real thread entry point. It waits for work events to be queued. Once
	 * an event is queued, it executes it and goes back to waiting.
	 */
	virtual uint32 Run() override;

public:

	/** Default constructor **/
	FMyQueuedThread()
		: DoWorkEvent(nullptr)
		, TimeToDie(false)
		, QueuedWork(nullptr)
		, OwningThreadPool(nullptr)
		, Thread(nullptr)
	{ }

	/**
	 * Creates the thread with the specified stack size and creates the various
	 * events to be able to communicate with it.
	 *
	 * @param InPool The thread pool interface used to place this thread back into the pool of available threads when its work is done
	 * @param InStackSize The size of the stack to create. 0 means use the current thread's stack size
	 * @param ThreadPriority priority of new thread
	 * @return True if the thread and all of its initialization was successful, false otherwise
	 */
	virtual bool Create(class FMyQueuedThreadPool* InPool, uint32 InStackSize = 0, EThreadPriority ThreadPriority = TPri_Normal);
	
	/**
	 * Tells the thread to exit. If the caller needs to know when the thread
	 * has exited, it should use the bShouldWait value and tell it how long
	 * to wait before deciding that it is deadlocked and needs to be destroyed.
	 * NOTE: having a thread forcibly destroyed can cause leaks in TLS, etc.
	 *
	 * @return True if the thread exited graceful, false otherwise
	 */
	virtual bool KillThread();

	/**
	 * Tells the thread there is work to be done. Upon completion, the thread
	 * is responsible for adding itself back into the available pool.
	 *
	 * @param InQueuedWork The queued work to perform
	 */
	void DoWork(IMyQueuedWork* InQueuedWork);

};

class FQueuedWorkCompare
{
public:
	inline bool operator() (const IMyQueuedWork* A, const IMyQueuedWork* B)
	{
		return A->GetPriority() < B->GetPriority();
	}
};

class FMyQueuedThreadPool
{
protected:

	/** The work queue to pull from. */
	std::priority_queue<IMyQueuedWork*, std::vector<IMyQueuedWork*>, FQueuedWorkCompare> QueuedWork;

	/** The thread pool to dole work out to. */
	TArray<FMyQueuedThread*> QueuedThreads;

	/** All threads in the pool. */
	TArray<FMyQueuedThread*> AllThreads;

	/** The synchronization object used to protect access to the queued work. */
	FCriticalSection* SynchQueue;

	/** If true, indicates the destruction process has taken place. */
	bool TimeToDie;

public:

	/** Default constructor. */
	FMyQueuedThreadPool()
		: SynchQueue(nullptr)
		, TimeToDie(0)
	{ }

	/** Virtual destructor (cleans up the synchronization objects). */
	virtual ~FMyQueuedThreadPool()
	{
		Destroy();
	}

	virtual bool Create(uint32 InNumQueuedThreads, uint32 StackSize = (32 * 1024), EThreadPriority ThreadPriority = TPri_Normal);

	virtual void Destroy();

	int32 GetNumQueuedJobs() const;
	virtual int32 GetNumThreads() const;
	void AddQueuedWork(IMyQueuedWork* InQueuedWork);

	virtual IMyQueuedWork* ReturnToPoolOrGetNextJob(FMyQueuedThread* InQueuedThread);
};





class FVoxelPolygonizerThread : public IMyQueuedWork
{
public:
	FVoxelPolygonizerThread(AVoxelChunkRender* Render);

	AVoxelChunkRender* Chunk;

	void DoThreadedWork() override;
	void Abandon() override;
	int GetPriority() const override
	{
		return 0;
	};
};

class FWorldGeneratorThread : public IMyQueuedWork
{
public:
	FWorldGeneratorThread(UVoxelChunk* Chunk);

	UVoxelChunk* Chunk;

	void DoThreadedWork() override;
	void Abandon() override;
	int GetPriority() const override
	{
		return 0;
	};
};

class FPostWorldGeneratorThread : public IMyQueuedWork
{
public:
	FPostWorldGeneratorThread(UVoxelChunk* Chunk);

	UVoxelChunk* Chunk;

	void DoThreadedWork() override;
	void Abandon() override;
	int GetPriority() const override
	{
		return 0;
	};
};