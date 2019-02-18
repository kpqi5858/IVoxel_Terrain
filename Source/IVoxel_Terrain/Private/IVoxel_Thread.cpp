#include "IVoxel_Thread.h"

IVoxel_PolygonizerThread::IVoxel_PolygonizerThread(AIVoxel_Chunk* Chunk, FIntVector ChunkPos, TSharedPtr<IVoxel_Polygonizer> Polygonizer)
	: Chunk(Chunk), Polygonizer(Polygonizer), ChunkPos(ChunkPos)
{
	Chunk->DoingThreadedJob.Increment();
}

IVoxel_PolygonizerThread::~IVoxel_PolygonizerThread()
{

}

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

	Comp->PolygonizedData = IVoxel_PolygonizedData();
	if (!Polygonizer->Polygonize(Comp->PolygonizedData))
	{
		UE_LOG(LogIVoxel, Error, TEXT("Cannot polygonize thread"));
	}
	
	Comp->IsPolygonizeDone = true;

	delete this;
}

void IVoxel_PolygonizerThread::Abandon()
{
	delete this;
}
