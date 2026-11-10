#include "ElderlyAICharacter.h"
#include "ElderlyAIController.h"
#include "ElderNavigationComponent.h"

#include "Components/ChildActorComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/LineBatchComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimSequence.h"
#include "Engine/StaticMesh.h"
#include "TimerManager.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Animation/BlendSpace.h"

AElderlyAICharacter::AElderlyAICharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	AIControllerClass = AElderlyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AutoPossessPlayer = EAutoReceiveInput::Disabled;
	CrutchMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CrutchMesh"));
	CrutchMesh->SetupAttachment(GetMesh(), TEXT("Crutch"));
	CrutchMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CrutchMesh->SetGenerateOverlapEvents(false);
	CrutchMesh->SetCanEverAffectNavigation(false);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CrutchAsset(TEXT("/Game/Project_File/Model/Crutch/Xingdongfuzhu04_01.Xingdongfuzhu04_01"));
	if (CrutchAsset.Succeeded()) CrutchMesh->SetStaticMesh(CrutchAsset.Object);
	ElderNavigation=CreateDefaultSubobject<UElderNavigationComponent>(TEXT("ElderNavigation"));
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, TurnRate, 0.f);
	GetCharacterMovement()->MaxWalkSpeed = GetEffectiveMoveSpeed();
	PlayerCameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("ElderPlayerCameraBoom"));
	PlayerCameraBoom->SetupAttachment(GetRootComponent());
	PlayerCameraBoom->TargetArmLength = 350.f;
	PlayerCameraBoom->SocketOffset = FVector(0.f, 50.f, 65.f);
	PlayerCameraBoom->bUsePawnControlRotation = true;
	PlayerCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ElderPlayerCamera"));
	PlayerCamera->SetupAttachment(PlayerCameraBoom, USpringArmComponent::SocketName);
	PlayerCamera->bAutoActivate = false;
	MaleNames = { TEXT("王建国"), TEXT("张德福"), TEXT("刘志强"), TEXT("陈国华") };
	FemaleNames = { TEXT("李秀兰"), TEXT("王桂英"), TEXT("张淑芬"), TEXT("陈玉兰") };
	ElderSightOrigin = CreateDefaultSubobject<USceneComponent>(TEXT("ElderSightOrigin"));
	ElderSightOrigin->SetupAttachment(GetMesh());
	SightConeScale = CreateDefaultSubobject<USceneComponent>(TEXT("SightConeScale"));
	SightConeScale->SetupAttachment(ElderSightOrigin);
	SightConeMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SightConeMesh"));
	SightConeMesh->SetupAttachment(SightConeScale);
	SightConeMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SightConeMesh->SetGenerateOverlapEvents(false);
	SightConeMesh->SetCastShadow(false);
	SightConeMesh->SetHiddenInGame(true);
	SightConeMesh->SetVisibility(false);
	SightLines = CreateDefaultSubobject<ULineBatchComponent>(TEXT("SightLines"));
	SightLines->SetupAttachment(GetRootComponent());
	SightLines->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SightLines->SetCastShadow(false);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cone(TEXT("/Game/Project_File/Model/Camera/SM_ElderSight_Forward.SM_ElderSight_Forward"));
	if (Cone.Succeeded()) SightConeMesh->SetStaticMesh(Cone.Object);
	static ConstructorHelpers::FObjectFinder<UBlendSpace> TurnAsset(TEXT("/Game/OldManAnimset/Animations/TurnInPlace.TurnInPlace"));
	if (TurnAsset.Succeeded()) TurnInPlaceBlendSpace = TurnAsset.Object;
	Wheelchair = CreateDefaultSubobject<UChildActorComponent>(TEXT("Wheelchair"));
	Wheelchair->SetupAttachment(GetMesh(), WheelchairSocketName);
	Wheelchair->SetHiddenInGame(true);
	Wheelchair->OnChildActorCreated().AddUObject(this, &AElderlyAICharacter::ConfigureWheelchairVisual);
	static ConstructorHelpers::FClassFinder<AActor> WheelchairClass(TEXT("/Game/Project_File/BP/Actor/BP_WheelChair"));
	if (WheelchairClass.Succeeded()) Wheelchair->SetChildActorClass(WheelchairClass.Class);
}

