#pragma once

#include "VoxelWorld.h"
#include "VoxelData.h"
#include "BlockState.h"

//First parameter is the index of array
//You can get X,Y,Z from it
//Returned void pointer will be casted to T
typedef void* FStorageCustomInitializer(int);

template<typename T>
class TAbstractBlockStorage
{
public:
	TAbstractBlockStorage()
	{
		checkNoEntry();
	}
	virtual ~TAbstractBlockStorage();

public:
	virtual void Initialize(FStorageCustomInitializer Initializer);
	//Initializes with default constructor
	virtual void Initialize();

	//Gets data
	virtual T* operator[](int Index);

	virtual void Lock();
	virtual void UnLock();

	virtual void Save(FArchive* Archive);
	virtual void Load(FArchive* Archive);
};

template<typename T>
class TBasicAbstractBlockStorage : public TAbstractBlockStorage<T>
{
private:
	FCriticalSection CriticalSection;

	T** InternalStorage = nullptr;

public:
	TBasicAbstractBlockStorage();
	virtual ~TBasicAbstractBlockStorage() override;

	virtual void Initialize(FStorageCustomInitializer CustomInitializer);
	virtual void Initialize();

	virtual T* operator[](int Index) override;

	virtual void Lock() override;
	virtual void UnLock() override;

	//Unimplemented
	virtual void Save(FArchive* Archive) override;
	//Unimplemented
	virtual void Load(FArchive* Archive) override;
};
