#include "BlockState.h"

FBlockState::FBlockState()
	: PositionIndex(0)
{
}

FBlockState::FBlockState(UVoxelChunk* OwnerChunk, uint16 PosIndex, UBlock* Block)
	: PositionIndex(PosIndex)
{
	Facing = EBlockFace::INVALID;
	if (!Block)
	{
		Block = GETBLOCK_T(TEXT("Air"));
	}
	Chunk = OwnerChunk;
}

UBlock* FBlockState::GetBlockDef()
{
	if (!BlockDef) BlockDef = GETBLOCK_T(TEXT("Air"));
	return BlockDef;
}

void FBlockState::SetBlockDef(UBlock* Block)
{
	BlockDef = Block;
}

FBlockPos FBlockState::GetBlockPos()
{
	return FBlockPos(Chunk, FVoxelUtilities::PositionFromIndex(PositionIndex));
}

bool FBlockState::IsValid()
{
	return BlockDef != nullptr;
}