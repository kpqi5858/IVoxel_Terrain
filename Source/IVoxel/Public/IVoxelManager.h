#pragma once

#include "CoreMinimal.h"
#include "Octree.h"
#include "RuntimeMeshComponent.h"
#include "RuntimeMeshShapeGenerator.h"
#include "IVoxelActor.h"
#include "RuntimeMeshBuilder.h"

class AIVoxelActor;
class FOctree;

struct IVoxelData
{
	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FRuntimeMeshTangent> Tangent;
	TArray<FColor> VertexColor;

	TArray<FRuntimeMeshCollisionBox> CBoxes;
};

class IVoxelManager
{
private:
	AIVoxelActor * World;
	bool NeedUpdate;
	IVoxelData PolyData;

public:
	FOctree * MainOctree;

	TSet<FOctree*> OctreeToOptimize;

	IVoxelManager(AIVoxelActor* World, uint8 OctreeDepth);
	~IVoxelManager();
	void Tick();
	void SetOctreeValue(FVector Location, int Depth, bool Value, FColor Color);
	void PolygonizeOctree(FVector OctreeLocation, uint8 RenderSize, uint8 RenderDepth, int RenderSection);
};
