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

	UPROPERTY()
	URuntimeMeshComponent* RMC;
	// Sets default values for this actor's properties
	AIVoxel_TerrainWorld();

	UPROPERTY(EditAnywhere)
	float VoxelSize;

	UPROPERTY(EditAnywhere)
	TArray<UMaterial*> VoxelMaterials;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UIVoxel_WorldGenerator> WorldGenerator;

	UPROPERTY(EditAnywhere)
	int KeepChunkRadius;

	UPROPERTY(EditAnywhere)
	int OctreeSize;

	UPROPERTY(EditAnywhere)
	int UpdatePerTicks;

	UPROPERTY(EditAnywhere)
	UCurveFloat* LodCurve;

	UPROPERTY(EditAnywhere)
	int ThreadCount;

	UPROPERTY(EditAnywhere)
	int CullingDepth;

	UPROPERTY(EditAnywhere)
	int CollisionMaxDepth;

	UPROPERTY()
	UIVoxel_WorldGenerator* WorldGeneratorInstanced;

	IVoxel_TerrainManager* Manager;

	bool TickFlag = false;
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

	UFUNCTION(BlueprintCallable)
	void DoLodTick();

	UFUNCTION(BlueprintCallable)
	void DebugRender();
};
