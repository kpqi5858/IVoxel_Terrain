#include "IVoxel_TerrainManager.h"

IVoxel_TerrainManager::IVoxel_TerrainManager(AIVoxel_TerrainWorld* world) 
	: World(world)
	, MesherThreadPool(FQueuedThreadPool::Allocate())
	, OctreeDepthInit(world->OctreeSize)
	, VoxelSizeInit(world->GetVoxelSize())
{
	MesherThreadPool->Create(world->ThreadCount, 1024*1024);
}

IVoxel_TerrainManager::~IVoxel_TerrainManager()
{
}

void IVoxel_TerrainManager::RegisterInvoker(TWeakObjectPtr<AActor> Actor)
{
	if (Actor.IsValid())
	{
		InvokersList.Add(Actor);
	}
	else
	{
		UE_LOG(LogIVoxel, Error, TEXT("RegisterInvoker with invalid actor"));
	}
}

void IVoxel_TerrainManager::CreateStartChunks()
{
	CreateChunk(FIntVector(0, 0, 0), false);
}

void IVoxel_TerrainManager::Tick()
{
	InternalTickCount++;
	if (World->TickFlag)
	{
		World->TickFlag = false;
		TSet<FVector> InvokersLoc;

		TSet<TWeakObjectPtr<AActor>> InvalidInvokers;

		for (auto& Invoker : InvokersList)
		{
			if (!Invoker.IsValid())
			{
				InvalidInvokers.Add(Invoker);
				continue;
			}
		}
		for (auto& IIV : InvalidInvokers)
		{
			InvokersList.Remove(IIV);
		}
	}
}

void IVoxel_TerrainManager::Destroy()
{
	MesherThreadPool->Destroy();
	delete MesherThreadPool;

	TArray<AIVoxel_Chunk*> ChunksToDelete;
	ChunksLoaded.GenerateValueArray(ChunksToDelete);

	for (auto& Chunk : ChunksToDelete)
	{
		Chunk->Destroy();
	}
	delete this;
}

//Currently only Marching Cubes polygonizer
TSharedPtr<IVoxel_Polygonizer> IVoxel_TerrainManager::GetPolygonizer(AIVoxel_Chunk* Chunk, FOctree* Node)
{
	return GetPolygonizer(Chunk, Node->Position, Node->Depth);
}

TSharedPtr<IVoxel_Polygonizer> IVoxel_TerrainManager::GetPolygonizer(AIVoxel_Chunk * Chunk, FIntVector ExactPos, uint8 Depth)
{
	auto Polygonizer = new IVoxel_MCPolygonizer(Chunk, ExactPos, Depth);
	return MakeShareable(Polygonizer);
}

void IVoxel_TerrainManager::DestroyedChunk(AIVoxel_Chunk* Chunk)
{
	ChunksLoaded.Remove(Chunk->ChunkLocation);
}

void IVoxel_TerrainManager::CreateChunk(FIntVector ChunkPos, bool AsyncMesher)
{
	if (ChunksLoaded.Contains(ChunkPos)) return;

	FActorSpawnParameters SpawnParm;
	SpawnParm.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	auto chunk = World->GetWorld()->SpawnActor<AIVoxel_Chunk>(FVector(0), FRotator(0), SpawnParm);
	chunk->Setup(World, ChunkPos);
	ChunksLoaded.Add(ChunkPos, chunk);
}

AIVoxel_Chunk* IVoxel_TerrainManager::GetChunkUnloaded()
{
	if (ChunksUnloaded.Num())
	{
		for (auto& El : ChunksUnloaded)
		{
			return El;
		}

		//Should not happen
		check(false);
	}
	else
	{
		FActorSpawnParameters SpawnParm;
		SpawnParm.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		auto chunk = World->GetWorld()->SpawnActor<AIVoxel_Chunk>(FVector(0), FRotator(0), SpawnParm);
		chunk->Setup(World, FIntVector(0));

		return chunk;
	}
	return nullptr;
}

void IVoxel_TerrainManager::UnloadChunk(AIVoxel_Chunk* Chunk)
{
	ChunksUnloaded.Add(Chunk);
}

inline FIntVector IVoxel_TerrainManager::WorldLocationToChunkIndex(FVector Pos)
{
	return FIntVector((Pos - World->GetActorLocation()) / (World->GetVoxelSize() * FVector(IVOX_CHUNKDATASIZE)));
}

inline FIntVector IVoxel_TerrainManager::WorldLocationToOctreePos(FVector Pos)
{
	return FIntVector((Pos - World->GetActorLocation()) / VoxelSizeInit / FVector(IVOX_CHUNKDATASIZE));
}

float IVoxel_TerrainManager::GetMinDistanceToInvokers(FVector Pos)
{
	float Ret = FLT_MAX;

	for (auto& Invoker : InvokersList)
	{
		if (Invoker.IsValid())
		{
			float Dist = (Invoker.Get()->GetActorLocation() - Pos).GetAbsMax();
			Ret = FMath::Min(Dist, Ret);
		}
	}
	return Ret;
}

bool IVoxel_TerrainManager::IsTooFarFromInvokers(FIntVector ChunkPos)
{
	return false;
	FVector ChunkPosV(ChunkPos);

	if (!InvokersList.Num()) return false;
	int LoadBox = World->KeepChunkRadius;

	for (auto& Invoker : InvokersList)
	{
		if (Invoker.IsValid())
		{
			FVector ActorIndexLoc = FVector(WorldLocationToChunkIndex(Invoker->GetActorLocation()));
			FBox LoadBox(ActorIndexLoc - FVector(LoadBox), ActorIndexLoc + FVector(LoadBox));
			if (LoadBox.IsInsideOrOn(ChunkPosV)) return false;
		}
	}
	return true;
}