#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IVoxel_Terrain.h"
#include "VoxelData.h"
#include "VoxelChunk.h"

#include "VoxelWorld.generated.h"

class AVoxelChunk;

UCLASS()
class IVOXEL_TERRAIN_API AVoxelWorld : public AActor
{
	GENERATED_BODY()
public:
	AVoxelWorld();

	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;


	AVoxelChunk* GetChunk(FIntVector Pos);
};