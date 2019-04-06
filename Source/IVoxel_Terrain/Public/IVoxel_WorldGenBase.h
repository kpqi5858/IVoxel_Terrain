#pragma once

#include "CoreMinimal.h"
#include "IVoxel_BlockData.h"
#include "FastNoise/FastNoise.h"
#include "UFNNoiseGenerator.h"
#include "IVoxel_WorldGenBase.generated.h"

UCLASS(abstract)
class IVOXEL_TERRAIN_API UIVoxel_WorldGenerator : public UObject
{
	GENERATED_BODY()
public:
	//Decreasing WorldGenScale drastically makes smoother terrain
	//Should get two sample and return middle data?
	float WorldGenScale = 1;

	FIVoxel_BlockData GetBlockData(float x, float y, float z)
	{
		FIVoxel_BlockData data = GetBlockDataImpl(x*WorldGenScale, y*WorldGenScale, z*WorldGenScale);
		data.Value = FMath::Clamp(data.Value, -1.0f, 1.0f);
		return data;
	}

	//Override this function and return data
	virtual FIVoxel_BlockData GetBlockDataImpl(float x, float y, float z)
	{
		unimplemented();
		return FIVoxel_BlockData();
	}

	//This function is called when AIVoxel_TerrainWorld initializes world generator
	virtual void ConstructionScript() { };

};

UCLASS(Blueprintable, abstract)
class IVOXEL_TERRAIN_API UIVoxel_BPWorldGenerator : public UIVoxel_WorldGenerator
{
	GENERATED_BODY()
public:
	bool HasPrintedUnimplementedError = false;

	//Construction script
	UFUNCTION(BlueprintImplementableEvent)
	void ConstructionScriptBP();

	//Implementation
	virtual void ConstructionScript() override
	{
		ConstructionScriptBP();
	}

	//Override this function in Blueprint
	UFUNCTION(BlueprintNativeEvent)
	FIVoxel_BlockData GetBlockDataBP(float X, float Y, float Z);
	
	//Implementation
	FIVoxel_BlockData GetBlockDataBP_Implementation(float x, float y, float z)
	{
		if (!HasPrintedUnimplementedError)
		{
			HasPrintedUnimplementedError = true;
			//UE_LOG(LogIVoxel, ELogVerbosity::Error, TEXT("No implementation GetBlockDataBP (%s)"), *GetName());
		}
		return FIVoxel_BlockData();
	}

	FIVoxel_BlockData GetBlockDataImpl(float x, float y, float z) override
	{
		return GetBlockDataBP(x, y, z);
	}

};

UCLASS()
class IVOXEL_TERRAIN_API UIVoxel_FlatWorldGenerator : public UIVoxel_WorldGenerator
{
	GENERATED_BODY()
public:
	FIVoxel_BlockData GetBlockDataImpl(float x, float y, float z) override
	{
		return FIVoxel_BlockData(-z, FColor::White, 0);
	}
};

UCLASS()
class IVOXEL_TERRAIN_API UIVoxel_NoiseWorldGenerator : public UIVoxel_WorldGenerator
{
	GENERATED_BODY()
public:
	FastNoise fn = FastNoise(FMath::Rand());
	FIVoxel_BlockData GetBlockDataImpl(float x, float y, float z) override
	{
		return FIVoxel_BlockData((fn.GetSimplex(x/1.5, y/1.5) * 75) - z, FColor::White, 0);
	}
};

UCLASS(Blueprintable)
class IVOXEL_TERRAIN_API UIVoxel_UFNWorldGenerator : public UIVoxel_WorldGenerator
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite)
	UUFNNoiseGenerator* NoiseGen;

	UPROPERTY(BlueprintReadWrite)
	float ZScale;

	FIVoxel_BlockData GetBlockDataImpl(float x, float y, float z) override
	{
		if (!NoiseGen)
		{
			return FIVoxel_BlockData();
		}
		else
		{
			return FIVoxel_BlockData((NoiseGen->GetNoise2D(x, y) * ZScale) - z, FColor::White, 0);
		}
	}
};