#pragma once

#include "CoreMinimal.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"


struct FOctreeData
{
	bool Value;
	FColor Color;

	FOctreeData() : Value(false), Color(FColor(0))
	{ };
};

class FOctree
{
private:
	FOctreeData Data;
public:
	FOctree(FIntVector Position, uint8 Depth, FOctree* Mother);
	~FOctree();

	/*
	* Childs Location
	* 01 Below 45
	* 23 Below 67
	*/
	FOctree* Mother;
	FOctree* Childs[8];
	bool HasChilds;
	const uint8 Depth;
	const FIntVector Position;

	//Whatever this octree is not actually block, But used for LOD
	bool IsFake;

	void Destroy();
	void DestroyChilds();
	void Subdivide();
	int Size() const;
	bool IsInOctree(FVector Location);

	FOctree* GetOctree(FVector Location, uint8 MaxDepth = 0);
	FOctree* GetChildOctree(FVector Location);
	void GetChildOctrees(TSet<FOctree*> &RetValue, uint8 MaxDepth = 0);

	void SubdivideToZero();
	void TestRender(UWorld* world, FVector Offset);
	void OptimizeOrMakeLod();

	void SetValue(bool SetValue);
	void SetColor(FColor SetValue);
	bool GetValue();
	FColor GetColor();
};