#pragma once

#include "CoreMinimal.h"
#include "RuntimeMeshComponent.h"
#include "IVoxel_BlockData.generated.h"

//Data for each block
USTRUCT(BlueprintType)
struct IVOXEL_TERRAIN_API FIVoxel_BlockData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float  Value;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FColor Color;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int BlockType;

public:
	FIVoxel_BlockData()
		: Value(-1), Color(FColor::White), BlockType(0)
	{}

	FIVoxel_BlockData(float val, FColor color, int type)
		: Value(val), Color(color), BlockType(type)
	{}
};
