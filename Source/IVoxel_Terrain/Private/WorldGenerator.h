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
	UVoxelChunk* OwnerChunk = nullptr;
	TSet<UVoxelChunk*> TouchedChunks;

public:
	UVoxelWorldGenerator()
	{ };
	virtual ~UVoxelWorldGenerator()
	{

	};

	virtual void Setup(UVoxelChunk* Owner)
	{
		OwnerChunk = Owner;
	};
	
	virtual void Generate()
	{
		check(OwnerChunk);

		OwnerChunk->BlockStateStorageLock();
		GenerateInternal();
		for (auto& Touched : TouchedChunks)
		{
			Touched->BlockStateStorageUnlock();
			Touched->WorldGeneratorsReferences.Decrement();
		}
		TouchedChunks.Empty();
		OwnerChunk->BlockStateStorageUnlock();
		OwnerChunk->WorldGenState = EWorldGenState::GENERATED;
	};

	virtual void GenerateInternal()
	{
		unimplemented();
		
		for (int X = 0; X < VOX_CHUNKSIZE; X++)
		{
			for (int Y = 0; Y < VOX_CHUNKSIZE; Y++)
			{
				for (int Z = 0; Z < VOX_CHUNKSIZE; Z++)
				{
					FIntVector GlobalPos = OwnerChunk->GetGlobalPosition_Min() + FIntVector(X, Y, Z);
					
				}
			}
		}
	};

protected:
	void EditBlock(FBlockPos Pos, FWorldGenBlockOperation Func)
	{
		UVoxelChunk* Chunk = Pos.GetChunk();
		if (Chunk != OwnerChunk && !TouchedChunks.Contains(Chunk))
		{
			TouchedChunks.Add(Chunk);

			Chunk->WorldGeneratorsReferences.Increment();
			Chunk->TrySetChunkState(EChunkState::CS_WorldGenGenerated);
			Chunk->BlockStateStorageLock();
		}
		Func(Pos, Chunk, Chunk->GetBlockState(Pos));
	};

	void SetBlockGen(FBlockPos Pos, UBlock* Block)
	{
		EditBlock(Pos, [&](FBlockPos Pos, UVoxelChunk* Chunk, FBlockState* State) {Chunk->SetBlock(Pos, Block); });
	}
};

UCLASS()
class UFlatWorldGenerator : public UVoxelWorldGenerator
{
	GENERATED_BODY()
public:
	virtual ~UFlatWorldGenerator()
	{ }

	virtual void GenerateInternal() override
	{
		for (int X = 0; X < VOX_CHUNKSIZE; X++)
		{
			for (int Y = 0; Y < VOX_CHUNKSIZE; Y++)
			{
				for (int Z = 0; Z < VOX_CHUNKSIZE; Z++)
				{
					FBlockPos Pos = FBlockPos(OwnerChunk, FIntVector(X, Y, Z));

					if (Pos.GlobalPos.Z <= 0)
					{
						SetBlockGen(Pos, GETBLOCK_C("SolidDefault"));
					}
					else
					{
						SetBlockGen(Pos, GETBLOCK_C("Air"));
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

	virtual void GenerateInternal() override
	{
		for (int X = 0; X < VOX_CHUNKSIZE; X++)
		{
			for (int Y = 0; Y < VOX_CHUNKSIZE; Y++)
			{
				for (int Z = 0; Z < VOX_CHUNKSIZE; Z++)
				{
					FBlockPos Pos = FBlockPos(OwnerChunk, FIntVector(X, Y, Z));

					SetBlockGen(Pos, GETBLOCK_C("Air"));
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

	virtual void GenerateInternal() override
	{
		UUFNNoiseGenerator* NoiseGen = UUFNBlueprintFunctionLibrary::CreateNoiseGenerator(this, ENoiseType::Simplex, ECellularDistanceFunction::Euclidean, ECellularReturnType::CellValue, EFractalType::Billow, EInterp::InterpHermite, FMath::Rand(), 3, 0.1);
		
		for (int X = 0; X < VOX_CHUNKSIZE; X++)
		{
			for (int Y = 0; Y < VOX_CHUNKSIZE; Y++)
			{
				for (int Z = 0; Z < VOX_CHUNKSIZE; Z++)
				{
					FBlockPos Pos = FBlockPos(OwnerChunk, FIntVector(X, Y, Z));

					if (NoiseGen->GetNoise3D(Pos.GlobalPos.X, Pos.GlobalPos.Y, Pos.GlobalPos.Z) < 0)
					{
						SetBlockGen(Pos, GETBLOCK_C("BPTest"));
					}
					else
					{
						SetBlockGen(Pos, GETBLOCK_C("Air"));
					}
				}
			}
		}
	};
};