void AElderlyAICharacter::BeginPlay()
{
	Super::BeginPlay();
	SetDesiredMoveSpeed(DesiredMoveSpeed);
	EnsureName();
	SetElderGender(Gender);
	RefreshSightCone();
	// Preserve the old component's visible/hidden state contract without rendering its rigid mesh.
	SightConeMesh->SetRenderInMainPass(false);
	SightConeMesh->SetRenderInDepthPass(false);
	UpdateMovementForState();
	SightPoseDelegate = GetMesh()->RegisterOnBoneTransformsFinalizedDelegate(FOnBoneTransformsFinalizedMultiCast::FDelegate::CreateUObject(this, &AElderlyAICharacter::UpdateSightHeading));
	GetCharacterMovement()->ProcessRootMotionPreConvertToWorld.BindUObject(this, &AElderlyAICharacter::AdjustTurnRootMotion);
	ConfigureWheelchairVisual(Wheelchair->GetChildActor());
}

void AElderlyAICharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(SightLinesTimer);
	SightLines->Flush();
	GetMesh()->UnregisterOnBoneTransformsFinalizedDelegate(SightPoseDelegate);
	GetCharacterMovement()->ProcessRootMotionPreConvertToWorld.Unbind();
	Super::EndPlay(EndPlayReason);
}

void AElderlyAICharacter::UpdateSightHeading()
{
	// Follow animated head POSITION, but match the pawn heading used by AI Sight.
	// The elderly idle pose bends the head down about 43 degrees; that is not its sight direction.
	ElderSightOrigin->SetWorldRotation(FRotator(0.f, GetActorRotation().Yaw, 0.f));
}

void AElderlyAICharacter::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshWheelchairAttachment();
	ConfigureWheelchairVisual(Wheelchair->GetChildActor());
	RefreshSightCone();
}

void AElderlyAICharacter::RefreshWheelchairAttachment()
{
	RefreshCrutchVisibility();
	if (!Wheelchair || !GetMesh()) return;
	const bool bSeated = CurrentState == EElderState::WheelchairIdle || CurrentState == EElderState::WheelchairMoving;
	if (GetWorld() && GetWorld()->IsGameWorld())
	{
		const float Height = bSeated ? WheelchairMeshHeightOffset : 0.f;
		GetMesh()->AddLocalOffset(FVector(0, 0, Height - AppliedWheelchairMeshHeight));
		AppliedWheelchairMeshHeight = Height;
	}
	if (GetMesh()->DoesSocketExist(WheelchairSocketName))
	{
		Wheelchair->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, WheelchairSocketName);
		Wheelchair->SetRelativeTransform(WheelchairSocketOffset);
	}
	else
	{
		// Never silently display a chair at an unrelated bone when replacing the character mesh.
		Wheelchair->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
		Wheelchair->SetRelativeTransform(FTransform::Identity);
	}
	const bool bShowChair = GetMesh()->DoesSocketExist(WheelchairSocketName) &&
		(CurrentState == EElderState::WheelchairIdle || CurrentState == EElderState::WheelchairMoving);
	Wheelchair->SetHiddenInGame(!bShowChair);
	ConfigureWheelchairVisual(Wheelchair->GetChildActor());
}

void AElderlyAICharacter::RefreshCrutchVisibility()
{
	if (!CrutchMesh || !GetMesh()) return;
	const bool bHide = CurrentState == EElderState::Falling || CurrentState == EElderState::Fallen || CurrentState == EElderState::GettingUp ||
		CurrentState == EElderState::WheelchairIdle || CurrentState == EElderState::WheelchairMoving;
	// Missing sockets must not leave the accessory floating at the mesh origin.
	const bool bShow = !bHide && GetMesh()->DoesSocketExist(TEXT("Crutch"));
	CrutchMesh->SetHiddenInGame(!bShow);
	CrutchMesh->SetVisibility(bShow);
}

void AElderlyAICharacter::ConfigureWheelchairVisual(AActor* Chair)
{
	if (!Chair) return;
	// Only this attached instance is visual. Standalone BP_WheelChair keeps its authored collision.
	Chair->SetActorEnableCollision(false);
	TArray<UPrimitiveComponent*> Primitives;
	Chair->GetComponents(Primitives);
	for (UPrimitiveComponent* Primitive : Primitives) Primitive->SetCanEverAffectNavigation(false);
	Chair->SetActorHiddenInGame(!GetMesh()->DoesSocketExist(WheelchairSocketName) ||
		(CurrentState != EElderState::WheelchairIdle && CurrentState != EElderState::WheelchairMoving));
}

