// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

DECLARE_LOG_CATEGORY_EXTERN(LogIVoxel, Log, All)

//Constants

#define VOX_CHUNKSIZE 16
#define VOX_CHUNKSIZE_ARRAY VOX_CHUNKSIZE*VOX_CHUNKSIZE*VOX_CHUNKSIZE

#define VOX_CHUNK_AI(x,y,z) (z*VOX_CHUNKSIZE*VOX_CHUNKSIZE)+(y*VOX_CHUNKSIZE*VOX_CHUNKSIZE)+x