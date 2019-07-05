#pragma once

#include "VoxelChunk.h"

class IVOXEL_TERRAIN_API FVoxelPolygonizer
{
private:
	UVoxelChunk* Chunk = nullptr;
	TSharedPtr<FVoxelPolygonizedData> PolygonizedData;

	bool IsFinished = false;

	float VoxelSize;

public:
	FVoxelPolygonizer(UVoxelChunk* ChunkToRender);

	void DoPolygonize();

	bool IsDone();
	//Also sets IsFinished flag to false
	TSharedPtr<FVoxelPolygonizedData> PopPolygonizedData();

private:
	inline void CreateFace(int X, int Y, int Z, int Section, EBlockFace Face);

	//Updates cache if necessary
	inline FFaceVisiblityCache GetFaceVisiblity(FBlockPos& Pos);
};