#pragma once

#include "CoreMinimal.h"
#include "IVoxel_Terrain.h"
#include "RuntimeMeshComponent.h"
#include "IVoxel_BlockData.h"

//Polygonized data (each sections)
struct IVoxel_PolygonizedSubData
{
	FOccluderVertexArray		Vertex;
	TArray<int>					Triangle;
	FOccluderVertexArray		Normal;
	TArray<FVector2D>			UV;
	TArray<FColor>				Color;
	TArray<FRuntimeMeshTangent> Tangent;
};

//Polygonized data, seperated with sections (different materials)
struct IVoxel_PolygonizedData
{
	TArray<IVoxel_PolygonizedSubData> PolygonizedSections;
};

class IVOXEL_TERRAIN_API IVoxel_Polygonizer
{
public:
	virtual bool Polygonize(IVoxel_PolygonizedData& OutData)
	{
		unimplemented();
		return false;
	};
};
