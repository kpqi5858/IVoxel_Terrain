#pragma once

#include "IVoxel_TerrainWorld.h"
#include "IVoxel_Polygonizer.h"
#include "IVoxel_NodeChunk.generated.h"

class AIVoxel_Chunk;
class IVoxel_PolygonizerThread;
struct IVoxel_PolygonizedData;

UCLASS()
class IVOXEL_TERRAIN_API UIVoxelNodeChunk : public URuntimeMeshComponent
{
	GENERATED_BODY()

private:
	AIVoxel_Chunk* Chunk;

	IVoxel_PolygonizerThread* PolygonizerThread = nullptr;

public:
	IVoxel_PolygonizedData* PolygonizedData = nullptr;

	FThreadSafeBool IsPolygonizeDone = false; //Needs to be thread-safe

	bool HasMesh = false;

	bool IsLoaded = false;

	float DeletionLeft = 0;

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


	void SetPolygonizerThread(IVoxel_PolygonizerThread* InThread);

	//Asserts if there's polygonizer remaining
	bool HasPolygonizerThread();

	//Try to set PolygonizedData if thread is valid to this chunk
	void SetPolygonizedData(IVoxel_PolygonizerThread* ThisThread, IVoxel_PolygonizedData* PData);
};