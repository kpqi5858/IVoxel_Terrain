#pragma once

#include "VoxelWorld.h"
#include "VoxelData.h"
#include "BlockState.h"

class FAbstractBlockStateStorage
{
public:
	FAbstractBlockStateStorage()
	{
		checkNoEntry();
	}
	virtual ~FAbstractBlockStateStorage();
public:
	virtual void Initialize(AVoxelChunk* Chunk);

	//Gets BlockState from FBlockPos
	virtual FBlockState* operator[](FBlockPos Pos);

	virtual void Lock();
	virtual void UnLock();

	virtual void Save(FArchive* Archive);
	virtual void Load(FArchive* Archive);
};

class FBasicBlockStateStorage : public FAbstractBlockStateStorage
{
private:
	FCriticalSection CriticalSection;

	FBlockState* InternalStorage;

	AVoxelChunk* TheChunk;

public:
	FBasicBlockStateStorage();
	virtual ~FBasicBlockStateStorage() override;

	virtual void Initialize(AVoxelChunk* Chunk) override;

	virtual FBlockState* operator[](FBlockPos Pos) override;

	virtual void Lock() override;
	virtual void UnLock() override;

	//Unimplemented
	virtual void Save(FArchive* Archive) override;
	//Unimplemented
	virtual void Load(FArchive* Archive) override;
};