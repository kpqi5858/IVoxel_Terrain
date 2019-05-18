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

}

AVoxelChunkRender* AVoxelWorld::CreateRenderActor()
{
	return (AVoxelChunkRender*) GetWorld()->SpawnActor(AVoxelChunkRender::StaticClass());
}

UVoxelChunk* AVoxelWorld::CreateVoxelChunk(FIntVector Index)
{
	auto NewChunk = NewObject<UVoxelChunk>(this);
	NewChunk->Initialize(Index);
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

UClass* AVoxelWorld::GetWorldGenerator()
{
	check(IsInitialized)
	return WorldGeneratorInit;
}

UVoxelChunk* AVoxelWorld::GetChunkFromIndex(FIntVector Pos)
{
	FScopeLock Lock(&LoadedChunkLock);
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

UVoxelChunk* AVoxelWorld::GetChunkFromBlockPos(FBlockPos Pos)
{
	FIntVector Index = Pos.GetChunkIndex();
	return GetChunkFromIndex(Index);
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