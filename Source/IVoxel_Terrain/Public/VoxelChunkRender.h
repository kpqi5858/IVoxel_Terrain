#pragma once

#include "VoxelChunk.h"

#include "VoxelChunkRender.generated.h"

class UVoxelChunk;

UCLASS(BlueprintType)
class IVOXEL_TERRAIN_API AVoxelChunkRender : public AActor
{
	GENERATED_BODY()
private:
	UVoxelChunk* TheChunk;

	bool Initialized = false;

public:
	AVoxelChunkRender();
	
	void Initialize(UVoxelChunk* Chunk);
	void DestroyRender();

	bool IsInitialized();

	void RenderTick();

	UVoxelChunk* GetVoxelChunk();
};