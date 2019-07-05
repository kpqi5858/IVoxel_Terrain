#pragma once

#include "VoxelChunk.h"
#include "VoxelPolygonizer.h"
#include "ProceduralMeshComponent.h"
#include "RuntimeMeshComponent.h"
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

	UPROPERTY()
	URuntimeMeshComponent* CustomMesh;

	TSharedPtr<FVoxelPolygonizer> Polygonizer;

	bool IsPolygonizing = false;

	TSharedPtr<FVoxelPolygonizedData> PolygonizedData;

	bool RePolygonize = false;

public:
	AVoxelChunkRender();
	~AVoxelChunkRender();

	void Initialize(UVoxelChunk* Chunk);
	void DestroyRender();

	bool IsInitialized();
	bool IsPolygonizingNow();

	void RenderTick();
	void Polygonize();

	FORCENOINLINE void RenderRequest();

	void ApplyPolygonizedData(FVoxelPolygonizedData* Data);

	UVoxelChunk* GetVoxelChunk();
};