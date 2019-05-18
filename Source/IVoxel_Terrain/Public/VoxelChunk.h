#pragma once

#include "CoreMinimal.h"
#include "IVoxel_Terrain.h"
#include "BlockStateStorage.h"
#include "GameFramework/Actor.h"
#include "VoxelChunk.generated.h"

class AVoxelWorld;
class FBlockState;
class AVoxelChunkRender;

UENUM(BlueprintType)
enum class EChunkState : uint8
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
class IVOXEL_TERRAIN_API UVoxelChunk : public UObject
{
    GENERATED_BODY()

protected:
    FIntVector ChunkPosition;
    AVoxelWorld* World;

    bool RenderDirty = false;

	TSharedPtr<TAbstractBlockStorage<FBlockState>> BlockStateStorage;

public:
	//Can be null
	UPROPERTY()
	AVoxelChunkRender* RenderActor = nullptr;

	UPROPERTY(BlueprintReadOnly)
	EChunkState ChunkState;

public:
    UVoxelChunk();

    virtual void ChunkTick();

    void Initialize();

    void InitRender();
    void DeInitRender();

	//Only lock when your operation is not in game thread
	void BlockStateStorageLock();
	void BlockStateStorageUnlock();

    FBlockState* GetBlockState(FBlockPos Pos);
	void SetBlock(FBlockPos Pos, UBlock* Block);

	bool ShouldBeTicked();
	bool ShouldBeDeleted();
public:
    AVoxelWorld* GetVoxelWorld();

    FIntVector GetChunkPosition();

    AVoxelChunkRender* GetRender();

	inline FIntVector GetGlobalPosition_Min()
	{
		return ChunkPosition * VOX_CHUNKSIZE;
	}
	inline FIntVector GetGlobalPosition_Max()
	{
		return (ChunkPosition + FIntVector(1)) * VOX_CHUNKSIZE - FIntVector(1);
	}

	inline bool IsInChunk(FIntVector GlobalPos)
	{
		FIntVector Min = GetGlobalPosition_Min();
		FIntVector Max = GetGlobalPosition_Max();
		return GlobalPos.X >= Min.X && GlobalPos.X < Max.X
			&& GlobalPos.Y >= Min.Y && GlobalPos.Y < Max.Y
			&& GlobalPos.Z >= Min.Z && GlobalPos.Z < Max.Z;
	}

    FIntVector LocalToGlobalPosition(FIntVector LocalPos);
    FIntVector GlobalToLocalPosition(FIntVector GlobalPos);

	FVector GetWorldPosition();
};
