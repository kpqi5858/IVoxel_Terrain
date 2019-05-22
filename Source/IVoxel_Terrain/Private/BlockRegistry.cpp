#include "BlockRegistry.h"

FBlockRegistryInstance::FBlockRegistryInstance()
{
	ensureMsgf(false, TEXT("Default constructor of FBlockRegistryInstance called"));
}

FBlockRegistryInstance::FBlockRegistryInstance(TArray<UClass*> Registry)
{
	for (auto& BlockClass : Registry)
	{
		check(BlockClass->IsChildOf(UBlock::StaticClass()) && !BlockClass->HasAnyClassFlags(EClassFlags::CLASS_Abstract));
		UBlock* Block = NewObject<UBlock>(GetTransientPackage(), BlockClass);
		Block->AddToRoot();

		FName BlockName = Block->RegistryName;
		if (!BlockName.IsValid())
		{
			UE_LOG(LogIVoxel, Error, TEXT("%s : RegistryName is not valid"), *Block->GetClass()->GetName());
			continue;
		}
		if (BlockInstanceRegistry.Contains(BlockName))
		{
			UE_LOG(LogIVoxel, Error, TEXT("%s : Duplicate RegistryName of %s"), *Block->GetClass()->GetName(), *BlockName.ToString());
			continue;
		}
		BlockInstanceRegistry.Add(BlockName, Block);
	}
}

FBlockRegistryInstance::~FBlockRegistryInstance()
{
	TArray<UBlock*> ValueArray;
	BlockInstanceRegistry.GenerateValueArray(ValueArray);

	for (auto& BlockClass : ValueArray)
	{
		BlockClass->RemoveFromRoot();
	}
}

UBlock* FBlockRegistryInstance::GetBlock(FName Name)
{
	UBlock* Result = nullptr;
	auto Find = BlockInstanceRegistry.Find(Name);

#if DO_CHECK
	ensureMsgf(Find, TEXT("GetBlock(%s) failed"), *Name.ToString());
#endif

	return Find ? *Find : nullptr;
}

UBlock* FBlockRegistryInstance::GetBlock(FText Name)
{
	return GetBlock(FName(*Name.ToString()));
}

UBlock* FBlockRegistryInstance::GetBlock(FString Name)
{
	return GetBlock(FName(*Name));
}

void FBlockRegistry::ReloadBlocks()
{
	BlockRegistry.Empty();

	for (TObjectIterator<UClass> It; It; ++It)
	{
		if (It->IsChildOf(UBlock::StaticClass()) && !It->HasAnyClassFlags(CLASS_Abstract))
		{
			BlockRegistry.Add(*It);
		}
	}

	UE_LOG(LogIVoxel, Warning, TEXT("Registered %d UBlock(s)"), BlockRegistry.Num());
}

TArray<UClass*> FBlockRegistry::GetBlockRegistryClasses()
{
	TArray<UClass*> Result;
	BlockRegistry.RemoveAll([](TWeakObjectPtr<UClass>& Ptr) { return !Ptr.IsValid(); });
	for (auto& BlockClass : BlockRegistry)
	{
		Result.Add(BlockClass.Get());
	}
	return Result;
}

TSharedPtr<FBlockRegistryInstance> FBlockRegistry::GetInstance()
{
	if (InstancePtr.IsValid())
	{
		return InstancePtr.Pin();
	}
	else
	{
		TSharedPtr<FBlockRegistryInstance> Inst = MakeShareable(new FBlockRegistryInstance(GetBlockRegistryClasses()));
		InstancePtr = Inst;
		return Inst;
	}
}

FBlockRegistryInstance* FBlockRegistry::GetInstance_Ptr()
{
	checkf(InstancePtr.IsValid(), "GetInstance_Ptr called with no reference");
	return InstancePtr.Pin().Get();
}
