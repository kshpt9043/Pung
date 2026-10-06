// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "PungCharacter.generated.h"

class UCameraComponent;
class UInputAction;
class UPungAirGunComponent;
struct FInputActionValue;
enum class EPungMatchPhase : uint8;

/**
 *  Pung 플레이어 캐릭터.
 *  이동은 기본 이동과 점프뿐이고 (GDD §4), 그 외 이동은 전부 공기총 넉백으로 한다.
 *  넉백, 리스폰 무적, 마지막 공격자 기록은 서버 권한으로 처리한다.
 */
UCLASS(abstract)
class PUNG_API APungCharacter : public ACharacter
{
	GENERATED_BODY()

	/** 1인칭 팔 메시. 본인에게만 보인다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<USkeletalMeshComponent> FirstPersonMesh;

	/** 1인칭 카메라. 서버의 조준 기준점과 일치하도록 눈높이(BaseEyeHeight)에 둔다. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UCameraComponent> FirstPersonCamera;

	/** 공기총 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UPungAirGunComponent> AirGun;

protected:

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> MouseLookAction;

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> FireAction;

	/** false 면 자기 넉백이 "마지막으로 민 사람" 기록을 덮어쓰지 않는다 (GDD §5.2, 테스트 항목) */
	UPROPERTY(EditAnywhere, Category="Knockback")
	bool bSelfKnockbackOverridesLastAttacker = false;

	/** 넉백 직후 이동 입력이 약해지는 시간. 맞은 사람이 키를 눌러 넉백을 상쇄하지 못하게 한다. (웹 원작 방식) */
	UPROPERTY(EditAnywhere, Category="Knockback", meta=(ClampMin="0", Units="s"))
	float KnockbackControlDuration = 0.45f;

	/** 넉백 직후 이동 입력에 곱하는 배율 */
	UPROPERTY(EditAnywhere, Category="Knockback", meta=(ClampMin="0", ClampMax="1"))
	float KnockbackControlScale = 0.1f;

	/**
	 *  땅에서 폭발을 맞아 떠오른 뒤 이 시간 안에는 점프를 받아준다 (폭발 점프 유예, 웹 원작 방식).
	 *  "발밑 사격 → 점프" 순서로 눌러도 점프가 씹히지 않게 한다.
	 */
	UPROPERTY(EditAnywhere, Category="Knockback", meta=(ClampMin="0", Units="s"))
	float BlastJumpGraceTime = 0.22f;

public:

	APungCharacter(const FObjectInitializer& ObjectInitializer);

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Kill Z 아래로 떨어지거나 Kill Z 볼륨에 들어가면 호출된다. 게임 모드에 사망을 알린 뒤 제거된다. */
	virtual void FellOutOfWorld(const UDamageType& DamageType) override;

	/** 매치 단계가 바뀌었을 때 게임 상태가 호출한다. 모든 머신에서 실행된다. */
	void HandleMatchPhaseChanged(EPungMatchPhase NewPhase);

	/** 폭발 점프 유예 중이면 유예를 소모하고 true. 이동 컴포넌트가 점프할 때 호출한다. */
	bool ConsumeBlastJumpGrace();

	bool IsInBlastJumpGrace() const;

	/** 매치 진행 중일 때만 이동/사격할 수 있다 */
	UFUNCTION(BlueprintPure, Category="Pung")
	bool CanAct() const;

	/** 서버 전용. 현재 속도에 넉백을 더하고, 누가 밀었는지 기록한다. */
	void ApplyKnockback(const FVector& Knockback, AController* InstigatorController);

	/** 서버 전용. 무적을 Duration 초 동안 켜거나 (0 이면 끌 때까지 유지), 끈다. */
	void SetInvulnerable(bool bNewInvulnerable, float Duration = 0.f);

	UFUNCTION(BlueprintPure, Category="Pung")
	bool IsInvulnerable() const { return bInvulnerable; }

	/** 서버 전용. 이 캐릭터를 마지막으로 민 컨트롤러와 그 시각 (월드 시간, 초) */
	AController* GetLastAttacker() const { return LastAttacker.Get(); }
	double GetLastAttackTime() const { return LastAttackTime; }

	USkeletalMeshComponent* GetFirstPersonMesh() const { return FirstPersonMesh; }
	UCameraComponent* GetFirstPersonCamera() const { return FirstPersonCamera; }
	UPungAirGunComponent* GetAirGun() const { return AirGun; }

protected:

	virtual void BeginPlay() override;

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	/** 폭발 점프 유예 중이면 공중에서도 점프할 수 있다 */
	virtual bool CanJumpInternal_Implementation() const override;

	void MoveInput(const FInputActionValue& Value);
	void LookInput(const FInputActionValue& Value);

	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoMove(float Right, float Forward);

	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoAim(float Yaw, float Pitch);

	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpStart();

	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoJumpEnd();

	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoFire();

	/** 넉백에 따른 속도 변화를 실제로 적용한다 (서버, 또는 서버를 따라하는 소유 클라이언트) */
	void LaunchFromKnockback(const FVector& Knockback);

	/** 서버가 적용한 넉백을 소유 클라이언트에서도 똑같이 적용해 이동 예측을 맞춘다 */
	UFUNCTION(Client, Reliable)
	void ClientApplyKnockback(FVector_NetQuantize10 Knockback);

	UFUNCTION()
	void OnRep_Invulnerable();

	/** 연출용 훅: 이 캐릭터가 밀려났을 때. 서버와 소유 클라이언트에서 실행된다. */
	UFUNCTION(BlueprintImplementableEvent, Category="Pung", meta=(DisplayName="On Knocked Back"))
	void BP_OnKnockedBack(const FVector& Knockback);

	/** 연출용 훅: 무적 상태가 바뀌었을 때. 모든 머신에서 실행된다. */
	UFUNCTION(BlueprintImplementableEvent, Category="Pung", meta=(DisplayName="On Invulnerability Changed"))
	void BP_OnInvulnerabilityChanged(bool bNewInvulnerable);

	UPROPERTY(ReplicatedUsing=OnRep_Invulnerable)
	bool bInvulnerable = false;

	FTimerHandle InvulnerabilityTimer;

	TWeakObjectPtr<AController> LastAttacker;

	double LastAttackTime = -1.0e9;

	/** 이 시각(월드 시간)까지 이동 입력이 약해진다. 서버와 소유 클라이언트에서 각자 잰다. */
	double KnockbackControlEndTime = -1.0e9;

	/** 이 시각(월드 시간)까지 공중에서도 점프를 받아준다. 서버와 소유 클라이언트에서 각자 잰다. */
	double BlastJumpGraceEndTime = -1.0e9;
};
