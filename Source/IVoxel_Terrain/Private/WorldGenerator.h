#pragma once

#include "VoxelWorld.h"

#include "WorldGenerator.generated.h"

typedef TFunction<void(FBlockState*)> FWorldGenBlockOperation;

UCLASS(Abstract)
class UVoxelWorldGenerator : public UObject
{
	GENERATED_BODY()
public:
	UVoxelChunk* OwnerChunk;
	TSet<UVoxelChunk*> TouchedChunks;

public:
	UVoxelWorldGenerator()
	{ };
	virtual ~UVoxelWorldGenerator()
	{

	};

	void Setup(UVoxelChunk* Owner)
	{
		OwnerChunk = Owner;
	}
	
	virtual void Generate()
	{
		OwnerChunk->BlockStateStorageLock();
		GenerateInternal();
		for (auto& Touched : TouchedChunks)
		{
			Touched->BlockStateStorageUnlock();
		}
		OwnerChunk->BlockStateStorageUnlock();
	}

	virtual void GenerateInternal()
	{
		unimplemented();
		
		for (int X = 0; X < VOX_CHUNKSIZE; X++)
		{
			for (int Y = 0; Y < VOX_CHUNKSIZE; Y++)
			{
				for (int Z = 0; Z < VOX_CHUNKSIZE; Z++)
				{
					FIntVector GlobalPos = OwnerChunk->GetGlobalPosition_Min() + FIntVector(X, Y, Z);
					
				}
			}
		}
	}

	void EditBlock(FBlockPos Pos, FWorldGenBlockOperation Func)
	{
		UVoxelChunk* Chunk = Pos.GetChunk();
		if (Chunk != OwnerChunk)
		{
			TouchedChunks.Add(Chunk);
			Chunk->BlockStateStorageLock();
		}
		Func(Chunk->GetBlockState(Pos));
	}
};

UCLASS()
class UFlatWorldGenerator : public UVoxelWorldGenerator
{
	GENERATED_BODY()
public:
	virtual ~UFlatWorldGenerator()
	{ }

	virtual void GenerateInternal() override
	{
		for (int X = 0; X < VOX_CHUNKSIZE; X++)
		{
			for (int Y = 0; Y < VOX_CHUNKSIZE; Y++)
			{
				for (int Z = 0; Z < VOX_CHUNKSIZE; Z++)
				{
					FIntVector GlobalPos = OwnerChunk->GetGlobalPosition_Min() + FIntVector(X, Y, Z);

				}
			}
		}
	};
};