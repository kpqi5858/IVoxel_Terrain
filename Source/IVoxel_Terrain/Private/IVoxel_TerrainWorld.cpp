// Fill out your copyright notice in the Description page of Project Settings.

#include "IVoxel_TerrainWorld.h"


// Sets default values
AIVoxel_TerrainWorld::AIVoxel_TerrainWorld()
	: KeepChunkRadius(5)
	, VoxelSize(75)
{
	RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	RootComponent = RootComp;

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

	WorldGeneratorInstanced->WorldGenScale = WorldGenScale;
	WorldGeneratorInstanced->AddToRoot();


	WorldGeneratorInstanced->ConstructionScript();

	Manager = new IVoxel_TerrainManager(this);
	Manager->CreateStartChunks();
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