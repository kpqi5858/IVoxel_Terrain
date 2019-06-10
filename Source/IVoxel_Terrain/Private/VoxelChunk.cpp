#include "VoxelChunk.h"
#include "VoxelWorld.h"
#include "VoxelChunkRender.h"
#include "WorldGenerator.h"
#include "BlockStateStorage.h"

UVoxelChunk::UVoxelChunk()
{
}

UVoxelChunk::~UVoxelChunk()
{
	if (BlockStateStorage) delete BlockStateStorage;
	if (FaceVisiblityCache) delete FaceVisiblityCache;
}

void UVoxelChunk::ChunkTick()
{
	check(!IsUnreachable());

	bool RenderedFlag = ShouldBeRendered();

	if (RenderedFlag)
	{
		TrySetChunkState(EChunkState::CS_Rendered);
	}

	EChunkState CurrentState = GetChunkState();
	//Invalid chunk should not be ticked
	check(CurrentState != EChunkState::CS_Invalid);

	//World Gen related
	if (ShouldUpdateFaceVisiblity())
	{
		WorldGenState = EWorldGenState::VISIBLITY_UPDATING;
		World->QueueUpdateFaceVisiblity(this);
	}

	if (WorldGenState == EWorldGenState::NOT_GENERATED && World->ShouldGenerateWorld(this))
	{
		WorldGenState = EWorldGenState::GENERTING_PRIME;
		World->QueueWorldGeneration(this);
	}
	
	if (ShouldPostGenerate())
	{
		WorldGenState = EWorldGenState::GENERATING_POST;
		World->QueuePostWorldGeneration(this);
	}
	
	//Out of range
	if (!World->ShouldGenerateWorld(this))
	{
		if (HasRender())
		{
			DeInitRender();
		}
		World->UnloadChunk(this);
		return;
	}

	if (RenderedFlag)
	{
		if (!HasRender())
		{
			InitRender();
		}
	}
	if (!RenderedFlag && HasRender())
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

	FaceVisiblityCache = new TBasicAbstractBlockStorage<FFaceVisiblityCache>();
	FaceVisiblityCache->Initialize();

	BlockStateStorage = new TBasicAbstractBlockStorage<FBlockState>();
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
	ProcessPrimeChunk();
	WorldGenState = EWorldGenState::GENERTED_PRIME;
}

void UVoxelChunk::PostGenerateWorld()
{
	WorldGenerator->PostGenerate(this);
	SetRenderDirty();
	WorldGenState = EWorldGenState::GENERATED_POST;
}

void UVoxelChunk::UpdateFaceVisiblityAll()
{
	FaceVisiblityCache->Lock();
	for (int Index = 0; Index < VOX_CHUNKSIZE_ARRAY; Index++)
	{
		FBlockPos Pos = FBlockPos(this, FVoxelUtilities::PositionFromIndex(Index));
		UpdateFaceVisiblity(Pos);
	}
	WorldGenState = EWorldGenState::VISIBLITY_UPDATED;
	FaceVisiblityCache->UnLock();
}

void UVoxelChunk::ProcessPrimeChunk()
{
	BlockStateStorageLock();
	for (int Index = 0; Index < VOX_CHUNKSIZE_ARRAY; Index++)
	{
		FBlockPos Pos = FBlockPos(this, FVoxelUtilities::PositionFromIndex(Index));
		UBlock* Block = PrimeChunk.Blocks[Index];
		UBlock* Old = GetBlockState(Pos)->GetBlockDef();
		if (Old == Block) continue; //SERIOUS TYPO
		ModifyBlockState(Pos, [&](FBlockState* State) {State->SetBlockDef(Block); }, false);
	}
	BlockStateStorageUnlock();
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
	//check(Pos.GetChunk() == this);
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
	BlockStateStorageLock();
	ModifyBlockState(Pos, [&](FBlockState* State) {State->SetBlockDef(Block); });
	BlockStateStorageUnlock();
}

FFaceVisiblityCache& UVoxelChunk::GetFaceVisiblityCache(FBlockPos& Pos)
{
	//check(Pos.GetChunk() == this);
	return *FaceVisiblityCache->Get(Pos.ArrayIndex());
}

void UVoxelChunk::UpdateBlock(FBlockPos& Pos)
{
	//Update face visiblity cache
	UpdateFaceVisiblity(Pos);
}

inline void UVoxelChunk::UpdateFaceVisiblity(FBlockPos& Pos)
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
	TArray<FIntVector> Poses;
	Poses.Add(ChunkPosition + FIntVector(1, 0, 0));
	Poses.Add(ChunkPosition + FIntVector(-1, 0, 0));
	Poses.Add(ChunkPosition + FIntVector(0, 1, 0));
	Poses.Add(ChunkPosition + FIntVector(0, -1, 0));
	Poses.Add(ChunkPosition + FIntVector(0, 0, 1));
	Poses.Add(ChunkPosition + FIntVector(0, 0, -1));

	World->GetChunksFromIndices(Poses, Ret);
}

void UVoxelChunk::GetAdjacentChunks_Corner(TArray<UVoxelChunk*>& Ret)
{
	TArray<FIntVector> Poses;

	for (int X = -1; X <= 1; X++)
	for (int Y = -1; Y <= 1; Y++)
	for (int Z = -1; Z <= 1; Z++)
	{
		if (X == 0 && Y == 0 && Z == 0) continue;
		FIntVector Pos = FIntVector(X, Y, Z);
		Poses.Add(Pos + ChunkPosition);
	}
	World->GetChunksFromIndices(Poses, Ret);
}

bool UVoxelChunk::ShouldBeRendered()
{
	bool Result = World->ShouldChunkRendered(this) && WorldGenState == EWorldGenState::GENERATED_POST;
	if (Result)
	{
		//Check adjacent chunk's world is generated
		TArray<UVoxelChunk*> AdjChunks;
		GetAdjacentChunks(AdjChunks);
		for (auto& Chunk : AdjChunks)
		{
			//If adjacent chunk's world is not fully generated
			//Don't render now
			if (Chunk->WorldGenState != EWorldGenState::GENERATED_POST)
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
	if (WorldGenState == EWorldGenState::VISIBLITY_UPDATED && World->ShouldChunkRendered(this))
	{
		bool Result = true;
		TArray<UVoxelChunk*> AdjChunks;
		GetAdjacentChunks_Corner(AdjChunks);
		for (auto& Chunk : AdjChunks)
		{
			//If adjacent chunk's world is not generated
			if (Chunk->WorldGenState < EWorldGenState::VISIBLITY_UPDATED)
			{
				Result = false;
				break;
			}
		}
		return Result;
	}
	return false;
}

bool UVoxelChunk::ShouldUpdateFaceVisiblity()
{
	bool Result = WorldGenState == EWorldGenState::GENERTED_PRIME && World->ShouldChunkRendered(this);
	if (Result)
	{
		TArray<UVoxelChunk*> AdjChunks;
		GetAdjacentChunks(AdjChunks);
		for (auto& Chunk : AdjChunks)
		{
			if (Chunk->WorldGenState < EWorldGenState::GENERTED_PRIME)
			{
				Result = false;
				break;
			}
		}
	}
	return Result;
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
