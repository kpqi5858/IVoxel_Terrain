#pragma once

#include "CoreMinimal.h"
#include "Block.h"
#include "VoxelData.h"

class UVoxelChunk;
class UBlock;
enum class EBlockFace : uint8;
struct FBlockPos;

class FBlockState
{
public:
	const FBlockPos Position;

	EBlockFace Facing = EBlockFace::INVALID;

	UVoxelChunk* Chunk;

private:
	UBlock* BlockDef;

private:
	FBlockState();

public:
	FBlockState(UVoxelChunk* OwnerChunk, FBlockPos Pos, UBlock* Block = nullptr);

	UBlock* GetBlockDef();
	void SetBlockDef(UBlock* Block);

	bool IsValid();
};