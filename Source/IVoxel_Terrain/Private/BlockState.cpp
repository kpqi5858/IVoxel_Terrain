#include "BlockState.h"

FBlockState::FBlockState(UVoxelChunk* OwnerChunk, FBlockPos Pos, UBlock* Block)
	: Position(Pos)
{
	if (!Block)
	{
		Block = GETBLOCK_T(TEXT("Air"));
	}
	Chunk = OwnerChunk;
}

UBlock* FBlockState::GetBlockDef()
{
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