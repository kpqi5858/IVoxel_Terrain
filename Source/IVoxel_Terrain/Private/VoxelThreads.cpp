#include "VoxelThreads.h"

FVoxelPolygonizerThread::FVoxelPolygonizerThread(AVoxelChunkRender* Render)
	: Chunk(Render)
{
}

void FVoxelPolygonizerThread::DoThreadedWork()
{
	Chunk->Polygonize();
	delete this;
}

void FVoxelPolygonizerThread::Abandon()
{
	delete this;
}

FWorldGeneratorThread::FWorldGeneratorThread(UVoxelChunk* Chunk)
	: Chunk(Chunk)
{
}

void FWorldGeneratorThread::DoThreadedWork()
{
	Chunk->GenerateWorld();
	delete this;
}

void FWorldGeneratorThread::Abandon()
{
	delete this;
}
