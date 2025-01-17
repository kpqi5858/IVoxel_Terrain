#include "VoxelChunk.h"
#include "VoxelWorld.h"
#include "VoxelThreads.h"
#include "VoxelChunkRender.h"
#include "WorldGenerator.h"
#include "BlockStateStorage.h"

//TODO : Prioritize chunk loading that close to invoker

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

	AdjacentCache.Init(this);
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

void UVoxelChunk::ProcessPrimeChunk()
{
	for (int Index = 0; Index < VOX_CHUNKSIZE_ARRAY; Index++)
	{
		FBlockPos Pos = FBlockPos(this, FVoxelUtilities::PositionFromIndex(Index));
		UBlock* Block = PrimeChunk.GetBlockDef(Index);

		SetBlock(Pos, Block, false);
	}
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
	BlockStorage->SetBlock(Pos.ArrayIndex(), Block);
	GetFaceVisiblityCache(Pos).SetDirty(true);
	SetRenderDirty();

	if (DoUpdate) UpdateBlock(Pos);
}

UBlock* UVoxelChunk::GetBlock(FBlockPos Pos)
{
	return BlockStorage->GetBlock(Pos.ArrayIndex());
}

void UVoxelChunk::UpdateBlock(FBlockPos& Pos)
{
}

inline void UVoxelChunk::UpdateFaceVisiblity(FBlockPos& Pos)
{
	auto& ThisVisiblity = GetFaceVisiblityCache(Pos);
	const int ThisType = GetBlock(Pos)->OpaqueType();

	ThisVisiblity.Data = 0;

	constexpr EBlockFace AllFaces[6] = { EBlockFace::FRONT, EBlockFace::BACK, EBlockFace::LEFT, EBlockFace::RIGHT, EBlockFace::TOP, EBlockFace::BOTTOM };

	for (auto Face : AllFaces)
	{
		FBlockPos NextPos = Pos;
		NextPos.GlobalPos += FVoxelUtilities::GetFaceOffset(Face);
		auto NextChunk = NextPos.GetChunkIndex() == ChunkPosition ? this : GetAdjacentChunkByFace(Face);
		auto& NextVisiblity = NextChunk->GetFaceVisiblityCache(NextPos);
		const int NextType = NextChunk->GetBlock(NextPos)->OpaqueType();

		const bool IsVisible = ThisType != NextType;
		const bool ThisVisible = ThisType && IsVisible;
		const bool NextVisible = NextType && IsVisible;

		ThisVisiblity.SetFaceVisible(Face, ThisVisible);
		if (NextVisiblity.SetFaceVisible(FVoxelUtilities::GetOppositeFace(Face), NextVisible) && this != NextChunk)
		{
			NextChunk->SetRenderDirty();
		}
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

	AdjacentCache.GetAdjacentChunks(Poses, Ret);
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
		Poses.Add(Pos);
	}
	AdjacentCache.GetAdjacentChunks(Poses, Ret);
}

UVoxelChunk* UVoxelChunk::GetAdjacentChunkByFace(EBlockFace Face)
{
	return AdjacentCache.GetAdjacentChunkByFace(Face);
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
	if (WorldGenState == EWorldGenState::GENERTED_PRIME && World->ShouldChunkRendered(this))
	{
		TArray<UVoxelChunk*> AdjChunks;
		GetAdjacentChunks_Corner(AdjChunks);
		for (auto& Chunk : AdjChunks)
		{
			//If adjacent chunk's world is not generated
			if (Chunk->WorldGenState < EWorldGenState::GENERTED_PRIME)
			{
				return false;
			}
		}
		return true;
	}
	return false;
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

void UVoxelChunk::QueuePolygonize()
{
	check(RenderActor);
	check(!RenderActor->IsPolygonizingNow());
	UniversalThread->InitThreadType(EUniversalThreadType::MESHER);
	World->QueueJob(UniversalThread, EThreadPoolToUse::RENDER);
}

bool UVoxelChunk::IsDoingWorkNow()
{
	return UniversalThread->IsDoingJobNow;
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

UPrimeChunk::UPrimeChunk()
{
	BlockStorage = new FBasicBlockStorage();
}

UPrimeChunk::~UPrimeChunk()
{
	delete BlockStorage;
}

void UPrimeChunk::SetBlockDef(int X, int Y, int Z, UBlock * Block)
{
	BlockStorage->SetBlock(VOX_CHUNK_AI(X, Y, Z), Block);
}

UBlock* UPrimeChunk::GetBlockDef(int Index)
{
	return BlockStorage->GetBlock(Index);
}
