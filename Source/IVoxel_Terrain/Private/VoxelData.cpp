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

int FBlockPos::ArrayIndex() const
{
	FIntVector ChunkLocalPos = GetLocalPos();
	return VOX_CHUNK_AI(ChunkLocalPos.X, ChunkLocalPos.Y, ChunkLocalPos.Z);
}
