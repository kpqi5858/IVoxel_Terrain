#pragma once

#include "VoxelChunk.h"

class FVoxelChunk;

UCLASS(BlueprintType)
class IVOXEL_TERRAIN_API AVoxelChunkRender : AActor
{
private:
	FVoxelChunk* TheChunk;

public:
	AVoxelChunkRender();
	
	void Initialize(FVoxelChunk*);
	void DestroyRender();

	FVoxelChunk* GetVoxelChunk();
};