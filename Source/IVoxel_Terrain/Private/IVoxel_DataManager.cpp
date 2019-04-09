#include "IVoxel_DataManager.h"

FIVoxel_DataManager::FIVoxel_DataManager(AIVoxel_Chunk* Chunk)
	: MainDataOctree(new FOctree(FIntVector(0), Chunk->Manager->OctreeDepthInit, nullptr)), Chunk(Chunk)
{
}

void FIVoxel_DataManager::EditedNode(FOctree* Node)
{
	FScopeLock sl(&EditedOctreeLock);
	check(Node->Depth == 0);
	Node->IsEditedLastLODMake = true;
	EditedOctree.Add(Node);
}

void FIVoxel_DataManager::Begin(FRWScopeLockType Type)
{
	if (Type == FRWScopeLockType::SLT_ReadOnly)
	{
		MakeLOD();
		DataLock.ReadLock();
	}
	else
	{
		DataLock.WriteLock();
	}
}

void FIVoxel_DataManager::End(FRWScopeLockType Type)
{
	if (Type == FRWScopeLockType::SLT_ReadOnly)
	{
		DataLock.ReadUnlock();
	}
	else
	{
		DataLock.WriteUnlock();
	}
}

inline void FIVoxel_DataManager::MakeLOD()
{
	FRWScopeLock(DataLock, FRWScopeLockType::SLT_Write);
	FScopeLock sl(&EditedOctreeLock);

	for (auto& Node : EditedOctree)
	{
		Node->MakeLOD(Chunk->IVoxWorld->WorldGeneratorInstanced, FVector(0), false);
	}
	EditedOctree.Reset();
}

FIVoxel_BlockData& FIVoxel_DataManager::RefSingleData(FIntVector Location)
{
	//Assume already locked

	FOctree* Node = MainDataOctree->GetOctree(Location, 0);
	EditedNode(Node);
	Chunk->UpdateChunkAt(Location);
	if (!Node->Data) Node->InitDataWithWorldGen(Chunk->IVoxWorld->WorldGeneratorInstanced, FVector(0));

	return *Node->SingleData(Location);
}

FIVoxel_BlockData FIVoxel_DataManager::GetSingleData(FIntVector Location, FOctree*& LastOctree)
{
	FIVoxel_BlockData* Pointer = nullptr;

	if (LastOctree && LastOctree->IsInOctree_External(Location))
	{
		Pointer = LastOctree->SingleData(Location);
	}
	else if (MainDataOctree->IsInOctree_External(Location))
	{
		FOctree* Node = MainDataOctree->GetOctree_NoSub(Location);
		if (Node->Depth == 0)
		{
			LastOctree = Node;
		}
		Pointer = Node->SingleData(Location);
	}

	if (Pointer)
	{
		return *Pointer;
	}
	else
	{
		return Chunk->IVoxWorld->WorldGeneratorInstanced->GetBlockData(Location.X, Location.Y, Location.Z);
	}
}

FIVoxel_BlockData FIVoxel_DataManager::GetSingleData(FIntVector Location)
{
	FIVoxel_BlockData* Pointer = MainDataOctree->SingleData(Location);

	if (Pointer)
	{
		return *Pointer;
	}
	else
	{
		return Chunk->IVoxWorld->WorldGeneratorInstanced->GetBlockData(Location.X, Location.Y, Location.Z);
	}
}

void FIVoxel_DataManager::GetData(FIntVector NodePos, uint8 Depth, FIVoxel_BlockData* ToFill)
{
	//Assume already locked
	MainDataOctree->GetData(NodePos, FVector(0), Depth, Chunk->IVoxWorld->WorldGeneratorInstanced, ToFill);
}
