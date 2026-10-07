// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "PungCharacter.generated.h"

class UCameraComponent;
class UInputAction;
class UMaterialInterface;
class UPungAirGunComponent;
class UPungItemComponent;
struct FInputActionValue;
enum class EPungMatchPhase : uint8;

/**
 *  (임시) 몸 외형 바꾸기. 봇 등급을 눈으로 구분하는 디버그용이다. 서버가 정하고 모두에게 복제된다.
 *  정식 스킨은 별도 외형 에셋으로 만든다 (GDD §3.5).
 */
USTRUCT(BlueprintType)
struct FPungBodyLook
{
	GENERATED_BODY()

	/** 몸 메시의 모든 머티리얼 슬롯을 이것으로 바꾼다. 비우면 그대로. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Look")
	TObjectPtr<UMaterialInterface> MaterialOverride;

	/** 켜면 몸 머티리얼의 색 파라미터(TintParameterName)를 BodyTint 로 바꾼다 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Look")
	bool bUseTint = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Look", meta=(EditCondition="bUseTint"))
	FLinearColor BodyTint = FLinearColor(1.f, 0.2f, 0.1f);

	/** 머티리얼의 벡터 파라미터 이름. 머티리얼 인스턴스를 열어 Parameter Groups 에서 확인한다. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Look", meta=(EditCondition="bUseTint"))
	FName TintParameterName = TEXT("Tint");

	bool IsSet() const { return MaterialOverride != nullptr || bUseTint; }
};

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

	/** 주운 아이템 (GDD §3.6) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UPungItemComponent> Items;

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

	/** 사용형 아이템 사용 (원작은 F) */
	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<UInputAction> UseItemAction;

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
	 *  남에게 맞아 공중에 뜨면 착지할 때까지 이동 입력에 곱하는 배율.
	 *  공중 제어(Air Control)로 넉백을 되돌려 버리지 못하게 한다. 봇의 길찾기 이동도 같이 막힌다.
	 */
	UPROPERTY(EditAnywhere, Category="Knockback", meta=(ClampMin="0", ClampMax="1"))
	float KnockedAirborneInputScale = 0.15f;

	/**
	 *  연타 누적 상한. 남에게 맞아 얻는 수평 속도가 "이번 한 방의 수평 속도 × 이 배율" 을 넘지 않는다.
	 *  한 방만 맞으면 영향이 없고, 공중에서 연달아 맞을 때 속도가 끝없이 쌓이는 것만 막는다.
	 *  배율이라 과충전, 펄스처럼 센 한 방도 그대로 살아 있다. 로켓 점프(자기 넉백)에는 적용하지 않는다. 0 이면 상한 없음.
	 */
	UPROPERTY(EditAnywhere, Category="Knockback", meta=(ClampMin="0"))
	float KnockbackStackLimitScale = 1.6f;

	/** 내 폭발(로켓 점프)로 떴을 때도 착지할 때까지 입력을 줄일지. 끄면 로켓 점프 중 공중 제어가 그대로다. */
	UPROPERTY(EditAnywhere, Category="Knockback")
	bool bReduceAirControlOnSelfKnockback = false;

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

	/** 착지하면 넉백 공중 조작 제한과 궤적 측정을 끝낸다 */
	virtual void Landed(const FHitResult& Hit) override;

	/** 매치 단계가 바뀌었을 때 게임 상태가 호출한다. 모든 머신에서 실행된다. */
	void HandleMatchPhaseChanged(EPungMatchPhase NewPhase);

	/** 폭발 점프 유예 중이면 유예를 소모하고 true. 이동 컴포넌트가 점프할 때 호출한다. */
	bool ConsumeBlastJumpGrace();

	bool IsInBlastJumpGrace() const;

	/** 지금 이동 입력에 곱할 배율. 넉백 직후에는 KnockbackControlScale, 평소에는 1. */
	float GetMoveInputScale() const;

	/** 매치 진행 중일 때만 이동/사격할 수 있다 */
	UFUNCTION(BlueprintPure, Category="Pung")
	bool CanAct() const;

	/** 서버 전용. 현재 속도에 넉백을 더하고, 누가 밀었는지 기록한다. */
	void ApplyKnockback(const FVector& Knockback, AController* InstigatorController);

	/** 서버 전용. 무적을 Duration 초 동안 켜거나 (0 이면 끌 때까지 유지), 끈다. */
	void SetInvulnerable(bool bNewInvulnerable, float Duration = 0.f);

	UFUNCTION(BlueprintPure, Category="Pung")
	bool IsInvulnerable() const { return bInvulnerable; }

	/** 서버 전용. (임시) 몸 외형을 바꾼다. 봇 컨트롤러가 빙의할 때 프로필 값으로 호출한다. */
	void SetBodyLook(const FPungBodyLook& NewLook);

	/** 무적 남은 시간 (초). 무적이 아니면 0, 끝나는 시간 없이 켜져 있으면 -1. 모든 머신에서 쓸 수 있다. */
	UFUNCTION(BlueprintPure, Category="Pung")
	float GetInvulnerabilityTimeRemaining() const;

	/** 서버 전용. 이 캐릭터를 마지막으로 민 컨트롤러와 그 시각 (월드 시간, 초) */
	AController* GetLastAttacker() const { return LastAttacker.Get(); }
	double GetLastAttackTime() const { return LastAttackTime; }

