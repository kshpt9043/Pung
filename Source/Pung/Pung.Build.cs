// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Pung : ModuleRules
{
	public Pung(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Public/Private 폴더를 쓰면 모듈 루트가 include 경로에서 빠지므로 직접 추가 (Pung.h 용)
		PublicIncludePaths.Add(ModuleDirectory);

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput" });

		PrivateDependencyModuleNames.AddRange(new string[] { });

		// 온라인 기능(Steam 등)을 쓸 때 주석 해제
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");
	}
}
