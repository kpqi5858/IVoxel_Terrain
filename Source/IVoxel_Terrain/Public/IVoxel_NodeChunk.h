#pragma once
#include "IVoxel_TerrainWorld.h"
#include "IVoxel_NodeChunk.generated.h"

class AIVoxel_Chunk;
struct IVoxel_PolygonizedData;

UCLASS()
class UIVoxelNodeChunk : public URuntimeMeshComponent
{
	GENERATED_BODY()
public:
	AIVoxel_Chunk* Chunk;
	IVoxel_PolygonizedData PolygonizedData;
	bool IsPolygonizeDone = false;
	bool HasMesh = false;

	bool QueuedDelete = false;

	FIntVector NodePos;
	uint8 NodeDepth;

	//When meshing is done, those chunk's OldChunkCount will decreased.
	TArray<UIVoxelNodeChunk*> OldChunks; 

	//If == 0, Clear all data and pushed to free chunks.
	int OldChunkCount; 

	void ChunkTick();

	void Setup(AIVoxel_Chunk* ParentChunk);
	
	void Load(FIntVector Pos, uint8 Depth);
	void Unload();

	inline void DecreaseOCReset();

	void IncreaseOC();
	void DecreaseOC();
};