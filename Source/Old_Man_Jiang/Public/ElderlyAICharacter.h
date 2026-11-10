#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ElderTypes.h"
#include "MonitorableActorInterface.h"
#include "ElderlyAICharacter.generated.h"

class UChildActorComponent;
class USceneComponent;
class UStaticMeshComponent;
class USkeletalMesh;
class UAnimSequence;
class UCameraComponent;
class USpringArmComponent;
class UBlendSpace;
class UCharacterMovementComponent;
class UElderNavigationComponent;
class ULineBatchComponent;

/** Native state/service base for BP_AI. The Blueprint remains the project-facing elderly actor. */
UCLASS(Blueprintable)
class OLD_MAN_JIANG_API AElderlyAICharacter : public ACharacter, public IMonitorableActorInterface
{
	GENERATED_BODY()
public:
	AElderlyAICharacter();
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Elder|Crutch") TObjectPtr<UStaticMeshComponent> CrutchMesh;
	void RefreshCrutchVisibility();
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Elder|AI Navigation") TObjectPtr<UElderNavigationComponent> ElderNavigation;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;

	/** The controller is the source of truth; no second mutable control-mode flag. */
	UFUNCTION(BlueprintPure, Category="Elder|Control") bool IsUnderPlayerControl() const;
	UFUNCTION(BlueprintCallable, Category="Elder|Control") void SetupForAIControl();
	UFUNCTION(BlueprintCallable, Category="Elder|Control") void SetupForPlayerControl();
	UFUNCTION(BlueprintPure, Category="Elder|Movement") bool CanElderMove() const;
	UFUNCTION(BlueprintCallable, Category="Elder|Movement") void SetDesiredMoveSpeed(float Speed);
	/** Actor-local input. ViewYaw is ignored and retained for Blueprint pin compatibility. */
	UFUNCTION(BlueprintCallable, Category="Elder|Movement") void MoveElder(FVector2D Input, float ViewYaw);
	UFUNCTION(BlueprintCallable, Category="Elder|Movement") void StopElderMovement();
	/** Release stops translation; ordinary committed turns finish, wheelchair steering stops immediately. */
	void ReleaseMovementInput();
	void ReportTurnPlaybackProgress(float NormalizedTime);
	void ReportTurnGraphInactive() { bNeedsTurnGraphReset=false; }
	float GetActiveTurnPlayRate() const { return ActiveTurnPlayRate; }
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Elder|Movement", meta=(ClampMin="0")) float DesiredMoveSpeed = 64.f;
	/** Ordinary walking only; the legacy base speed still controls wheelchair travel. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Elder|Movement", meta=(ClampMin="0", ClampMax="2")) float WalkingSpeedScale = 0.8f;
	/** Cadence multiplier for locomotion samples only, not idle, root turns or seated poses. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Elder|Movement", meta=(ClampMin="0.1", ClampMax="3")) float WalkingAnimationRateScale = 1.5f;
	UFUNCTION(BlueprintPure, Category="Elder|Movement") float GetEffectiveMoveSpeed() const;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Elder|Movement", meta=(ClampMin="0")) float TurnRate = 110.f;
	/** Gate ordinary idle starts on heading; Walking steers without stopping. Wheelchair uses continuous steering. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Elder|Turning") bool bTurnBeforeMove = true;
	/** Walking steering interpolation; independent of stationary root animation playback. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Elder|Turning", meta=(ClampMin="0.1", UIMin="2", UIMax="5")) float TurnSpeed = 3.f;
	/** Angular speed ceiling prevents an abrupt reversal even with a large target error. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Elder|Turning", meta=(ClampMin="1", Units="deg/s")) float MaxMovingTurnRate = 90.f;
	/** Held steering axis rotates the seated actor; independent of camera and ordinary root turns. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Elder|Wheelchair", meta=(ClampMin="0", Units="deg/s")) float WheelchairTurnRate = 90.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Elder|Turning", meta=(ClampMin="1", ClampMax="45")) float TurnStartAngle = 10.f;
	/** Coalesce short key overlaps before committing one root-motion turn, for all directions. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Elder|Turning", meta=(ClampMin="0", ClampMax="0.3", Units="s")) float TurnInputGracePeriod = 0.12f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Elder|Movement", meta=(ClampMin="0", Units="s")) float MoveHoldThreshold = 0.2f;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Elder|Movement") FVector2D MoveInput = FVector2D::ZeroVector;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Elder|Movement") float TargetFacingYaw = 0.f;
	/** Legacy read-only field retained for asset compatibility; no longer read or written by movement. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Transient, Category="Elder|Wheelchair", meta=(DeprecatedProperty, DeprecationMessage="Movement now uses current Actor Forward/Right, not a takeover frame.")) float WheelchairControlYaw = 0.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Elder|Turning", meta=(ClampMin="0.1", ClampMax="4")) float TurnAnimationPlayRate = 1.f;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Elder|Turning") TObjectPtr<UBlendSpace> TurnInPlaceBlendSpace;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Elder|Turning") bool bTurningInPlace = false;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Elder|Turning") float TurnBlendPosition = 50.f;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Elder|Turning") float TurnAnimationPhase = 0.f;
	UFUNCTION(BlueprintPure, Category="Elder|Turning") bool ShouldPlayTurnAnimation() const;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Elder|Camera") TObjectPtr<USpringArmComponent> PlayerCameraBoom;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Elder|Camera") TObjectPtr<UCameraComponent> PlayerCamera;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Elder|Identity", meta=(ExposeOnSpawn="true")) FString ElderName;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Elder|Identity", meta=(ExposeOnSpawn="true")) EElderGender Gender = EElderGender::Male;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Elder|Identity") TArray<FString> MaleNames;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Elder|Identity") TArray<FString> FemaleNames;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Elder|State") EElderState CurrentState = EElderState::Standing;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Elder|State") EElderState PreviousState = EElderState::Standing;
	UPROPERTY(BlueprintAssignable, Category="Elder|Events") FOnElderStateChanged OnElderStateChanged;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Elder|Assets") TObjectPtr<USkeletalMesh> MaleMesh;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Elder|Assets") TObjectPtr<USkeletalMesh> FemaleMesh;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Elder|Assets") TObjectPtr<UAnimSequence> StandAnimation;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Elder|Assets") TObjectPtr<UAnimSequence> SitAnimation;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Elder|Assets") TObjectPtr<UAnimSequence> WalkAnimation;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Elder|Assets") TObjectPtr<UAnimSequence> FallAnimation;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Elder|Assets") TObjectPtr<UAnimSequence> FallenAnimation;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Elder|Assets") TObjectPtr<UAnimSequence> GetUpAnimation;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Elder|Assets") TObjectPtr<UAnimSequence> WheelchairIdleAnimation;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Elder|Assets") TObjectPtr<UAnimSequence> WheelchairMoveAnimation;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Elder|State", meta=(ClampMin="0.01")) float DefaultFallingDuration = 0.8f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Elder|Sight") TObjectPtr<USceneComponent> ElderSightOrigin;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Elder|Sight") TObjectPtr<USceneComponent> SightConeScale;
	/** SM_ElderSight_Forward is a rigidly calibrated copy; shared camera geometry is unchanged. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Elder|Sight") FVector SightMeshTip = FVector::ZeroVector;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Elder|Sight") FVector SightMeshBaseCenter = FVector(750.f, 0.f, 0.f);
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Elder|Sight") float SightMeshBaseRadius = 603.305f;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Elder|Sight") TObjectPtr<UStaticMeshComponent> SightConeMesh;
	/** Display-only half angles; independent from the AI Perception configuration. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Elder|Sight|Visualization", meta=(ClampMin="1", ClampMax="85", Units="deg")) float SightDisplayHorizontalHalfAngle = 35.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Elder|Sight|Visualization", meta=(ClampMin="1", ClampMax="85", Units="deg")) float SightDisplayVerticalHalfAngle = 22.f;
	/** Visualization distance only; capped by actual AI sight radius. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Elder|Sight|Visualization", meta=(ClampMin="0", Units="cm")) float SightDisplayDistance = 600.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Elder|Sight|Visualization") FLinearColor SightDisplayColor = FLinearColor(0.55f, 0.85f, 1.f, 1.f);
	/** Visualization origin offset from the head: X forward, Y right, Z up, in cm. Does not change AI perception. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Elder|Sight|Visualization", meta=(DisplayName="Sight Origin Offset (Forward Right Up)", Units="cm")) FVector SightDisplayOriginOffset = FVector(15.f, 0.f, 0.f);
	/** Enables user-facing fall triggers without removing the underlying state system. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Elder|State") bool bEnableFallTrigger = false;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Elder|Sight|Visualization") TObjectPtr<ULineBatchComponent> SightLines;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Elder|Sight") float SightRadius = 1500.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Elder|Sight") float LoseSightRadius = 1800.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Elder|Sight") float PeripheralVisionHalfAngleDegrees = 55.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Elder|Sight") FName SightSocketName = TEXT("head");
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Elder|Wheelchair") TObjectPtr<UChildActorComponent> Wheelchair;
	/** Authored on the current skeleton; the chair follows the socket without legacy capsule offsets. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Elder|Wheelchair") FName WheelchairSocketName = TEXT("Socket_Wheel_Chair");
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Elder|Wheelchair") FTransform WheelchairSocketOffset = FTransform::Identity;
	/** Lift both sitter and attached chair; calibrated to the current socket/model wheel bottoms. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Elder|Wheelchair", meta=(Units="cm")) float WheelchairMeshHeightOffset = 18.15f;
	UFUNCTION(BlueprintCallable, Category="Elder|Wheelchair") void RefreshWheelchairAttachment();

	UFUNCTION(BlueprintCallable, Category="Elder|Identity") void SetElderGender(EElderGender NewGender);
	UFUNCTION(BlueprintCallable, Category="Elder|State") void SetElderState(EElderState NewState);
	UFUNCTION(BlueprintCallable, Category="Elder|State") void SetStanding();
	UFUNCTION(BlueprintCallable, Category="Elder|State") void SetSitting();
	UFUNCTION(BlueprintCallable, Category="Elder|State") void StartWalking();
	UFUNCTION(BlueprintCallable, Category="Elder|State") void StartFalling();
	UFUNCTION(BlueprintCallable, Category="Elder|State") void SetFallen();
	/** Starts the authored non-looping recovery only from Fallen. */
	UFUNCTION(BlueprintCallable, Category="Elder|State") bool StartGettingUp();
	UFUNCTION(BlueprintCallable, Category="Elder|State") void EnterWheelchair();
	UFUNCTION(BlueprintCallable, Category="Elder|State") void ExitWheelchair();
	UFUNCTION(BlueprintCallable, Category="Elder|State") void SetWheelchairMoving(bool bMoving);
	UFUNCTION(BlueprintCallable, Category="Elder|Sight") void SetSightConeVisible(bool bVisible);
	UFUNCTION(BlueprintCallable, Category="Elder|Sight") void ToggleSightCone();
	UFUNCTION(BlueprintCallable, Category="Elder|Sight") void RefreshSightCone();
	virtual FString GetMonitorName_Implementation() const override;
	virtual EElderState GetMonitorState_Implementation() const override;
	virtual FString GetMonitorStateString_Implementation() const override;

