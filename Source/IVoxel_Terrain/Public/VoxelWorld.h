#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "IVoxel_Terrain.h"
#include "VoxelChunk.h"
#include "BlockRegistry.h"

#include "VoxelWorld.generated.h"

class UVoxelChunk;
class UVoxelWorldGenerator;
class AVoxelChunkRender;
struct FVoxelInvoker;
struct FBlockPos;
class FMyQueuedThreadPool;

UCLASS()
class IVOXEL_TERRAIN_API AVoxelWorld : public AActor
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere)
	FIntVector RenderChunkSize;

	UPROPERTY(EditAnywhere)
	FIntVector PreGenerateChunkSize;
	
	UPROPERTY(EditAnywhere)
	float VoxelSize;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UVoxelWorldGenerator> WorldGeneratorClass;

	//How frequent invokers creating/updating chunks
	UPROPERTY(EditAnywhere)
	int CreateChunkInterval;

	UPROPERTY(EditAnywhere)
	int WorldGeneratorThreads = 2;

	UPROPERTY(EditAnywhere)
	int PolygonizerThreads = 2;

	UPROPERTY(EditAnywhere)
	int PrimeUpdateThreshold = 10;

private:
	UPROPERTY()
	TMap<FIntVector, UVoxelChunk*> LoadedChunk;

	TMap<FIntVector, UVoxelChunk*> LoadedChunk_Internal;

	TSet<UVoxelChunk*> TickListCache;

	TSet<UVoxelChunk*> InactiveChunkList;

	FRWLock LoadedChunkLock;

	TArray<FVoxelInvoker> InvokersList;

	UPROPERTY()
	TArray<AVoxelChunkRender*> FreeRender;

	TArray<AVoxelChunkRender*> WaitingRender;

	UPROPERTY()
	UVoxelWorldGenerator* InstancedWorldGenerator = nullptr;

	bool IsInitialized;

	float VoxelSizeInit;

	long InternalTicks = 0;

	TSharedPtr<FBlockRegistryInstance> RegistryReference;

	FMyQueuedThreadPool* PolygonizerThreadPool;
	FMyQueuedThreadPool* WorldGeneratorThreadPool;
	
	int PrimeUpdateCount = 0;

public:
	FThreadSafeCounter MesherThreads;
	FThreadSafeCounter WorldGenThreads;

private:
	AVoxelChunkRender* CreateRenderActor();

	UVoxelChunk* CreateVoxelChunk(FIntVector Index);

	UVoxelChunk* GetChunkFromIndex_Internal(FIntVector Pos, bool DoLock);
public:
	AVoxelWorld();

	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

	virtual void Tick(float DeltaSeconds) override;

	void RegisterTickList(UVoxelChunk* Chunk);
	void DeregisterTickList(UVoxelChunk* Chunk);

	void UnloadChunk(UVoxelChunk* Chunk);
	void LoadChunk(UVoxelChunk* Chunk);

	AVoxelChunkRender* GetFreeRenderActor();
	void FreeRenderActor(AVoxelChunkRender* RenderActor);

	void Initialize();

	UFUNCTION(BlueprintCallable)
	void RegisterInvoker(AActor* Object, bool DoRender);

	float GetVoxelSize();
	UClass* GetWorldGeneratorClass();

	UVoxelWorldGenerator* GetWorldGenerator();

	//It can create chunk
	UVoxelChunk* GetChunkFromIndex(FIntVector Pos, bool DoLock = true);
	UFUNCTION(BlueprintCallable)
	UVoxelChunk* GetChunkFromBlockPos(FBlockPos Pos, bool DoLock = true);

	UFUNCTION(BlueprintCallable)
	FIntVector WorldPosToVoxelPos(FVector Pos);

	bool ShouldChunkRendered(UVoxelChunk* Chunk);
	bool ShouldGenerateWorld(UVoxelChunk* Chunk);

	void QueueWorldGeneration(UVoxelChunk* Chunk);
	void QueuePostWorldGeneration(UVoxelChunk* Chunk);

	void QueuePolygonize(AVoxelChunkRender* Render);

	float GetDistanceToInvoker(UVoxelChunk* Chunk, bool Render);

	bool ShouldUpdatePrime();
};