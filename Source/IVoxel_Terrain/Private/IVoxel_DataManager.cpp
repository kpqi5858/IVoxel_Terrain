#include "IVoxel_DataManager.h"

FIVoxel_DataManager::FIVoxel_DataManager(AIVoxel_Chunk* Chunk)
	: Chunk(Chunk)
{
	MainDataOctree = MakeShareable(new FOctree(FIntVector(0), Chunk->Manager->OctreeDepthInit, nullptr));
}

void FIVoxel_DataManager::EditedNode(FOctree* Node)
{
	FScopeLock sl(&EditedOctreeLock);
	check(Node->Depth == 0);
	
	EditedOctree.Add(Node);
}

void FIVoxel_DataManager::Begin()
{
	MakeLOD();
}

void FIVoxel_DataManager::End()
{
	MakeLOD();
}

inline void FIVoxel_DataManager::MakeLOD()
{
	FScopeLock sl(&EditedOctreeLock);
	for (auto& Node : EditedOctree)
	{
		Node->MakeLOD(Chunk->IVoxWorld->WorldGeneratorInstanced, FVector(0), false);
	}
	EditedOctree.Reset();
}