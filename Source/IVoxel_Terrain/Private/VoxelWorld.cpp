#include "VoxelWorld.h"
#include "VoxelChunkRender.h"
#include "WorldGenerator.h"

AVoxelWorld::AVoxelWorld()
{
	PrimaryActorTick.bHighPriority = true;
}

void AVoxelWorld::BeginPlay()
{
	Initialize();
}

void AVoxelWorld::EndPlay(EEndPlayReason::Type EndPlayReason)
{

}

void AVoxelWorld::Tick(float DeltaSeconds)
{
	InternalTicks++;

	//Remove invalid invokers
	InvokersList.RemoveAll([](FVoxelInvoker& VoxelInvoker)
	{
		return !VoxelInvoker.IsValid();
	});

	//Create chunks around invokers
	//And lock LoadedChunk
	if (InternalTicks % CreateChunkInterval == 0)
	{
		FScopeLock Lock(&LoadedChunkLock);

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

				Chunk->TrySetChunkState(EChunkState::CS_NoRender);
			}
		}
	}

	//Tick loaded chunks
	TArray<UVoxelChunk*> ToTick;
	ToTick.Reserve(LoadedChunk.Num());

	LoadedChunk.GenerateValueArray(ToTick);
	for (auto& Chunk : ToTick)
	{
		if (Chunk->ShouldBeTicked)
		{
			Chunk->ChunkTick();
		}
	}
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
	check(!RenderActor->IsInitialized());
	FreeRender.Push(RenderActor);
}

void AVoxelWorld::Initialize()
{
	check(!IsInitialized);
	VoxelSizeInit = VoxelSize;
	WorldGeneratorInit = WorldGenerator.Get();
	if (!WorldGenerator)
	{
		UE_LOG(LogIVoxel, Error, TEXT("World generator is null"));
		WorldGeneratorInit = UFlatWorldGenerator::StaticClass();
	}
}

void AVoxelWorld::RegisterInvoker(AActor* Object, bool DoRender)
{
	check(IsInitialized);
	InvokersList.Add(FVoxelInvoker(Object, DoRender));
}

float AVoxelWorld::GetVoxelSize()
{
	check(IsInitialized);
	return VoxelSizeInit;
}

UClass* AVoxelWorld::GetWorldGeneratorClass()
{
	check(IsInitialized)
	return WorldGeneratorInit;
}

UVoxelChunk* AVoxelWorld::GetChunkFromIndex_Internal(FIntVector Pos)
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
	if (DoLock)
	{
		FScopeLock Lock(&LoadedChunkLock);
		return GetChunkFromIndex_Internal(Pos);
	}
	else
	{
		return GetChunkFromIndex_Internal(Pos);
	}
}

UVoxelChunk* AVoxelWorld::GetChunkFromBlockPos(FBlockPos Pos, bool DoLock)
{
	FIntVector Index = Pos.GetChunkIndex();
	return GetChunkFromIndex(Index, DoLock);
}

FIntVector AVoxelWorld::WorldPosToVoxelPos(FVector Pos)
{
	return FIntVector(Pos / GetVoxelSize());
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
				|| PosDifference.Y <= RenderChunkSize.Y
				|| PosDifference.Z <= RenderChunkSize.Z)
			{
				Result = true;
				break;
			}
		}
	}
	return Result;
}

bool AVoxelWorld::ShouldGenerateWorld(UVoxelChunk * Chunk)
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
				|| PosDifference.Y <= MaxDifference.Y
				|| PosDifference.Z <= MaxDifference.Z)
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
	//Currently only synchronous
	Chunk->GenerateWorld();
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