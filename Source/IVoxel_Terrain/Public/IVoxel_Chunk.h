#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RuntimeMeshComponent.h"
#include "RuntimeMesh.h"
#include "IVoxel_Terrain.h"
#include "IVoxel_TerrainWorld.h"
#include "IVoxel_WorldGenBase.h"
#include "IVoxel_BlockData.h"
#include "IVoxel_MCPolygonizer.h"
#include "IVoxel_NodeChunk.h"
#include "IVoxel_DataManager.h"
#include "IVoxel_Chunk.generated.h"


class AIVoxel_TerrainWorld;
struct IVoxel_PolygonizedData;
class IVoxel_PolygonizerThread;
class FOctree;
class IVoxel_TerrainManager;
class UIVoxelNodeChunk;
class FIVoxel_DataManager;

//Chunk and data
UCLASS()
class IVOXEL_TERRAIN_API AIVoxel_Chunk : public AActor
{
	GENERATED_BODY()

public:
	AIVoxel_Chunk();

	UPROPERTY(EditAnywhere)
	USceneComponent* RootComp;

	TSharedPtr<FIVoxel_DataManager> DataOctree;

	TSharedPtr<FOctree> RenderOctree;

	//Rebuilding lod makes homeless(?) nodes
	//So don't use pointer in TMap key
	//Use FIntVector to find positionally-equal node
	UPROPERTY()
	TMap<FIntVector, UIVoxelNodeChunk*> LoadedLeaves;

	UPROPERTY()
	TArray<UIVoxelNodeChunk*> FreeLeaves;

	TSet<UIVoxelNodeChunk*> ToUpdate;

	FCriticalSection TickListLock;

	TSet<UIVoxelNodeChunk*> TickList;
	
	bool PendingToDelete = false;

	FThreadSafeCounter DoingThreadedJob;
private:
	uint64 InternalTicks = 0;

public:
	void Setup(AIVoxel_TerrainWorld* World, FIntVector ChunkLoc);

	virtual void Tick(float DeltaTime) override;

	bool IsSaveDirty;
	FIntVector ChunkLocation;
	AIVoxel_TerrainWorld* IVoxWorld;

	//Cached
	IVoxel_TerrainManager* Manager;

	//Builds octree, and render
	void RenderOctreeTick();

	inline void ApplyPolygonized(UIVoxelNodeChunk* RMC, IVoxel_PolygonizedData& Data);
	
	inline FVector GetWorldLocation();

	FVector WorldPosToLocalPos(FVector Pos);
	FVector LocalPosToWorldPos(FVector Pos);

	inline FVector GetDataLocation();

	void UpdateChunkAt(FIntVector Pos);

	UIVoxelNodeChunk* GetFreeNodeChunk(FIntVector NodePos, uint8 NodeDepth);

	void UnloadRMC(FIntVector Pos); //Queue to unload chunk
	void UnloadRMC(UIVoxelNodeChunk* Chunk); //Real unload

	uint8 GetLodFor(FOctree* Node);

	void RegisterTickList(UIVoxelNodeChunk* Chunk);
	void DeRegisterTickList(UIVoxelNodeChunk* Chunk);

	UFUNCTION(BlueprintCallable)
	void EditWorldTest(FVector Position);

	static inline int IndexFor(int x, int y, int z)
	{
		check(IVOX_CHUNKDATASIZE > x && x >= 0);
		check(IVOX_CHUNKDATASIZE > y && y >= 0);
		check(IVOX_CHUNKDATASIZE > z && z >= 0);

		return x + (IVOX_CHUNKDATASIZE * y) + (IVOX_CHUNKDATASIZE*IVOX_CHUNKDATASIZE * z);
	}
	static inline int IndexFor(FIntVector xyz)
	{
		int x = xyz.X, y = xyz.Y, z = xyz.Z;
		check(IVOX_CHUNKDATASIZE > x && x >= 0);
		check(IVOX_CHUNKDATASIZE > y && y >= 0);
		check(IVOX_CHUNKDATASIZE > z && z >= 0);

		return x
			+ (IVOX_CHUNKDATASIZE * y)
			+ (IVOX_CHUNKDATASIZE*IVOX_CHUNKDATASIZE * z);
	}

	static inline FIntVector AsLocation(int num);
};