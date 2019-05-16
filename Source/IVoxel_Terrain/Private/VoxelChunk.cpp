#include "VoxelChunk.h"
#include "VoxelWorld.h"

FVoxelChunk::FVoxelChunk()
{
}

void FVoxelChunk::ChunkTick()
{
}

void FVoxelChunk::Initialize()
{
}

void FVoxelChunk::InitRender()
{
}

void FVoxelChunk::DeInitRender()
{
}

FBlockState * FVoxelChunk::GetBlockState(FIntVector LocalPos)
{
	return nullptr;
}

AVoxelWorld * FVoxelChunk::GetVoxelWorld()
{
	return nullptr;
}

FIntVector FVoxelChunk::GetChunkPosition()
{
	return FIntVector();
}

AVoxelChunkRender * FVoxelChunk::GetRender()
{
	return nullptr;
}

FIntVector FVoxelChunk::LocalToGlobalPosition(FIntVector LocalPos)
{
	return FIntVector();
}

FIntVector FVoxelChunk::GlobalToLocalPosition(FIntVector GlobalPos)
{
	return FIntVector();
}
