// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;
using System.Collections.Generic;

public class IVoxel_TerrainEditorTarget : TargetRules
{
	public IVoxel_TerrainEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;

		ExtraModuleNames.AddRange( new string[] { "IVoxel_Terrain" , "IVoxel" } );
	}
}
