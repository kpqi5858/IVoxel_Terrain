#include "VoxelWorld.h"
#include "VoxelChunkRender.h"

AVoxelWorld::AVoxelWorld()
{
	PrimaryActorTick.bHighPriority = true;
}

void AVoxelWorld::BeginPlay()
{
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

UVoxelChunk* AVoxelWorld::GetChunkFromIndex(FIntVector Pos)
{
	check(IsInitialized);
	return nullptr;
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