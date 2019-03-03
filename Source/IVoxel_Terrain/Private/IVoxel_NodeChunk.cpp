#include "IVoxel_NodeChunk.h"

void UIVoxelNodeChunk::Load(FIntVector Pos, uint8 Depth)
{
	NodePos = Pos;
	NodeDepth = Depth;
	IsPolygonizeDone = false;
	HasMesh = false;
	IgnoreMesh = false;
	DeletionLeft = 0.f;
}
void UIVoxelNodeChunk::Unload()
{
	if (!HasMesh) //Called Unload() but polygonizer job is not finished
	{
		//Try to retract queued polygonizer work
		bool Result = Chunk->Manager->MesherThreadPool->RetractQueuedWork(PolygonizerThread);
		PolygonizerThread = nullptr;
		//Don't care whatever RetractQueuedWork succeed
		//Unload() called means this chunk will deregistered in tick list
		Chunk->DoingThreadedJob.Decrement();
	}
	DecreaseOCReset();
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
		IsPolygonizeDone = false;
		Chunk->ApplyPolygonized(this, PolygonizedData);
		HasMesh = true;
		PolygonizerThread = nullptr;
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
		Chunk->QueueUnload(this);
	}
}