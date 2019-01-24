#pragma once

#include "CoreMinimal.h"
#include "CoreUObject.h"
#include "OCLData.generated.h"

USTRUCT(BlueprintType)
struct OPENCL_API FOpenCLDeviceData
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "OpenCL Device Data")
	FString DeviceName;

	UPROPERTY(BlueprintReadOnly, Category = "OpenCL Device Data")
	FString DeviceId;

	//used to actually run computation
	void* RawDeviceId;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "OpenCL Device Data")
	FString Platform;

	UPROPERTY(BlueprintReadOnly, Category = "OpenCL Device Data")
	FString HardwareVersion;

	UPROPERTY(BlueprintReadOnly, Category = "OpenCL Device Data")
	FString SoftwareVersion;

	UPROPERTY(BlueprintReadOnly, Category = "OpenCL Device Data")
	FString OpenCLVersion;

	UPROPERTY(BlueprintReadOnly, Category = "OpenCL Device Data")
	int32 ParallelComputeUnits;

	FString ToPrintString();
};