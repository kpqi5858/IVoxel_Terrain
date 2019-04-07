// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RuntimeMeshComponent.h"
#include "RuntimeMesh.h"
#include "Transvoxel.cpp"
#include "FastNoise/FastNoise.h"
#include "IVoxel_Polygonizer.h"
#include "IVoxel_WorldGenBase.h"
#include "IVoxel_TerrainManager.h"
#include "IVoxel_TerrainWorld.generated.h"

class IVoxel_TerrainManager;

UCLASS()
class IVOXEL_TERRAIN_API AIVoxel_TerrainWorld : public AActor
{
	GENERATED_BODY()
	
public:	
	UPROPERTY(EditAnywhere)
	USceneComponent* RootComp;
	
	// Sets default values for this actor's properties
	AIVoxel_TerrainWorld();

	UPROPERTY(EditAnywhere)
	float VoxelSize = 80;

	UPROPERTY(EditAnywhere)
	TArray<UMaterial*> VoxelMaterials;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UIVoxel_WorldGenerator> WorldGenerator;

	UPROPERTY(EditAnywhere)
	int KeepChunkRadius;

	UPROPERTY(EditAnywhere)
	int OctreeSize = 8;

	UPROPERTY(EditAnywhere)
	int UpdatePerTicks = 30;

	UPROPERTY(EditAnywhere)
	bool bEnableUV;

	UPROPERTY(EditAnywhere)
	int ThreadCount = 2;

	UPROPERTY(EditAnywhere)
	int CullingDepth = 32;

	UPROPERTY(EditAnywhere)
	int CollisionMaxDepth = 3;
	
	UPROPERTY(EditAnywhere)
	float DeletionDelay = 1;

	UPROPERTY(EditAnywhere)
	float WorldGenScale;

	UPROPERTY()
	UIVoxel_WorldGenerator* WorldGeneratorInstanced;

	IVoxel_TerrainManager* Manager;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	float GetVoxelSize();
	
	UFUNCTION(BlueprintCallable)
	void RegisterInvoker(AActor* Actor);
};
