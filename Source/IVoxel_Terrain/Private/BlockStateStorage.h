#pragma once

#include "VoxelWorld.h"
#include "VoxelData.h"
#include "BlockState.h"

//UBlock storage inside chunk
class FAbstractBlockStorage
{
public:
	FAbstractBlockStorage()
	{

	}
	virtual ~FAbstractBlockStorage()
	{

	};

	virtual void SetBlock(int Index, UBlock* Block) = 0;
	virtual UBlock* GetBlock(int Index) = 0;

	virtual void Lock() = 0;
	virtual void UnLock() = 0;
};

class FBasicBlockStorage : public FAbstractBlockStorage
{
protected:
	FCriticalSection CriticalSection;

	UBlock* InternalStorage[VOX_CHUNKSIZE_ARRAY];

public:
	FBasicBlockStorage()
	{
		for (auto& Block : InternalStorage)
		{
			Block = GETBLOCK_C("Air");
		}
	}
	virtual ~FBasicBlockStorage()
	{
	};

	virtual void SetBlock(int Index, UBlock* Block) override
	{
		check(Index >= 0 && Index < VOX_CHUNKSIZE_ARRAY);
		InternalStorage[Index] = Block;
	}

	virtual UBlock* GetBlock(int Index) override
	{
		return InternalStorage[Index];
	}

	virtual void Lock() override
	{
		CriticalSection.Lock();
	}

	virtual void UnLock() override
	{
		CriticalSection.Unlock();
	}
};

//Storage for additional block info
//Ex) Color, Plant growth state, etc..
class FBlockDataStorage
{

};