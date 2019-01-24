// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

DECLARE_LOG_CATEGORY_EXTERN(LogIVoxel, Log, All)

//Constants

#define IVOX_CHUNKDATASIZE 16

static_assert(IVOX_CHUNKDATASIZE % 2 == 0, "IVOX_CHUNKDATASIZE must be even");

//Constants based on constants

//Half of the IVOX_CHUNKDATASIZE

#define IVOX_HALFCHUNKDATASIZE IVOX_CHUNKDATASIZE / 2

//Array size of chunk's data
#define IVOX_CHUMKDATAARRAYSIZE IVOX_CHUNKDATASIZE*IVOX_CHUNKDATASIZE*IVOX_CHUNKDATASIZE