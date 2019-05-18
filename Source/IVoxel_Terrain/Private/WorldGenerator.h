#pragma once

#include "VoxelWorld.h"

#include "WorldGenerator.generated.h"

typedef TFunction<void(FBlockState*)> FWorldGenBlockFunction;

UCLASS(Abstract)
class UVoxelWorldGenerator : public UObject
{
	GENERATED_BODY()
public:
	UVoxelWorldGenerator()
	{
		unimplemented();
	};
	virtual ~UVoxelWorldGenerator()
	{

	};

	virtual void Generate(UVoxelChunk* Chunk)
	{
		unimplemented();

		Chunk->BlockStateStorageLock();
		for (int X = 0; X < VOX_CHUNKSIZE; X++)
		{
			for (int Y = 0; Y < VOX_CHUNKSIZE; Y++)
			{
				for (int Z = 0; Z < VOX_CHUNKSIZE; Z++)
				{
					FIntVector GlobalPos = Chunk->GetGlobalPosition_Min() + FIntVector(X, Y, Z);
					//Do sturffs here
				}
			}
		}
		Chunk->BlockStateStorageUnlock();
	}

	void EditBlock(FWorldGenBlockFunction Func)
	{

	}
};

UCLASS()
class UFlatWorldGenerator : public UVoxelWorldGenerator
{
	GENERATED_BODY()
public:
	virtual ~UFlatWorldGenerator()
	{ }

	virtual void Generate(UVoxelChunk* Chunk) override
	{
		for (int X = 0; X < VOX_CHUNKSIZE; X++)
		{
			for (int Y = 0; Y < VOX_CHUNKSIZE; Y++)
			{
				for (int Z = 0; Z < VOX_CHUNKSIZE; Z++)
				{
					FIntVector GlobalPos = Chunk->GetGlobalPosition_Min() + FIntVector(X, Y, Z);
					if (GlobalPos.Z <= 0)
					{
						
					}
					else
					{

					}
				}
			}
		}
	};
};