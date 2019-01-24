#pragma once

#include "CoreMinimal.h"
#include "IVoxel_Chunk.h"
#include "IVoxel_TerrainWorld.h"
#include "IVoxel_BlockData.h"
#include "IVoxel_WorldGenBase.h"
#include "IVoxel_Thread.h"
#include "IVoxel_Octree.h"

class AIVoxel_TerrainWorld;
class AIVoxel_Chunk;
class IVoxel_Polygonizer;
class FOctree;

class IVOXEL_TERRAIN_API IVoxel_TerrainManager
{
public:
	IVoxel_TerrainManager(AIVoxel_TerrainWorld* world);
	~IVoxel_TerrainManager();

public:
	int OctreeDepthInit;
	float VoxelSizeInit;

private:
	AIVoxel_TerrainWorld* World;

	TSet<TWeakObjectPtr<AActor>> InvokersList;

	TMap<FIntVector, AIVoxel_Chunk*> ChunksLoaded;

	TSet<AIVoxel_Chunk*> ChunksUnloaded;

	TMap<FIntVector, AIVoxel_Chunk*> ChunksNodeLoaded;

	uint32 InternalTickCount = 0;
public:
	TSet<FIntVector> EmptyChunksCached;
	
	TSet<AIVoxel_Chunk*> ChunksToApplyMesh;

	FQueuedThreadPool* MesherThreadPool;

	void CreateStartChunks();

	void RegisterInvoker(TWeakObjectPtr<AActor> Actor);
	void Tick();

	void DebugRender(UWorld* World);

	void Destroy();

	float GetMinDistanceToInvokers(FVector Pos);

	TSharedPtr<IVoxel_Polygonizer> GetPolygonizer(AIVoxel_Chunk* Chunk, FOctree* Node);
	TSharedPtr<IVoxel_Polygonizer> GetPolygonizer(AIVoxel_Chunk* Chunk, FIntVector ExactPos, uint8 Depth);

	void DestroyedChunk(AIVoxel_Chunk* Chunk);
	void CreateChunk(FIntVector ChunkPos, bool AsyncMesher);

	AIVoxel_Chunk* GetChunkUnloaded();
	void UnloadChunk(AIVoxel_Chunk* Chunk);

	bool IsTooFarFromInvokers(FIntVector ChunkPos);
	inline FIntVector WorldLocationToChunkIndex(FVector Pos);
	inline FIntVector WorldLocationToOctreePos(FVector Pos);
};