#include "OCLUtility.h"
#include "CoreMinimal.h"
#include <string>

TArray<uint8> FOCLUtility::FStringToCharArray(const FString& InString)
{
	TArray<uint8> CharArray;
	CharArray.AddUninitialized(InString.Len());

	for (int i=0; i<CharArray.Num();i++)
	{
		CharArray[i] = InString[i];
	}

	return CharArray;
}
