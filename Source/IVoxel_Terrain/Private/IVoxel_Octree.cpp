#include "IVoxel_Octree.h"

FOctree::FOctree()
	: FOctree(FIntVector(0), 0, nullptr)
{
	
}

FOctree::FOctree(FIntVector Position, uint8 Depth, FOctree* Parent)
	: Depth(Depth), Position(Position), HasChilds(false), Data(nullptr), Parent(Parent), IsEditedLastLODMake(false)
{
	check(Depth < 32); //Size() max
}

FOctree::~FOctree()
{
	if (HasChilds) DestroyChilds();
	if (Data) 
		delete[] Data;
}

inline void FOctree::Subdivide()
{
	check(!HasChilds);
	check(Depth > 0);
	int s = Size() / 4;

	/*
	* Childs Location
	* Up     Below
	* 0 1    4 5
	* 2 3    6 7
	*/

	Childs[0] = new FOctree(Position + FIntVector(+s, +s, +s), Depth - 1, this);
	Childs[1] = new FOctree(Position + FIntVector(-s, +s, +s), Depth - 1, this);
	Childs[2] = new FOctree(Position + FIntVector(+s, -s, +s), Depth - 1, this);
	Childs[3] = new FOctree(Position + FIntVector(-s, -s, +s), Depth - 1, this);
	Childs[4] = new FOctree(Position + FIntVector(+s, +s, -s), Depth - 1, this);
	Childs[5] = new FOctree(Position + FIntVector(-s, +s, -s), Depth - 1, this);
	Childs[6] = new FOctree(Position + FIntVector(+s, -s, -s), Depth - 1, this);
	Childs[7] = new FOctree(Position + FIntVector(-s, -s, -s), Depth - 1, this);

	HasChilds = true;
}

//Inaccurate
/*
inline bool FOctree::IsInOctree(FVector Location)
{
	FVector P1 = FVector(Position + FIntVector(Size()/2));
	FVector P2 = FVector(Position - FIntVector(Size()/2));
	return Location.X <= P1.X && Location.X >= P2.X  //X
		&& Location.Y <= P1.Y && Location.Y >= P2.Y  //Y
		&& Location.Z <= P1.Z && Location.Z >= P2.Z; //Z
}
*/

inline bool FOctree::IsInOctree(FIntVector Location)
{
	FIntVector P1 = GetMaximumPosition();
	FIntVector P2 = GetMinimalPosition();
	return Location.X <= P1.X && Location.X >= P2.X  //X
		&& Location.Y <= P1.Y && Location.Y >= P2.Y  //Y
		&& Location.Z <= P1.Z && Location.Z >= P2.Z; //Z
}

int FOctree::Size() const
{
	return SizeFor(Depth);
}

void FOctree::Destroy()
{
	if (HasChilds)
	{
		DestroyChilds();
	}
	delete this;
}

inline void FOctree::DestroyChilds()
{
	check(HasChilds);
	for (auto& child : Childs)
	{
		child->Destroy();
		child = nullptr; //Is this necessery?
	}
	HasChilds = false;
}

FOctree* FOctree::GetOctree(FIntVector Location, uint8 MaxDepth)
{
	check(IsInOctree(Location));

	if (Depth > MaxDepth)
	{
		if (!HasChilds) Subdivide();
		return GetChildOctree(Location)->GetOctree(Location, MaxDepth);
	}
	else
	{
		return this;
	}
}

FOctree* FOctree::GetOctree_NoSub(FIntVector Location)
{
	check(IsInOctree(Location));

	if (HasChilds)
	{
		return GetChildOctree(Location)->GetOctree_NoSub(Location);
	}
	else
	{
		return this;
	}
}

FOctree* FOctree::GetOctreeExact(FIntVector Location)
{
	check(IsInOctree(Location));

	//Wrong location paramter, or should not happen
	if (Depth == 0 && Location != Position) check(false);

	if (Location == Position) return this;
	else if (HasChilds)
	{
		return GetChildOctree(Location)->GetOctreeExact(Location);
	}
	else return nullptr; //Return nullptr if no exact octree found
}

/*
* Childs Location
* Up     Below
* 0 1    4 5
* 2 3    6 7
*/
FOctree* FOctree::GetChildOctree(FIntVector Location)
{
	check(HasChilds);
	check(IsInOctree(Location));

	int Result = 0;
	FIntVector Center = Position;
	Result = Result | (Center.X > Location.X ? 1 : 0); //X
	Result = Result | (Center.Y > Location.Y ? 2 : 0); //Y
	Result = Result | (Center.Z > Location.Z ? 4 : 0); //Z
	return Childs[Result];
}

void FOctree::GetChildOctrees(TSet<FOctree*>& RetValue, uint8 MaxDepth)
{
	if (Depth < MaxDepth) return;

	RetValue.Add(this);
	if (!HasChilds) return;
	for (auto& child : Childs)
	{
		child->GetChildOctrees(RetValue, MaxDepth);
	}
}

void FOctree::GetChildOctrees_NoChild(TSet<FOctree*> &RetValue)
{
	if (!HasChilds)
	{
		RetValue.Add(this);
	}
	else
	{
		for (auto& child : Childs)
		{
			child->GetChildOctrees_NoChild(RetValue);
		}
	}
}

