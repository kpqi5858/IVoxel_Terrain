#pragma once

#include "IVoxel_TerrainManager.h"

class FOctree;

class FIVoxel_DataManager
{
public:
	FIVoxel_DataManager(AIVoxel_Chunk* Chunk);

	AIVoxel_Chunk* Chunk;

	TSet<FOctree*> EditedOctree; //Will call MakeLOD function

	TSharedPtr<FOctree> MainDataOctree;

	//If edited an octree node, please call this function
	void EditedNode(FOctree* Node);

	void Begin(); //Locks access for MainDataOctree
	void End();   //Unlocks access for MainDataOctree

	inline void MakeLOD();
};