void AElderlyAICharacter::EnsureName()
{
	if (!ElderName.IsEmpty()) return;
	const TArray<FString>& Pool = Gender == EElderGender::Male ? MaleNames : FemaleNames;
	ElderName = Pool.IsEmpty() ? TEXT("未命名老人") : Pool[FMath::RandHelper(Pool.Num())];
}

void AElderlyAICharacter::SetElderGender(EElderGender NewGender)
{
	Gender = NewGender;
	if (USkeletalMesh* NewMesh = NewGender == EElderGender::Male ? MaleMesh : FemaleMesh) GetMesh()->SetSkeletalMesh(NewMesh);
	RefreshWheelchairAttachment();
	RefreshSightCone();
}

void AElderlyAICharacter::SetElderState(EElderState NewState)
{
	if (CurrentState == NewState) return;
	const bool bWasWheelchair = CurrentState == EElderState::WheelchairIdle || CurrentState == EElderState::WheelchairMoving;
	const bool bNowWheelchair = NewState == EElderState::WheelchairIdle || NewState == EElderState::WheelchairMoving;
	if (bWasWheelchair != bNowWheelchair) ClearManualIntent();
	CancelInputTurn();
	PreviousState = CurrentState;
	CurrentState = NewState;
	UpdateMovementForState();
	RefreshWheelchairAttachment();
	ApplyElderStateVisuals(NewState, PreviousState);
	OnElderStateChanged.Broadcast(this, ElderName, PreviousState, NewState);
	if (NewState == EElderState::Falling)
	{
		const float Duration = FallAnimation ? FMath::Max(0.1f, FallAnimation->GetPlayLength()) : DefaultFallingDuration;
		GetWorldTimerManager().SetTimer(FallingTimer, this, &AElderlyAICharacter::FinishFalling, Duration, false);
	}
	else GetWorldTimerManager().ClearTimer(FallingTimer);
	if (NewState == EElderState::GettingUp)
	{
		const float Duration = GetUpAnimation ? FMath::Max(0.1f, GetUpAnimation->GetPlayLength()) : 0.1f;
		GetWorldTimerManager().SetTimer(GettingUpTimer, this, &AElderlyAICharacter::FinishGettingUp, Duration, false);
	}
	else GetWorldTimerManager().ClearTimer(GettingUpTimer);
}

