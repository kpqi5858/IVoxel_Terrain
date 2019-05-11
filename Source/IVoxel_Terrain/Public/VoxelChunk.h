#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VoxelChunk.generated.h"

class AVoxelWorld;
class FBlockState;

UCLASS()
class IVOXEL_TERRAIN_API AVoxelChunk : public AActor
{
	GENERATED_BODY()

private:
	FIntVector ChunkPosition;
	AVoxelWorld* World;

public:
	AVoxelChunk();

	void ChunkTick();
	void Initialize();

	FBlockState* GetBlockState(FIntVector LocalPos);

public:
	AVoxelWorld* GetVoxelWorld();
	FIntVector GetChunkPosition();

	FIntVector LocalToGlobalPosition(FIntVector LocalPos);
	FIntVector GlobalToLocalPosition(FIntVector GlobalPos);
};