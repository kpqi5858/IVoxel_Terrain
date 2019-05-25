#include "VoxelChunkRender.h"
#include "VoxelWorld.h"

AVoxelChunkRender::AVoxelChunkRender()
{
	RMC = CreateDefaultSubobject<URuntimeMeshComponent>(TEXT("RMC"));
	RootComponent = RMC;
	TheChunk = nullptr;
}

void AVoxelChunkRender::Initialize(UVoxelChunk* Chunk)
{
	TheChunk = Chunk;
	Initialized = true;
	RMC->SetVisibility(true);
	Polygonizer = new FVoxelPolygonizer(Chunk);
	SetActorLocation(FVector(Chunk->GetGlobalPosition_Min()) * Chunk->GetVoxelWorld()->GetVoxelSize());
}

void AVoxelChunkRender::DestroyRender()
{
	RMC->ClearAllMeshSections();
	RMC->SetVisibility(false);
	TheChunk = nullptr;
	Initialized = false;
	delete Polygonizer;
}

bool AVoxelChunkRender::IsInitialized()
{
	return Initialized;
}

bool AVoxelChunkRender::IsPolygonizingNow()
{
	return IsPolygonizing;
}

void AVoxelChunkRender::RenderTick()
{
	check(Initialized);
	if (Polygonizer->IsDone())
	{
		ApplyPolygonizedData(Polygonizer->PopPolygonizedData());
		IsPolygonizing = false;
	}
}

void AVoxelChunkRender::Polygonize()
{
	if (IsPolygonizing) return;
	IsPolygonizing = true;
	Polygonizer->DoPolygonize();
}

void AVoxelChunkRender::ApplyPolygonizedData(FVoxelPolygonizedData* Data)
{
	for (int Index = 0; Index < Data->Sections.Num(); Index++)
	{
		auto& Section = Data->Sections[Index];
		if (Section.Material)
			RMC->SetMaterial(Index, Section.Material);
		RMC->CreateMeshSection(Index, Section.Vertex, Section.Triangle, Section.Normal, Section.UV, Section.Color, TArray<FRuntimeMeshTangent>(), true);
	}
}

UVoxelChunk* AVoxelChunkRender::GetVoxelChunk()
{
	return TheChunk;
}