void AElderlyAICharacter::FinishFalling() { if (CurrentState == EElderState::Falling) SetElderState(EElderState::Fallen); }
void AElderlyAICharacter::FinishGettingUp() { if (CurrentState == EElderState::GettingUp) SetElderState(EElderState::Standing); }
void AElderlyAICharacter::SetStanding() { SetElderState(EElderState::Standing); }
void AElderlyAICharacter::SetSitting() { SetElderState(EElderState::Sitting); }
void AElderlyAICharacter::StartWalking() { SetElderState(EElderState::Walking); }
void AElderlyAICharacter::StartFalling() { SetElderState(EElderState::Falling); }
void AElderlyAICharacter::SetFallen() { SetElderState(EElderState::Fallen); }
bool AElderlyAICharacter::StartGettingUp()
{
	if (CurrentState != EElderState::Fallen || !GetUpAnimation) return false;
	SetElderState(EElderState::GettingUp);
	return true;
}
void AElderlyAICharacter::EnterWheelchair() { SetElderState(EElderState::WheelchairIdle); }
void AElderlyAICharacter::ExitWheelchair() { SetElderState(EElderState::Standing); }
void AElderlyAICharacter::SetWheelchairMoving(bool bMoving) { SetElderState(bMoving ? EElderState::WheelchairMoving : EElderState::WheelchairIdle); }
void AElderlyAICharacter::SetSightConeVisible(bool bVisible)
{
	SightConeMesh->SetHiddenInGame(!bVisible);
	SightConeMesh->SetVisibility(bVisible);
	GetWorldTimerManager().ClearTimer(SightLinesTimer);
	SightLines->Flush();
	if (bVisible && GetWorld() && GetWorld()->IsGameWorld())
	{
		UpdateSightLines();
		GetWorldTimerManager().SetTimer(SightLinesTimer, this, &AElderlyAICharacter::UpdateSightLines, 0.033f, true);
	}
}
void AElderlyAICharacter::ToggleSightCone() { SetSightConeVisible(SightConeMesh->bHiddenInGame); }
void AElderlyAICharacter::UpdateSightLines()
{
	if (!GetWorld() || !ElderSightOrigin || !SightLines || SightConeMesh->bHiddenInGame) return;
	SightLines->Flush();
	const FVector Head = ElderSightOrigin->GetComponentLocation();
	const FRotator Heading = ElderSightOrigin->GetComponentRotation();
	const float H = FMath::Clamp(SightDisplayHorizontalHalfAngle, 1.f, 85.f);
	const float V = FMath::Clamp(SightDisplayVerticalHalfAngle, 1.f, 85.f);
	const float Distance = FMath::Clamp(SightDisplayDistance, 0.f, FMath::Max(0.f, SightRadius));
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ElderSightDisplay), false, this);
	TArray<AActor*> Attached;
	GetAttachedActors(Attached, true, true);
	Params.AddIgnoredActors(Attached);
	if (Wheelchair && Wheelchair->GetChildActor()) Params.AddIgnoredActor(Wheelchair->GetChildActor());
	const FVector DesiredStart = Head + Heading.RotateVector(SightDisplayOriginOffset);
	FHitResult OffsetHit;
	// If the short offset itself is obstructed, do not start rays on or beyond a wall.
	if (GetWorld()->LineTraceSingleByChannel(OffsetHit, Head, DesiredStart, ECC_Visibility, Params)) return;
	const FVector Start = DesiredStart;
	constexpr int32 Samples = 32;
	TArray<FBatchedLine> Lines;
	Lines.Reserve(Samples + 1);
	for (int32 Index = 0; Index <= Samples; ++Index)
	{
		const float A = 2.f * PI * Index / Samples;
		const float Yaw = Index == Samples ? 0.f : H * FMath::Cos(A);
		const float Pitch = Index == Samples ? 0.f : V * FMath::Sin(A);
		const FVector Direction = (Heading.Quaternion() * FRotator(Pitch, Yaw, 0.f).Quaternion()).GetForwardVector();
		const FVector End = Start + Direction * Distance;
		FHitResult Hit;
		const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);
		Lines.Emplace(Start, bHit ? Hit.ImpactPoint : End, SightDisplayColor, 0.1f, 1.f, SDPG_World);
	}
	SightLines->DrawLines(Lines);
}
void AElderlyAICharacter::RefreshSightCone()
{
	if (!GetMesh() || !SightConeScale) return;
	// Reassert the hierarchy for previously saved Blueprint component templates.
	SightConeScale->AttachToComponent(ElderSightOrigin, FAttachmentTransformRules::KeepRelativeTransform);
	SightConeMesh->AttachToComponent(SightConeScale, FAttachmentTransformRules::KeepRelativeTransform);
	if (GetMesh()->DoesSocketExist(SightSocketName))
	{
		ElderSightOrigin->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepRelativeTransform, SightSocketName);
		ElderSightOrigin->SetRelativeLocation(FVector::ZeroVector);
	}
	else
	{
		ElderSightOrigin->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
		ElderSightOrigin->SetRelativeLocation(FVector(0, 0, BaseEyeHeight));
		ElderSightOrigin->SetRelativeRotation(FRotator::ZeroRotator);
	}
	UpdateSightHeading();
	const FVector MeshAxis = SightMeshBaseCenter - SightMeshTip;
	const FQuat Alignment = FQuat::FindBetweenNormals(MeshAxis.GetSafeNormal(), FVector::ForwardVector);
	SightConeMesh->SetRelativeTransform(FTransform(Alignment, -Alignment.RotateVector(SightMeshTip)));
	const float Length = FMath::Max(1.f, SightRadius);
	const float Radius = Length * FMath::Tan(FMath::DegreesToRadians(FMath::Clamp(PeripheralVisionHalfAngleDegrees, 1.f, 85.f)));
	// Scale in the calibrated cone basis, not the model's diagonal import axes.
	SightConeScale->SetRelativeScale3D(FVector(Length / FMath::Max(1.f, MeshAxis.Size()), Radius / FMath::Max(1.f, SightMeshBaseRadius), Radius / FMath::Max(1.f, SightMeshBaseRadius)));
}
void AElderlyAICharacter::UpdateMovementForState()
{
	GetCharacterMovement()->MaxWalkSpeed = GetEffectiveMoveSpeed();
	if (CanElderMove())
	{
		if (GetCharacterMovement()->MovementMode == MOVE_None) GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}
	else
	{
		ClearManualIntent();
		CancelInputTurn();
		ConsumeMovementInputVector();
		GetCharacterMovement()->StopMovementImmediately();
		GetCharacterMovement()->DisableMovement();
	}
}

