#include "BlockStateStorage.h"


template<typename T>
inline TBasicAbstractBlockStorage<T>::TBasicAbstractBlockStorage()
{
}

template<typename T>
inline TBasicAbstractBlockStorage<T>::~TBasicAbstractBlockStorage()
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

template<typename T>
void TBasicAbstractBlockStorage<T>::Initialize()
{
	checkf(!InternalStorage, "Storage already initialized");

	InternalStorage = new T*[VOX_CHUNKSIZE_ARRAY];
	FMemory::Memset(InternalStorage, 0, sizeof(T*)*VOX_CHUNKSIZE_ARRAY);

	for (int i = 0; i < VOX_CHUNKSIZE_ARRAY; i++)
	{
		InternalStorage[i] = new T;
	}
}

template<typename T>
void TBasicAbstractBlockStorage<T>::Initialize(FStorageCustomInitializer CustomInitializer)
{
	checkf(!InternalStorage, "Storage already initialized");

	InternalStorage = new T*[VOX_CHUNKSIZE_ARRAY];
	FMemory::Memset(InternalStorage, 0, sizeof(T*)*VOX_CHUNKSIZE_ARRAY);

	for (int i = 0; i < VOX_CHUNKSIZE_ARRAY; i++)
	{
		T* Ptr = reinterpret_cast<T*>(CustomInitializer(i));
		check(Ptr);
		InternalStorage[i] = Ptr;
	}
}

template<typename T>
T* TBasicAbstractBlockStorage<T>::operator[](int Index)
{
	checkf(InternalStorage, "Storage not initialized");
	checkf(Index >= 0 && Index < VOX_CHUNKSIZE_ARRAY, "Out of index : %d", Index);
	return InternalStorage[Index];
}

template<typename T>
inline void TBasicAbstractBlockStorage<T>::Lock()
{
	CriticalSection.Lock();
}

template<typename T>
inline void TBasicAbstractBlockStorage<T>::UnLock()
{
	CriticalSection.Unlock();
}

template<typename T>
inline void TBasicAbstractBlockStorage<T>::Save(FArchive* Archive)
{
}

template<typename T>
inline void TBasicAbstractBlockStorage<T>::Load(FArchive* Archive)
{
}