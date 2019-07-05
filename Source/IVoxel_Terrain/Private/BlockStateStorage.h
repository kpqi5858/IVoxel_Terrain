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
};

class FBasicBlockStorage : public FAbstractBlockStorage
{
protected:
	uint16 InternalStorage[VOX_CHUNKSIZE_ARRAY];

public:
	FBasicBlockStorage()
	{
		uint16 AirIndex = GETBLOCK_C("Air")->UniqueIndex;

		for (auto& Block : InternalStorage)
		{
			Block = AirIndex;
		}
	}
	virtual ~FBasicBlockStorage() override
	{
	};

	virtual void SetBlock(int Index, UBlock* Block) override
	{
		check(Index >= 0 && Index < VOX_CHUNKSIZE_ARRAY);
		InternalStorage[Index] = Block->UniqueIndex;
	}

	virtual UBlock* GetBlock(int Index) override
	{
		return GETBLOCK_INDEX(InternalStorage[Index]);
	}
};

//Storage for additional block info
//Ex) Color, Plant growth state, etc..
class FBlockDataStorage
{

};