#pragma once

#include "IVoxel_TerrainManager.h"

class FOctree;
class AIVoxel_Chunk;
struct FIVoxel_BlockData;

class FIVoxel_DataManager
{
private:
	TSharedRef<FOctree> MainDataOctree;

	TSet<FOctree*> EditedOctree; //Will call MakeLOD function

	FCriticalSection EditedOctreeLock;

	FRWLock DataLock;

public:
	FIVoxel_DataManager(AIVoxel_Chunk* Chunk);

	AIVoxel_Chunk* Chunk;


	//If edited an octree node, please call this function
	void EditedNode(FOctree* Node);

	void Begin(FRWScopeLockType Type); //Locks access for MainDataOctree
	void End(FRWScopeLockType Type);   //Unlocks access for MainDataOctree

	inline void MakeLOD();

	FIVoxel_BlockData& RefSingleData(FIntVector Location);
	FIVoxel_BlockData GetSingleData(FIntVector Location);

	void GetData(FIntVector NodePos, uint8 Depth, FIVoxel_BlockData* ToFill);
};