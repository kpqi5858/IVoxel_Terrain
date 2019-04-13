#include "IVoxel_NodeChunk.h"

void UIVoxelNodeChunk::Load(FIntVector Pos, uint8 Depth)
{
	NodePos = Pos;
	NodeDepth = Depth;
	IsPolygonizeDone = false;
	HasMesh = false;
	IsLoaded = true;
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
	IsLoaded = false;
}

void UIVoxelNodeChunk::Setup(AIVoxel_Chunk* ParentChunk)
{
	Chunk = ParentChunk;
}

void UIVoxelNodeChunk::ChunkTick()
{
	if (IsPolygonizeDone)
	{
		check(PolygonizedData);

		IsPolygonizeDone = false;

		Chunk->ApplyPolygonized(this, *PolygonizedData);
		delete PolygonizedData;
		PolygonizedData = nullptr;

		HasMesh = true;

		PolygonizerThread = nullptr;

		DecreaseOCReset();
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

void UIVoxelNodeChunk::SetPolygonizerThread(IVoxel_PolygonizerThread* InThread)
{
	if (IsLoaded)
	{
		PolygonizerThread = InThread;
	}
	else
	{
		//Do not handle
	}
}

bool UIVoxelNodeChunk::HasPolygonizerThread()
{
	return PolygonizerThread != nullptr;
}

void UIVoxelNodeChunk::SetPolygonizedData(IVoxel_PolygonizerThread* ThisThread, IVoxel_PolygonizedData* PData)
{
	if ((PolygonizerThread && PolygonizerThread->ThreadUniqueID == ThisThread->ThreadUniqueID) || (!PolygonizerThread))
	{
		PolygonizedData = PData;
		IsPolygonizeDone = true;
	}
}