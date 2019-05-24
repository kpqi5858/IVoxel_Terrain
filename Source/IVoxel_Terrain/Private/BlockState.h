#pragma once

#include "CoreMinimal.h"
#include "Block.h"
#include "BlockRegistry.h"
#include "VoxelData.h"

class UVoxelChunk;
class UBlock;
enum class EBlockFace : uint8;
struct FBlockPos;

class FBlockState
{
public:
	FBlockPos Position;

	EBlockFace Facing;

	UVoxelChunk* Chunk;

private:
	UBlock* BlockDef;

public:
	FBlockState();
	FBlockState(UVoxelChunk* OwnerChunk, FBlockPos Pos, UBlock* Block = nullptr);

	UBlock* GetBlockDef();
	void SetBlockDef(UBlock* Block);

	bool IsValid();
};