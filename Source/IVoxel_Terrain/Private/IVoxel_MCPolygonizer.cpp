#include "IVoxel_MCPolygonizer.h"

IVoxel_MCPolygonizer::IVoxel_MCPolygonizer(AIVoxel_Chunk* Chunk, FIntVector NodePos, uint8 NodeDepth)
	: ChunkData(Chunk), NodePos(NodePos), Depth(NodeDepth)
{
	VoxelMaterialMax = ChunkData->IVoxWorld->VoxelMaterials.Num();
}

bool IVoxel_MCPolygonizer::Polygonize(IVoxel_PolygonizedData& Result)
{
	check(ChunkData);

	float isolevel = 0;
	float VoxelSize = ChunkData->IVoxWorld->GetVoxelSize();

	FVector RealNodePos = FVector(NodePos);

	ChunkData->DataOctree->Begin();

	ChunkData->DataOctree->MainDataOctree->GetData(NodePos, ChunkData->GetDataLocation(), Depth, ChunkData->IVoxWorld->WorldGeneratorInstanced, CachedData);

	ChunkData->DataOctree->End();

	//Cache vertex index seperate with material
	VertexIndexCache.Init(VertexCacheSectionData(), VoxelMaterialMax);
	Result.PolygonizedSections.Init(IVoxel_PolygonizedSubData(), VoxelMaterialMax);

	FIVoxel_BlockData ThisData[8];

	for (int x = 0; x < IVOX_CHUNKDATASIZE; x++)
	for (int y = 0; y < IVOX_CHUNKDATASIZE; y++)
	for (int z = 0; z < IVOX_CHUNKDATASIZE; z++)
	{
		FIntVector Pos = FIntVector(x,y,z);

		for (int i = 0; i < 8; i++) //Get values to determine casecodes, weights.
		{
			FIntVector corn = Transvoxel::Corner[i] + Pos;
			//FMemory::Memcpy(&ThisData[i], &CachedData[AIVoxel_Chunk::IndexFor(corn)], sizeof(FIVoxel_BlockData)); //Is this faster?
			ThisData[i] = GetBlockData(corn);
		}

		uint8 Casecode = 0, t = 1;
		for (int i = 0; i < 8; i++) //Calculate casecodes
		{
			if (ThisData[i].Value < isolevel) Casecode |= t;
			t = t << 1;
		}

		if (Casecode != 0 && Casecode != 255)
		{
			auto CellClass = Transvoxel::regularCellClass[Casecode];
			auto Vertexdata = Transvoxel::regularVertexData[Casecode];
			auto Celldata = Transvoxel::regularCellData[CellClass];
			int VertexIndex = 0;

			TArray<int> VertexIndices;
			VertexIndices.SetNumUninitialized(Celldata.GetVertexCount());

			int CurrentSectionIndex = SafeSectionIndex(ThisData[0].BlockType);
			auto& CurrentSection = Result.PolygonizedSections[CurrentSectionIndex];

			for (int i = 0; i < Celldata.GetVertexCount(); i++)
			{
				auto Edgecode = Vertexdata[i];

				auto CacheFlag = Edgecode >> 12;

				auto EdgeIndex = ((Edgecode >> 8) & 0x0F)-1;

				
				if (GetCachedVertex(Pos, CacheFlag, ThisData[0].BlockType, EdgeIndex, VertexIndex))
				{
					VertexIndices[i] = VertexIndex;
					continue;
				}

				auto e1 = (Edgecode >> 4) & 0x0F;
				auto e2 = Edgecode & 0x0F;

				FVector v1 = FVector(Pos + Transvoxel::Corner[e1]);
				FVector v2 = FVector(Pos + Transvoxel::Corner[e2]);
				
				float Val1 = ThisData[e1].Value;
				float Val2 = ThisData[e2].Value;

				/*if (Depth)
				{
					FindBestVertexInLODChain(Depth, v1, v2, Val1, Val2);
				}*/

				FColor Color = Val1 < 0 ? ThisData[e1].Color : ThisData[e2].Color;

				VertexIndex = CurrentSection.Vertex.Num();

				FVector Intersect = VertexInterpolate(v1, v2, Val1, Val2);

				CurrentSection.Normal.Add(CalculateGradient(Intersect));

				FVector2D UVs = FVector2D(RealNodePos.X + Intersect.X, RealNodePos.Y + Intersect.Y);

				CurrentSection.Vertex.Add(Intersect * VoxelSize);
				CurrentSection.UV.Add(UVs);
				VertexIndices[i] = VertexIndex;
				CurrentSection.Color.Add(Color);

				if (CacheFlag & 0x08)
					CacheVertex(Pos, ThisData[0].BlockType, EdgeIndex, VertexIndex);
			}

			for (int i = 0; i < Celldata.GetTriangleCount() * 3; i+=3)
			{
				int VI1 = VertexIndices[Celldata.vertexIndex[i]];
				int VI2 = VertexIndices[Celldata.vertexIndex[i+1]];
				int VI3 = VertexIndices[Celldata.vertexIndex[i+2]];
				/*
				FVector Normal = CalculateNormal(CurrentSection.Vertex[VI1], CurrentSection.Vertex[VI2], CurrentSection.Vertex[VI3]);
				FVector2D UV(Normal.X / 2 + 0.5, Normal.Y / 2 + 0.5);
				
				CurrentSection.Normal[VI1] += Normal;
				CurrentSection.Normal[VI2] += Normal;
				CurrentSection.Normal[VI3] += Normal;

				CurrentSection.UV[VI1] = UV;
				CurrentSection.UV[VI2] = UV;
				CurrentSection.UV[VI3] = UV;
				*/
				CurrentSection.Triangle.Add(VI1);
				CurrentSection.Triangle.Add(VI2);
				CurrentSection.Triangle.Add(VI3);
			}
		}
	}


	return true;
}

