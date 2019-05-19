#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "IVoxel_Terrain.h"
#include "VoxelData.h"
#include "VoxelChunk.h"

#include "VoxelWorld.generated.h"

class UVoxelChunk;

UCLASS()
class IVOXEL_TERRAIN_API AVoxelWorld : public AActor
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite)
	FIntVector RenderChunkSize;

	UPROPERTY(BlueprintReadWrite)
	FIntVector PreGenerateChunkSize;
	
	UPROPERTY(BlueprintReadWrite)
	float VoxelSize;

	UPROPERTY(BlueprintReadWrite)
	TSubclassOf<UVoxelWorldGenerator> WorldGenerator;

	//How frequent invokers creating/updating chunks
	UPROPERTY(BlueprintReadWrite)
	int CreateChunkInterval;

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

	void RegisterInvoker(AActor* Object, bool DoRender);

	float GetVoxelSize();
	UClass* GetWorldGeneratorClass();

	//It can create chunk
	UVoxelChunk* GetChunkFromIndex(FIntVector Pos, bool DoLock = true);
	UVoxelChunk* GetChunkFromBlockPos(FBlockPos Pos, bool DoLock = true);

	FIntVector WorldPosToVoxelPos(FVector Pos);

	bool ShouldChunkRendered(UVoxelChunk* Chunk);
	bool ShouldGenerateWorld(UVoxelChunk* Chunk);

	void QueueWorldGeneration(UVoxelChunk* Chunk);

	float GetDistanceToInvoker(UVoxelChunk* Chunk, bool Render);
};