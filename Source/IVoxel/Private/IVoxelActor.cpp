#include "IVoxelActor.h"

AIVoxelActor::AIVoxelActor()
	: IsInitialized(false)
{
	PrimaryActorTick.bCanEverTick = true;

	ConstructorHelpers::FObjectFinder<UMaterialInterface> CubeMaterial(TEXT("/Game/Textures/M_VoxelMaterial"));

	RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("RootComp"));
	RootComponent = RootComp;

	RMC = CreateDefaultSubobject<URuntimeMeshComponent>(TEXT("RMC"));
	RMC->SetupAttachment(RootComp);

	RMC->SetMaterial(0, CubeMaterial.Object);
	UE_LOG(LogGarbage, Warning, TEXT("Init"));
	
}
void AIVoxelActor::Initialize()
{
	if (IsInitialized)
	{
		UE_LOG(LogGarbage, Error, TEXT("%s has already initialized"), *GetNameSafe(this));
		return;
	}
	Manager = new IVoxelManager(this, ChunkDepth);
	RMC->CreateMeshSection(0, false, false, 1, false, false);
	IsInitialized = true;
}

void AIVoxelActor::BeginPlay()
{
	check(IsInGameThread());
	Super::BeginPlay();
	//RMC->GetRuntimeMesh()->SetConvexCollisionSection(1, TArray<FVector>());
	RMC->SetCollisionUseComplexAsSimple(false);
	if (AutoInitialize && !IsInitialized)
	{
		Initialize();
	}
}
void AIVoxelActor::Uninitialize()
{
	delete Manager;
	RMC->CreateMeshSection(0, false, false, 1, false, false);
	RMC->GetRuntimeMesh()->ClearCollisionBoxes();
	IsInitialized = false;
}


void AIVoxelActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (IsInitialized)
	{
		Manager->Tick();
	}
}

void AIVoxelActor::SetOctreeValue(FVector Location, int Depth, bool Value, FColor Color)
{
	Manager->SetOctreeValue(Location, Depth, Value, Color);
}

void AIVoxelActor::RenderOctree(FVector Location, int RenderDepth, int ChildDepth, bool Debug)
{
	if (Debug)
	{
		Manager->MainOctree->TestRender(GetWorld(), FVector(0));
	}
	else
	{
		Manager->PolygonizeOctree(Location, RenderDepth, ChildDepth, 0);
	}
}

void AIVoxelActor::SetSimulatePhysics_Experimental(bool Set)
{
	RMC->SetSimulatePhysics(Set);
}

FVector AIVoxelActor::WorldPositionToLocalPosition(FVector WorldLocation, FRotator Rotation)
{
	FVector BasePosition =  ( WorldLocation - RMC->GetComponentLocation() ) / RMC->GetComponentScale();
	return Rotation.UnrotateVector(BasePosition);
	if (!Rotation.IsZero() || true) //Use Rotation Matrix to Calcuate position
	{
		float cX = BasePosition.X;
		float cY = BasePosition.Y;
		float cZ = BasePosition.Z;
		
		float rX = -Rotation.Roll;
		float rY = -Rotation.Pitch;
		float rZ = -Rotation.Yaw;

		{ //X Rotation
			cX = cX;
			cY = (cY * FMath::Cos(rX)) - (cZ * FMath::Sin(rX));
			cZ = (cY * FMath::Sin(rX)) + (cZ * FMath::Cos(rX));
		}
		{ //Y Rotation
			cX = (cX * FMath::Cos(rY)) + (cZ * FMath::Sin(rY));
			cY = cY;
			cZ= -(cX * FMath::Sin(rY)) + (cZ * FMath::Cos(rY));
		}
		{ //Z Rotation
			cX = (cX * FMath::Cos(rZ)) - (cY * FMath::Sin(rZ));
			cY = (cX * FMath::Sin(rZ)) + (cY * FMath::Cos(rZ));
			cZ = cZ;
		}
		return FVector(cX, cY, cZ);
	}
	return BasePosition;
}