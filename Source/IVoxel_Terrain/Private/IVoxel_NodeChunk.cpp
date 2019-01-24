#include "IVoxel_NodeChunk.h"

void UIVoxelNodeChunk::Load(FIntVector Pos, uint8 Depth)
{
	NodePos = Pos;
	NodeDepth = Depth;
	IsPolygonizeDone = false;
	HasMesh = false;
}
void UIVoxelNodeChunk::Unload()
{
	IsPolygonizeDone = false;
	HasMesh = false;
}

void UIVoxelNodeChunk::Setup(AIVoxel_Chunk* ParentChunk)
{
	Chunk = ParentChunk;
}

void UIVoxelNodeChunk::ChunkTick()
{
	if (IsPolygonizeDone)
	{
		Chunk->ApplyPolygonized(this, PolygonizedData);
		IsPolygonizeDone = false;
		HasMesh = true;
		DecreaseOCReset();
		Chunk->DoingThreadedJob.Decrement();
	}
}

void UIVoxelNodeChunk::DecreaseOCReset()
{
	for (auto& Ch : OldChunks)
	{
		Ch->DecreaseOC();
	}
	OldChunks.Reset();
}

void UIVoxelNodeChunk::IncreaseOC()
{
	OldChunkCount++;
}

void UIVoxelNodeChunk::DecreaseOC()
{
	check(OldChunkCount > 0);
	OldChunkCount--;
	
	if (OldChunkCount == 0)
	{
		ClearAllMeshSections();
		Chunk->UnloadRMC(this);
	}
}