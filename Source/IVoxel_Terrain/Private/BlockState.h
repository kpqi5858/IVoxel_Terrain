#pragma once

#include "CoreMinimal.h"
#include "Block.h"
#include "VoxelData.h"

struct FBlockPos;

class FBlockState
{
public:
	const FBlockPos Position;

	EBlockFace Facing;

private:
	UBlock* BlockDef;

private:
	FBlockState();

public:
	FBlockState(FBlockPos Pos);
	FBlockState(UBlock* Block, FBlockPos Pos);

	UBlock* GetBlockDef();
	void SetBlockDef(UBlock* Block);

	bool IsValid();

	//Is this face actually visible for rendering?
	bool IsFaceVisible(EBlockFace Face);
};