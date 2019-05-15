#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VoxelChunk.generated.h"

class AVoxelWorld;
class FBlockState;
class AVoxelChunkRender;

UCLASS(Blueprintable)
class IVOXEL_TERRAIN_API FVoxelChunk : public UObject
{
	GENERATED_BODY()

private:
	FIntVector ChunkPosition;
	AVoxelWorld* World;

	//Can be null
	AVoxelChunkRender* RenderActor;
public:
	FVoxelChunk();

	void ChunkTick();
	void Initialize();

	void InitRender();
	void DeInitRender();

	FBlockState* GetBlockState(FIntVector LocalPos);

public:
	AVoxelWorld* GetVoxelWorld();

	FIntVector GetChunkPosition();

	AVoxelChunkRender* GetRender();

	FIntVector LocalToGlobalPosition(FIntVector LocalPos);
	FIntVector GlobalToLocalPosition(FIntVector GlobalPos);
};
