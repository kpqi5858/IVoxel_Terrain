#include "VoxelChunk.h"
#include "VoxelWorld.h"

UVoxelChunk::UVoxelChunk()
{
}

void UVoxelChunk::ChunkTick()
{
}

void UVoxelChunk::Initialize()
{
	BlockStateStorage = MakeShareable(new TBasicAbstractBlockStorage<FBlockState>());
	BlockStateStorage->Initialize(
	[&](int Index)
	{
		return (void*) new FBlockState(FBlockPos(this, FVoxelUtilities::PositionFromIndex(Index)));
	});
}

void UVoxelChunk::InitRender()
{
}

void UVoxelChunk::DeInitRender()
{
}

void UVoxelChunk::BlockStateStorageLock()
{
	BlockStateStorage->Lock();
}

void UVoxelChunk::BlockStateStorageUnlock()
{
	BlockStateStorage->UnLock();
}

FBlockState* UVoxelChunk::GetBlockState(FBlockPos Pos)
{
	return BlockStateStorage->Get(Pos.ArrayIndex());
}

void UVoxelChunk::SetBlock(FBlockPos Pos, UBlock* Block)
{
	GetBlockState(Pos)->SetBlockDef(Block);
}

bool UVoxelChunk::ShouldBeTicked()
{
	return ChunkState == EChunkState::CS_RenderCreated;
}

bool UVoxelChunk::ShouldBeDeleted()
{
	//If ChunkState is not CS_WorldGenGenerated and this chunk is out of range, return true
	return false;
}

AVoxelWorld* UVoxelChunk::GetVoxelWorld()
{
	return World;
}

FIntVector UVoxelChunk::GetChunkPosition()
{
	return ChunkPosition;
}

AVoxelChunkRender* UVoxelChunk::GetRender()
{
	return RenderActor;
}

FIntVector UVoxelChunk::LocalToGlobalPosition(FIntVector LocalPos)
{
	check(!VOX_IS_OUTOFLOCALPOS(LocalPos));
	return GetGlobalPosition_Min() + LocalPos;
}

FIntVector UVoxelChunk::GlobalToLocalPosition(FIntVector GlobalPos)
{
	FIntVector Result = GlobalPos - GetGlobalPosition_Min();
	check(!VOX_IS_OUTOFLOCALPOS(Result));
	return Result;
}

FVector UVoxelChunk::GetWorldPosition()
{
	return FVector(GetGlobalPosition_Min()) * World->GetVoxelSize();
}
