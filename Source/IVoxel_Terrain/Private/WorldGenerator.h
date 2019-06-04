#pragma once

#include "VoxelWorld.h"
#include "VoxelData.h"
#include "FastNoise.h"
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
class U3DNoiseWorldGenerator : public UVoxelWorldGenerator
{
	GENERATED_BODY()
public:
	virtual ~U3DNoiseWorldGenerator()
	{
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
				}
			}
		}
	};
};