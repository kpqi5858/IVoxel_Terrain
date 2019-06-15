#pragma once

#include "CoreMinimal.h"
#include "Block.h"
#include "BlockRegistry.h"
#include "VoxelData.h"

class UVoxelChunk;
class UBlock;
enum class EBlockFace : uint8;
struct FBlockPos;

//19 Byte
class FBlockState
{
public:
	const uint16 PositionIndex;

	EBlockFace Facing;

	UVoxelChunk* Chunk;

private:
	UBlock* BlockDef = nullptr;

public:
	FBlockState();
	FBlockState(UVoxelChunk* OwnerChunk, uint16 PosIndex, UBlock* Block = nullptr);

	UBlock* GetBlockDef();
	void SetBlockDef(UBlock* Block);

	FBlockPos GetBlockPos();

	bool IsValid();
};

//Additional block info
//Ex) Color, Plant growth state, etc..
struct FBlockData
{
	uint8 Data[VOX_BLOCKDATASIZE];

	FBlockData()
	{
		FMemory::Memset(Data, 0, sizeof(Data));
	}
};