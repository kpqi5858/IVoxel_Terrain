#include "VoxelChunk.h"
#include "VoxelWorld.h"
#include "VoxelChunkRender.h"
#include "WorldGenerator.h"
#include "BlockStateStorage.h"

UVoxelChunk::UVoxelChunk()
{
}

void UVoxelChunk::ChunkTick()
{
	check(!IsUnreachable());
	//Correct ChunkState
	if (ShouldBeRendered())
	{
		TrySetChunkState(EChunkState::CS_Rendered);
	}

	EChunkState CurrentState = GetChunkState();
	//Invalid chunk should not be ticked
	check(CurrentState != EChunkState::CS_Invalid);

	if (WorldGenState == EWorldGenState::GENERATED)
	{
		RenderDirty = true;
		WorldGenState = EWorldGenState::DIRTYSET;
	}

	if (WorldGenState == EWorldGenState::NOT_GENERATED && World->ShouldGenerateWorld(this))
	{
		WorldGenState = EWorldGenState::GENERATING;
		World->QueueWorldGeneration(this);
	}

	if (ShouldBeRendered())
	{
		if (!HasRender())
		{
			InitRender();
		}
	}
	if (!ShouldBeRendered() && HasRender())
	{
		DeInitRender();
	}

	if (HasRender())
	{
		if (RenderDirty && !RenderActor->IsPolygonizingNow())
		{
			RenderDirty = false;
			World->QueuePolygonize(RenderActor);
		}
		RenderActor->RenderTick();
	}
}

void UVoxelChunk::Initialize(AVoxelWorld* VoxelWorld, FIntVector ChunkIndex)
{
	ChunkPosition = ChunkIndex;
	World = VoxelWorld;
	WorldGenerator = NewObject<UVoxelWorldGenerator>(this, VoxelWorld->GetWorldGeneratorClass());
	WorldGenerator->Setup(this);

	FaceVisiblityCache = MakeShareable(new TBasicAbstractBlockStorage<FFaceVisiblityCache>());
	FaceVisiblityCache->Initialize();

	BlockStateStorage = MakeShareable(new TBasicAbstractBlockStorage<FBlockState>());
	BlockStateStorage->Initialize(
	[&](int Index)
	{
		return (void*) new FBlockState(this, FBlockPos(this, FVoxelUtilities::PositionFromIndex(Index)));
	});
}

void UVoxelChunk::InitRender()
{
	check(!RenderActor);
	RenderActor = World->GetFreeRenderActor();
	RenderActor->Initialize(this);
}

void UVoxelChunk::DeInitRender()
{
	check(RenderActor);
	RenderActor->DestroyRender();
	World->FreeRenderActor(RenderActor);
	RenderActor = nullptr;
}

bool UVoxelChunk::HasRender()
{
	return RenderActor != nullptr;
}

void UVoxelChunk::GenerateWorld()
{
	check(WorldGenerator);

	WorldGenerator->Generate();
}

void UVoxelChunk::BlockStateStorageLock()
{
	BlockStateStorage->Lock();
}

void UVoxelChunk::BlockStateStorageUnlock()
{
	BlockStateStorage->UnLock();
}

EChunkState UVoxelChunk::GetChunkState()
{
	FScopeLock Lock(&ChunkStateLock);
	return ChunkState;
}

void UVoxelChunk::TrySetChunkState(EChunkState NewState)
{
	FScopeLock Lock(&ChunkStateLock);

	//Need to seperate cases
	switch (NewState)
	{
	//Not special
	case EChunkState::CS_NoRender :
	case EChunkState::CS_Rendered :
	case EChunkState::CS_QueuedDeletion :
	{
		ChunkState = NewState;
		break;
	}
	//World generator references this chunk
	case EChunkState::CS_WorldGenGenerated : 
	{
		//Other states are more important(?)
		if (ChunkState == EChunkState::CS_Invalid || ChunkState == EChunkState::CS_QueuedDeletion)
		{
			ChunkState = NewState;
		}
		break;
	}
	default:
	{
		check(NewState == EChunkState::CS_Invalid);
		ChunkState = NewState;
	}
	}
}

bool UVoxelChunk::IsValidChunk()
{
	return ChunkState != EChunkState::CS_Invalid && ChunkState != EChunkState::CS_QueuedDeletion;
}

FBlockState* UVoxelChunk::GetBlockState(FBlockPos Pos)
{
	check(Pos.GetChunk() == this);
	return BlockStateStorage->Get(Pos.ArrayIndex());
}

void UVoxelChunk::SetBlock(FBlockPos Pos, UBlock* Block)
{
	check(Pos.GetChunk() == this);
	SetRenderDirty();
	GetBlockState(Pos)->SetBlockDef(Block);
	UpdateBlock(Pos);
}

FFaceVisiblityCache& UVoxelChunk::GetFaceVisiblityCache(FBlockPos Pos)
{
	check(Pos.GetChunk() == this);
	return *FaceVisiblityCache->Get(Pos.ArrayIndex());
}

void UVoxelChunk::UpdateBlock(FBlockPos Pos)
{
	//Update FaceVisiblityCache

	FFaceVisiblityCache& FaceCache = GetFaceVisiblityCache(Pos);
	FBlockState* BlockState = GetBlockState(Pos);
	UBlock* BlockDef = BlockState->GetBlockDef();

	FaceCache.Data = 0;
	const EBlockFace AllFaces[6] = { EBlockFace::FRONT, EBlockFace::BACK, EBlockFace::LEFT, EBlockFace::RIGHT, EBlockFace::TOP, EBlockFace::BOTTOM };

	for (auto Face : AllFaces)
	{
		FBlockPos NextPos = FBlockPos(Pos);
		NextPos.GlobalPos += FVoxelUtilities::GetFaceOffset(Face);
		FBlockState* NextState = NextPos.GetChunk()->GetBlockState(NextPos);
		UBlock* NextBlockDef = NextState->GetBlockDef();

		bool Visible = BlockDef->IsFaceVisible(Face) != NextBlockDef->IsFaceVisible(FVoxelUtilities::GetOppositeFace(Face));

		NextPos.GetChunk()->GetFaceVisiblityCache(NextPos).SetFaceVisible(FVoxelUtilities::GetOppositeFace(Face), Visible);
		NextPos.GetChunk()->SetRenderDirty();
		FaceCache.SetFaceVisible(Face, Visible);
	}
}

bool UVoxelChunk::ShouldBeRendered()
{
	return World->ShouldChunkRendered(this) && WorldGenState >= EWorldGenState::GENERATED;
}

bool UVoxelChunk::ShouldBeTicked()
{
	return ChunkState != EChunkState::CS_Invalid;
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

void UVoxelChunk::SetRenderDirty()
{
	RenderDirty = true;
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
