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
	UVoxelChunk* TheChunk;

	AVoxelWorld* VoxelWorld;

	bool Initialized = false;

	UProceduralMeshComponent* CustomMesh;

	TSharedPtr<FVoxelPolygonizer> Polygonizer;

	FThreadSafeBool IsPolygonizing = false;

	TSharedPtr<FVoxelPolygonizedData> PolygonizedData;

public:
	AVoxelChunkRender();
	~AVoxelChunkRender();

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