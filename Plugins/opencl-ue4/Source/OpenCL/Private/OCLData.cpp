#include "OCLData.h"

FString FOpenCLDeviceData::ToPrintString()
{
	FString Result;
	Result += FString::Printf(TEXT("Device %s\n"), *DeviceName);
	Result += FString::Printf(TEXT("    Platform:         %s\n"), *Platform);
	Result += FString::Printf(TEXT("    Hardware version: %s\n"), *HardwareVersion);
	Result += FString::Printf(TEXT("    Software version: %s\n"), *SoftwareVersion);
	Result += FString::Printf(TEXT("    OpenCL C version: %s\n"), *OpenCLVersion);
	Result += FString::Printf(TEXT("    Parallel compute units: %d\n"), ParallelComputeUnits);
	return Result;
}
