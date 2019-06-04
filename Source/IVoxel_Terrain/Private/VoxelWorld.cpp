#include "VoxelWorld.h"
#include "VoxelChunkRender.h"
#include "WorldGenerator.h"
#include "VoxelData.h"
#include "VoxelThreads.h"

AVoxelWorld::AVoxelWorld()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bHighPriority = true;
	PolygonizerThreadPool = new FMyQueuedThreadPool;
	WorldGeneratorThreadPool = new FMyQueuedThreadPool;
}

void AVoxelWorld::BeginPlay()
{
	Super::BeginPlay();
	Initialize();
}

void AVoxelWorld::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	delete PolygonizerThreadPool;
	delete WorldGeneratorThreadPool;
}

void AVoxelWorld::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	InternalTicks++;

	//Remove invalid invokers
	InvokersList.RemoveAllSwap([](FVoxelInvoker& VoxelInvoker)
	{
		return !VoxelInvoker.IsValid();
	});

	WaitingRender.RemoveAllSwap([&](AVoxelChunkRender* Render)
	{
		bool Result = !Render->IsPolygonizingNow();
		if (Result) FreeRender.Add(Render);
		return Result;
	});

	ChunkUpdateCount = 0;

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(1, 0, FColor::White, FString::Printf(TEXT("WorldGen : %d, Mesher : %d"), WorldGeneratorThreadPool->GetNumQueuedJobs(), PolygonizerThreadPool->GetNumQueuedJobs()));
	}
	//Create chunks around invokers
	//And lock LoadedChunk
	if (InternalTicks % CreateChunkInterval == 0)
	{
		FRWScopeLock Lock(LoadedChunkLock, FRWScopeLockType::SLT_Write);

		for (auto& Invoker : InvokersList)
		{
			AActor* InvokerActor = Invoker.Object.Get();
			FIntVector ChunkPos = FBlockPos(this, WorldPosToVoxelPos(InvokerActor->GetActorLocation())).GetChunkIndex();

			FIntVector MinPos = ChunkPos - (RenderChunkSize + PreGenerateChunkSize);
			FIntVector MaxPos = ChunkPos + (RenderChunkSize + PreGenerateChunkSize);

			for (int X = MinPos.X; X < MaxPos.X; X++)
			for (int Y = MinPos.Y; Y < MaxPos.Y; Y++)
			for (int Z = MinPos.Z; Z < MaxPos.Z; Z++)
			{
				//Already locked LoadedChunk
				UVoxelChunk* Chunk = GetChunkFromIndex(FIntVector(X, Y, Z), false);

				LoadChunk(Chunk);
			}
		}
	}

	check(RegistryReference.IsValid());

	//Tick loaded chunks
	auto CopyTickList = TickListCache;

	for (auto& Chunk : CopyTickList)
	{
		if (Chunk->ShouldBeTicked())
		{
			Chunk->ChunkTick();
		}
	}
}

void AVoxelWorld::RegisterTickList(UVoxelChunk* Chunk)
{
	//Tick list related should be not multithreaded
	check(IsInGameThread());
	TickListCache.Add(Chunk);
}

void AVoxelWorld::DeregisterTickList(UVoxelChunk* Chunk)
{
	check(IsInGameThread());
	check(TickListCache.Contains(Chunk));
	TickListCache.Remove(Chunk);
}

void AVoxelWorld::UnloadChunk(UVoxelChunk* Chunk)
{
	DeregisterTickList(Chunk);
	InactiveChunkList.Add(Chunk);
	Chunk->TrySetChunkState(EChunkState::CS_Invalid);
}

void AVoxelWorld::LoadChunk(UVoxelChunk* Chunk)
{
	if (InactiveChunkList.Contains(Chunk))
	{
		InactiveChunkList.Remove(Chunk);
	}
	RegisterTickList(Chunk);
	Chunk->TrySetChunkState(EChunkState::CS_NoRender);
}

