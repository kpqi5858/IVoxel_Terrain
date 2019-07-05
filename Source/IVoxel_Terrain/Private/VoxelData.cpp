#include "VoxelData.h"
#include "VoxelChunk.h"
#include "VoxelWorld.h"

FBlockPos::FBlockPos()
	: World(nullptr), GlobalPos(FIntVector(0))
{ }

FBlockPos::FBlockPos(UVoxelChunk* mChunk, FIntVector LocalPos)
{
	GlobalPos = mChunk->LocalToGlobalPosition(LocalPos);
	World = mChunk->GetVoxelWorld();
}

FBlockPos::FBlockPos(AVoxelWorld* VoxelWorld, FIntVector GlobalPosition)
{
	World = VoxelWorld;
	GlobalPos = GlobalPosition;
}

FIntVector FBlockPos::GetGlobalPosition() const
{
	return GlobalPos;
}

AVoxelWorld* FBlockPos::GetWorld() const
{
	return World;
}

FIntVector FBlockPos::GetChunkIndex() const
{
	//-1 / 16 needs to be -1, not zero
	auto CustomDiv = [](int Val, int Div) 
	{
		const int Result = Val / Div;
		return Div * Result == Val ? Result : Result - ((Val < 0) ^ (Div < 0));
	};

	return FIntVector(CustomDiv(GlobalPos.X, VOX_CHUNKSIZE)
	, CustomDiv(GlobalPos.Y, VOX_CHUNKSIZE)
	, CustomDiv(GlobalPos.Z, VOX_CHUNKSIZE));
}

FIntVector FBlockPos::GetLocalPos() const
{
	//Wtf c++ implementation is different from python implementation
	auto CustomModulo = [](int Val, int Div) {const int Result = Val % Div; return Result < 0 ? Result + Div : Result; };
	return FIntVector(CustomModulo(GlobalPos.X, VOX_CHUNKSIZE)
		, CustomModulo(GlobalPos.Y, VOX_CHUNKSIZE)
		, CustomModulo(GlobalPos.Z, VOX_CHUNKSIZE));
}
UVoxelChunk* FBlockPos::GetChunk() const
{
	return World->GetChunkFromBlockPos(*this);
}

void FAdjacentChunkCache::Init(UVoxelChunk* Chunk)
{
	World = Chunk->GetVoxelWorld();
	Location = Chunk->GetChunkPosition();
}

void FAdjacentChunkCache::GetAdjacentChunks(TArray<FIntVector>& Poses, TArray<UVoxelChunk*>& Ret)
{
	FScopeLock Lock(&CritSection);

	TArray<FIntVector> NotFound;
	for (auto& Pos : Poses)
	{
		auto Chunk = GetCache(GetArrayIndex(Pos));
		if (Chunk.IsValid())
		{
			Ret.Add(*Chunk);
		}
		else
		{
			NotFound.Add(Pos + Location);
		}
	}

	TArray<UVoxelChunk*> NotFoundRet;
	World->GetChunksFromIndices(NotFound, NotFoundRet);

	for (auto& Chunk : NotFoundRet)
	{
		FIntVector Key = Chunk->GetChunkPosition() - Location;
		auto& ChunkCache = GetCache(GetArrayIndex(Key));
		ChunkCache.Set(Chunk);
	}
}

void FAdjacentChunkCache::FCacheStruct::Set(UVoxelChunk * Chunk)
{
	WeakPtr = Chunk->GetWeakPtr();
	RealPtr = Chunk;
}
