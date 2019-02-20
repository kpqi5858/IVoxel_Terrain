#include "IVoxel_Chunk.h"


AIVoxel_Chunk::AIVoxel_Chunk()
{
	RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	RootComponent = RootComp;

	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bHighPriority = true;
}

void AIVoxel_Chunk::Setup(AIVoxel_TerrainWorld* World, FIntVector ChunkLoc)
{
	IVoxWorld = World;
	ChunkLocation = ChunkLoc;

	Manager = IVoxWorld->Manager;

	RenderOctree = MakeShareable(new FOctree(FIntVector(0), Manager->OctreeDepthInit, nullptr));
	DataOctree = MakeShareable(new FIVoxel_DataManager(this));

	SetActorLocation(GetWorldLocation()/2);

	InitLeaves();
}

void AIVoxel_Chunk::InitLeaves()
{
	FOctree* Initial = new FOctree(FIntVector(0), Manager->OctreeDepthInit, nullptr);

	Initial->LodSubdivide(this);

	TSet<FOctree*> Nodes;
	Initial->GetChildOctrees_NoChild(Nodes);

	int ToInit = Nodes.Num();
	ToInit += ToInit / 2; //ToInit *= 1.5

	for (int i = 0; i < ToInit; i++)
	{
		auto Comp = NewNodeChunk();
		FreeLeaves.Add(Comp);
	}
	delete Initial;
}

void AIVoxel_Chunk::Tick(float DeltaTime)
{
	InternalTicks++;

	//Chunk Tick
	{
		FScopeLock Lock(&TickListLock);
		for (auto& Chunk : TickList)
		{
			Chunk->ChunkTick();
		}
	}

	GEngine->AddOnScreenDebugMessage(1, 0.0f, FColor::White, TEXT("Polygonizer threads : " + FString::FromInt(DoingThreadedJob.GetValue())));

	TSet<UIVoxelNodeChunk*> ToDelete;

	for (auto& Chunk : QueuedUnload)
	{
		Chunk->DeletionLeft -= DeltaTime;
		if (Chunk->DeletionLeft <= 0)
		{
			UnloadRMC(Chunk);
			ToDelete.Add(Chunk);
		}
	}
	for (auto& Chunk : ToDelete)
	{
		QueuedUnload.Remove(Chunk);
	}

	//if (DoingThreadedJob.GetValue()) return; //Don't update if thread is running

	int Rate = IVoxWorld->UpdatePerTicks;
	if (Rate == 0) Rate = 1;

	if (InternalTicks % Rate == 0) RenderOctreeTick();

	for (auto& Ch : ToUpdate)
	{
		auto Polygonizer = Manager->GetPolygonizer(this, Ch->NodePos, Ch->NodeDepth);
		auto PolygonizerThread = new IVoxel_PolygonizerThread(this, Ch->NodePos, Polygonizer); //Will automatically deleted

		checkf(!Ch->PolygonizerThread, TEXT("Two or more threads towarding same node chunks."));
		Ch->PolygonizerThread = PolygonizerThread;
		Manager->MesherThreadPool->AddQueuedWork(PolygonizerThread);
	}

	ToUpdate.Reset();
}

