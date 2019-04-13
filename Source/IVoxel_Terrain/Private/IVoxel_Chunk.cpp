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

	TSet<UIVoxelNodeChunk*> Ignored;

	for (auto& Cho : ToUpdate)
	{
		auto Ch = Cho;

		if (!Ch->IsLoaded)
		{
			continue;
		}

		if (Ch->HasPolygonizerThread())
		{
			Ignored.Add(Ch);
			continue;
		}

		auto Polygonizer = Manager->GetPolygonizer(this, Ch->NodePos, Ch->NodeDepth);
		auto PolygonizerThread = new IVoxel_PolygonizerThread(this, Ch->NodePos, Polygonizer); //Will automatically deleted

		Ch->SetPolygonizerThread(PolygonizerThread);
		Manager->MesherThreadPool->AddQueuedWork(PolygonizerThread);
	}

	ToUpdate.Reset();
	ToUpdate.Append(Ignored);

	//Chunk Tick
	{
		FScopeLock Lock(&TickListLock);
		for (auto& Chunk : TickList)
		{
			Chunk->ChunkTick();
		}

	}

	int Rate = IVoxWorld->UpdatePerTicks;
	if (Rate == 0) Rate = 1;

	if (InternalTicks % Rate == 0) RenderOctreeTick();


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

		check(!Comp->HasPolygonizerThread());

		Comp->SetPolygonizerThread(PolygonizerThread);
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
		ensureAlways(false);
		return;
	}
	check(Data.PolygonizedSections.Num() == IVoxWorld->VoxelMaterials.Num());

	const bool ShouldCreateCollision = RMC->NodeDepth <= IVoxWorld->CollisionMaxDepth;
	const bool EnableUV = IVoxWorld->bEnableUV;
	const bool MatchesEnableTessellationCond = IVoxWorld->bEnableTessellation;

	for (int index = 0; index < Data.PolygonizedSections.Num(); index++)
	{
		auto& Section = Data.PolygonizedSections[index];

		ESectionUpdateFlags UpdateFlags = ESectionUpdateFlags::None;

		if (MatchesEnableTessellationCond)
		{
			UpdateFlags |= ESectionUpdateFlags::CalculateTessellationIndices;
		}

		if (Section.Vertex.Num() && false)
		{
			RMC->UpdateMeshSection(index, Section.Vertex, Section.Triangle, Section.Normal, TArray<FVector2D>(), Section.Color, TArray<FRuntimeMeshTangent>());
		}
		else
		{
			RMC->CreateMeshSection(index, Section.Vertex, Section.Triangle, Section.Normal, EnableUV ? Section.UV : TArray<FVector2D>(), Section.Color, /*Section.Tangent*/TArray<FRuntimeMeshTangent>(), ShouldCreateCollision, EUpdateFrequency::Average, UpdateFlags);
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
	//check(!Chunk->HasPolygonizerThread());

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

void AIVoxel_Chunk::EditWorldTest(FVector Position, float Radius, bool bCreate)
{
	FIntVector BasePos = FIntVector((Position - GetActorLocation()) / Manager->VoxelSizeInit);

	int IntRadius = FMath::CeilToInt(Radius) + 2;
	
	DataOctree->Begin(FRWScopeLockType::SLT_Write);

	for (int X = -IntRadius; X < IntRadius; X++)
	for (int Y = -IntRadius; Y < IntRadius; Y++)
	for (int Z = -IntRadius; Z < IntRadius; Z++)
	{
		FIntVector CurrentPos = BasePos + FIntVector(X, Y, Z);
		float Dist = FVector(X, Y, Z).Size();

		if (Dist <= Radius + 2)
		{
			float Value = FMath::Clamp(Radius - Dist, -2.f, 2.f) / 2.f;
			if (Value == 0) Value = 0.0001f;
			Value *= bCreate ? -1 : 1;

			FIVoxel_BlockData& Block = DataOctree->RefSingleData(CurrentPos);
			float OldValue = Block.Value;

			bool Valid;
			if ((Value <= 0 && bCreate) || (Value > 0 && !bCreate))
			{
				Valid = true;
			}
			else
			{
				Valid = (OldValue >= 0) == (Value >= 0);
			}
			if (Valid)
			{
				Block.Value = Value;
			}
		}
	}

	DataOctree->End(FRWScopeLockType::SLT_Write);
}

uint8 AIVoxel_Chunk::GetLodFor(FOctree* Node)
{
	int MaxDepth = Manager->OctreeDepthInit;

	FVector MinPos = FVector(Node->GetMinimalPosition()) * Manager->VoxelSizeInit + GetActorLocation() / IVOX_CHUNKDATASIZE;
	FVector MaxPos = FVector(Node->GetMaximumPosition()) * Manager->VoxelSizeInit + GetActorLocation() / IVOX_CHUNKDATASIZE;
	FBox OctreeBox = FBox(MinPos, MaxPos);

	float Dist = Manager->GetMinDistanceToInvokers_Box(OctreeBox) / Manager->VoxelSizeInit / IVOX_CHUNKDATASIZE;

	Dist = FMath::Max(1.0f, Dist);

	return FMath::Clamp(FMath::FloorToInt(FMath::Log2(Dist) - 0.5), 0, 32);
}

inline FIntVector AIVoxel_Chunk::AsLocation(int num)
{
	check(num < IVOX_CHUMKDATAARRAYSIZE)
	return FIntVector(num % IVOX_CHUNKDATASIZE
					, num / IVOX_CHUNKDATASIZE % IVOX_CHUNKDATASIZE
					, num / IVOX_CHUNKDATASIZE / IVOX_CHUNKDATASIZE / IVOX_CHUNKDATASIZE);
}