#pragma once
#include "IVoxel_TerrainWorld.h"
#include "IVoxel_Polygonizer.h"
#include "IVoxel_NodeChunk.generated.h"

class AIVoxel_Chunk;
class IVoxel_PolygonizerThread;
struct IVoxel_PolygonizedData;

UCLASS()
class UIVoxelNodeChunk : public URuntimeMeshComponent
{
	GENERATED_BODY()
public:
	AIVoxel_Chunk* Chunk;

	IVoxel_PolygonizerThread* PolygonizerThread;

	IVoxel_PolygonizedData PolygonizedData;

	FThreadSafeBool IsPolygonizeDone = false; //Needs to be thread-safe

	bool HasMesh = false;

	bool IgnoreMesh = false;

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