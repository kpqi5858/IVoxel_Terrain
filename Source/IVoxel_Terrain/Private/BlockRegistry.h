#pragma once

#include "Block.h"
#include "CoreUObject/Public/UObject/UObjectIterator.h"

//Parameter is FName, FString, or FText
#define GETBLOCK(x) FBlockRegistry::GetInstance_Ptr()->GetBlock(x)
//Parameter is from TEXT(x) macro
#define GETBLOCK_T(x) FBlockRegistry::GetInstance_Ptr()->GetBlock_(x);
//Parameter is const char*
#define GETBLOCK_C(x) GETBLOCK_T(TEXT(x));

class UBlock;

class IVOXEL_TERRAIN_API FBlockRegistryInstance
{
private:
	TMap<FName, UBlock*> BlockInstanceRegistry;

private:
	FBlockRegistryInstance();

public:
	FBlockRegistryInstance(TArray<UClass*> Registry);
	~FBlockRegistryInstance();

	UBlock* GetBlock(FName Name);
	UBlock* GetBlock(FText Name);
	UBlock* GetBlock(FString Name);

	UBlock* GetBlock_(FName Name)
	{
		return GetBlock(Name);
	}
};

class IVOXEL_TERRAIN_API FBlockRegistry
{
private:
	static TWeakPtr<FBlockRegistryInstance> InstancePtr;
	static TArray<TWeakObjectPtr<UClass>> BlockRegistry;

public:
	//Iterates every class that inherits UBlock, and add to registry
	static void ReloadBlocks();

	static TArray<UClass*> GetBlockRegistryClasses();

	//AVoxelWorld should have this in member
	static TSharedPtr<FBlockRegistryInstance> GetInstance();

	//Macro only
	static FBlockRegistryInstance* GetInstance_Ptr();
};