void AIVoxel_Chunk::RenderOctreeTick()
{
	check(RenderOctree.IsValid());

	TSet<FOctree*> OldOctreeNodes;
	TSet<FIntVector> OldOctreeLocs;

	RenderOctree->GetChildOctrees_NoChild(OldOctreeNodes);

	for (auto& Node : OldOctreeNodes)
	{
		OldOctreeLocs.Add(Node->Position);
	}

	TSharedPtr<FOctree> NewOctree = MakeShareable(new FOctree(FIntVector(0), Manager->OctreeDepthInit, nullptr));
	NewOctree->LodSubdivide(this);

	TSet<FOctree*> NewOctreeNodes;
	TSet<FIntVector> NewOctreeLocs;

	NewOctree->GetChildOctrees_NoChild(NewOctreeNodes);

	for (auto& Node : NewOctreeNodes)
	{
		NewOctreeLocs.Add(Node->Position);
	}

	auto NodesToCreate = NewOctreeLocs.Difference(OldOctreeLocs);
	auto NodesToDelete = OldOctreeLocs.Difference(NewOctreeLocs);

	UE_LOG(LogIVoxel, Error, TEXT("Created %d Deleted %d"), NodesToCreate.Num(), NodesToDelete.Num());
	
	if (NodesToCreate.Num() > 10000)
	{
		UE_LOG(LogIVoxel, Error, TEXT("Too many nodes in octree"));
		return;
	}
	
	int CullingDepth = IVoxWorld->CullingDepth;

	TMap<FIntVector, UIVoxelNodeChunk*> ChunksToDeleteMap;

	for (auto& DLoc : NodesToDelete)
	{
		if (NewOctreeLocs.Contains(DLoc)) check(false);

		auto Comp = LoadedLeaves.Find(DLoc);
		if (!Comp)
		{
			continue;
		}

		auto RealComp = *Comp;
		if (RealComp->HasMesh || RealComp->OldChunkCount)
		{
			ChunksToDeleteMap.Add(DLoc, RealComp);
			RealComp->IncreaseOC();
		}
		RealComp->Unload();

		DeRegisterTickList(RealComp);

		LoadedLeaves.Remove(DLoc);
	}
	for (auto& CLoc : NodesToCreate)
	{
		auto Node = NewOctree->GetOctreeExact(CLoc);

		if (Node->Depth > CullingDepth) continue; //Don't mesh node's depth bigger than culling

		auto Comp = GetFreeNodeChunk(CLoc, Node->Depth);
		Comp->Load(CLoc, Node->Depth);

		LoadedLeaves.Add(CLoc, Comp);

		TSet<FOctree*> OldNodes;
		RenderOctree->GetChildOctreesIntersect(CLoc, OldNodes);

		for (auto& OldNode : OldNodes)
		{
			auto ChunksToDelete = ChunksToDeleteMap.Find(OldNode->Position);
			if (ChunksToDelete)
			{
				Comp->OldChunks.Add(*ChunksToDelete);
				auto RealPtr = *ChunksToDelete;
				RealPtr->IncreaseOC();
			}
		}

		Comp->SetRelativeLocation(FVector(Node->GetMinimalPosition()) * Manager->VoxelSizeInit);
		Comp->SetWorldScale3D(FVector(Node->Size() / IVOX_CHUNKDATASIZE)); //Is this causes lag?

		auto Polygonizer = Manager->GetPolygonizer(this, Node);
		auto PolygonizerThread = new IVoxel_PolygonizerThread(this, CLoc, Polygonizer); //Will automatically deleted
		checkf(!Comp->PolygonizerThread, TEXT("Two or more threads towarding same node chunks."));
		Comp->PolygonizerThread = PolygonizerThread;
		Manager->MesherThreadPool->AddQueuedWork(PolygonizerThread);
	}

	TArray<UIVoxelNodeChunk*> DeleteChunks;
	ChunksToDeleteMap.GenerateValueArray(DeleteChunks);

	for (auto& chunk : DeleteChunks)
	{
		chunk->DecreaseOC();
	}
	RenderOctree = NewOctree;
}

inline void AIVoxel_Chunk::ApplyPolygonized(UIVoxelNodeChunk* RMC, IVoxel_PolygonizedData& Data)
{
	if (Data.PolygonizedSections.Num() == 0)
	{
		UE_LOG(LogIVoxel, Error, TEXT("PolygonizedData sections num is 0"));
		ensure(false);
		return;
	}
	check(Data.PolygonizedSections.Num() == IVoxWorld->VoxelMaterials.Num());

	bool ShouldCreateCollision = RMC->NodeDepth <= IVoxWorld->CollisionMaxDepth;

	for (int index = 0; index < Data.PolygonizedSections.Num(); index++)
	{
		auto& Section = Data.PolygonizedSections[index];
		if (Section.Vertex.Num() && false)
		{
			RMC->UpdateMeshSection(index, Section.Vertex, Section.Triangle, Section.Normal, TArray<FVector2D>(), Section.Color, TArray<FRuntimeMeshTangent>());
		}
		else
		{
			RMC->CreateMeshSection(index, Section.Vertex, Section.Triangle, Section.Normal, Section.UV, Section.Color, /*Section.Tangent*/TArray<FRuntimeMeshTangent>(), ShouldCreateCollision);
		}
	}
}

inline FVector AIVoxel_Chunk::GetWorldLocation()
{
	return GetDataLocation()
		* Manager->VoxelSizeInit
		+ IVoxWorld->GetActorLocation();
}

inline FVector AIVoxel_Chunk::GetDataLocation()
{
	return FVector(IVOX_CHUNKDATASIZE)
		* FVector(ChunkLocation)
		* FOctree::SizeFor(Manager->OctreeDepthInit);
}

FVector AIVoxel_Chunk::WorldPosToLocalPos(FVector Pos)
{
	return (Pos / Manager->VoxelSizeInit - IVoxWorld->GetActorLocation()) - GetDataLocation();
}

FVector AIVoxel_Chunk::LocalPosToWorldPos(FVector Pos)
{
	return (Pos * Manager->VoxelSizeInit + IVoxWorld->GetActorLocation()) + GetDataLocation();
}

