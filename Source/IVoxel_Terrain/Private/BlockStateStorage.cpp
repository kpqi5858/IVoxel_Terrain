#include "BlockStateStorage.h"

FBasicBlockStateStorage::FBasicBlockStateStorage()
{

}

FBasicBlockStateStorage::~FBasicBlockStateStorage()
{
}

void FBasicBlockStateStorage::Initialize(AVoxelChunk* Chunk)
{
	TheChunk = Chunk;
	InternalStorage = new FBlockState[VOX_CHUNKSIZE_ARRAY];
}

FBlockState* FBasicBlockStateStorage::operator[](FBlockPos Pos)
{
	return &InternalStorage[Pos.ArrayIndex()];
}

void FBasicBlockStateStorage::Lock()
{
	CriticalSection.Lock();
}

void FBasicBlockStateStorage::UnLock()
{
	CriticalSection.Unlock();
}

void FBasicBlockStateStorage::Save(FArchive* Archive)
{
}

void FBasicBlockStateStorage::Load(FArchive* Archive)
{
}
