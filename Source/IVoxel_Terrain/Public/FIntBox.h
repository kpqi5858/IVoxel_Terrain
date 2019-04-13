#pragma once

#include "CoreMinimal.h"


//Partially from FBox source, Minimal features only
struct FIntBox
{
public:
	FIntVector Min;

	FIntVector Max;
	
	//No IsValid

public:
	FIntBox (const FIntVector& InMin, const FIntVector InMax)
		: Min(InMin)
		, Max(InMax)
	{ }

public:
	FORCEINLINE bool operator==(const FIntBox& Other) const
	{
		return (Min == Other.Min) && (Max == Other.Max);
	}
};