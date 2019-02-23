#include "IVoxel_CubePolygonizer.h"

IVoxel_CubePolygonizer::IVoxel_CubePolygonizer(AIVoxel_Chunk* Chunk, FIntVector NodePos, uint8 NodeDepth)
	: Chunk(Chunk), NodePos(NodePos), Depth(NodeDepth)
{
	VoxelMaterialMax = Chunk->IVoxWorld->VoxelMaterials.Num();
}

bool IVoxel_CubePolygonizer::Polygonize(IVoxel_PolygonizedData& Result)
{
	CachedData = new FIVoxel_BlockData[IVOX_CHUMKDATAARRAYSIZE];

	float VoxelSize = Chunk->IVoxWorld->GetVoxelSize() * ((float)IVOX_CHUNKDATASIZE / (IVOX_CHUNKDATASIZE - 1))
		* (FOctree::SizeFor(Depth) / 2);

	Chunk->DataOctree->Begin(FRWScopeLockType::SLT_ReadOnly);

	Chunk->DataOctree->GetData(NodePos, Depth, CachedData);

	Chunk->DataOctree->End(FRWScopeLockType::SLT_ReadOnly);


	Result.PolygonizedSections.Init(IVoxel_PolygonizedSubData(), VoxelMaterialMax);


	//Greedy meshing start

	int Mask[IVOX_CHUNKDATASIZE*IVOX_CHUNKDATASIZE];

	int XYZ[3] = {0, 0, 0};
	int Q[3] = {0, 0, 0};

	int U, V, n;
	EFaceDirection Face;

	for (int BackFace = 0; BackFace < 2; BackFace++)
	{
		for (int D = 0; D < 3; D++)
		{
			U = (D + 1) % 3;
			V = (D + 2) % 3;

			XYZ[0] = 0; XYZ[1] = 0; XYZ[2] = 0;
			Q[0] = 0; Q[1] = 0; Q[2] = 0; Q[D] = 1;

			if (D == 0) Face = BackFace ? D_RIGHT : D_LEFT;
			if (D == 1) Face = BackFace ? D_FRONT : D_BACK;
			if (D == 2) Face = BackFace ? D_TOP : D_BOTTOM;

			for (XYZ[D] = 0; XYZ[D] < IVOX_CHUNKDATASIZE; XYZ[D]++)
			{
				n = 0;
				for (XYZ[V] = 0; XYZ[V] < IVOX_CHUNKDATASIZE; XYZ[V]++)
				{
					for (XYZ[U] = 0; XYZ[U] < IVOX_CHUNKDATASIZE; XYZ[U]++)
					{
						Mask[n++] = IsFaceSolid(XYZ[0], XYZ[1], XYZ[2], Face) ? -1 : CachedData[AIVoxel_Chunk::IndexFor(XYZ[0], XYZ[1], XYZ[2])].BlockType;
					}
				}
				n = 0;

				for (int j = 0; j < IVOX_CHUNKDATASIZE; j++)
				{
					for (int i = 0; i < IVOX_CHUNKDATASIZE; i++)
					{
						if (Mask[n] >= 0)
						{
							int w = 0, h = 0, k = 0;
							//Find the width of this mask section, w == current width
							for (w = 1; w + i < IVOX_CHUNKDATASIZE && Mask[n + w] >= 0 && Mask[n + w] == Mask[n]; w++) {};

							bool Done = false;
							for (h = 1; j + h < IVOX_CHUNKDATASIZE; h++)
							{
								for (k = 0; k < w; k++)
								{
									if (Mask[n + k + h * IVOX_CHUNKDATASIZE] < 0 || Mask[n + k + h * IVOX_CHUNKDATASIZE] != Mask[n])
									{
										Done = true;
										break;
									}
								}
								if (Done) break;
							}

							XYZ[U] = i;
							XYZ[V] = j;

							int r, s, t;
							int DU[3], DV[3];

							DU[0] = 0; DU[1] = 0; DU[2] = 0; DU[U] = w;
							DV[0] = 0; DV[1] = 0; DV[2] = 0; DV[V] = h;

							if (BackFace)
							{
								r = XYZ[0] + Q[0];
								s = XYZ[1] + Q[1];
								t = XYZ[2] + Q[2];
							}
							else
							{
								r = XYZ[0];
								s = XYZ[1];
								t = XYZ[2];
							}
							FVector P1, P2, P3, P4;
							P1 = FVector(r, s, t) * VoxelSize;
							P2 = FVector(r + DU[0], s + DU[1], t + DU[2]) * VoxelSize;
							P3 = FVector(r + DV[0], s + DV[1], t + DV[2]) * VoxelSize;
							P4 = FVector(r + DU[0] + DV[0], s + DU[1] + DV[1], t + DU[2] + DV[2]) * VoxelSize;

							auto& Section = Result.PolygonizedSections[Mask[n]];
							
							int Num = Section.Vertex.Num();

							Section.Vertex.Add(P1);
							Section.Vertex.Add(P2);
							Section.Vertex.Add(P3);
							Section.Vertex.Add(P4);

							URuntimeMeshShapeGenerator::ConvertQuadToTriangles(Section.Triangle, Num, Num + 1, Num + 2, Num + 3);
							/*
							switch (Face)
							{
							case D_TOP:
								URuntimeMeshShapeGenerator::ConvertQuadToTriangles(Section.Triangle, Num + 1, Num + 3, Num, Num + 2);
								break;
							case D_BOTTOM:
								URuntimeMeshShapeGenerator::ConvertQuadToTriangles(Section.Triangle, Num + 3, Num + 1, Num + 2, Num);
								break;
							case D_LEFT:
								break;
							case D_RIGHT:
								break;
							case D_FRONT:
								break;
							case D_BACK:
								break;
							}*/
							//Clear the mask
							for (int l = 0; l < h; l++)
							{
								for (k = 0; k < w; k++)
								{
									Mask[n + k + l * IVOX_CHUNKDATASIZE] = -1;
								}
							}

							n += w;
							i += w;
						}
						else
						{
							n++;
							i++;
						}


					}
				}
			}
		}
	}

	delete[] CachedData;

	return true;
}
inline void IVoxel_CubePolygonizer::Face(IVoxel_PolygonizedSubData& Section, int x, int y, int z, FCubeFace Face)
{
	FVector FaceVertex[8] = {FVector(0, 1, 1), FVector(1, 1, 1), FVector(1, 0 ,1), FVector(0, 0, 1),
							 FVector(0, 1, 0), FVector(1, 1, 0), FVector(1, 0, 0), FVector(0, 0, 0)};
}
inline bool IVoxel_CubePolygonizer::IsFaceSolid(FIntVector Pos)
{
	if (Pos.X < 0 || Pos.X >= IVOX_CHUNKDATASIZE
		|| Pos.Y < 0 || Pos.Y >= IVOX_CHUNKDATASIZE
		|| Pos.Z < 0 || Pos.Z >= IVOX_CHUNKDATASIZE) return true;
	return CachedData[AIVoxel_Chunk::IndexFor(Pos)].Value < 0;
}
