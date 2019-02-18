// Fill out your copyright notice in the Description page of Project Settings.

#include "IVoxel_TerrainWorld.h"


// Sets default values
AIVoxel_TerrainWorld::AIVoxel_TerrainWorld()
	: KeepChunkRadius(5)
	, VoxelSize(75)
{
	RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	RootComponent = RootComp;

	RMC = CreateDefaultSubobject<URuntimeMeshComponent>(TEXT("RMC"));
	RMC->SetupAttachment(RootComp);
	
	PrimaryActorTick.bCanEverTick = true;
}

float AIVoxel_TerrainWorld::GetVoxelSize()
{
	return VoxelSize;
}

// Called when the game starts or when spawned
void AIVoxel_TerrainWorld::BeginPlay()
{
	Super::BeginPlay();

	if (WorldGenerator)
	{
		WorldGeneratorInstanced = NewObject<UIVoxel_WorldGenerator>(this, WorldGenerator);
	}
	else
	{
		WorldGeneratorInstanced = NewObject<UIVoxel_FlatWorldGenerator>(this, UIVoxel_FlatWorldGenerator::StaticClass());
	}
	WorldGeneratorInstanced->AddToRoot();
	check(WorldGeneratorInstanced);

	WorldGeneratorInstanced->ConstructionScript();

	Manager = new IVoxel_TerrainManager(this);
	Manager->CreateStartChunks();
	/*
	auto FlatWorldGen = new IVoxel_NoiseWorldGenerator();

	for (int x = -5; x < 5; x++)
	for (int y = -5; y < 5; y++)
	for (int z = -5; z < 5; z++)
	{
		FActorSpawnParameters SpawnParm;
		SpawnParm.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		auto chunk = GetWorld()->SpawnActor<AIVoxel_Chunk>(GetActorLocation(), GetActorRotation(), SpawnParm);
		chunk->Setup(this, FIntVector(x, y, z));
		chunk->GenerateChunkData(FlatWorldGen);

		auto Polygonizer = new IVoxel_MCPolygonizer(chunk);
		IVoxel_PolygonizedData PolygonizedData;

		if (Polygonizer->Polygonize(PolygonizedData))
		{
			UE_LOG(LogIVoxel, Warning, TEXT("%d %d"), PolygonizedData.PolygonizedSections[0].Vertex.Num(), PolygonizedData.PolygonizedSections[0].Triangle.Num())
			chunk->ApplyPolygonized(PolygonizedData);
		}
		else
		{
			UE_LOG(LogIVoxel, Error, TEXT("Can't polygonize"));
		}
		delete Polygonizer;
	}
	delete FlatWorldGen;
	*/
}

// Called every frame
void AIVoxel_TerrainWorld::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	check(Manager);
	Manager->Tick();
}

void AIVoxel_TerrainWorld::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	check(Manager);
	Manager->Destroy();
	WorldGeneratorInstanced->RemoveFromRoot();
}

void AIVoxel_TerrainWorld::RegisterInvoker(AActor* Actor)
{
	check(Manager);
	Manager->RegisterInvoker(Actor);
}

void AIVoxel_TerrainWorld::DoLodTick()
{
	TickFlag = true;
}