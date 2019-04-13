#include "IVoxel_Thread.h"

IVoxel_PolygonizerThread::IVoxel_PolygonizerThread(AIVoxel_Chunk* Chunk, FIntVector ChunkPos, TSharedPtr<IVoxel_Polygonizer> Polygonizer)
	: Chunk(Chunk), Polygonizer(Polygonizer), ChunkPos(ChunkPos)
{
	static uint64 UniqueID = 0;
	ThreadUniqueID = UniqueID;
	UniqueID++;

	Chunk->DoingThreadedJob.Increment();
}

IVoxel_PolygonizerThread::~IVoxel_PolygonizerThread()
{

}

//Possible error : Two different thread points toward same node chunk.
void IVoxel_PolygonizerThread::DoThreadedWork()
{
	check(Polygonizer.IsValid());

	auto CompPtr = Chunk->LoadedLeaves.Find(ChunkPos);
	if (!CompPtr) //Unloaded during jobs
	{
		delete this;
		return;
	}
	auto Comp = *CompPtr;

	auto PData = new IVoxel_PolygonizedData();

	if (!Polygonizer->Polygonize(*PData))
	{
		UE_LOG(LogIVoxel, Error, TEXT("Cannot polygonize thread"));
	}

	Comp->SetPolygonizedData(this, PData);

	Chunk->DoingThreadedJob.Decrement();
	delete this;
}

void IVoxel_PolygonizerThread::Abandon()
{
	delete this;
}
