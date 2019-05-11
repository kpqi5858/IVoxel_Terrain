#pragma once

#include "CoreMinimal.h"
#include "Block.h"
#include "VoxelData.h"

struct FBlockPos;

class FBlockState
{
public:
	UBlock* BlockDef;
	FBlockPos Position;

	int Data1;
	int Data2;
	float Data3;
	float Data4;
public:
	//AIR
	FBlockState();
	FBlockState(UBlock* Block, FBlockPos Pos);
};