AVoxelChunkRender* AVoxelWorld::CreateRenderActor()
{
	return (AVoxelChunkRender*) GetWorld()->SpawnActor(AVoxelChunkRender::StaticClass());
}

UVoxelChunk* AVoxelWorld::CreateVoxelChunk(FIntVector Index)
{
	auto NewChunk = NewObject<UVoxelChunk>(this);
	NewChunk->Initialize(this, Index);
	return NewChunk;
}

AVoxelChunkRender* AVoxelWorld::GetFreeRenderActor()
{
	if (FreeRender.Num())
	{
		return FreeRender.Pop();
	}
	else
	{
		return CreateRenderActor();
	}
}

void AVoxelWorld::FreeRenderActor(AVoxelChunkRender* RenderActor)
{
	//Chunk maybe polygonizing now
	check(!RenderActor->IsInitialized());
	WaitingRender.Push(RenderActor);
}

void AVoxelWorld::Initialize()
{
	check(!IsInitialized);
	PolygonizerThreadPool->Create(PolygonizerThreads, 2048*2048);
	WorldGeneratorThreadPool->Create(WorldGeneratorThreads, 2048 * 2048);
	VoxelSizeInit = VoxelSize;
	FBlockRegistry::ReloadBlocks();
	RegistryReference = FBlockRegistry::GetInstance();

	InstancedWorldGenerator = NewObject<UVoxelWorldGenerator>(this, GetWorldGeneratorClass());

	FIntVector Temp = RenderChunkSize * 2;

	//Create render chunks now
	int NumToCreate = Temp.X * Temp.Y * Temp.Z;
	NumToCreate += (NumToCreate / 4);
	for (int i = 0; i < NumToCreate; i++)
	{
		FreeRender.Add(CreateRenderActor());
	}

	IsInitialized = true;
}

void AVoxelWorld::RegisterInvoker(AActor* Object, bool DoRender)
{
	InvokersList.Add(FVoxelInvoker(Object, DoRender));
}

float AVoxelWorld::GetVoxelSize()
{
	check(IsInitialized);
	return VoxelSizeInit;
}

UClass* AVoxelWorld::GetWorldGeneratorClass()
{
	auto TheClass = WorldGeneratorClass.Get();
	if (!TheClass)
	{
		UE_LOG(LogIVoxel, Error, TEXT("World generator is null"));
		TheClass = UFlatWorldGenerator::StaticClass();
	}
	return TheClass;
}

UVoxelWorldGenerator* AVoxelWorld::GetWorldGenerator()
{
	check(IsInitialized);
	return InstancedWorldGenerator;
}

UVoxelChunk* AVoxelWorld::GetChunkFromIndex_Internal(FIntVector Pos, bool DoLock)
{
	check(IsInitialized);

	UVoxelChunk** Find = LoadedChunk.Find(Pos);
	if (Find) return *Find;
	else
	{
		auto Chunk = CreateVoxelChunk(Pos);
		LoadedChunk.Add(Pos, Chunk);
		return Chunk;
	}
}

UVoxelChunk* AVoxelWorld::GetChunkFromIndex(FIntVector Pos, bool DoLock)
{
	check(IsInitialized);
	if (DoLock)
	{
		{
			FRWScopeLock Lock(LoadedChunkLock, FRWScopeLockType::SLT_ReadOnly);
			auto Find = LoadedChunk.Find(Pos);
			if (Find) return *Find;
		}
		FRWScopeLock Lock(LoadedChunkLock, FRWScopeLockType::SLT_Write);

		auto Chunk = CreateVoxelChunk(Pos);
		LoadedChunk.Add(Pos, Chunk);
		return Chunk;
	}
	else
	{
		auto Find = LoadedChunk.Find(Pos);
		if (Find) return *Find;

		else
		{
			auto Chunk = CreateVoxelChunk(Pos);
			LoadedChunk.Add(Pos, Chunk);
			return Chunk;
		}

	}
}


UVoxelChunk* AVoxelWorld::GetChunkFromBlockPos(FBlockPos Pos, bool DoLock)
{
	FIntVector Index = Pos.GetChunkIndex();
	return GetChunkFromIndex(Index, DoLock);
}

