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

	virtual bool IsOpaque()
	{
		return false;
	};

	virtual bool IsSolid()
	{
		return false;
	};

	virtual bool IsFaceVisible(EBlockFace Face)
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

	virtual bool IsOpaque() override { return false; };
	virtual bool IsSolid() override { return false; };
	virtual bool IsFaceVisible(EBlockFace Face) override { return false; };
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

	virtual bool IsOpaque() override { return true; };
	virtual bool IsSolid() override { return true; };
	virtual bool IsFaceVisible(EBlockFace Face) override { return true; };
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

	virtual UMaterialInterface* GetMaterial() override
	{
		return MaterialToUse;
	}

	UFUNCTION(BlueprintNativeEvent)
	bool IsOpaque_B();
	bool IsOpaque_B_Implementation()
	{
		return false;
	};
	UFUNCTION(BlueprintNativeEvent)
	bool IsSolid_B();
	bool IsSolid_B_Implementation()
	{
		return false;
	};
	UFUNCTION(BlueprintNativeEvent)
	bool IsFaceVisible_B(EBlockFace Face);
	bool IsFaceVisible_B_Implementation(EBlockFace Face)
	{
		return false;
	};

	virtual bool IsOpaque() override { return IsOpaque_B(); };
	virtual bool IsSolid() override { return IsSolid_B(); };
	virtual bool IsFaceVisible(EBlockFace Face) override { return IsFaceVisible_B(Face); };
};