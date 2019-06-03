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

	if (PrimeGenerated == 1 && World->ShouldUpdatePrime())
	{
		PrimeGenerated = 2;
		ProcessPrimeChunk();
		WorldGenState = EWorldGenState::DIRTYSET;
	}

	if (WorldGenState == EWorldGenState::NOT_GENERATED && World->ShouldGenerateWorld(this))
	{
		WorldGenState = EWorldGenState::GENERATING;
		World->QueueWorldGeneration(this);
	}

	if (ShouldPostGenerate())
	{
		World->QueuePostWorldGeneration(this);
	}

	if (!World->ShouldGenerateWorld(this))
	{
		if (HasRender())
		{
			DeInitRender();
		}
		World->UnloadChunk(this);
		return;
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
		RenderActor->RenderTick();
		if (RenderDirty && !RenderActor->IsPolygonizingNow())
		{
			RenderDirty = false;
			World->QueuePolygonize(RenderActor);
		}
	}
}

void UVoxelChunk::Initialize(AVoxelWorld* VoxelWorld, FIntVector ChunkIndex)
{
	ChunkPosition = ChunkIndex;
	World = VoxelWorld;

	WorldGenerator = VoxelWorld->GetWorldGenerator();

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
	WorldGenerator->GeneratePrime(this);
	PrimeGenerated = 1;
}

void UVoxelChunk::PostGenerateWorld()
{
	WorldGenerator->PostGenerate(this);
	PostGeneration = 1;
	SetRenderDirty();
}

void UVoxelChunk::ProcessPrimeChunk()
{
	for (int Index = 0; Index < VOX_CHUNKSIZE_ARRAY; Index++)
	{
		SetBlock(FBlockPos(this, FVoxelUtilities::PositionFromIndex(Index)), PrimeChunk.Blocks[Index]);
	}
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
		if (ChunkState == EChunkState::CS_Invalid)
		{
			SetRenderDirty();
		}
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

FBlockState* UVoxelChunk::GetBlockState(FBlockPos& Pos)
{
	check(Pos.GetChunk() == this);
	return BlockStateStorage->Get(Pos.ArrayIndex());
}

void UVoxelChunk::ModifyBlockState(FBlockPos& Pos, StateModifyFunction Func, bool SetDirty)
{
	FBlockState* State = GetBlockState(Pos);
	Func(State);
	if (SetDirty)
	{
		SetRenderDirty();
		UpdateBlock(Pos);
	}
}

void UVoxelChunk::SetBlock(FBlockPos Pos, UBlock* Block)
{
	UBlock* Old = GetBlockState(Pos)->GetBlockDef();
	if (Old == Block) return;
	ModifyBlockState(Pos, [&](FBlockState* State) {State->SetBlockDef(Block); });
}

FFaceVisiblityCache& UVoxelChunk::GetFaceVisiblityCache(FBlockPos& Pos)
{
	check(Pos.GetChunk() == this);
	return *FaceVisiblityCache->Get(Pos.ArrayIndex());
}

void UVoxelChunk::UpdateBlock(FBlockPos& Pos)
{
	auto& ThisVisiblity = GetFaceVisiblityCache(Pos);
	const int ThisType = GetBlockState(Pos)->GetBlockDef()->OpaqueType();

	ThisVisiblity.Data = 0;

	constexpr EBlockFace AllFaces[6] = { EBlockFace::FRONT, EBlockFace::BACK, EBlockFace::LEFT, EBlockFace::RIGHT, EBlockFace::TOP, EBlockFace::BOTTOM };

	for (auto Face : AllFaces)
	{
		FBlockPos NextPos = FBlockPos(Pos);
		NextPos.GlobalPos += FVoxelUtilities::GetFaceOffset(Face);
		auto NextChunk = NextPos.GetChunk();
		auto& NextVisiblity = NextChunk->GetFaceVisiblityCache(NextPos);
		const int NextType = NextChunk->GetBlockState(NextPos)->GetBlockDef()->OpaqueType();

		const bool IsVisible = ThisType != NextType;
		const bool ThisVisible = ThisType && IsVisible;
		const bool NextVisible = NextType && IsVisible;

		ThisVisiblity.SetFaceVisible(Face, ThisVisible);
		if (NextVisiblity.SetFaceVisible(FVoxelUtilities::GetOppositeFace(Face), NextVisible))
			NextChunk->SetRenderDirty();
	}
}

void UVoxelChunk::GetAdjacentChunks(TArray<UVoxelChunk*>& Ret)
{
	Ret.Add(World->GetChunkFromIndex(ChunkPosition + FIntVector(1, 0, 0)));
	Ret.Add(World->GetChunkFromIndex(ChunkPosition + FIntVector(-1, 0, 0)));
	Ret.Add(World->GetChunkFromIndex(ChunkPosition + FIntVector(0, 1, 0)));
	Ret.Add(World->GetChunkFromIndex(ChunkPosition + FIntVector(0, -1, 0)));
	Ret.Add(World->GetChunkFromIndex(ChunkPosition + FIntVector(0, 0, 1)));
	Ret.Add(World->GetChunkFromIndex(ChunkPosition + FIntVector(0, 0, -1)));
}

void UVoxelChunk::GetAdjacentChunks_Corner(TArray<UVoxelChunk*>& Ret)
{
	for (int X = -1; X <= 1; X++)
	for (int Y = -1; Y <= 1; Y++)
	for (int Z = -1; Z <= 1; Z++)
	{
		if (X == 0 && Y == 0 && Z == 0) continue;
		FIntVector Pos = FIntVector(X, Y, Z);
		Ret.Add(World->GetChunkFromIndex(Pos + ChunkPosition));
	}
}

bool UVoxelChunk::ShouldBeRendered()
{
	bool Result = WorldGenState >= EWorldGenState::GENERATED && World->ShouldChunkRendered(this) && PostGeneration;
	if (Result)
	{
		//Check adjacent chunk's world is generated
		TArray<UVoxelChunk*> AdjChunks;
		GetAdjacentChunks(AdjChunks);
		for (auto& Chunk : AdjChunks)
		{
			//If adjacent chunk's world is not fully generated
			//Don't render now
			if (!Chunk->PostGeneration)
			{
				Result = false;
				break;
			}
		}
	}
	return Result;
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

bool UVoxelChunk::ShouldPostGenerate()
{
	if (PrimeGenerated == 2 && !PostGeneration)
	{
		bool Result = true;
		TArray<UVoxelChunk*> AdjChunks;
		GetAdjacentChunks_Corner(AdjChunks);
		for (auto& Chunk : AdjChunks)
		{
			//If adjacent chunk's world is not generated
			if (Chunk->WorldGenState < EWorldGenState::GENERATED)
			{
				Result = false;
				break;
			}
		}
		return Result;
	}
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

UPrimeChunk::UPrimeChunk()
{
}

void UPrimeChunk::SetBlockDef(int X, int Y, int Z, UBlock* Block)
{
	Blocks[VOX_CHUNK_AI(X, Y, Z)] = Block;
}
