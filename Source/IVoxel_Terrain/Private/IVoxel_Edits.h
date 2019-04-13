#pragma once

#include "IVoxel_TerrainWorld.h"
#include "IVoxel_Edits.generated.h"

UCLASS(abstract)
class IVOXEL_TERRAIN_API UIVoxel_EditBase : public UObject
{
	GENERATED_BODY()

public:
	UIVoxel_EditBase()
	{

	}

	virtual ~UIVoxel_EditBase() { };

public:
	FIntVector IssuedLocation;

	FHitResult HitResult;
};

UCLASS(Blueprintable, abstract)
class IVOXEL_TERRAIN_API UIVoxel_BPEdit : public UIVoxel_EditBase
{
	GENERATED_BODY()

};