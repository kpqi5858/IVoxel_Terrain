#pragma once

#include "CoreMinimal.h"
#include "IVoxel_Chunk.h"
#include "IVoxel_Terrain.h"
#include "IVoxel_Polygonizer.h"
#include "IVoxel_Chunk.h"
#include "RuntimeMeshLibrary.h"

class AIVoxel_Chunk;
struct IVoxel_PolygonizedData;
struct FIVoxel_BlockData;

struct VertexCacheData
{
	int Index[4];

	VertexCacheData()
	{
		Index[0] = -1;
		Index[1] = -1;
		Index[2] = -1;
		Index[3] = -1;
	}
};

struct VertexCacheSectionData
{
	VertexCacheData Data[IVOX_CHUMKDATAARRAYSIZE];
};

class IVOXEL_TERRAIN_API IVoxel_MCPolygonizer : public IVoxel_Polygonizer
{
private:
	TArray<VertexCacheSectionData> VertexIndexCache;
	int VoxelMaterialMax; //Cached

	FIVoxel_BlockData CachedData[IVOX_CHUMKDATAARRAYSIZE];

	FIVoxel_BlockData* ExtendedCachedData; //For gradient normals, etc

public:
	virtual ~IVoxel_MCPolygonizer() {}
	IVoxel_MCPolygonizer(AIVoxel_Chunk* Chunk, FIntVector NodePos, uint8 NodeDepth);

	FIntVector NodePos;
	uint8 Depth;

	AIVoxel_Chunk* ChunkData;

	FOctree* LastOctree = nullptr;

	bool Polygonize(IVoxel_PolygonizedData& Result) override;

	inline bool GetCachedVertex(FIntVector Pos, uint16 CacheFlag, uint16 BlockType, int EdgeIndex, int& Out);

	inline void CacheVertex(FIntVector Pos, uint16 BlockType, int EdgeIndex, int Index);

	inline int SafeSectionIndex(int Original);

private:
	inline FVector CalculateGradient(FVector Point);

	inline FIVoxel_BlockData GetBlockData(FIntVector Pos);

	inline FIVoxel_BlockData GetBlockData_Ex(FIntVector Pos);

	inline FIVoxel_BlockData GetBlockData_Ex(FVector Pos);

	inline void FindBestVertexInLODChain(int Level, FVector& P0, FVector& P1, float& V0, float& V1);

	inline FIntVector LocalVertexPosToGlobal(FVector Point);

	inline FVector GlobalToLocalVertex(FIntVector Point);

	inline FIVoxel_BlockData GetBlockData_Global(FIntVector Pos);
public:
	static inline FVector VertexInterpolate(FVector P1, FVector P2, float D1, float D2);

	static inline FVector CalculateNormal(FVector P1, FVector P2, FVector P3);
};