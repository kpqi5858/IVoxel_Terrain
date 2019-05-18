#pragma once

#include "CoreMinimal.h"
#include "VoxelData.h"

#include "Block.generated.h"

//Represents "Block"'s property, features, etc
UCLASS(Blueprintable, abstract)
class UBlock : public UObject
{
	GENERATED_BODY()
public:
	bool IsOpaque();
	bool IsFaceVisible(EBlockFace Face);
};