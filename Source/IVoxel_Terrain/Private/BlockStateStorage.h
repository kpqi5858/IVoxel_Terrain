#pragma once

#include "VoxelWorld.h"
#include "VoxelData.h"
#include "BlockState.h"

//First parameter is the index of array
//You can get X,Y,Z from it
//Returned void pointer will be casted to T*
typedef TFunction<void*(int)> FStorageCustomInitializer;

template<typename T>
class TAbstractBlockStorage
{
public:
	TAbstractBlockStorage()
	{
		
	}
	virtual ~TAbstractBlockStorage()
	{

	};

public:
	virtual void Initialize(FStorageCustomInitializer Initializer) = 0;
	//Initializes with default constructor
	virtual void Initialize() = 0;

	//Gets data
	virtual T* Get(int Index) = 0;

	virtual void Lock() = 0;
	virtual void UnLock() = 0;

	virtual void Save(FArchive* Archive) = 0;
	virtual void Load(FArchive* Archive) = 0;
};

template<typename T>
class TBasicAbstractBlockStorage : public TAbstractBlockStorage<T>
{
protected:
	FCriticalSection CriticalSection;

	T** InternalStorage = nullptr;

public:
	TBasicAbstractBlockStorage()
	{
	}
	virtual ~TBasicAbstractBlockStorage() override
	{
		if (InternalStorage)
		{
			for (int i = 0; i < VOX_CHUNKSIZE_ARRAY; i++)
			{
				delete InternalStorage[i];
			}
			delete[] InternalStorage;
		}
	}

	virtual void Initialize(FStorageCustomInitializer CustomInitializer)
	{
		checkf(!InternalStorage, TEXT("Storage already initialized"));

		InternalStorage = new T*[VOX_CHUNKSIZE_ARRAY];
		FMemory::Memset(InternalStorage, 0, sizeof(T*)*VOX_CHUNKSIZE_ARRAY);

		for (int i = 0; i < VOX_CHUNKSIZE_ARRAY; i++)
		{
			T* Ptr = reinterpret_cast<T*>(CustomInitializer(i));
			check(Ptr);
			InternalStorage[i] = Ptr;
		}
	}
	virtual void Initialize()
	{
		checkf(!InternalStorage, TEXT("Storage already initialized"));

		InternalStorage = new T*[VOX_CHUNKSIZE_ARRAY];
		FMemory::Memset(InternalStorage, 0, sizeof(T*)*VOX_CHUNKSIZE_ARRAY);

		for (int i = 0; i < VOX_CHUNKSIZE_ARRAY; i++)
		{
			InternalStorage[i] = new T;
		}
	}

	virtual T* Get(int Index) override
	{
		checkf(InternalStorage, TEXT("Storage not initialized"));
		checkf(Index >= 0 && Index < VOX_CHUNKSIZE_ARRAY, TEXT("Out of index : %d"), Index);
		return InternalStorage[Index];
	}

	virtual void Lock() override
	{
		CriticalSection.Lock();
	}
	virtual void UnLock() override
	{
		CriticalSection.Unlock();
	}

	//Unimplemented
	virtual void Save(FArchive* Archive) override
	{
	}
	//Unimplemented
	virtual void Load(FArchive* Archive) override
	{
	}
};