bool AElderlyAICharacter::CanElderMove() const
{
	return CurrentState == EElderState::Standing || CurrentState == EElderState::Walking ||
		CurrentState == EElderState::WheelchairIdle || CurrentState == EElderState::WheelchairMoving;
}

bool AElderlyAICharacter::IsUnderPlayerControl() const { return GetController() && GetController()->IsPlayerController(); }

void AElderlyAICharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	if (IsUnderPlayerControl()) SetupForPlayerControl(); else SetupForAIControl();
	if (!IsUnderPlayerControl()) ElderNavigation->NotifyAIControllerReady();
}

void AElderlyAICharacter::UnPossessed()
{
	ElderNavigation->SuspendForPlayerControl();
	ClearManualIntent();
	CancelInputTurn();
	// Possession is not a state transition: keep identity, state and AnimInstance intact.
	ConsumeMovementInputVector();
	GetCharacterMovement()->StopMovementImmediately();
	PlayerCamera->Deactivate();
	Super::UnPossessed();
}

void AElderlyAICharacter::SetupForAIControl()
{
	ClearManualIntent();
	CancelInputTurn();
	bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, TurnRate, 0.f);
	SetDesiredMoveSpeed(DesiredMoveSpeed);
	UpdateMovementForState();
	PlayerCamera->Deactivate();
}

void AElderlyAICharacter::SetupForPlayerControl()
{
	SetupForAIControl();
	PlayerCamera->Activate();
}

void AElderlyAICharacter::SetDesiredMoveSpeed(float Speed)
{
	DesiredMoveSpeed = FMath::Max(0.f, Speed);
	GetCharacterMovement()->MaxWalkSpeed = GetEffectiveMoveSpeed();
}

float AElderlyAICharacter::GetEffectiveMoveSpeed() const
{
	const bool bWheelchair = CurrentState == EElderState::WheelchairIdle || CurrentState == EElderState::WheelchairMoving;
	return FMath::Max(0.f, DesiredMoveSpeed) * (bWheelchair ? 1.f : FMath::Clamp(WalkingSpeedScale, 0.f, 2.f));
}

