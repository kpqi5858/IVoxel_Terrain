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
	TSubclassOf<UVoxelWorldGenerator> WorldGenerator;

	//How frequent invokers creating/updating chunks
	UPROPERTY(EditAnywhere)
	int CreateChunkInterval;

	UPROPERTY(EditAnywhere)
	int WorldGeneratorThreads = 1;

	UPROPERTY(EditAnywhere)
	int PolygonizerThreads = 2;

private:
	UPROPERTY()
	TMap<FIntVector, UVoxelChunk*> LoadedChunk;

	FCriticalSection LoadedChunkLock;

	TArray<FVoxelInvoker> InvokersList;

	UPROPERTY()
	TArray<AVoxelChunkRender*> FreeRender;

	UClass* WorldGeneratorInit;

	bool IsInitialized;

	float VoxelSizeInit;

	long InternalTicks = 0;

	TSharedPtr<FBlockRegistryInstance> RegistryReference;

	FQueuedThreadPool* PolygonizerThreadPool;
	FQueuedThreadPool* WorldGeneratorThreadPool;

private:
	AVoxelChunkRender* CreateRenderActor();

	UVoxelChunk* CreateVoxelChunk(FIntVector Index);

	UVoxelChunk* GetChunkFromIndex_Internal(FIntVector Pos);
public:
	AVoxelWorld();

	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

	virtual void Tick(float DeltaSeconds) override;

	AVoxelChunkRender* GetFreeRenderActor();
	void FreeRenderActor(AVoxelChunkRender* RenderActor);

	void Initialize();

	UFUNCTION(BlueprintCallable)
	void RegisterInvoker(AActor* Object, bool DoRender);

	float GetVoxelSize();
	UClass* GetWorldGeneratorClass();

	//It can create chunk
	UVoxelChunk* GetChunkFromIndex(FIntVector Pos, bool DoLock = true);
	UFUNCTION(BlueprintCallable)
	UVoxelChunk* GetChunkFromBlockPos(FBlockPos Pos, bool DoLock = true);

	UFUNCTION(BlueprintCallable)
	FIntVector WorldPosToVoxelPos(FVector Pos);

	bool ShouldChunkRendered(UVoxelChunk* Chunk);
	bool ShouldGenerateWorld(UVoxelChunk* Chunk);

	void QueueWorldGeneration(UVoxelChunk* Chunk);
	void QueuePolygonize(AVoxelChunkRender* Render);

	float GetDistanceToInvoker(UVoxelChunk* Chunk, bool Render);
};