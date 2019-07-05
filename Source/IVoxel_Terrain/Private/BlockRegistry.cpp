#include "BlockRegistry.h"
#include "AssetRegistryModule.h"
#include "Engine/Blueprint.h"

TWeakPtr<FBlockRegistryInstance> FBlockRegistry::InstancePtr = TWeakPtr<FBlockRegistryInstance>();
TArray<TWeakObjectPtr<UClass>> FBlockRegistry::BlockRegistry = TArray< TWeakObjectPtr<UClass>>();
FBlockRegistryInstance* FBlockRegistry::InstancePtrRaw = nullptr;

FBlockRegistryInstance::FBlockRegistryInstance()
{
	ensureMsgf(false, TEXT("Default constructor of FBlockRegistryInstance called"));
}

FBlockRegistryInstance::FBlockRegistryInstance(TArray<UClass*> Registry)
{
	TArray<FName> RegisteredNames;
	RegisteredNames.Reserve(Registry.Num());

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
		RegisteredNames.Add(BlockName);
	}
	check(RegisteredNames.Num() == BlockInstanceRegistry.Num());
	checkf(RegisteredNames.Num() < 65536, TEXT("UniqueIndex collision, RegisteredNames.Num() = %d"), RegisteredNames.Num());

	RegisteredNames.Sort();
	UniqueIndices.AddDefaulted(RegisteredNames.Num());

	for (int Index = 0; Index < RegisteredNames.Num(); Index++)
	{
		auto Block = BlockInstanceRegistry.FindChecked(RegisteredNames[Index]);
		Block->UniqueIndex = Index;
		UniqueIndices[Index] = Block;
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

	return Find ? *Find : GetBlock_(TEXT("Air"));
}

UBlock* FBlockRegistryInstance::GetBlock(FText Name)
{
	return GetBlock(FName(*Name.ToString()));
}

UBlock* FBlockRegistryInstance::GetBlock(FString Name)
{
	return GetBlock(FName(*Name));
}

UBlock* FBlockRegistryInstance::GetBlockByIndex(uint16 Index)
{
	return UniqueIndices[Index];
}

void FBlockRegistry::ReloadBlocks()
{
	BlockRegistry.Empty();
	const auto BlockClass = UBlock::StaticClass();

	FAssetRegistryModule& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	TArray<FAssetData> AssetData;
	AssetRegistry.Get().GetAssetsByClass(UBlueprint::StaticClass()->GetFName(), AssetData);

	//Load blueprint assets to register them
	for (auto& Asset : AssetData)
	{
		//This thing will load every blueprint, which is BAD
		auto Obj = Asset.GetClass();
		if (Obj == BlockClass)
		{
			Asset.GetAsset();
		}
	}

	TArray<UObject*> Objs;
	GetObjectsOfClass(BlockClass, Objs);

	for (TObjectIterator<UClass> It; It; ++It)
	{
		if (It->IsChildOf(UBlock::StaticClass()) && !It->HasAnyClassFlags(CLASS_Abstract) && !It->GetName().StartsWith(TEXT("SKEL_")) && !It->GetName().StartsWith(TEXT("REINST_")))
		{
			BlockRegistry.AddUnique(*It);
		}
	}
	/*
	for (TObjectIterator<UBlueprintGeneratedClass> It; It; ++It)
	{
		if (It->IsChildOf(UBlueprintBlock::StaticClass()) && !It->GetName().StartsWith(TEXT("SKEL_")))
		{
			UE_LOG(LogIVoxel, Warning, TEXT("%s %s"), *It->GetName(), *It->GetClass()->GetName());
			BlockRegistry.AddUnique(*It);
		}
	}*/
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
		InstancePtrRaw = Inst.Get();
		return Inst;
	}
}

FBlockRegistryInstance* FBlockRegistry::GetInstance_Ptr()
{
	checkf(InstancePtr.IsValid(), TEXT("GetInstance_Ptr called with no reference"));
	return InstancePtrRaw;
}
