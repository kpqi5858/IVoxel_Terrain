#pragma once

#include "CoreMinimal.h"
#include "IVoxel_Polygonizer.h"
#include "IVoxel_Chunk.h"
#include "HAL/ThreadingBase.h"

struct IVoxel_PolygonizedData;
class AIVoxel_Chunk;
class IVoxel_Polygonizer;

class IVoxel_PolygonizerThread : public IQueuedWork
{
public:
	IVoxel_PolygonizerThread(AIVoxel_Chunk* Chunk, FIntVector ChunkPos, TSharedPtr<IVoxel_Polygonizer> Polygonizer);
	~IVoxel_PolygonizerThread();

	AIVoxel_Chunk* Chunk;
	TSharedPtr<IVoxel_Polygonizer> Polygonizer;
	FIntVector ChunkPos;

	uint64 ThreadUniqueID;

	void DoThreadedWork() override;
	void Abandon() override;
};