void FOctree::GetData(FIntVector OctreeLoc, FVector Offset, uint8 TargetDepth, UIVoxel_WorldGenerator* WorldGen, FIVoxel_BlockData* OutArray)
{
	check(IsInOctree(OctreeLoc));

	if (Position == OctreeLoc)
	{
		if (HasLODData())
		{
			check(Depth > 0);
			FMemory::Memcpy(OutArray, Data, sizeof(FIVoxel_BlockData) * IVOX_CHUMKDATAARRAYSIZE);
		}
		else if (Depth == 0 && Data)
		{
			FMemory::Memcpy(OutArray, Data, sizeof(FIVoxel_BlockData) * IVOX_CHUMKDATAARRAYSIZE);
		}
		else
		{
			FillArrayWithWorldGenerator(OctreeLoc, TargetDepth, WorldGen, OutArray, Offset);
		}
	}
	else
	{
		FOctree* ExactNode = GetOctreeExact(OctreeLoc);
		if (ExactNode)
		{
			ExactNode->GetData(OctreeLoc, Offset, TargetDepth, WorldGen, OutArray);
			return;
		}
		else
		{
			FillArrayWithWorldGenerator(OctreeLoc, TargetDepth, WorldGen, OutArray, Offset);
		}
	}
}

void FOctree::SetData(FIntVector LocalPos, FIVoxel_BlockData Block, FVector Offset, UIVoxel_WorldGenerator* WorldGen)
{
	check(Depth == 0);

	if (!Data)
	{
		InitDataWithWorldGen(WorldGen, Offset);
	}
	Data[IndexFor(LocalPos)] = Block;
	IsEditedLastLODMake = true;
}

void FOctree::MakeLOD(UIVoxel_WorldGenerator* WorldGen, FVector Offset, bool bBlendData)
{
	if (HasChilds)
	{
		check(Depth > 0);

		if (!Data)
		{
			InitDataDefault();
		}

		int HalfDataSize = IVOX_CHUNKDATASIZE / 2;

		for (int X = 0; X < IVOX_CHUNKDATASIZE; X++)
		for (int Y = 0; Y < IVOX_CHUNKDATASIZE; Y++)
		for (int Z = 0; Z < IVOX_CHUNKDATASIZE; Z++)
		{
			int XSign = X >= HalfDataSize ? 1 : -1;
			int YSign = Y >= HalfDataSize ? 1 : -1;
			int ZSign = Z >= HalfDataSize ? 1 : -1;

			FOctree* Child = GetChildOctree(Position + FIntVector(XSign, YSign, ZSign));

			if (!Child->IsEditedLastLODMake) continue;
			if (bBlendData)
			{
				if (Child->Data)
				{

				}
				else
				{
					FVector GenPos = GetWorldGenPos(Position, FIntVector(X, Y, Z), Depth, Offset);
					Data[IndexFor(X, Y, Z)] = WorldGen->GetBlockData(GenPos.X, GenPos.Y, GenPos.Z);
				}
			}
			else
			{
				if (Child->Data)
				{
					Data[IndexFor(X, Y, Z)] = Child->Data[IndexFor(X * 2 % IVOX_CHUNKDATASIZE, Y * 2 % IVOX_CHUNKDATASIZE, Z * 2 % IVOX_CHUNKDATASIZE)];
				}
				else
				{
					FVector GenPos = GetWorldGenPos(Position, FIntVector(X, Y, Z), Depth, Offset);
					Data[IndexFor(X, Y, Z)] = WorldGen->GetBlockData(GenPos.X, GenPos.Y, GenPos.Z);
				}
			}
		}

		for (auto& child : Childs)
		{
			child->IsEditedLastLODMake = false;
		}

		IsEditedLastLODMake = true;
	}
	if (Parent)
	{
		Parent->MakeLOD(WorldGen, Offset, bBlendData);
	}
}

void FOctree::LodSubdivide(AIVoxel_Chunk* Chunk)
{
	uint8 RequiredDepth = Chunk->GetLodFor(this);
	if (Depth > RequiredDepth)
	{
		Subdivide();
		for (auto& Child : Childs)
		{
			Child->LodSubdivide(Chunk);
		}
	}
	else if (Depth < RequiredDepth)
	{
		if (HasChilds) DestroyChilds();
	}
}

void FOctree::GetChildOctreesIntersect(FIntVector Target, TSet<FOctree*>& RetValue)
{
	if (IsInOctree(Target))
	{
		if (!HasChilds) RetValue.Add(this);
		else
		{
			for (auto& child : Childs)
			{
				child->GetChildOctreesIntersect(Target, RetValue);
			}
		}
	}
}

inline FIntVector FOctree::GetMinimalPosition()
{
	return GetMinimalPosition(Position, Depth);
}

inline FIntVector FOctree::GetMaximumPosition()
{
	return GetMaximumPosition(Position, Depth);
}

FVector FOctree::GetMinimalPosition_World()
{
	return GetMinimalPosition_World(Position, Depth);
}

FVector FOctree::GetMaximumPosition_World()
{
	return GetMaximumPosition_World(Position, Depth);
}

FVector FOctree::GetPosition_World()
{
	return GetPosition_World(Position, Depth);
}
inline void FOctree::InitDataWithWorldGen(UIVoxel_WorldGenerator* WorldGen, FVector Offset)
{
	InitDataInternal();
	FillArrayWithWorldGenerator(Position, Depth, WorldGen, Data, Offset);
}

void FOctree::InitDataDefault()
{
	InitDataInternal();
}

inline void FOctree::InitDataInternal()
{
	check(!Data);
	Data = new FIVoxel_BlockData[IVOX_CHUMKDATAARRAYSIZE];
}

void FOctree::DebugRender(UWorld* World)
{
	DrawDebugBox(World, FVector(Position)*2, FVector(Size()), FColor(1, 0, 0), false, 1);
	if (HasChilds)
	{
		for (auto& child : Childs)
		{
			child->DebugRender(World);
		}
	}
}