inline bool IVoxel_MCPolygonizer::GetCachedVertex(FIntVector Pos, uint16 CacheFlag, uint16 BlockType, int EdgeIndex, int& Out)
{
	short ValidityMask = (Pos.X != 0) + 2 * (Pos.Y != 0) + 4 * (Pos.Z != 0);
	if ((ValidityMask & CacheFlag) == CacheFlag)
	{
		FIntVector Offset((CacheFlag & 0x01) != 0
						, (CacheFlag & 0x02) != 0
						, (CacheFlag & 0x04) != 0);

		Out = VertexIndexCache[SafeSectionIndex(BlockType)]
			.Data[AIVoxel_Chunk::IndexFor(Pos-Offset)].Index[EdgeIndex];
		if (Out < 0) return false;
		return true;
	}
	return false;
}

inline void IVoxel_MCPolygonizer::CacheVertex(FIntVector Pos, uint16 BlockType, int EdgeIndex, int Index)
{
	VertexIndexCache[SafeSectionIndex(BlockType)]
		.Data[AIVoxel_Chunk::IndexFor(Pos)].Index[EdgeIndex] = Index;
}

inline int IVoxel_MCPolygonizer::SafeSectionIndex(int Original)
{
	return FMath::Clamp(Original, 0, VoxelMaterialMax-1);
}

inline FVector IVoxel_MCPolygonizer::VertexInterpolate(FVector P1, FVector P2, float D1, float D2)
{
	/*
	FVector r;
	if (FMath::Abs(D1) < 0.00001) return P1;
	if (FMath::Abs(D2) < 0.00001) return P2;
	if (FMath::Abs(D1 - D2) < 0.00001) return P1;
	auto mu = -D1 / (D2 - D1);
	return P1 + (P2 - P1) * mu;*/
	return FMath::Lerp(P1, P2, (-D1) / (D2 - D1));
}

inline FIVoxel_BlockData IVoxel_MCPolygonizer::GetBlockData(FIntVector Pos)
{
	if (Pos.GetMax() < IVOX_CHUNKDATASIZE && Pos.GetMin() >= 0)
	{
		return CachedData[AIVoxel_Chunk::IndexFor(Pos)];
	}
	else
	{
		return GetBlockData_Ex(Pos);
	}
}

inline FIVoxel_BlockData IVoxel_MCPolygonizer::GetBlockData_Ex(FIntVector Pos)
{
	FIVoxel_BlockData Out;
	ChunkData->DataOctree->MainDataOctree->SingleData(FOctree::GetMinimalPosition(NodePos, Depth) + (Pos * FOctree::StepEachBlock(Depth)), ChunkData->IVoxWorld->WorldGeneratorInstanced, Out);
	return Out;
}

inline void IVoxel_MCPolygonizer::FindBestVertexInLODChain(int Level, FVector& P0, FVector& P1, float& V0, float& V1)
{
	for (int Lev = Level; Lev > 0; Lev--)
	{
		const auto MidPoint = (P0 + P1) / 2;

		const auto MidValue = GetBlockData_Ex(FIntVector(MidPoint)).Value;
		const auto P0Value = GetBlockData_Ex(FIntVector(P0)).Value;
		const auto P1Value = GetBlockData_Ex(FIntVector(P1)).Value;

		if ((P0Value * MidValue) <= 0)
		{
			P1 = MidPoint;
		}
		else
		{
			P0 = MidPoint;
		}
		V0 = P0Value;
		V1 = P1Value;
		check(P0.X == P1.X || P0.Y == P1.Y || P0.Z == P1.Z);
	}
}

inline FVector IVoxel_MCPolygonizer::CalculateNormal(FVector P1, FVector P2, FVector P3)
{
	float A = P1.Y * (P2.Z - P3.Z) + P2.Y * (P3.Z - P1.Z) + P3.Y * (P1.Z - P2.Z);
	float B = P1.Z * (P2.X - P3.X) + P2.Z * (P3.X - P1.X) + P3.Z * (P1.X - P2.X);
	float C = P1.X * (P2.Y - P3.Y) + P2.X * (P3.Y - P1.Y) + P3.X * (P1.Y - P2.Y);

	FVector R(A, B, C);
	R.Normalize();
	return -R;
}

inline FVector IVoxel_MCPolygonizer::CalculateGradient(FVector Point)
{
	FIntVector IV = FIntVector(Point);

	const FIntVector UX = FIntVector(1, 0, 0);
	const FIntVector UY = FIntVector(0, 1, 0);
	const FIntVector UZ = FIntVector(0, 0, 1);

	FVector Normal = FVector(GetBlockData_Ex(IV + UX).Value - GetBlockData_Ex(IV - UX).Value
							 , GetBlockData_Ex(IV + UY).Value - GetBlockData_Ex(IV - UY).Value
							 , GetBlockData_Ex(IV + UZ).Value - GetBlockData_Ex(IV - UZ).Value);
	return Normal.GetSafeNormal();
}