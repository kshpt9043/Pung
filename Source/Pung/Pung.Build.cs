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

		// 온라인 세션 (Steam). 공개 헤더(PungSessionSubsystem.h)가 OSS 타입을 쓰므로 Public 에 둔다.
		// 실제 Steam 연동은 .uproject 플러그인과 DefaultEngine.ini 설정으로 고른다.
		PublicDependencyModuleNames.AddRange(new string[] { "OnlineSubsystem", "OnlineSubsystemUtils" });

		// 봇 (AIController, BT 노드, NavMesh 조회). 공개 헤더가 BT 타입을 쓰므로 Public 에 둔다.
		PublicDependencyModuleNames.AddRange(new string[] { "AIModule", "GameplayTasks", "NavigationSystem" });

		PrivateDependencyModuleNames.AddRange(new string[] { "EngineSettings" });
	}
}