void AElderlyAICharacter::MoveElder(FVector2D Input, float ViewYaw)
{
	if (!IsUnderPlayerControl() || !CanElderMove()) return;
	if (Input.IsNearlyZero()) { ReleaseMovementInput(); return; }
	const float Now = GetWorld()->GetTimeSeconds();
	if (IsWheelchairState())
	{
		// Wheelchair axes are independent throttle and steering, not a heading vector.
		// Preserve analog values; a digital W+D must not reduce speed or turn rate.
		if (FMath::IsNearlyZero(MoveInput.Y) && !FMath::IsNearlyZero(Input.Y)) InputStartedAt = Now;
		MoveInput = FVector2D(FMath::Clamp(Input.X, -1., 1.), FMath::Clamp(Input.Y, -1., 1.));
		bHasFacingRequest = false;
		bMovingTurn = false;
		if (bTurningInPlace) CancelInputTurn();
		if (FMath::IsNearlyZero(MoveInput.Y) || Now - InputStartedAt < FMath::Max(0.f, MoveHoldThreshold))
			StopInputTranslation();
		else
		{
			if (CurrentState == EElderState::WheelchairIdle) SetElderState(EElderState::WheelchairMoving);
			AddMovementInput(GetActorForwardVector().GetSafeNormal2D(), MoveInput.Y);
		}
		GetCharacterMovement()->bOrientRotationToMovement = false;
		SetActorTickEnabled(!FMath::IsNearlyZero(MoveInput.X));
		return;
	}
	const bool bNewPress = MoveInput.IsNearlyZero();
	const FVector2D NewInput = Input.GetClampedToMaxSize(1.f);
	const bool bNewDirection = MoveInput.IsNearlyZero() ||
		!MoveInput.GetSafeNormal().Equals(NewInput.GetSafeNormal(), .001f);
	if (MoveInput.IsNearlyZero()) InputStartedAt = Now;
	MoveInput = NewInput;
	if (!bTurnBeforeMove)
	{
		bMovingTurn = false;
		bHasFacingRequest = false;
		if (bTurningInPlace) CancelInputTurn();
		else SetActorTickEnabled(false);
		GetCharacterMovement()->bOrientRotationToMovement = false;
	}
	else if (!bTurningInPlace) GetCharacterMovement()->bOrientRotationToMovement = !bMovingTurn;
	// ViewYaw is retained only for existing Blueprint-call compatibility. Neither
	// the camera nor a takeover-time reference owns the manual movement frame.
	const FVector ForwardDirection = GetActorForwardVector().GetSafeNormal2D();
	const FVector RightDirection = GetActorRightVector().GetSafeNormal2D();
	const FVector Direction = (ForwardDirection * MoveInput.Y + RightDirection * MoveInput.X).GetSafeNormal();
	if (bNewDirection)
	{
		// A new press from rest is a fresh idle gesture, even during a release tail.
		if (bNewPress) bMovingTurn = false;
		// Latch this gesture's destination, not a permanent input frame. Recomputing
		// the destination every turning frame would make held A/D circle forever.
		TargetFacingYaw = Direction.Rotation().Yaw;
		FacingChangedAt = Now;
		bHasFacingRequest = bTurnBeforeMove;
		// Only ordinary walking gets non-blocking steering. A stationary gesture
		// keeps the original tap/hold root turn; wheelchair was handled above.
		if (bTurnBeforeMove && !bTurningInPlace && !bNeedsTurnGraphReset &&
			((CurrentState == EElderState::Walking && !bNewPress) || bMovingTurn))
		{
			bMovingTurn = true;
			bHasFacingRequest = false;
			SetActorTickEnabled(true);
		}
	}
	if (bMovingTurn)
	{
		// Continue forward while Tick smoothly changes that forward direction.
		// Do not clear velocity, re-enter TIP, or toggle Walking while cornering.
		if (Now - InputStartedAt < FMath::Max(0.f, MoveHoldThreshold)) { StopInputTranslation(); return; }
		if (CurrentState == EElderState::Standing) SetElderState(EElderState::Walking);
		GetCharacterMovement()->bOrientRotationToMovement = false;
		AddMovementInput(GetActorForwardVector().GetSafeNormal2D(), MoveInput.Size());
		return;
	}
	ProcessPendingFacing();
	if (bTurningInPlace || bNeedsTurnGraphReset || bHasFacingRequest || Now - InputStartedAt < FMath::Max(0.f, MoveHoldThreshold))
	{
		StopInputTranslation();
		return;
	}
	if (CurrentState == EElderState::Standing) SetElderState(EElderState::Walking);
	else if (CurrentState == EElderState::WheelchairIdle) SetElderState(EElderState::WheelchairMoving);
	// SetElderState cancels old turns and restores AI defaults; apply the manual
	// rotation authority after that transition, including the first moving frame.
	GetCharacterMovement()->bOrientRotationToMovement = bTurnBeforeMove;
	// After turn-before-move the actor now faces the requested local direction.
	// With turning disabled retain direct local strafe/backward input instead.
	AddMovementInput(bTurnBeforeMove ? GetActorForwardVector().GetSafeNormal2D() : Direction, MoveInput.Size());
}

void AElderlyAICharacter::ClearManualIntent()
{
	MoveInput = FVector2D::ZeroVector;
	InputStartedAt = -1.f;
	bHasFacingRequest = false;
	bMovingTurn = false;
}

void AElderlyAICharacter::StopInputTranslation()
{
	ConsumeMovementInputVector();
	GetCharacterMovement()->StopMovementImmediately();
	if (CurrentState == EElderState::Walking) SetElderState(EElderState::Standing);
	else if (CurrentState == EElderState::WheelchairMoving) SetElderState(EElderState::WheelchairIdle);
}

void AElderlyAICharacter::ProcessPendingFacing()
{
	if (!bHasFacingRequest || bTurningInPlace || bMovingTurn) return;
	SetActorTickEnabled(true);
	if (bNeedsTurnGraphReset) return; // Wait for the real inactive animation branch.
	if (FMath::Abs(FMath::FindDeltaAngleDegrees(GetActorRotation().Yaw, TargetFacingYaw)) <= TurnStartAngle)
	{
		bHasFacingRequest = false;
		SetActorTickEnabled(false);
		return;
	}
	// A tap preserves its final intent even if released before the coalescing window.
	if (!MoveInput.IsNearlyZero() && GetWorld()->GetTimeSeconds() - FacingChangedAt < FMath::Clamp(TurnInputGracePeriod, 0.f, .3f)) return;
	BeginInputTurn(TargetFacingYaw);
}

