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

FIntVector FBlockPos::GetGlobalPosition()
{
	return GlobalPos;
}

AVoxelWorld* FBlockPos::GetWorld()
{
	return World;
}

FIntVector FBlockPos::GetChunkIndex()
{
	return FIntVector(GlobalPos.X % VOX_CHUNKSIZE
		, GlobalPos.Y % VOX_CHUNKSIZE
		, GlobalPos.Z % VOX_CHUNKSIZE);
}

UVoxelChunk* FBlockPos::GetChunk()
{
	return World->GetChunkFromBlockPos(*this);
}

int FBlockPos::ArrayIndex()
{
	FIntVector ChunkLocalPos = GetChunkIndex();
	return VOX_CHUNK_AI(ChunkLocalPos.X, ChunkLocalPos.Y, ChunkLocalPos.Z);
}