	/** 서버 전용 (기록용). 남에게 마지막으로 밀렸을 때 내 위치 */
	FVector GetLastHitLocation() const { return LastHitLocation; }

	/** 서버 전용 (기록용). 최근 Window 초 안에 Attacker 에게 밀린 횟수 */
	int32 CountRecentHitsBy(const AController* Attacker, double Window) const;

	/** 스폰된 시각 (월드 시간, 초) */
	double GetSpawnTime() const { return SpawnTime; }

	USkeletalMeshComponent* GetFirstPersonMesh() const { return FirstPersonMesh; }
	UCameraComponent* GetFirstPersonCamera() const { return FirstPersonCamera; }
	UPungAirGunComponent* GetAirGun() const { return AirGun; }
	UPungItemComponent* GetItems() const { return Items; }

protected:

	virtual void BeginPlay() override;

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	/** 폭발 점프 유예 중이면 공중에서도 점프할 수 있다 */
	virtual bool CanJumpInternal_Implementation() const override;

	/** 궤적 측정 (pung.Debug.Trajectory) 에서 기본 점프도 재기 위함 */
	virtual void OnJumped_Implementation() override;

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

	UFUNCTION(BlueprintCallable, Category="Input")
	virtual void DoUseItem();

	/** 넉백에 따른 속도 변화를 실제로 적용한다 (서버, 또는 서버를 따라하는 소유 클라이언트) */
	void LaunchFromKnockback(const FVector& Knockback, bool bSelf);

	/** 서버가 적용한 넉백을 소유 클라이언트에서도 똑같이 적용해 이동 예측을 맞춘다 */
	UFUNCTION(Client, Reliable)
	void ClientApplyKnockback(FVector_NetQuantize10 Knockback, bool bSelf);

	UFUNCTION()
	void OnRep_Invulnerable();

	UFUNCTION()
	void OnRep_BodyLook();

	/** 몸 메시에 BodyLook 을 적용한다 */
	void ApplyBodyLook();

	/** 연출용 훅: (임시) 몸 외형이 정해졌을 때. 모든 머신에서 실행된다. 머티리얼 대신 BP 에서 직접 꾸밀 때 쓴다. */
	UFUNCTION(BlueprintImplementableEvent, Category="Pung", meta=(DisplayName="On Body Look Changed"))
	void BP_OnBodyLookChanged(const FPungBodyLook& Look);

	UPROPERTY(ReplicatedUsing=OnRep_BodyLook)
	FPungBodyLook BodyLook;

	/** 연출용 훅: 이 캐릭터가 밀려났을 때. 서버와 소유 클라이언트에서 실행된다. */
	UFUNCTION(BlueprintImplementableEvent, Category="Pung", meta=(DisplayName="On Knocked Back"))
	void BP_OnKnockedBack(const FVector& Knockback);

	/** 연출용 훅: 무적 상태가 바뀌었을 때. 모든 머신에서 실행된다. */
	UFUNCTION(BlueprintImplementableEvent, Category="Pung", meta=(DisplayName="On Invulnerability Changed"))
	void BP_OnInvulnerabilityChanged(bool bNewInvulnerable);

	UPROPERTY(ReplicatedUsing=OnRep_Invulnerable)
	bool bInvulnerable = false;

	/** 무적이 끝나는 서버 시각. 끝나는 시간 없이 켜졌거나 무적이 아니면 0. */
	UPROPERTY(Replicated)
	double InvulnerableEndServerTime = 0.0;

	FTimerHandle InvulnerabilityTimer;

	TWeakObjectPtr<AController> LastAttacker;

	double LastAttackTime = -1.0e9;

	FVector LastHitLocation = FVector::ZeroVector;

	/** 남에게 밀린 기록 (누가, 언제). 오래된 것은 지운다. */
	TArray<TPair<TWeakObjectPtr<AController>, double>> RecentHits;

	double SpawnTime = 0.0;

	/** 이 시각(월드 시간)까지 이동 입력이 약해진다. 서버와 소유 클라이언트에서 각자 잰다. */
	double KnockbackControlEndTime = -1.0e9;

	/** 이 시각(월드 시간)까지 공중에서도 점프를 받아준다. 서버와 소유 클라이언트에서 각자 잰다. */
	double BlastJumpGraceEndTime = -1.0e9;

	/** 남에게 맞아 떴고 아직 착지하지 않았다. 서버와 소유 클라이언트에서 각자 관리한다. */
	bool bKnockedAirborne = false;

	// ---- 궤적 측정 (pung.Debug.Trajectory) ----

	/** 궤적 측정을 시작한다 (이미 재는 중이면 새로 시작) */
	void StartTrajectory(const TCHAR* Label);

	/** 궤적 한 점을 찍는다 */
	void SampleTrajectory();

	/** 측정을 끝내고 결과를 화면과 로그에 남긴다 */
	void FinishTrajectory(const TCHAR* Ending);

	FTimerHandle TrajectoryTimer;
	FString TrajectoryLabel;
	FVector TrajectoryStart = FVector::ZeroVector;
	FVector TrajectoryLast = FVector::ZeroVector;
	float TrajectoryMaxZ = 0.f;
	double TrajectoryStartTime = 0.0;
	bool bTrajectoryLeftGround = false;
};
