#pragma once

#include "VoxelChunk.h"

class IVOXEL_TERRAIN_API FVoxelPolygonizer
{
private:
	UVoxelChunk* Chunk;
	FVoxelPolygonizedData* PolygonizedData = nullptr;

	bool IsFinished = false;
	float VoxelSize;

public:
	FVoxelPolygonizer(UVoxelChunk* ChunkToRender);

	void DoPolygonize();

	bool IsDone();
	FVoxelPolygonizedData* GetPolygonizedData();

private:
	inline void CreateFace(int X, int Y, int Z, int Section, EBlockFace Face);
};