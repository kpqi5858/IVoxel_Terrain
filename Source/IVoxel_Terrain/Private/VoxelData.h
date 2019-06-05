#pragma once

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

	static EBlockFace GetOppositeFace(EBlockFace Face)
	{
		switch (Face)
		{
		case EBlockFace::INVALID:
		{
			check(false);
			return EBlockFace::INVALID;
		}
		case EBlockFace::FRONT:
			return EBlockFace::BACK;
		case EBlockFace::BACK:
			return EBlockFace::FRONT;
		case EBlockFace::LEFT:
			return EBlockFace::RIGHT;
		case EBlockFace::RIGHT:
			return EBlockFace::LEFT;
		case EBlockFace::TOP:
			return EBlockFace::BOTTOM;
		case EBlockFace::BOTTOM:
			return EBlockFace::TOP;
		}

		return EBlockFace::INVALID;
	}

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
		const int OriginalIndex = Index;

		const int Z = Index / (VOX_CHUNKSIZE*VOX_CHUNKSIZE);
		Index -= Z * (VOX_CHUNKSIZE*VOX_CHUNKSIZE);
		const int Y = Index / (VOX_CHUNKSIZE);
		Index -= Y * (VOX_CHUNKSIZE);
		const int X = Index;

		FIntVector Result = FIntVector(X, Y, Z);
		//check(!VOX_IS_OUTOFLOCALPOS(Result));
		checkf((VOX_CHUNK_AI(X, Y, Z)) == OriginalIndex, TEXT("%d(%d,%d,%d) %d"), VOX_CHUNK_AI(X, Y, Z), X, Y, Z, OriginalIndex);
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

struct FVoxelPolygonizedSection
{
	TArray<FVector> Vertex;
	TArray<int32> Triangle;
	TArray<FVector> Normal;
	TArray<FVector2D> UV;
	TArray<FColor> Color;
	UMaterialInterface* Material;
};

struct FVoxelPolygonizedData
{
	TArray<FVoxelPolygonizedSection> Sections;
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

public:
	FBlockPos();

	FBlockPos(UVoxelChunk* mChunk, FIntVector LocalPos);

	FBlockPos(AVoxelWorld* VoxelWorld, FIntVector GlobalPosition);

	FIntVector GetGlobalPosition() const;
	
	AVoxelWorld* GetWorld() const;

	FIntVector GetChunkIndex() const;

	FIntVector GetLocalPos() const;

	UVoxelChunk* GetChunk() const;

public:
	int ArrayIndex() const;
};

struct FFaceVisiblityCache
{
	uint8 Data = 0;

	FORCEINLINE bool IsThisFaceVisible(EBlockFace Face) const
	{
		uint8 N = 1 << static_cast<int>(Face);
		return Data & N;
	}

	//Returns true if modified
	FORCEINLINE bool SetFaceVisible(EBlockFace Face, bool Value)
	{
		uint8 Old = Data;

		uint8 N = 1 << static_cast<int>(Face);
		if (Value)
		{
			Data |= N;
		}
		else
		{
			Data &= ~N;
		}

		return Data != Old;
	}
};