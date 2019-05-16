#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IVoxel_Terrain.h"
#include "VoxelData.h"
#include "VoxelChunk.h"

#include "VoxelWorld.generated.h"

class FVoxelChunk;

UCLASS()
class IVOXEL_TERRAIN_API AVoxelWorld : public AActor
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprinReadWrite)
	FIntVector RenderChunkSize;

	UPROPERTY(BlueprintReadWrite)
	FIntVector PreGenerateChunkSize;

	UPROPERTY(BlueprintReadWrite)
	
public:
	AVoxelWorld();

	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

	virtual void Tick(float DeltaSeconds) override;

	FVoxelChunk* GetChunk(FIntVector Pos);
};