#pragma once

#include "IVoxel_Polygonizer.h"
#include "RuntimeMeshShapeGenerator.h"
#include "IVoxel_Chunk.h"

struct FIVoxel_BlockData;
class AIVoxel_Chunk;
struct IVoxel_PolygonizedData;

enum EFaceDirection : uint8
{
	D_LEFT, D_RIGHT, D_TOP, D_BOTTOM, D_FRONT, D_BACK
};

struct FCubeFace
{
	FIntVector Extent; //If this is FIntVector(0), it is invalid
	EFaceDirection Direction;
	FIVoxel_BlockData BlockData;
	bool Flipped;

	FCubeFace(FIntVector Extent, EFaceDirection Direction, FIVoxel_BlockData BlockData, bool bFlipped)
		: Extent(Extent), Direction(Direction), BlockData(BlockData), Flipped(bFlipped)
	{
	}

	bool IsValidFace()
	{
		return !Extent.IsZero();
	}
};

class IVOXEL_TERRAIN_API IVoxel_CubePolygonizer : public IVoxel_Polygonizer
{
private:
	int VoxelMaterialMax; //Cached

	FIntVector NodePos;
	uint8 Depth;

	AIVoxel_Chunk* Chunk;

	FIVoxel_BlockData* CachedData;
	FIVoxel_BlockData* ExtendedCachedData; //For gradient normals, etc

public:
	IVoxel_CubePolygonizer(AIVoxel_Chunk* Chunk, FIntVector NodePos, uint8 NodeDepth);

	virtual ~IVoxel_CubePolygonizer() {}

	bool Polygonize(IVoxel_PolygonizedData& Result) override;

	inline void Face(IVoxel_PolygonizedSubData& Section, int x, int y, int z, FCubeFace Face);

	inline bool IsFaceSolid(FIntVector Pos);

	inline bool IsFaceSolid(int x, int y, int z)
	{
		return IsFaceSolid(FIntVector(x, y, z));
	};

	inline bool IsFaceSolid(int x, int y, int z, EFaceDirection Dir)
	{
		int XOffset = 0, YOffset = 0, ZOffset = 0;

		switch (Dir)
		{
		case D_TOP:
			ZOffset = 1;
			break;
		case D_BOTTOM:
			ZOffset = -1;
			break;
		case D_LEFT:
			XOffset = -1;
			break;
		case D_RIGHT:
			XOffset = 1;
			break;
		case D_FRONT:
			YOffset = 1;
			break;
		case D_BACK:
			YOffset = -1;
			break;
		}
		return IsFaceSolid(x + XOffset, y + YOffset, z + ZOffset);
	}
};