void AIVoxel_Chunk::UpdateChunkAt(FIntVector Pos)
{
	auto Node = RenderOctree->GetOctree_NoSub(Pos);
	auto Chunk = *LoadedLeaves.Find(Node->Position);
	check(Chunk->DeletionLeft == 0.f);

	ToUpdate.Add(Chunk);
}

UIVoxelNodeChunk* AIVoxel_Chunk::GetFreeNodeChunk(FIntVector NodePos, uint8 NodeDepth)
{
	if (FreeLeaves.Num())
	{
		auto Comp = FreeLeaves.Pop();
		RegisterTickList(Comp);
		return Comp;
	}
	else
	{
		auto Comp = NewNodeChunk();
		RegisterTickList(Comp);
		return Comp;
	}
	return nullptr;
}

inline UIVoxelNodeChunk* AIVoxel_Chunk::NewNodeChunk()
{
	auto Comp = NewObject<UIVoxelNodeChunk>(this);
	Comp->Setup(this);
	Comp->SetupAttachment(RootComponent);
	Comp->RegisterComponent();
	for (int i = 0; i < IVoxWorld->VoxelMaterials.Num(); i++)
	{
		Comp->SetMaterial(i, IVoxWorld->VoxelMaterials[i]);
		Comp->CreateMeshSection(i, TArray<FVector>(), TArray<int32>(), TArray<FVector>(), TArray<FVector2D>(), TArray<FColor>(), TArray<FRuntimeMeshTangent>(), true);
	}
	return Comp;
}

void AIVoxel_Chunk::QueueUnload(UIVoxelNodeChunk* Chunk)
{
	Chunk->DeletionLeft = IVoxWorld->DeletionDelay;
	QueuedUnload.Add(Chunk);
}

void AIVoxel_Chunk::UnloadRMC(UIVoxelNodeChunk* Chunk)
{
	check(!FreeLeaves.Contains(Chunk));
	Chunk->ClearAllMeshSections();
	FreeLeaves.Add(Chunk);
}

void AIVoxel_Chunk::RegisterTickList(UIVoxelNodeChunk* Chunk)
{
	FScopeLock Lock(&TickListLock);
	TickList.Add(Chunk);
}

void AIVoxel_Chunk::DeRegisterTickList(UIVoxelNodeChunk* Chunk)
{
	FScopeLock Lock(&TickListLock);
	TickList.Remove(Chunk);
}

void AIVoxel_Chunk::EditWorldTest(FVector Position, bool bCreate)
{
	FIntVector BasePos = FIntVector((Position - GetActorLocation()) / Manager->VoxelSizeInit);

	int Radius = 5;

	for (int X = -Radius; X < Radius; X++)
	for (int Y = -Radius; Y < Radius; Y++)
	for (int Z = -Radius; Z < Radius; Z++)
	{
		FIntVector Temp = BasePos + FIntVector(X, Y, Z);
		float Val = Radius - FVector(BasePos - Temp).Size();
		if (Val < 0) continue;
		FOctree* Target = DataOctree->MainDataOctree->GetOctree(Temp, 0);
		Target->SetData(Temp - Target->GetMinimalPosition(), FIVoxel_BlockData(bCreate ? Val : -Val, FColor::White, 0), FVector(0), IVoxWorld->WorldGeneratorInstanced);
		UpdateChunkAt(Target->Position);
		DataOctree->EditedNode(Target);
	}
}

uint8 AIVoxel_Chunk::GetLodFor(FOctree* Node)
{
	auto Curve = IVoxWorld->LodCurve;
	int MaxDepth = Manager->OctreeDepthInit;

	FVector Pos = FVector(Node->Position) * Manager->VoxelSizeInit + GetActorLocation() / IVOX_CHUNKDATASIZE;
	float Dist = Manager->GetMinDistanceToInvokers(Pos) / Manager->VoxelSizeInit / IVOX_CHUNKDATASIZE;
	Dist = FMath::Max(1.0f, Dist);

	return FMath::Clamp(FMath::FloorToInt(float(FMath::Log2(Dist)) - 0.4), 0, 32);
}

inline FIntVector AIVoxel_Chunk::AsLocation(int num)
{
	check(num < IVOX_CHUMKDATAARRAYSIZE)
	return FIntVector(num % IVOX_CHUNKDATASIZE
					, num / IVOX_CHUNKDATASIZE % IVOX_CHUNKDATASIZE
					, num / IVOX_CHUNKDATASIZE / IVOX_CHUNKDATASIZE / IVOX_CHUNKDATASIZE);
}