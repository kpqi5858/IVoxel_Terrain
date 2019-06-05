#include "VoxelPolygonizer.h"
#include "VoxelWorld.h"

FVoxelPolygonizer::FVoxelPolygonizer(UVoxelChunk* ChunkToRender)
{
	Chunk = ChunkToRender;
	VoxelSize = Chunk->GetVoxelWorld()->GetVoxelSize();
}

void FVoxelPolygonizer::DoPolygonize()
{
	ensureAlways(!IsFinished);
	IsFinished = false;

	PolygonizedData = new FVoxelPolygonizedData();

	TArray<UMaterialInterface*> Materials;

	Chunk->BlockStateStorageLock();

	const EBlockFace AllFaces[6] = { EBlockFace::FRONT, EBlockFace::BACK, EBlockFace::LEFT, EBlockFace::RIGHT, EBlockFace::TOP, EBlockFace::BOTTOM };

	for (int X = 0; X < VOX_CHUNKSIZE; X++)
	for (int Y = 0; Y < VOX_CHUNKSIZE; Y++)
	for (int Z = 0; Z < VOX_CHUNKSIZE; Z++)
	{
		const FIntVector LocalPos = FIntVector(X, Y, Z);
		FBlockPos BlockPos = FBlockPos(Chunk, LocalPos);
		const auto ThisBlock = Chunk->GetBlockState(BlockPos);
		const auto BlockDef = ThisBlock->GetBlockDef();

		int SectionNum = Materials.IndexOfByKey(BlockDef->GetMaterial());
		if (SectionNum == INDEX_NONE)
		{
			PolygonizedData->Sections.AddDefaulted();
			Materials.Add(BlockDef->GetMaterial());
			SectionNum = Materials.Num() - 1;
			PolygonizedData->Sections[SectionNum].Material = BlockDef->GetMaterial();
		}
		check(PolygonizedData->Sections.IsValidIndex(SectionNum));

		auto& ThisVisiblity = Chunk->GetFaceVisiblityCache(BlockPos);

		for (EBlockFace Face : AllFaces)
		{
			if (ThisVisiblity.IsThisFaceVisible(Face))
			{
				CreateFace(X, Y, Z, SectionNum, Face);
			}
		}
	}

	Chunk->BlockStateStorageUnlock();
	IsFinished = true;
}

bool FVoxelPolygonizer::IsDone()
{
	return IsFinished;
}

FVoxelPolygonizedData* FVoxelPolygonizer::PopPolygonizedData()
{
	check(IsFinished);
	IsFinished = false;
	return PolygonizedData;
}

inline bool FVoxelPolygonizer::IsThisFaceVisible(FBlockPos Pos, EBlockFace Face)
{
	return Pos.GetChunk()->GetFaceVisiblityCache(Pos).IsThisFaceVisible(Face);
}

