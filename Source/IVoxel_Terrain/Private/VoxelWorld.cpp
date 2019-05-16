#include "VoxelWorld.h"

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

FVoxelChunk* AVoxelWorld::GetChunk(FIntVector Pos)
{
	return nullptr;
}
