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

private:
	UPROPERTY()
	TMap<FIntVector, UVoxelChunk*> LoadedChunk;

	FCriticalSection LoadedChunkLock;

	TSet<FVoxelInvoker> InvokersList;

	UPROPERTY()
	TArray<AVoxelChunkRender*> FreeRender;

	UClass* WorldGeneratorInit;

	bool IsInitialized;

	float VoxelSizeInit;

private:
	AVoxelChunkRender* CreateRenderActor();

	UVoxelChunk* CreateVoxelChunk(FIntVector Index);

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
	UClass* GetWorldGenerator();

	UVoxelChunk* GetChunkFromIndex(FIntVector Pos);
	UVoxelChunk* GetChunkFromBlockPos(FBlockPos Pos);

	float GetDistanceToInvoker(UVoxelChunk* Chunk, bool Render);
};