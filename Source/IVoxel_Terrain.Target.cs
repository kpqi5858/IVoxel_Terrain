// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;
using System.Collections.Generic;

public class IVoxel_TerrainTarget : TargetRules
{
	public IVoxel_TerrainTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;

		ExtraModuleNames.AddRange( new string[] { "IVoxel_Terrain" , "IVoxel" } );
	}
}