inline void FVoxelPolygonizer::CreateFace(int X, int Y, int Z, int Section, EBlockFace Face)
{
	FVoxelPolygonizedSection& ThisSection = PolygonizedData->Sections[Section];

	const int TriIndex = ThisSection.Vertex.Num();

	const float VoxSize = VoxelSize;

	FVector BoxVerts[8];
	BoxVerts[0] = FVector(0, VoxSize, VoxSize);
	BoxVerts[1] = FVector(VoxSize, VoxSize, VoxSize);
	BoxVerts[2] = FVector(VoxSize, 0, VoxSize);
	BoxVerts[3] = FVector(0, 0, VoxSize);

	BoxVerts[4] = FVector(0, VoxSize, 0);
	BoxVerts[5] = FVector(VoxSize, VoxSize, 0);
	BoxVerts[6] = FVector(VoxSize, 0, 0);
	BoxVerts[7] = FVector(0, 0, 0);

	auto VertexBuilder = [&](FVector V, FVector N, FVector2D UV)
	{
		ThisSection.Vertex.Add(V + FVector(X*VoxSize, Y*VoxSize, Z*VoxSize));
		ThisSection.Normal.Add(N);
		ThisSection.UV.Add(UV);
	};
	
	FVector Normal;

	switch (Face)
	{
	//Pos Z
	case EBlockFace::TOP :
	{
		Normal = FVector(0.0f, 0.0f, 1.0f);
		VertexBuilder(BoxVerts[0], Normal, FVector2D(0.0f, 0.0f));
		VertexBuilder(BoxVerts[1], Normal, FVector2D(0.0f, 1.0f));
		VertexBuilder(BoxVerts[2], Normal, FVector2D(1.0f, 1.0f));
		VertexBuilder(BoxVerts[3], Normal, FVector2D(1.0f, 0.0f));
		break;
	}
	//Neg Z
	case EBlockFace::BOTTOM :
	{
		Normal = FVector(0.0f, 0.0f, -1.0f);
		VertexBuilder(BoxVerts[7], Normal, FVector2D(0.0f, 0.0f));
		VertexBuilder(BoxVerts[6], Normal, FVector2D(0.0f, 1.0f));
		VertexBuilder(BoxVerts[5], Normal, FVector2D(1.0f, 1.0f));
		VertexBuilder(BoxVerts[4], Normal, FVector2D(1.0f, 0.0f));
		break;
	}
	//Neg Y
	case EBlockFace::LEFT:
	{
		Normal = FVector(0.0f, -1.0f, 0.0f);
		VertexBuilder(BoxVerts[7], Normal, FVector2D(0.0f, 0.0f));
		VertexBuilder(BoxVerts[3], Normal, FVector2D(0.0f, 1.0f));
		VertexBuilder(BoxVerts[2], Normal, FVector2D(1.0f, 1.0f));
		VertexBuilder(BoxVerts[6], Normal, FVector2D(1.0f, 0.0f));
		break;
	}
	//Pos Y
	case EBlockFace::RIGHT:
	{
		Normal = FVector(0.0f, 1.0f, 0.0f);
		VertexBuilder(BoxVerts[5], Normal, FVector2D(0.0f, 0.0f));
		VertexBuilder(BoxVerts[1], Normal, FVector2D(0.0f, 1.0f));
		VertexBuilder(BoxVerts[0], Normal, FVector2D(1.0f, 1.0f));
		VertexBuilder(BoxVerts[4], Normal, FVector2D(1.0f, 0.0f));
		break;
	}
	//Pos X
	case EBlockFace::FRONT:
	{
		Normal = FVector(1.0f, 0.0f, 0.0f);
		VertexBuilder(BoxVerts[6], Normal, FVector2D(0.0f, 0.0f));
		VertexBuilder(BoxVerts[2], Normal, FVector2D(0.0f, 1.0f));
		VertexBuilder(BoxVerts[1], Normal, FVector2D(1.0f, 1.0f));
		VertexBuilder(BoxVerts[5], Normal, FVector2D(1.0f, 0.0f));
		break;
	}
	//Neg X
	case EBlockFace::BACK:
	{
		Normal = FVector(-1.0f, 0.0f, 0.0f);
		VertexBuilder(BoxVerts[4], Normal, FVector2D(0.0f, 0.0f));
		VertexBuilder(BoxVerts[0], Normal, FVector2D(0.0f, 1.0f));
		VertexBuilder(BoxVerts[3], Normal, FVector2D(1.0f, 1.0f));
		VertexBuilder(BoxVerts[7], Normal, FVector2D(1.0f, 0.0f));
		break;
	}
	default:
		check(false);
	}

	ThisSection.Triangle.Add(TriIndex);
	ThisSection.Triangle.Add(TriIndex+1);
	ThisSection.Triangle.Add(TriIndex+3);
	ThisSection.Triangle.Add(TriIndex+1);
	ThisSection.Triangle.Add(TriIndex+2);
	ThisSection.Triangle.Add(TriIndex+3);
}
