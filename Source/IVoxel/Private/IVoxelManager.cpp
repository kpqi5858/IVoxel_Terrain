#include "IVoxelManager.h"


IVoxelManager::IVoxelManager(AIVoxelActor* World, uint8 OctreeDepth)
	: World(World), MainOctree(new FOctree(FIntVector(0), OctreeDepth, nullptr))
{
	
}

IVoxelManager::~IVoxelManager()
{
	MainOctree->Destroy();
	delete MainOctree;
}

void IVoxelManager::Tick()
{
	if (NeedUpdate)
	{
		World->RMC->CreateMeshSection(0, PolyData.Vertices, PolyData.Triangles, PolyData.Normals, PolyData.UVs, PolyData.VertexColor, PolyData.Tangent, false);
		World->RMC->GetOrCreateRuntimeMesh()->SetCollisionBoxes(PolyData.CBoxes);
		NeedUpdate = false;
	}
}

void IVoxelManager::SetOctreeValue(FVector Location, int Depth, bool Value, FColor Color)
{
	FOctree* Octree = MainOctree->GetOctree(Location, Depth);
	Octree->SetValue(Value);
	Octree->SetColor(Color);
	if (Octree->Mother)
	{
		Octree->Mother->OptimizeOrMakeLod();
	}
}


void IVoxelManager::PolygonizeOctree(FVector OctreeLocation, uint8 RenderSize, uint8 RenderDepth, int RenderSection)
{
	if (RenderDepth > MainOctree->Depth)
	{
		UE_LOG(LogGarbage, Error, TEXT("%s : Requested RenderDepth is higher than Max Depth"), *World->GetName());
		return;
	}
	FOctree* Chunk = MainOctree->GetOctree(OctreeLocation, RenderSize);
	TSet<FOctree*> Octrees;
	Chunk->GetChildOctrees(Octrees, RenderDepth);
	IVoxelData Data;

	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FRuntimeMeshTangent> Tangent;
	TArray<FColor> VertexColor;

	TArray<FRuntimeMeshCollisionBox> CBoxes;
	for (auto& Octree : Octrees)
	{
		if (Octree->GetValue() && (Octree->Depth == RenderDepth ? true : !Octree->IsFake))
		{
			URuntimeMeshShapeGenerator::CreateBoxMesh(Octree->Size(), FVector(Octree->Position), Octree->GetColor(), Data.Vertices, Data.Triangles, Data.Normals, Data.UVs, Data.Tangent, Data.VertexColor);
			FRuntimeMeshCollisionBox box;
			box.Center = FVector(Octree->Position);
			box.Extents = FVector(Octree->Size()*2);
			box.Rotation = FRotator(0);
			Data.CBoxes.Add(box);
		}
	}
	//World->RMC->CreateMeshSection(RenderSection, Data.Vertices, Data.Triangles, Data.Normals, Data.UVs, Data.VertexColor, Data.Tangent, false);
	//World->RMC->GetOrCreateRuntimeMesh()->SetCollisionBoxes(Data.CBoxes);
	//World->RMC->SetConvexCollisionSection(1, Vertices);
	PolyData = Data;
	NeedUpdate = true;
}
