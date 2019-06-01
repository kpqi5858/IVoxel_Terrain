#pragma once

#include "VoxelChunkRender.h"

class FVoxelPolygonizerThread : public IQueuedWork
{
public:
	FVoxelPolygonizerThread(AVoxelChunkRender* Render);

	AVoxelChunkRender* Chunk;

	void DoThreadedWork() override;
	void Abandon() override;
};

class FWorldGeneratorThread : public IQueuedWork
{
public:
	FWorldGeneratorThread(UVoxelChunk* Chunk);

	UVoxelChunk* Chunk;

	void DoThreadedWork() override;
	void Abandon() override;
};

class FPostWorldGeneratorThread : public IQueuedWork
{
public:
	FPostWorldGeneratorThread(UVoxelChunk* Chunk);

	UVoxelChunk* Chunk;

	void DoThreadedWork() override;
	void Abandon() override;
};