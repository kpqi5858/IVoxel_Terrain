#pragma once

#include "VoxelWorld.h"
#include "VoxelData.h"
#include "FastNoise_IV/FastNoise.h"
#include "UFNBlueprintFunctionLibrary.h"
#include "WorldGenerator.generated.h"

class UVoxelChunk;
class FBlockState;

typedef TFunction<void(FBlockPos, UVoxelChunk*, FBlockState*)> FWorldGenBlockOperation;

UCLASS(Abstract)
class UVoxelWorldGenerator : public UObject
{
	GENERATED_BODY()
public:
	UVoxelWorldGenerator()
	{ };
	virtual ~UVoxelWorldGenerator()
	{

	};
	
	virtual void Setup()
	{

	};

	virtual void GeneratePrime(UVoxelChunk* Chunk)
	{
		GeneratePrimeInternal(Chunk);
	};

	virtual void GeneratePrimeInternal(UVoxelChunk* Chunk)
	{
		unimplemented();
		UPrimeChunk& PC = Chunk->PrimeChunk;

		for (int X = 0; X < VOX_CHUNKSIZE; X++)
		{
			for (int Y = 0; Y < VOX_CHUNKSIZE; Y++)
			{
				for (int Z = 0; Z < VOX_CHUNKSIZE; Z++)
				{
					FIntVector GlobalPos = Chunk->GetGlobalPosition_Min() + FIntVector(X, Y, Z);
				}
			}
		}
	};

	virtual void PostGenerate(UVoxelChunk* Chunk)
	{

	}
};

UCLASS()
class UFlatWorldGenerator : public UVoxelWorldGenerator
{
	GENERATED_BODY()
public:
	virtual ~UFlatWorldGenerator()
	{ }

	virtual void GeneratePrimeInternal(UVoxelChunk* Chunk) override
	{
		UPrimeChunk& PC = Chunk->PrimeChunk;
		auto SolidDefault = GETBLOCK_C("SolidDefault");
		auto Air = GETBLOCK_C("Air");

		for (int X = 0; X < VOX_CHUNKSIZE; X++)
		{
			for (int Y = 0; Y < VOX_CHUNKSIZE; Y++)
			{
				for (int Z = 0; Z < VOX_CHUNKSIZE; Z++)
				{
					FIntVector GlobalPos = Chunk->GetGlobalPosition_Min() + FIntVector(X, Y, Z);

					if (GlobalPos.Z <= 0)
					{
						PC.SetBlockDef(X, Y, Z, SolidDefault);
					}
					else
					{
						PC.SetBlockDef(X, Y, Z, Air);
					}
				}
			}
		}
	};
};

UCLASS()
class UEmptyWorldGenerator : public UVoxelWorldGenerator
{
	GENERATED_BODY()
public:
	virtual ~UEmptyWorldGenerator()
	{ }

	virtual void GeneratePrimeInternal(UVoxelChunk* Chunk) override
	{
		UPrimeChunk& PC = Chunk->PrimeChunk;

		for (int X = 0; X < VOX_CHUNKSIZE; X++)
		{
			for (int Y = 0; Y < VOX_CHUNKSIZE; Y++)
			{
				for (int Z = 0; Z < VOX_CHUNKSIZE; Z++)
				{
					FIntVector GlobalPos = Chunk->GetGlobalPosition_Min() + FIntVector(X, Y, Z);
					PC.SetBlockDef(X, Y, Z, GETBLOCK_C("Air"));
				}
			}
		}
	};
};

UCLASS()
class UTestWorldGenerator : public UVoxelWorldGenerator
{
	GENERATED_BODY()
public:
	UPROPERTY()
	UUFNNoiseGenerator* HeightMapGen;

	virtual ~UTestWorldGenerator()
	{
	}

	virtual void Setup() override
	{
		HeightMapGen = UUFNBlueprintFunctionLibrary::CreateNoiseGenerator(this, ENoiseType::SimplexFractal, ECellularDistanceFunction::Euclidean, ECellularReturnType::Distance2, EFractalType::FBM, EInterp::InterpHermite, FMath::Rand(), 5, 0.05);
	}

	virtual void GeneratePrimeInternal(UVoxelChunk* Chunk) override
	{
		UPrimeChunk& PC = Chunk->PrimeChunk;

		for (int X = 0; X < VOX_CHUNKSIZE; X++)
		{
			for (int Y = 0; Y < VOX_CHUNKSIZE; Y++)
			{
				for (int Z = 0; Z < VOX_CHUNKSIZE; Z++)
				{
					FIntVector GlobalPos = Chunk->GetGlobalPosition_Min() + FIntVector(X, Y, Z);

					float NoiseVal = HeightMapGen->GetNoise3D(GlobalPos.X, GlobalPos.Y, GlobalPos.Z);
					
					if (NoiseVal < -0.3)
					{
						PC.SetBlockDef(X, Y, Z, GETBLOCK_C("SolidDefault"));
					}
					else
					{
						PC.SetBlockDef(X, Y, Z, GETBLOCK_C("Air"));
					}
				}
			}
		}
	};
};

