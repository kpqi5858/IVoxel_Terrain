#pragma once

#include "VoxelChunk.h"
#include "VoxelPolygonizer.h"
#include "ProceduralMeshComponent.h"
#include "VoxelChunkRender.generated.h"

class UVoxelChunk;

UCLASS(BlueprintType)
class IVOXEL_TERRAIN_API AVoxelChunkRender : public AActor
{
	GENERATED_BODY()
private:
	UPROPERTY()
	UVoxelChunk* TheChunk;

	AVoxelWorld* VoxelWorld;

	bool Initialized = false;

	UProceduralMeshComponent* CustomMesh;

	FVoxelPolygonizer* Polygonizer = nullptr;

	FThreadSafeBool IsPolygonizing = false;

	FVoxelPolygonizedData* PolygonizedData = nullptr;

public:
	AVoxelChunkRender();
	
	void Initialize(UVoxelChunk* Chunk);
	void DestroyRender();

	bool IsInitialized();
	bool IsPolygonizingNow();

	void RenderTick();
	void Polygonize();

	void RenderRequest();

	void ApplyPolygonizedData(FVoxelPolygonizedData* Data);

	UVoxelChunk* GetVoxelChunk();
};