FIntVector AVoxelWorld::WorldPosToVoxelPos(FVector Pos)
{
	float Size = GetVoxelSize();
	return FIntVector(FMath::FloorToInt(Pos.X / Size)
	, FMath::FloorToInt(Pos.Y / Size)
	, FMath::FloorToInt(Pos.Z / Size));
}

bool AVoxelWorld::ShouldChunkRendered(UVoxelChunk* Chunk)
{
	bool Result = false;

	FIntVector ChunkPos = Chunk->GetChunkPosition();

	for (auto& Invoker : InvokersList)
	{
		if (Invoker.IsValid() && Invoker.ShouldRender)
		{
			FIntVector PosDifference = FBlockPos(this, WorldPosToVoxelPos(Invoker.Object->GetActorLocation())).GetChunkIndex() - ChunkPos;
			PosDifference.X = FMath::Abs(PosDifference.X);
			PosDifference.Y = FMath::Abs(PosDifference.Y);
			PosDifference.Z = FMath::Abs(PosDifference.Z);

			if (PosDifference.X <= RenderChunkSize.X
				&& PosDifference.Y <= RenderChunkSize.Y
				&& PosDifference.Z <= RenderChunkSize.Z)
			{
				Result = true;
				break;
			}
		}
	}
	return Result;
}

bool AVoxelWorld::ShouldGenerateWorld(UVoxelChunk* Chunk)
{
	bool Result = false;

	FIntVector ChunkPos = Chunk->GetChunkPosition();
	FIntVector MaxDifference = RenderChunkSize + PreGenerateChunkSize;

	for (auto& Invoker : InvokersList)
	{
		if (Invoker.IsValid() && Invoker.ShouldRender)
		{
			FIntVector PosDifference = FBlockPos(this, WorldPosToVoxelPos(Invoker.Object->GetActorLocation())).GetChunkIndex() - ChunkPos;
			PosDifference.X = FMath::Abs(PosDifference.X);
			PosDifference.Y = FMath::Abs(PosDifference.Y);
			PosDifference.Z = FMath::Abs(PosDifference.Z);

			if (PosDifference.X <= MaxDifference.X
				&& PosDifference.Y <= MaxDifference.Y
				&& PosDifference.Z <= MaxDifference.Z)
			{
				Result = true;
				break;
			}
		}
	}
	return Result;
}

void AVoxelWorld::QueueWorldGeneration(UVoxelChunk* Chunk)
{
	WorldGeneratorThreadPool->AddQueuedWork(new FWorldGeneratorThread(Chunk));
}

void AVoxelWorld::QueuePostWorldGeneration(UVoxelChunk* Chunk)
{
	WorldGeneratorThreadPool->AddQueuedWork(new FPostWorldGeneratorThread(Chunk));
}

void AVoxelWorld::QueueUpdateFaceVisiblity(UVoxelChunk* Chunk)
{
	PolygonizerThreadPool->AddQueuedWork(new FUpdateVisiblityThread(Chunk));
}

void AVoxelWorld::QueuePolygonize(AVoxelChunkRender* Render)
{
	PolygonizerThreadPool->AddQueuedWork(new FVoxelPolygonizerThread(Render));
}

float AVoxelWorld::GetDistanceToInvoker(UVoxelChunk* Chunk, bool Render)
{
	check(IsInitialized);
	float MinDist = FLT_MAX;

	for (auto& Invoker : InvokersList)
	{
		if (Invoker.IsValid() && Render == Invoker.ShouldRender)
		{
			float Dist = FVector::Dist(Chunk->GetWorldPosition(), Invoker.Object->GetActorLocation());
			MinDist = FMath::Min(Dist, MinDist);
		}
	}
	return MinDist;
}

bool AVoxelWorld::ShouldUpdateChunk()
{
	if (ChunkUpdateCount < ChunkUpdateThreshold)
	{
		ChunkUpdateCount++;
		return true;
	}
	return false;
}
