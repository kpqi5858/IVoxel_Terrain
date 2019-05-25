#include "VoxelBlueprintLibrary.h"

UBlock* UVoxelBlueprintLibrary::GetBlockFromRegistry(FName Name)
{
	return GETBLOCK(Name);
}