	UFUNCTION(BlueprintImplementableEvent, Category="Elder|Animation") void ApplyElderStateVisuals(EElderState NewState, EElderState OldState);
private:
	FTimerHandle SightLinesTimer;
	void UpdateSightLines();
	float AppliedWheelchairMeshHeight = 0.f;
	FTimerHandle FallingTimer;
	FTimerHandle GettingUpTimer;
	void EnsureName();
	void FinishFalling();
	void FinishGettingUp();
	void UpdateMovementForState();
	void ConfigureWheelchairVisual(AActor* Chair);
	void UpdateSightHeading();
	FDelegateHandle SightPoseDelegate;
	void BeginInputTurn(float TargetYaw);
	void CancelInputTurn();
	FTransform AdjustTurnRootMotion(const FTransform& Delta, UCharacterMovementComponent* Movement, float DeltaSeconds);
	float TurnRootYawScale = 1.f;
	float TurnStartYaw = 0.f;
	float TurnYawDelta = 0.f;
	float TurnDuration = 1.f;
	float TurnElapsed = 0.f;
	float ActiveTurnPlayRate = 1.f;
	bool bRootMotionTurn = false;
	bool bNeedsTurnGraphReset = false;
	float InputStartedAt = -1.f;
	float FacingChangedAt = 0.f;
	bool bHasFacingRequest = false;
	// Transient rotation phase only, not a second owner of the elderly state.
	bool bMovingTurn = false;
	void UpdateMovingTurn(float DeltaSeconds);
	bool IsWheelchairState() const;
	void ProcessPendingFacing();
	void StopInputTranslation();
	void ClearManualIntent();
};
