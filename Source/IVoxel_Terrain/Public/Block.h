#pragma once

#include "CoreMinimal.h"
#include "VoxelData.h"
#include "Materials/MaterialInstance.h"

#include "Block.generated.h"

enum class EBlockFace : uint8;

//Represents Block's property, features, etc
UCLASS(Abstract)
class UBlock : public UObject
{
	GENERATED_BODY()
public:
	UBlock()
	{

	};

	UPROPERTY(EditDefaultsOnly)
	FName RegistryName;

	//If this is nullptr, the default engine material is used
	virtual UMaterialInterface* GetMaterial()
	{
		return nullptr;
	};

	//0 : Invisible, like Air block
	//1~: Different opaque type makes face visible
	virtual int OpaqueType()
	{
		return false;
	};

	//Is this block has collision?
	virtual bool IsSolid()
	{
		return false;
	};
};

UCLASS()
class UAirBlock : public UBlock
{
	GENERATED_BODY()
public:
	UAirBlock()
	{
		RegistryName = FName(TEXT("Air"));
	};

	virtual UMaterialInterface* GetMaterial() override
	{
		return nullptr;
	}

	virtual int OpaqueType() override { return 0; };
	virtual bool IsSolid() override { return false; };
};

UCLASS()
class UDefaultSolidBlock : public UBlock
{
	GENERATED_BODY()
public:
	UDefaultSolidBlock()
	{
		RegistryName = FName(TEXT("SolidDefault"));
	};

	virtual UMaterialInterface* GetMaterial() override
	{
		return nullptr;
	}

	virtual int OpaqueType() override { return 1; };
	virtual bool IsSolid() override { return false; };
};

UCLASS(Blueprintable, Abstract)
class UBlueprintBlock : public UBlock
{
	GENERATED_BODY()
public:
	UBlueprintBlock()
	{};

	UPROPERTY(EditAnywhere)
	UMaterialInterface* MaterialToUse;

	UPROPERTY(EditAnywhere)
	int OpaqueTypeBP = 0;

	UPROPERTY(EditAnywhere)
	bool bSolid = false;

	virtual UMaterialInterface* GetMaterial() override
	{
		return MaterialToUse;
	}

	
	virtual int OpaqueType() override { return OpaqueTypeBP; };
	virtual bool IsSolid() override { return bSolid; };
};