void AElderlyAICharacter::StopElderMovement()
{
	ClearManualIntent();
	CancelInputTurn();
	StopInputTranslation();
}

void AElderlyAICharacter::ReleaseMovementInput()
{
	if (IsWheelchairState()) { StopElderMovement(); return; }
	MoveInput = FVector2D::ZeroVector;
	InputStartedAt = -1.f;
	StopInputTranslation();
	ProcessPendingFacing();
}

void AElderlyAICharacter::ReportTurnPlaybackProgress(float NormalizedTime)
{
	if (bTurningInPlace && bRootMotionTurn) TurnAnimationPhase=FMath::Clamp(NormalizedTime,0.f,1.f);
}

bool AElderlyAICharacter::ShouldPlayTurnAnimation() const
{
	return bTurningInPlace && CurrentState == EElderState::Standing && TurnInPlaceBlendSpace;
}

void AElderlyAICharacter::BeginInputTurn(float TargetYaw)
{
	StopInputTranslation();
	TurnStartYaw = GetActorRotation().Yaw;
	TurnYawDelta = FMath::FindDeltaAngleDegrees(TurnStartYaw, TargetYaw);
	TurnBlendPosition = FMath::GetMappedRangeValueClamped(FVector2D(-180, 180), FVector2D(0, 100), TurnYawDelta);
	TurnAnimationPhase = 0.f;
	TurnElapsed = 0.f;
	ActiveTurnPlayRate = FMath::Max(.1f, TurnAnimationPlayRate);
	TurnDuration = FMath::Abs(TurnYawDelta) / FMath::Max(1.f, TurnRate);
	bRootMotionTurn = TurnInPlaceBlendSpace && CurrentState == EElderState::Standing;
	TurnRootYawScale = 1.f;
	if (bRootMotionTurn)
	{
		TArray<FBlendSampleData> Samples;
		int32 Cache = INDEX_NONE;
		TurnInPlaceBlendSpace->GetSamplesFromBlendInput(FVector(TurnBlendPosition, 0, 0), Samples, Cache, true);
		TurnDuration = TurnInPlaceBlendSpace->GetAnimationLengthFromSampleData(Samples) / ActiveTurnPlayRate;
		float AuthoredAngle = 0.f;
		for (const FBlendSampleData& Sample : Samples)
		{
			if (!Sample.Animation) continue;
			const FTransform Total = Sample.Animation->ExtractRootMotionFromRange(0., Sample.Animation->GetPlayLength(), FAnimExtractContext(0., true));
			AuthoredAngle += FMath::Abs(Total.Rotator().Yaw) * Sample.GetClampedWeight();
		}
		// Asset names are approximate: TIP_L90 actually authors 82.08 degrees.
		// Adapt the extracted deltas rather than snapping the capsule at the end.
		if (AuthoredAngle > 1.f) TurnRootYawScale = FMath::Abs(TurnYawDelta) / AuthoredAngle;
	}
	TurnDuration = FMath::Max(0.1f, TurnDuration);
	GetCharacterMovement()->bOrientRotationToMovement = false;
	bTurningInPlace = true;
	SetActorTickEnabled(true);
}

FTransform AElderlyAICharacter::AdjustTurnRootMotion(const FTransform& Delta, UCharacterMovementComponent* Movement, float DeltaSeconds)
{
	// A fading standing turn cannot rotate/translate the seated character. Wheelchair
	// locomotion is owned by CharacterMovement, including procedural input turns.
	if (CurrentState == EElderState::WheelchairIdle || CurrentState == EElderState::WheelchairMoving) return FTransform::Identity;
	if (!bTurningInPlace || !bRootMotionTurn) return Delta;
	FTransform Result = Delta;
	FRotator Rotation = Delta.Rotator();
	Rotation.Yaw *= TurnRootYawScale;
	Result.SetRotation(Rotation.Quaternion());
	return Result;
}

void AElderlyAICharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!IsUnderPlayerControl() || !CanElderMove()) { ClearManualIntent(); CancelInputTurn(); return; }
	if (IsWheelchairState())
	{
		// Input owns yaw only while held. No fixed destination and no release tail.
		GetCharacterMovement()->bOrientRotationToMovement = false;
		const float Step = MoveInput.X * FMath::Max(0.f, WheelchairTurnRate) * FMath::Max(0.f, DeltaSeconds);
		AddActorWorldRotation(FRotator(0.f, Step, 0.f));
		SetActorTickEnabled(!FMath::IsNearlyZero(MoveInput.X));
		return;
	}
	if (bMovingTurn) { UpdateMovingTurn(DeltaSeconds); return; }
	if (!bTurningInPlace) { ProcessPendingFacing(); return; }
	ConsumeMovementInputVector();
	GetCharacterMovement()->StopMovementImmediately();
	// Leave the final sample active for one movement update so UE can consume its
	// last root-motion delta. Never restore the old yaw when the pose blends out.
	if (TurnAnimationPhase >= 1.f) { CancelInputTurn(); return; }
	TurnElapsed += DeltaSeconds;
	if (bRootMotionTurn && TurnElapsed > TurnDuration + 2.f)
	{
		UE_LOG(LogTemp, Error, TEXT("Elder turn player did not finish; check ABP_AI turn graph and ElderAnimInstance parent."));
		StopElderMovement();
		return;
	}
	if (!bRootMotionTurn)
	{
		// Ordinary standing without an authored animation uses the procedural fallback.
		TurnAnimationPhase = FMath::Min(1.f, TurnAnimationPhase + DeltaSeconds / TurnDuration);
		SetActorRotation(FRotator(0, TurnStartYaw + TurnYawDelta * TurnAnimationPhase, 0));
	}
}

void AElderlyAICharacter::UpdateMovingTurn(float DeltaSeconds)
{
	// Sole rotation authority for walking steering and its short release tail.
	// Static TIP cannot be active here; AI and wheelchair do not enter this phase.
	GetCharacterMovement()->bOrientRotationToMovement = false;
	const FRotator Current(0.f, GetActorRotation().Yaw, 0.f);
	const FRotator Target(0.f, TargetFacingYaw, 0.f);
	const float StepTime = FMath::Clamp(DeltaSeconds, 0.f, .05f);
	const FRotator Interpolated = FMath::RInterpTo(Current, Target, StepTime, FMath::Max(.1f, TurnSpeed));
	const float MaxStep = FMath::Max(1.f, MaxMovingTurnRate) * StepTime;
	const float Step = FMath::Clamp(FMath::FindDeltaAngleDegrees(Current.Yaw, Interpolated.Yaw), -MaxStep, MaxStep);
	SetActorRotation(FRotator(0.f, Current.Yaw + Step, 0.f));
	if (FMath::Abs(FMath::FindDeltaAngleDegrees(GetActorRotation().Yaw, TargetFacingYaw)) < .05f)
	{
		// No final snap. The residual is sub-visible; held input now follows Forward.
		bMovingTurn = false;
		SetActorTickEnabled(false);
		GetCharacterMovement()->bOrientRotationToMovement = bTurnBeforeMove;
	}
}

void AElderlyAICharacter::CancelInputTurn()
{
	if (bTurningInPlace && bRootMotionTurn) bNeedsTurnGraphReset=true;
	bRootMotionTurn = false;
	bTurningInPlace = false;
	const bool bManualWheelchair = IsUnderPlayerControl() && IsWheelchairState();
	SetActorTickEnabled(bHasFacingRequest || bMovingTurn || (bManualWheelchair && !FMath::IsNearlyZero(MoveInput.X)));
	GetCharacterMovement()->bOrientRotationToMovement = !bMovingTurn && !bManualWheelchair;
}
bool AElderlyAICharacter::IsWheelchairState() const
{
	return CurrentState == EElderState::WheelchairIdle || CurrentState == EElderState::WheelchairMoving;
}
FString AElderlyAICharacter::GetMonitorName_Implementation() const { return ElderName; }
EElderState AElderlyAICharacter::GetMonitorState_Implementation() const { return CurrentState; }
FString AElderlyAICharacter::GetMonitorStateString_Implementation() const
{
	switch (CurrentState) { case EElderState::Standing:return TEXT("站立"); case EElderState::Sitting:return TEXT("坐姿"); case EElderState::Walking:return TEXT("行走"); case EElderState::Falling:return TEXT("正在跌倒"); case EElderState::Fallen:return TEXT("倒地"); case EElderState::WheelchairIdle:return TEXT("轮椅静止"); case EElderState::WheelchairMoving:return TEXT("轮椅移动"); case EElderState::GettingUp:return TEXT("正在起身"); default:return TEXT("未知"); }
}
