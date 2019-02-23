#include "IVoxel_DataManager.h"

FIVoxel_DataManager::FIVoxel_DataManager(AIVoxel_Chunk* Chunk)
	: Chunk(Chunk), MainDataOctree(new FOctree(FIntVector(0), Chunk->Manager->OctreeDepthInit, nullptr))
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

FIVoxel_BlockData FIVoxel_DataManager::GetSingleData(FIntVector Location)
{
	FRWScopeLock(DataLock, FRWScopeLockType::SLT_ReadOnly);

	FIVoxel_BlockData* Pointer = MainDataOctree->SingleData(Location);
	if (Pointer)
	{
		return *Pointer;
	}
	else
	{
		FIVoxel_BlockData Block = Chunk->IVoxWorld->WorldGeneratorInstanced->GetBlockData(Location.X, Location.Y, Location.Z);
		return Block;
	}
}

void FIVoxel_DataManager::GetData(FIntVector NodePos, uint8 Depth, FIVoxel_BlockData* ToFill)
{
	//Assume already locked
	MainDataOctree->GetData(NodePos, FVector(0), Depth, Chunk->IVoxWorld->WorldGeneratorInstanced, ToFill);
}
