#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "IVoxel_Terrain.h"
#include "VoxelChunk.h"
#include "BlockRegistry.h"
#include <chrono>

#include "VoxelWorld.generated.h"

class UVoxelChunk;
class UVoxelWorldGenerator;
class AVoxelChunkRender;
struct FVoxelInvoker;
struct FBlockPos;
class FMyQueuedThreadPool;
class FVoxelChunkLoaderThread;
class IMyQueuedWork;

class FScopeTimer
{
public:
	std::chrono::system_clock::time_point Start;

	double Threshold;

	FScopeTimer(double BreakThreshold)
		: Threshold(BreakThreshold)
	{
		Start = std::chrono::system_clock::now();
	}
	~FScopeTimer()
	{
		std::chrono::duration<double> Dur = std::chrono::system_clock::now() - Start;
		double RealDur = Dur.count();
		if (RealDur > Threshold)
		{
			ensureMsgf(false, TEXT("%f / %f"), Threshold, RealDur);
		}
	}
};

enum class EThreadPoolToUse : uint8
{
	INVALID, WORLDGEN, RENDER
};

struct FVoxelChunkToLoad
{
	const FIntVector IndexToLoad;
	const int Priority;

	FVoxelChunkToLoad(FIntVector Index, int Priority)
		: IndexToLoad(Index), Priority(Priority)
	{
	}

	bool operator<(const FVoxelChunkToLoad& Other)
	{
		return Priority < Other.Priority;
	}
};

UCLASS()
class IVOXEL_TERRAIN_API AVoxelWorld : public AActor
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere)
	FIntVector RenderChunkSize;

	UPROPERTY(EditAnywhere)
	FIntVector PreGenerateChunkSize;
	
	UPROPERTY(EditAnywhere)
	float VoxelSize;

	UPROPERTY(EditAnywhere)
	TSubclassOf<UVoxelWorldGenerator> WorldGeneratorClass;

	//How frequent invokers creating/updating chunks
	UPROPERTY(EditAnywhere)
	int CreateChunkInterval;

	UPROPERTY(EditAnywhere)
	int WorldGeneratorThreads = 2;

	UPROPERTY(EditAnywhere)
	int PolygonizerThreads = 2;

	//How many chunks will updated in tick
	UPROPERTY(EditAnywhere)
	int ChunkUpdateThreshold = 100;

private:
	TMap<FIntVector, UVoxelChunk*> LoadedChunk;

	TSet<UVoxelChunk*> TickListCache;

	TSet<UVoxelChunk*> InactiveChunkList;

	FRWLock LoadedChunkLock;

	TArray<FVoxelInvoker> InvokersList;

	UPROPERTY()
	TArray<AVoxelChunkRender*> FreeRender;

	TArray<AVoxelChunkRender*> WaitingRender;

	TSet<AVoxelChunkRender*> AllRenders;

	UPROPERTY()
	UVoxelWorldGenerator* InstancedWorldGenerator = nullptr;

	bool IsInitialized;

	float VoxelSizeInit;

	long InternalTicks = 0;

	TSharedPtr<FBlockRegistryInstance> RegistryReference;

	FMyQueuedThreadPool* PolygonizerThreadPool;
	FMyQueuedThreadPool* WorldGeneratorThreadPool;
	
	int ChunkUpdateCount = 0;

	FVoxelChunkLoaderThread* ChunkLoaderThread;

	TArray<FIntVector> ChunkToInit;
	int ChunkToInitNum = 9999999;

private:
	AVoxelChunkRender* CreateRenderActor();

	UVoxelChunk* CreateVoxelChunk(FIntVector Index);

	UVoxelChunk* GetChunkFromIndex_Internal(FIntVector Pos, bool DoLock);
public:
	AVoxelWorld();

	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

	void Destroy();

	virtual void Tick(float DeltaSeconds) override;

	void RegisterTickList(UVoxelChunk* Chunk);
	void DeregisterTickList(UVoxelChunk* Chunk);

	void UnloadChunk(UVoxelChunk* Chunk);
	void LoadChunk(UVoxelChunk* Chunk);

	void InitChunkAroundInvoker();

	AVoxelChunkRender* GetFreeRenderActor();
	void FreeRenderActor(AVoxelChunkRender* RenderActor);

	void Initialize();

	UFUNCTION(BlueprintCallable)
	void RegisterInvoker(AActor* Object, bool DoRender);

	float GetVoxelSize();
	UClass* GetWorldGeneratorClass();

	UVoxelWorldGenerator* GetWorldGenerator();

	//It can create chunk
	UVoxelChunk* GetChunkFromIndex(FIntVector Pos, bool DoLock = true);
	UVoxelChunk* GetChunkFromBlockPos(FBlockPos Pos, bool DoLock = true);

	//Maybe faster (Fewer lock)
	void GetChunksFromIndices(TArray<FIntVector>& Pos, TArray<UVoxelChunk*>& Result);

	UFUNCTION(BlueprintCallable)
	FIntVector WorldPosToVoxelPos(FVector Pos);

	bool ShouldChunkRendered(UVoxelChunk* Chunk);
	bool ShouldGenerateWorld(UVoxelChunk* Chunk);

	void QueueJob(IMyQueuedWork* Work, EThreadPoolToUse ThreadPool);

	void QueueChunkInit();

	float GetDistanceToInvoker(UVoxelChunk* Chunk, bool Render);

	bool ShouldUpdateChunk();
};