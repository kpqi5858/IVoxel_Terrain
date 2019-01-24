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
{	/*
	const int LoadArea = 1;
	for (int x = -LoadArea; x < LoadArea; x++)
		for (int y = -LoadArea; y < LoadArea; y++)
			for (int z = -LoadArea; z < LoadArea; z++)
			{
				CreateChunk(FIntVector(x, y, z), false);
			}*/
	CreateChunk(FIntVector(0, 0, 0), false);
	/*
	FOctree* MainOctree = new FOctree(FIntVector(0), World->KeepChunkRadius);
	const int InitinalOctreeBox = 3;
	for (int x = -InitinalOctreeBox; x < InitinalOctreeBox; x++)
	for (int y = -InitinalOctreeBox; y < InitinalOctreeBox; y++)
	for (int z = -InitinalOctreeBox; z < InitinalOctreeBox; z++)
	{
		MainOctree->GetOctree(FVector(x, y, z), 0);
	}

	TSet<FOctree*> Octrees;
	MainOctree->GetChildOctrees(Octrees, 0);

	for (auto& Node : Octrees)
	{
		if (Node->HasChilds) continue;
		FActorSpawnParameters SpawnParm;
		SpawnParm.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		auto chunk = World->GetWorld()->SpawnActor<AIVoxel_Chunk>(FVector(0), FRotator(0), SpawnParm);
		chunk->Setup(World, Node->Position);
		FVector AL = Node->GetMinimalPosition_World() * World->GetVoxelSize() / 2;
		chunk->SetActorLocation(AL);
		MainOctree->GetData(Node->Position, Node->Depth, World->WorldGeneratorInstanced, chunk->TileData);
		chunk->SetActorScale3D(FVector(Node->Size()/2));

		auto Polygonizer = GetPolygonizer(chunk);
		IVoxel_PolygonizedData PData;
		Polygonizer->Polygonize(PData);
		chunk->ApplyPolygonized(PData);
	}

	MainOctree->Destroy();
	*/
}

void IVoxel_TerrainManager::Tick()
{
	InternalTickCount++;
	/*
	if (InternalTickCount % 5 == 0)
	{
		TSet<TWeakObjectPtr<AActor>> InvalidInvokers;

		for (auto& Invoker : InvokersList)
		{
			if (!Invoker.IsValid())
			{
				InvalidInvokers.Add(Invoker);
				continue;
			}
			FIntVector InvokerPos = WorldLocationToChunkIndex(Invoker->GetActorLocation());
			const int LoadArea = World->KeepChunkRadius * 2;
			for (int x = -LoadArea; x < LoadArea; x+=2)
			for (int y = -LoadArea; y < LoadArea; y+=2)
			for (int z = -LoadArea; z < LoadArea; z+=2)
			{
				CreateChunk(FIntVector(x, y, z) + InvokerPos, true);
			}

		}

		for (auto& IIV : InvalidInvokers)
		{
			InvokersList.Remove(IIV);
		}
	}*/
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
/*
void IVoxel_TerrainManager::RenderOctreeTick(TSet<FVector>& LoadLoc)
{
	TSet<FOctree*> OldOctreeNodes;
	MainRenderOctree->GetChildOctrees(OldOctreeNodes, 0);

	TSet<FIntVector> OldOctreeLocs;
	for (auto& Node : OldOctreeNodes)
	{
		if (!Node->HasChilds)
			OldOctreeLocs.Add(Node->Position);
	}

	TSharedPtr<FOctree> NewOctree = MakeShareable(new FOctree(FIntVector(0), OctreeDepthInit));

	TSet<FOctree*> NewOctreeNodes;
	TSet<FIntVector> NewOctreeLocs;

	const int LoadArea = World->KeepChunkRadius;

	for (auto& InvokerLoc : LoadLoc)
	{
		FIntVector InvokerPos = WorldLocationToOctreePos(InvokerLoc);
		for (int x = -LoadArea; x < LoadArea; x++)
		for (int y = -LoadArea; y < LoadArea; y++)
		for (int z = -LoadArea; z < LoadArea; z++)
		{
			NewOctree->GetOctree(FVector(InvokerPos + FIntVector(x, y, z)));
		}
	}
	NewOctree->GetChildOctrees(NewOctreeNodes, 0);
	for (auto& Node : NewOctreeNodes)
	{
		if (!Node->HasChilds)
			NewOctreeLocs.Add(Node->Position);
	}

	TSet<FIntVector> NodesToCreate = NewOctreeLocs.Difference(OldOctreeLocs);
	TSet<FIntVector> NodesToDelete = OldOctreeLocs.Difference(NewOctreeLocs);

	
	for (auto& Dloc : NodesToDelete)
	{
		auto ChunkPtr = ChunksNodeLoaded.Find(Dloc);
		if (!ChunkPtr)
		{
			UE_LOG(LogIVoxel, Error, TEXT("No loaded chunk"));
			continue;
		}
		auto Chunk = *ChunkPtr;
		ChunksNodeLoaded.Remove(Dloc);
		UnloadChunk(Chunk);
	}
	for (auto& Cloc : NodesToCreate)
	{
		auto Chunk = GetChunkUnloaded();
		FOctree* Node = NewOctree->GetOctreeExact(Cloc);

		CreateChunkOctree(Chunk, Node, false);
	}

	UE_LOG(LogIVoxel, Warning, TEXT("Created %d Deleted %d"), NodesToCreate.Num(), NodesToDelete.Num());
	NewOctree->DebugRender(World->GetWorld());
	MainRenderOctree = NewOctree;
}
*/
void IVoxel_TerrainManager::Destroy()
{
	MesherThreadPool->Destroy();
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

void IVoxel_TerrainManager::DebugRender(UWorld* World)
{
	//MainRenderOctree->DebugRender(World);
}