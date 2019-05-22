#pragma once

#include "VoxelChunk.h"
#include "VoxelWorld.h"
#include "IVoxel_Terrain.h"
#include "VoxelData.generated.h"

//TODO : Move the implementation to .cpp files

//Structs
class AVoxelWorld;
class UVoxelChunk;

UENUM(BlueprintType)
enum class EBlockFace : uint8
{
	INVALID, FRONT, BACK, LEFT, RIGHT, TOP, BOTTOM
};

class FVoxelUtilities
{
public:
	static FIntVector GetFaceOffset(EBlockFace Face)
	{
		switch (Face)
		{
		case EBlockFace::INVALID :
		{
			ensureMsgf(false, TEXT("GetFaceOffset called with invalid face"));
			return FIntVector(0);
		}
		case EBlockFace::FRONT :
			return FIntVector(1, 0, 0);
		case EBlockFace::BACK :
			return FIntVector(-1, 0, 0);
		case EBlockFace::LEFT :
			return FIntVector(0, -1, 0);
		case EBlockFace::RIGHT :
			return FIntVector(0, 1, 0);
		case EBlockFace::TOP :
			return FIntVector(0, 0, 1);
		case EBlockFace::BOTTOM :
			return FIntVector(0, 0, -1);
		default:
			check(false);
			return FIntVector(0);
		}
	};

	static FVector GetFaceOffset_Vector(EBlockFace Face)
	{
		switch (Face)
		{
		case EBlockFace::INVALID:
		{
			ensureMsgf(false, TEXT("GetFaceOffset called with invalid face"));
			return FVector(0);
		}
		case EBlockFace::FRONT:
			return FVector(1, 0, 0);
		case EBlockFace::BACK:
			return FVector(-1, 0, 0);
		case EBlockFace::LEFT:
			return FVector(0, -1, 0);
		case EBlockFace::RIGHT:
			return FVector(0, 1, 0);
		case EBlockFace::TOP:
			return FVector(0, 0, 1);
		case EBlockFace::BOTTOM:
			return FVector(0, 0, -1);
		default:
			check(false);
			return FVector(0);
		}
	};

	static EBlockFace FaceFromNormal(FVector Normal)
	{
		const EBlockFace AllFaces[6] = { EBlockFace::FRONT, EBlockFace::BACK, EBlockFace::LEFT, EBlockFace::RIGHT, EBlockFace::TOP, EBlockFace::BOTTOM };

		Normal.Normalize();

		float MinDist = FLT_MAX;
		auto BestFace = EBlockFace::INVALID;

		for (auto Face : AllFaces)
		{
			float Dist = FVector::Dist(GetFaceOffset_Vector(Face), Normal);
			if (Dist < MinDist)
			{
				BestFace = Face;
				MinDist = Dist;
			}
		}

		return BestFace;
	};

	static FIntVector PositionFromIndex(int Index)
	{
		int OriginalIndex = Index;

		int Z = Index / (VOX_CHUNKSIZE*VOX_CHUNKSIZE);
		Index -= Z * (VOX_CHUNKSIZE*VOX_CHUNKSIZE);
		int Y = Index / (VOX_CHUNKSIZE);
		Index -= Y * (VOX_CHUNKSIZE);
		int X = Index;

		FIntVector Result = FIntVector(X, Y, Z);
		check(!VOX_IS_OUTOFLOCALPOS(Result));
		check(VOX_CHUNK_AI(X, Y, Z) == OriginalIndex);
		return Result;
	}
};

struct FVoxelInvoker
{
	TWeakObjectPtr<AActor> Object;
	bool ShouldRender;

	FVoxelInvoker(AActor* Actor, bool DoRender)
	{
		check(Actor->IsValidLowLevel());
		Object = Actor;
		ShouldRender = DoRender;
	}
	bool IsValid()
	{
		return Object.IsValid();
	}
};

//Struct that handles voxel's position
//Do not edit the values of struct after initialization
USTRUCT(BlueprintType)
struct FBlockPos
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY(BlueprintReadWrite)
	AVoxelWorld* World;

	UPROPERTY(BlueprintReadWrite)
	FIntVector GlobalPos;

private:
	inline void ValidCheck()
	{
		check(World);
	}

public:
	FBlockPos()
		: World(nullptr), GlobalPos(FIntVector(0))
	{ }

	FBlockPos(UVoxelChunk* mChunk, FIntVector LocalPos)
	{
		GlobalPos = mChunk->LocalToGlobalPosition(LocalPos);
		World = mChunk->GetVoxelWorld();
	}

	FBlockPos(AVoxelWorld* VoxelWorld, FIntVector GlobalPosition)
	{
		World = VoxelWorld;
		GlobalPos = GlobalPosition;
	}

	FIntVector GetGlobalPosition()
	{
		return GlobalPos;
	}
	
	AVoxelWorld* GetWorld()
	{
		return World;
	}

	FIntVector GetChunkIndex()
	{
		return FIntVector(GlobalPos.X % VOX_CHUNKSIZE
						, GlobalPos.Y % VOX_CHUNKSIZE
						, GlobalPos.Z % VOX_CHUNKSIZE);
	}

	UVoxelChunk* GetChunk()
	{
		return World->GetChunkFromBlockPos(*this);
	}
public:
	int ArrayIndex()
	{
		FIntVector ChunkLocalPos = GetChunkIndex();
		return VOX_CHUNK_AI(ChunkLocalPos.X, ChunkLocalPos.Y, ChunkLocalPos.Z);
	}
};