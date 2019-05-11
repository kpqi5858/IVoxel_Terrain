#pragma once

#include "Block.h"
#include "VoxelChunk.h"
#include "IVoxel_Terrain.h"
#include "VoxelData.generated.h"

class AVoxelChunk;
class AVoxelWorld;

USTRUCT(BlueprintType)
struct FBlockPos
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY(BlueprintReadWrite)
	AVoxelChunk* Chunk;

	UPROPERTY(BlueprintReadWrite)
	FIntVector ChunkLocalPos;
private:
	inline void ChunkLocalPosValidCheck()
	{
		check(ChunkLocalPos.GetMin() >= 0);
		check(ChunkLocalPos.GetMax() < VOX_CHUNKSIZE);
	}
	inline void ValidCheck()
	{
		check(Chunk);
		ChunkLocalPosValidCheck();
	}

public:
	FBlockPos()
		: Chunk(nullptr), ChunkLocalPos(FIntVector(0))
	{ }

	FBlockPos(AVoxelChunk* mChunk, FIntVector LocalPos)
		: Chunk(mChunk), ChunkLocalPos(LocalPos)
	{ }

	FIntVector GetGlobalPosition()
	{
		ValidCheck();
		return Chunk->LocalToGlobalPosition(ChunkLocalPos);
	}
	
	AVoxelWorld* GetWorld()
	{
		return Chunk->GetVoxelWorld();
	}
public:
	int ArrayIndex()
	{
		ChunkLocalPosValidCheck();
		return VOX_CHUNK_AI(ChunkLocalPos.X, ChunkLocalPos.Y, ChunkLocalPos.Z);
	}
};