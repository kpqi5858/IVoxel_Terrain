#pragma once

#include "CoreMinimal.h"
#include "Runtime/Engine/Classes/Components/ActorComponent.h"
#include "OCLData.h"
#include "DirectoryWatcherModule.h"
#include "IDirectoryWatcher.h"
#include "OpenCLComponent.generated.h"

UENUM(BlueprintType)
enum EKernelFileChangeAction
{
	FCA_Unknown,
	FCA_Added,
	FCA_Modified,
	FCA_Removed
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOCLWatchSignature, const FString&, FileChanged, EKernelFileChangeAction, ChangeType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOCLResultSignature, const FString& , Result, bool, Success);

UCLASS(ClassGroup = "Computing", meta = (BlueprintSpawnableComponent))
class OPENCL_API UOpenCLComponent : public UActorComponent
{
	GENERATED_UCLASS_BODY()
public:

	UPROPERTY(BlueprintAssignable, Category = "OpenCL Events")
	FOCLResultSignature OnResult;

	UPROPERTY(BlueprintAssignable, Category = "OpenCL Events")
	FOCLWatchSignature OnKernelSourceChanged;

	/** Devices specified here will be the ones that will be used when running RunOpenCLKernel*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "OpenCL Properties")
	TArray<FOpenCLDeviceData> DeviceGroup;

	/**Should we watch Content/Kernels on startup?*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "OpenCL Properties")
	bool bWatchKernelsFolderOnStartup;

	/** Amount of additional delay before trying to read the watched kernel file*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "OpenCL Properties")
	float WatchedDiskReadDelay;

	/** How long we should lock out notifications after a change*/
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "OpenCL Properties")
	float WatchedNotificationLockout;

	UFUNCTION(BlueprintPure, Category = "OpenCL Functions")
	bool IsWatchingFolders();

	/** Did we enumerate at least one valid opencl device?*/
	UFUNCTION(BlueprintCallable, Category = "OpenCL Functions")
	bool HasValidHardware();

	/** Get a list of all the OpenCL device available and the compute capability */
	UFUNCTION(BlueprintCallable, Category = "OpenCL Functions")
	TArray<FOpenCLDeviceData> EnumerateDevices();

	UFUNCTION(BlueprintCallable, Category = "OpenCL Functions")
	bool ReadKernelFromFile(const FString& FilePath, FString& OutKernelSource, bool bIsContentRelative = true);

	
	/** Run specified kernel on the current device group with passed in arguments */
	UFUNCTION(BlueprintCallable, Category = "OpenCL Functions")
	void RunOpenCLKernel(const FString& Kernel, const FString& KernelName = TEXT("main"), const FString& InputArgs = TEXT(""));

	/** Start watching kernel file for changes */
	UFUNCTION(BlueprintCallable, Category = "OpenCL Functions")
	void WatchKernelFolder(const FString& ProjectRelativeFolder = TEXT("Kernels"));

	/** Stop watching kernel file for changes */
	UFUNCTION(BlueprintCallable, Category = "OpenCL Functions")
	void UnwatchKernelFolder(const FString& ProjectRelativeFolder = TEXT("Kernels"));


protected:
	IDirectoryWatcher::FDirectoryChanged Changed;
	FDelegateHandle DelegateHandle;
	TArray<FString> WatchedFolders;
	FDateTime LastWatchEventCall;

	virtual void InitializeComponent() override;
	virtual void UninitializeComponent() override;
};