#include "VoxelChunkRender.h"
#include "VoxelWorld.h"

AVoxelChunkRender::AVoxelChunkRender()
{
	CustomMesh = CreateDefaultSubobject<URuntimeMeshComponent>(TEXT("CustomMesh"));
	RootComponent = CustomMesh;
	TheChunk = nullptr;
}

AVoxelChunkRender::~AVoxelChunkRender()
{
}

void AVoxelChunkRender::Initialize(UVoxelChunk* Chunk)
{
	check(!Initialized);

	TheChunk = Chunk;
	Initialized = true;
	CustomMesh->SetVisibility(true);
	Polygonizer = MakeShareable(new FVoxelPolygonizer(TheChunk));

	SetActorLocation(FVector(Chunk->GetGlobalPosition_Min()) * Chunk->GetVoxelWorld()->GetVoxelSize());

	//Use async physics cooking
	//CustomMesh->bUseAsyncCooking = true;
	CustomMesh->SetCollisionUseAsyncCooking(true);

	VoxelWorld = Chunk->GetVoxelWorld();
}

void AVoxelChunkRender::DestroyRender()
{
	check(Initialized);

	CustomMesh->ClearAllMeshSections();
	CustomMesh->SetVisibility(false);
	TheChunk = nullptr;
	Initialized = false;

	PolygonizedData.Reset();
}

bool AVoxelChunkRender::IsInitialized()
{
	return Initialized;
}

bool AVoxelChunkRender::IsPolygonizingNow()
{
	if (!Initialized && Polygonizer->IsDone())
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
		PolygonizedData = Polygonizer->PopPolygonizedData();
		IsPolygonizing = false;
	}

	if (PolygonizedData.IsValid())
	{
		bool Reset = false;

		if (RePolygonize)
		{
			RePolygonize = false;
			TheChunk->SetRenderDirty();
			Reset = true;
		}
		else if (VoxelWorld->ShouldUpdateChunk())
		{
			ApplyPolygonizedData(PolygonizedData.Get());
			Reset = true;
		}

		if (Reset)
			PolygonizedData.Reset();
	}

}

void AVoxelChunkRender::Polygonize()
{
	//Polygonizer thread can still remain after deiniting this
	if (!Initialized) return;
	Polygonizer->DoPolygonize();
}

FORCENOINLINE void AVoxelChunkRender::RenderRequest()
{
	if (IsPolygonizing)
	{
		RePolygonize = true;
		return;
	}
	TheChunk->QueuePolygonize();
	IsPolygonizing = true;
}

void AVoxelChunkRender::ApplyPolygonizedData(FVoxelPolygonizedData* Data)
{
	int CurSections = CustomMesh->GetNumSections();
	int DataSections = Data->Sections.Num();

	for (int Index = 0; Index < FMath::Max(CurSections, DataSections); Index++)
	{
		CustomMesh->ClearMeshSection(Index);
		if (Index >= DataSections)
		{
			continue;
		}
		auto& Section = Data->Sections[Index];
		if (Section.Material)
			CustomMesh->SetMaterial(Index, Section.Material);
		if (Section.Vertex.Num() < 3 || Section.Triangle.Num() == 0) continue;
		
		CustomMesh->CreateMeshSection(Index, Section.Vertex, Section.Triangle, Section.Normal, Section.UV, Section.Color, TArray<FRuntimeMeshTangent>(), true);
	}
}

UVoxelChunk* AVoxelChunkRender::GetVoxelChunk()
{
	return TheChunk;
}
