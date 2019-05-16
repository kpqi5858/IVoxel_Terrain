#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VoxelChunk.generated.h"

class AVoxelWorld;
class FBlockState;
class AVoxelChunkRender;

UENUM(BlueprintType)
enum EChunkState : uint8
{
	//Invalid chunk
	CS_Invalid,
	//Pre generated chunk, has no render
	CS_PreGenerated,
	//Temporary chunk generated by world generator
	CS_WorldGenGenerated,
	//This chunk has render actor and might rendered
	CS_RenderCreated,
	//This chunk is being deleted
	CS_QueuedDeletion
};

UCLASS(Blueprintable)
class IVOXEL_TERRAIN_API FVoxelChunk : public UObject
{
    GENERATED_BODY()

protected:
    FIntVector ChunkPosition;
    AVoxelWorld* World;

    bool RenderDirty = false;

public:
	//Can be null
	UPROPERTY()
	AVoxelChunkRender* RenderActor = nullptr;

	UPROPERTY(BlueprintReadOnly)
	EChunkState ChunkState;


public:
    FVoxelChunk();

    virtual void ChunkTick();

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