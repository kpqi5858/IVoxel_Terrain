#include "VoxelChunkRender.h"
#include "VoxelWorld.h"

AVoxelChunkRender::AVoxelChunkRender()
{
	CustomMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("CustomMesh"));
	RootComponent = CustomMesh;
	TheChunk = nullptr;
}

void AVoxelChunkRender::Initialize(UVoxelChunk* Chunk)
{
	check(!Initialized);

	TheChunk = Chunk;
	Initialized = true;
	CustomMesh->SetVisibility(true);
	Polygonizer = new FVoxelPolygonizer(Chunk);
	SetActorLocation(FVector(Chunk->GetGlobalPosition_Min()) * Chunk->GetVoxelWorld()->GetVoxelSize());

	VoxelWorld = Chunk->GetVoxelWorld();
}

void AVoxelChunkRender::DestroyRender()
{
	check(Initialized);

	CustomMesh->ClearAllMeshSections();
	CustomMesh->SetVisibility(false);
	TheChunk = nullptr;
	Initialized = false;
}

bool AVoxelChunkRender::IsInitialized()
{
	return Initialized;
}

bool AVoxelChunkRender::IsPolygonizingNow()
{
	if (Polygonizer->IsDone())
	{
		IsPolygonizing = false;
	}
	return IsPolygonizing;
}

void AVoxelChunkRender::RenderTick()
{
	check(Initialized);
	if (Polygonizer->IsDone())
	{
		if (PolygonizedData) delete PolygonizedData;
		PolygonizedData = Polygonizer->PopPolygonizedData();
		IsPolygonizing = false;
	}

	if (PolygonizedData && VoxelWorld->ShouldUpdateChunk())
	{
		ApplyPolygonizedData(PolygonizedData);
		delete PolygonizedData;
		PolygonizedData = nullptr;
	}

}

void AVoxelChunkRender::Polygonize()
{
	//Polygonizer thread is still remaining after deiniting this
	if (!Initialized) return;

	if (IsPolygonizing) return;
	IsPolygonizing = true;
	Polygonizer->DoPolygonize();
}

void AVoxelChunkRender::RenderRequest()
{
}

void AVoxelChunkRender::ApplyPolygonizedData(FVoxelPolygonizedData* Data)
{
	for (int Index = 0; Index < Data->Sections.Num(); Index++)
	{
		auto& Section = Data->Sections[Index];
		if (Section.Material)
			CustomMesh->SetMaterial(Index, Section.Material);
		if (Section.Vertex.Num() < 3 || Section.Triangle.Num() == 0) continue;
		CustomMesh->CreateMeshSection(Index, Section.Vertex, Section.Triangle, Section.Normal, Section.UV, Section.Color, TArray<FProcMeshTangent>(), true);
	}
}

UVoxelChunk* AVoxelChunkRender::GetVoxelChunk()
{
	return TheChunk;
}
