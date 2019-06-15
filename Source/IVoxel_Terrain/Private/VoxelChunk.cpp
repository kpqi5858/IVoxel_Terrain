#include "VoxelChunk.h"
#include "VoxelWorld.h"
#include "VoxelThreads.h"
#include "VoxelChunkRender.h"
#include "WorldGenerator.h"
#include "BlockStateStorage.h"

//The problem is:
//IDK!!!!!!!!

UVoxelChunk::UVoxelChunk()
{
	ThisSharedPtr = TSharedPtr<UVoxelChunk>(this, FNonDeleter<UVoxelChunk>());
}

UVoxelChunk::~UVoxelChunk()
{
	if (BlockStorage) delete BlockStorage;
	if (UniversalThread) delete UniversalThread;
}

void UVoxelChunk::ChunkTick()
{
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
		QueueFaceVisiblityUpdate();
	}

	if (WorldGenState == EWorldGenState::NOT_GENERATED && World->ShouldGenerateWorld(this))
	{
		WorldGenState = EWorldGenState::GENERTING_PRIME;
		QueuePreWorldGeneration();
	}
	
	if (ShouldPostGenerate())
	{
		WorldGenState = EWorldGenState::GENERATING_POST;
		QueuePostWorldGeneration();
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
			RenderActor->RenderRequest();
		}
	}
}

void UVoxelChunk::Initialize(AVoxelWorld* VoxelWorld, FIntVector ChunkIndex)
{
	ChunkPosition = ChunkIndex;
	World = VoxelWorld;

	WorldGenerator = VoxelWorld->GetWorldGenerator();

	FMemory::Memset(FaceVisiblityCache, 0, sizeof(FaceVisiblityCache));

	BlockStorage = new FBasicBlockStorage();

	UniversalThread = new FChunkUniversalThread(this);
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
	for (int Index = 0; Index < VOX_CHUNKSIZE_ARRAY; Index++)
	{
		FBlockPos Pos = FBlockPos(this, FVoxelUtilities::PositionFromIndex(Index));
		UpdateFaceVisiblity(Pos);
	}
	WorldGenState = EWorldGenState::VISIBLITY_UPDATED;
}

void UVoxelChunk::ProcessPrimeChunk()
{
	BlockStateStorageLock();
	for (int Index = 0; Index < VOX_CHUNKSIZE_ARRAY; Index++)
	{
		FBlockPos Pos = FBlockPos(this, FVoxelUtilities::PositionFromIndex(Index));
		UBlock* Block = PrimeChunk.Blocks[Index];

		SetBlock(Pos, Block, false);
	}
	BlockStateStorageUnlock();
}

void UVoxelChunk::BlockStateStorageLock()
{
	BlockStorage->Lock();
}

