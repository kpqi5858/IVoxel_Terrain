#include "BlockState.h"

FBlockState::FBlockState()
{
}

FBlockState::FBlockState(UVoxelChunk* OwnerChunk, FBlockPos Pos, UBlock* Block)
	: Position(Pos)
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

bool FBlockState::IsValid()
{
	return BlockDef != nullptr;
}