#pragma once

#include "CoreMinimal.h"
#include "IVoxel_TerrainWorld.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"

class AIVoxel_Chunk;

//TOOD: Seperate World generator each chunk
//This will make way better code
class FOctree
{
public:
	FOctree();
	FOctree(FIntVector Position, uint8 Depth, FOctree* Parent);
	//Use Destroy()
	~FOctree();

	/*
	* Childs Location
	* 01 Below 45
	* 23 Below 67
	*
	* This array is not initialized
	* So depend on HasChilds flag
	*/
	FOctree* Childs[8];

	FOctree* Parent;

	bool HasChilds;

	//Don't read world generator frequently, mark it edited and parent will calculate edited child only.
	bool IsEditedLastLODMake;

	const uint8 Depth; //Should support max 31 depth
	const FIntVector Position;
	
	FIVoxel_BlockData* Data;

	//Destroys octree itself, and also childs
	void Destroy();

	//Destroy childs only
	inline void DestroyChilds();

	//Subdivide, Make childs, If childs exist, skips
	inline void Subdivide();
	
	int Size() const;

	inline bool IsInOctree(FIntVector Location);

	bool IsInOctree_External(FIntVector Location);

	//Initialize data with world generator
	void InitDataWithWorldGen(UIVoxel_WorldGenerator* WorldGen, FVector Offset = FVector(0));

	//Initialize data with default constructor
	void InitDataDefault();

	//Allocates data array (With default constructor)
	inline void InitDataInternal();

	//Can subdivide to find most accurate octree node
	FOctree* GetOctree(FIntVector Location, uint8 MaxDepth = 0);

	//Can't subdivide
	FOctree* GetOctree_NoSub(FIntVector Location);

	//Exact Position, Actually FIntVector is identifier for octrees
	//If no exact position node is found, returns nullptr
	FOctree* GetOctreeExact(FIntVector Location);

	//Based on position
	inline FOctree* GetChildOctree(FIntVector Location);

	//Get child octrees starting from its octree, cant subdivide
	void GetChildOctrees(TSet<FOctree*> &RetValue, uint8 MaxDepth = 0);

	//Get child octrees which has no childs
	void GetChildOctrees_NoChild(TSet<FOctree*> &RetValue);

	//Get data in that octree, Array size is always IVOX_CHUMKDATAARRAYSIZE.
	void GetData(FIntVector OctreeLoc, FVector Offset, uint8 TargetDepth, UIVoxel_WorldGenerator* WorldGen, FIVoxel_BlockData* OutArray);

	void SetData(FIntVector LocalPos, FIVoxel_BlockData Block, FVector Offset, UIVoxel_WorldGenerator* WorldGen);
	
	//Caches lod based on childs
	//Recursive function to main octree
	//bBlendData - Blend the data instead of just copy with pattern, better detail, more expensive
	void MakeLOD(UIVoxel_WorldGenerator* WorldGen, FVector Offset, bool bBlendData);


	FIVoxel_BlockData* SingleData(FIntVector Pos);

	//Subdivides with LodCurve
	void LodSubdivide(AIVoxel_Chunk* Chunk);

	void GetChildOctreesIntersect(FIntVector Target, TSet<FOctree*> &RetValue);

	FIntVector GetMinimalPosition();
	FIntVector GetMaximumPosition();

	void DebugRender(UWorld* world);

	inline bool HasLODData()
	{
		return HasChilds && Data;
	}

	static inline FIntVector GetMinimalPosition(FIntVector Position, uint8 Depth)
	{
		return Position - FIntVector(SizeFor(Depth) / 2);
	}

	static inline FIntVector GetMaximumPosition(FIntVector Position, uint8 Depth)
	{
		return Position + FIntVector(SizeFor(Depth) / 2);
	}

	static inline int IndexFor(int x, int y, int z)
	{
		check(IVOX_CHUNKDATASIZE > x && x >= 0);
		check(IVOX_CHUNKDATASIZE > y && y >= 0);
		check(IVOX_CHUNKDATASIZE > z && z >= 0);

		return x + (IVOX_CHUNKDATASIZE * y) + (IVOX_CHUNKDATASIZE*IVOX_CHUNKDATASIZE * z);
	}

	static inline int IndexFor(FIntVector xyz)
	{
		int x = xyz.X, y = xyz.Y, z = xyz.Z;
		//Checks will done in IndexFor(int, int, int)
		return IndexFor(x, y, z);
	}

	static inline int SizeFor(uint8 Dep)
	{
		return IVOX_CHUNKDATASIZE << Dep;
	}

	static int StepEachBlock(uint8 Dep)
	{
		return SizeFor(Dep) / IVOX_CHUNKDATASIZE;
	}

	static inline FVector GetWorldGenPos(FIntVector ExactPos, FIntVector Global, uint8 TargetDepth, FVector Offset)
	{
		int LodStep = StepEachBlock(TargetDepth);

		FVector loc = FVector(GetMinimalPosition(ExactPos, TargetDepth) + (Global * LodStep));
		loc += Offset;
		return loc;
	}
	//Fill out given data array with world generator values, Array length is IVOX_CHUMKDATAARRAYSIZE.
	static inline void FillArrayWithWorldGenerator(FIntVector ExactPos, uint8 TargetDepth, UIVoxel_WorldGenerator* WorldGen, FIVoxel_BlockData* ToFill, FVector Offset = FVector::ZeroVector)
	{
		int LodStep = SizeFor(TargetDepth);
		for (int x = 0; x < IVOX_CHUNKDATASIZE; x++)
		for (int y = 0; y < IVOX_CHUNKDATASIZE; y++)
		for (int z = 0; z < IVOX_CHUNKDATASIZE; z++)
		{
			FIntVector Temp = FIntVector(x, y, z);
			FVector loc = GetWorldGenPos(ExactPos, Temp, TargetDepth, Offset);

			ToFill[IndexFor(x, y, z)] = WorldGen->GetBlockData(loc.X, loc.Y, loc.Z);
		}
	}
};