void UVoxelChunk::BlockStateStorageUnlock()
{
	BlockStorage->UnLock();
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

void UVoxelChunk::SetBlock(FBlockPos Pos, UBlock* Block, bool DoUpdate)
{
	UBlock* Old = GetBlock(Pos);
	if (Old == Block) return;
	BlockStateStorageLock();
	BlockStorage->SetBlock(Pos.ArrayIndex(), Block);
	BlockStateStorageUnlock();
	if (DoUpdate) UpdateBlock(Pos);
}

UBlock* UVoxelChunk::GetBlock(FBlockPos Pos)
{
	return BlockStorage->GetBlock(Pos.ArrayIndex());
}

void UVoxelChunk::UpdateBlock(FBlockPos& Pos)
{
	//Update face visiblity cache
	UpdateFaceVisiblity(Pos);
}

inline void UVoxelChunk::UpdateFaceVisiblity(FBlockPos& Pos)
{
	auto& ThisVisiblity = GetFaceVisiblityCache(Pos);
	const int ThisType = GetBlock(Pos)->OpaqueType();

	ThisVisiblity.Data = 0;

	constexpr EBlockFace AllFaces[6] = { EBlockFace::FRONT, EBlockFace::BACK, EBlockFace::LEFT, EBlockFace::RIGHT, EBlockFace::TOP, EBlockFace::BOTTOM };

	for (auto Face : AllFaces)
	{
		FBlockPos NextPos = FBlockPos(Pos);
		NextPos.GlobalPos += FVoxelUtilities::GetFaceOffset(Face);
		auto NextChunk = IsInChunk(NextPos.GlobalPos) ? this : GetAdjacentChunkByFace(Face);
		auto& NextVisiblity = NextChunk->GetFaceVisiblityCache(NextPos);
		const int NextType = NextChunk->GetBlock(NextPos)->OpaqueType();

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

	Poses.Add(FIntVector(1, 0, 0));
	Poses.Add(FIntVector(-1, 0, 0));
	Poses.Add(FIntVector(0, 1, 0));
	Poses.Add(FIntVector(0, -1, 0));
	Poses.Add(FIntVector(0, 0, 1));
	Poses.Add(FIntVector(0, 0, -1));

	GetAdjacentChunksImpl(Poses, Ret);
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
	GetAdjacentChunksImpl(Poses, Ret);
}

void UVoxelChunk::GetAdjacentChunksImpl(TArray<FIntVector>& Poses, TArray<UVoxelChunk*>& Ret)
{
	FScopeLock Lock(&AdjacentCacheLock);

	TArray<FIntVector> NotFound;
	for (auto& Pos : Poses)
	{
		FIntVector AdjacentKey = Pos;
		auto Chunk = AdjacentCache.Find(AdjacentKey);
		if (Chunk && Chunk->Key.IsValid())
		{
			Ret.Add(Chunk->Value);
		}
		else
		{
			NotFound.Add(Pos + ChunkPosition);
		}
	}

	TArray<UVoxelChunk*> NotFoundRet;
	World->GetChunksFromIndices(NotFound, NotFoundRet);
	for (auto& Chunk : NotFoundRet)
	{
		FIntVector AdjacentKey = Chunk->GetChunkPosition() - ChunkPosition;
		AdjacentCache.Add(AdjacentKey, MakeTuple(Chunk->GetWeakPtr(), Chunk));
	}
	Ret.Append(NotFoundRet);
}

UVoxelChunk* UVoxelChunk::GetAdjacentChunkByFace(EBlockFace Face)
{
	FIntVector Key = FVoxelUtilities::GetFaceOffset(Face);
	{
		FScopeLock Lock(&AdjacentCacheLock);
		auto Find = AdjacentCache.Find(Key);
		if (Find && Find->Key.IsValid()) return Find->Value;
	}
	TArray<FIntVector> Poses;
	TArray<UVoxelChunk*> Ret;
	Poses.Add(Key);
	GetAdjacentChunksImpl(Poses, Ret);
	return Ret[0];
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
		return true;
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

void UVoxelChunk::QueuePreWorldGeneration()
{
	UniversalThread->InitThreadType(EUniversalThreadType::WORLDGEN_PRE);
	World->QueueJob(UniversalThread, EThreadPoolToUse::WORLDGEN);
}

void UVoxelChunk::QueuePostWorldGeneration()
{
	UniversalThread->InitThreadType(EUniversalThreadType::WORLDGEN_POST);
	World->QueueJob(UniversalThread, EThreadPoolToUse::WORLDGEN);
}

void UVoxelChunk::QueueFaceVisiblityUpdate()
{
	UniversalThread->InitThreadType(EUniversalThreadType::VISIBLITY);
	World->QueueJob(UniversalThread, EThreadPoolToUse::WORLDGEN);
}

void UVoxelChunk::QueuePolygonize()
{
	UniversalThread->InitThreadType(EUniversalThreadType::MESHER);
	World->QueueJob(UniversalThread, EThreadPoolToUse::RENDER);
}

TWeakPtr<UVoxelChunk> UVoxelChunk::GetWeakPtr()
{
	return ThisSharedPtr;
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
	check(RenderActor);
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