UCLASS(Blueprintable)
class UTerrainGenerator : public UVoxelWorldGenerator
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite)
	UUFNNoiseGenerator* HeightMapGen;

	UPROPERTY(BlueprintReadWrite)
	UUFNNoiseGenerator* CaveGen;


	UPROPERTY(BlueprintReadWrite)
	int Height = 100;

	UPROPERTY(BlueprintReadWrite)
	UBlock* TopBlock;

	UPROPERTY(BlueprintReadWrite)
	UBlock* DownBlock;

	UFUNCTION(BlueprintImplementableEvent)
	void Setup_BP();

	virtual void Setup() override
	{
		Setup_BP();
	}
	
	virtual void GeneratePrimeInternal(UVoxelChunk* Chunk) override
	{
		UPrimeChunk& PC = Chunk->PrimeChunk;

		for (int X = 0; X < VOX_CHUNKSIZE; X++)
		{
			for (int Y = 0; Y < VOX_CHUNKSIZE; Y++)
			{
				FIntVector GlobalPosXY = Chunk->GetGlobalPosition_Min() + FIntVector(X, Y, 0);
				float NoiseVal = HeightMapGen->GetNoise2D(GlobalPosXY.X, GlobalPosXY.Y) * Height;

				for (int Z = 0; Z < VOX_CHUNKSIZE; Z++)
				{
					FIntVector GlobalPos = Chunk->GetGlobalPosition_Min() + FIntVector(X, Y, Z);
					if (GlobalPos.Z < NoiseVal)
					{
						if (FMath::IsNearlyEqual(NoiseVal, GlobalPos.Z, 1))
						{
							PC.SetBlockDef(X, Y, Z, TopBlock);
						}
						else
						{
							PC.SetBlockDef(X, Y, Z, DownBlock);
							float CaveNoise = CaveGen->GetNoise3D(GlobalPos.X, GlobalPos.Y, GlobalPos.Z);
							if (CaveNoise < -0.5)
							{
								PC.SetBlockDef(X, Y, Z, GETBLOCK_C("Air"));
							}
						}

					}
					else
					{
						PC.SetBlockDef(X, Y, Z, GETBLOCK_C("Air"));
					}
				}
			}
		}
	}
};

UCLASS(Blueprintable)
class USkyIslandsGanerator : public UVoxelWorldGenerator
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite)
	UUFNNoiseGenerator* IslandGen;

	UPROPERTY(BlueprintReadWrite)
	UUFNNoiseGenerator* MaskGen;

	UPROPERTY(BlueprintReadWrite)
	UBlock* BlockToUse;

	UFUNCTION(BlueprintImplementableEvent)
	void Setup_BP();

	virtual void Setup() override
	{
		Setup_BP();
	}

	virtual void GeneratePrimeInternal(UVoxelChunk* Chunk) override
	{
		UPrimeChunk& PC = Chunk->PrimeChunk;

		auto GlobalMask = [](int Z)
		{
			const int Middle = 100;
			return FMath::Clamp(Middle - (FMath::Abs(Z)), 0, 1);
		};

		for (int X = 0; X < VOX_CHUNKSIZE; X++)
		{
			for (int Y = 0; Y < VOX_CHUNKSIZE; Y++)
			{
				for (int Z = 0; Z < VOX_CHUNKSIZE; Z++)
				{
					FIntVector GlobalPos = Chunk->GetGlobalPosition_Min() + FIntVector(X, Y, Z);
					
					float NoiseVal = IslandGen->GetNoise3D(GlobalPos.X, GlobalPos.Y, GlobalPos.Z);
					float MaskVal = MaskGen->GetNoise3D(GlobalPos.X, GlobalPos.Y, GlobalPos.Z);

					float FinalNoise = NoiseVal * FMath::Clamp(MaskVal, 0.f, 1.f) * GlobalMask(GlobalPos.Z);

					if (FinalNoise < 0)
					{
						PC.SetBlockDef(X, Y, Z, BlockToUse);
					}
					else
					{
						PC.SetBlockDef(X, Y, Z, GETBLOCK_C("Air"));
					}
				}
			}
		}
	};
};

UCLASS(Blueprintable)
class UCaveGenerator : public UVoxelWorldGenerator
{
	GENERATED_BODY()

public:
	virtual ~UCaveGenerator()
	{ }

	UPROPERTY(EditDefaultsOnly)
	float Freq = 0.1f;

	UPROPERTY(EditDefaultsOnly)
	float Threshold = 0.f;

	UPROPERTY(EditDefaultsOnly)
	float Jitter = 0.4f;


	FastNoise CaveGen1;
	FastNoise CaveGen2;

	virtual void Setup() override
	{
		CaveGen1.SetSeed(FMath::Rand());
		CaveGen1.SetFrequency(Freq);
		CaveGen1.SetCellularReturnType(FastNoise::CellularReturnType::Distance2Div);
		CaveGen1.SetCellularJitter(Jitter);

		CaveGen2 = CaveGen1;
		CaveGen2.SetSeed(FMath::Rand());
	}

	virtual void GeneratePrimeInternal(UVoxelChunk* Chunk) override
	{
		UPrimeChunk& PC = Chunk->PrimeChunk;

		for (int X = 0; X < VOX_CHUNKSIZE; X++)
		{
			for (int Y = 0; Y < VOX_CHUNKSIZE; Y++)
			{
				for (int Z = 0; Z < VOX_CHUNKSIZE; Z++)
				{
					FIntVector GlobalPos = Chunk->GetGlobalPosition_Min() + FIntVector(X, Y, Z);
					FVector FloatPos = FVector(GlobalPos);

					float N0 = CaveGen1.GetCellular(FloatPos.X, FloatPos.Y, FloatPos.Z);
					FloatPos += FVector(Freq * 0.5f);

					float N1 = CaveGen2.GetCellular(FloatPos.X, FloatPos.Y, FloatPos.Z);
					float CaveNoise = FMath::Min(N0, N1);

					if (CaveNoise < Threshold)
					{
						PC.SetBlockDef(X, Y, Z, GETBLOCK_C("SolidDefault"));
					}
					else
					{
						PC.SetBlockDef(X, Y, Z, GETBLOCK_C("Air"));
					}

				}
			}
		}
	};
};