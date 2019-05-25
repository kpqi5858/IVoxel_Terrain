#pragma once

#include "VoxelChunk.h"
#include "VoxelPolygonizer.h"
#include "RuntimeMeshComponent.h"
#include "VoxelChunkRender.generated.h"

class UVoxelChunk;

UCLASS(BlueprintType)
class IVOXEL_TERRAIN_API AVoxelChunkRender : public AActor
{
	GENERATED_BODY()
private:
	UPROPERTY()
	UVoxelChunk* TheChunk;

	bool Initialized = false;

	URuntimeMeshComponent* RMC;

	FVoxelPolygonizer* Polygonizer = nullptr;

	FThreadSafeBool IsPolygonizing = false;

public:
	AVoxelChunkRender();
	
	void Initialize(UVoxelChunk* Chunk);
	void DestroyRender();

	bool IsInitialized();
	bool IsPolygonizingNow();

	void RenderTick();
	void Polygonize();

	void ApplyPolygonizedData(FVoxelPolygonizedData* Data);

	UVoxelChunk* GetVoxelChunk();
};