#pragma once

#include "Octree.h"
#include "CoreMinimal.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "RuntimeMeshComponent.h"
#include "ConstructorHelpers.h"
#include "IVoxelManager.h"

#include "IVoxelActor.generated.h"

class IVoxelManager;
class URuntimeMeshComponent;

UCLASS()
class AIVoxelActor : public AActor 
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere)
	USceneComponent* RootComp;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	URuntimeMeshComponent* RMC;
	
	IVoxelManager* Manager;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int ChunkDepth = 7;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool AutoInitialize = true;

	UPROPERTY(BlueprintReadOnly)
	bool IsInitialized;

	AIVoxelActor();

	UFUNCTION(BlueprintCallable)
	void Initialize();
	UFUNCTION(BlueprintCallable)
	void Uninitialize();

	void Tick(float DeltaTime) override;
	void BeginPlay() override;

	UFUNCTION(BlueprintCallable)
	void SetOctreeValue(FVector Location, int Depth, bool Value, FColor Color);

	UFUNCTION(BlueprintCallable)
	void RenderOctree(FVector Location, int RenderDepth, int ChildDepth, bool Debug);

	UFUNCTION(BlueprintCallable)
	void SetSimulatePhysics_Experimental(bool Set);

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FVector WorldPositionToLocalPosition(FVector WorldLocation, FRotator Rotation);
};