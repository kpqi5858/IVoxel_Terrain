#include "VoxelPolygonizer.h"
#include "VoxelWorld.h"

FVoxelPolygonizer::FVoxelPolygonizer(UVoxelChunk* ChunkToRender)
{
	Chunk = ChunkToRender;
	VoxelSize = Chunk->GetVoxelWorld()->GetVoxelSize();
}

void FVoxelPolygonizer::DoPolygonize()
{
	check(!IsFinished);
	IsFinished = false;

	if (PolygonizedData)
		delete PolygonizedData;

	PolygonizedData = new FVoxelPolygonizedData();

	TArray<UMaterialInterface*> Materials;

	Chunk->BlockStateStorageLock();

	const EBlockFace AllFaces[6] = { EBlockFace::FRONT, EBlockFace::BACK, EBlockFace::LEFT, EBlockFace::RIGHT, EBlockFace::TOP, EBlockFace::BOTTOM };

	for (int X = 0; X < VOX_CHUNKSIZE; X++)
	for (int Y = 0; Y < VOX_CHUNKSIZE; Y++)
	for (int Z = 0; Z < VOX_CHUNKSIZE; Z++)
	{
		const FIntVector LocalPos = FIntVector(X, Y, Z);
		const auto ThisBlock = Chunk->GetBlockState(FBlockPos(Chunk, LocalPos));
		const auto BlockDef = ThisBlock->GetBlockDef();

		int SectionNum = Materials.IndexOfByKey(BlockDef->GetMaterial());
		if (SectionNum == INDEX_NONE)
		{
			PolygonizedData->Sections.AddDefaulted();
			Materials.Add(BlockDef->GetMaterial());
			SectionNum = Materials.Num() - 1;
		}
		check(PolygonizedData->Sections.IsValidIndex(SectionNum));

		for (EBlockFace Face : AllFaces)
		{

		}
	}

	Chunk->BlockStateStorageUnlock();
	IsFinished = true;
}

bool FVoxelPolygonizer::IsDone()
{
	return IsFinished;
}

FVoxelPolygonizedData* FVoxelPolygonizer::GetPolygonizedData()
{
	check(IsFinished);
	return PolygonizedData;
}

inline void FVoxelPolygonizer::CreateFace(int X, int Y, int Z, int Section, EBlockFace Face)
{

}
