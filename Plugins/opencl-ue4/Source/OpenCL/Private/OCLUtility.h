#pragma once

class FOCLUtility
{
public:
	/** Correctly convert FStrings to OpenCL char* */
	static TArray<uint8> FStringToCharArray(const FString& InString);
};