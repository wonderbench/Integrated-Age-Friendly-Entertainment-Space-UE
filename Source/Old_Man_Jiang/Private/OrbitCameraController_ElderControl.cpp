#include "OrbitCameraController.h"
#include "OrbitCameraCharacter.h"
#include "ElderlyAICharacter.h"
#include "ElderlyAIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Engine/LocalPlayer.h"
#include "EngineUtils.h"
#include "Engine/StaticMeshActor.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerInput.h"
#include "TimerManager.h"

bool AOrbitCameraController::IsControllingElder() const
{
	return IsValid(ControlledElder) && GetPawn() == ControlledElder;
}

void AOrbitCameraController::RefreshElderCeilingVisibility()
{
	SetElderCeilingVisible(IsControllingElder());
}

void AOrbitCameraController::SetElderCeilingVisible(bool bVisible)
{
	// This project uses a local player. Never modify editor-world assets or gameplay lights.
	if (!IsLocalController() || !GetWorld() || !GetWorld()->IsGameWorld() || ElderCeilingTag.IsNone()) return;
	for (TActorIterator<AStaticMeshActor> It(GetWorld()); It; ++It)
	{
		if (IsValid(*It) && It->ActorHasTag(ElderCeilingTag))
		{
			It->SetActorHiddenInGame(!bVisible);
		}
	}
}

TArray<AElderlyAICharacter*> AOrbitCameraController::GetAvailableElders() const
{
	TArray<AElderlyAICharacter*> Result;
	if (GetWorld()) for (TActorIterator<AElderlyAICharacter> It(GetWorld()); It; ++It)
	{
		if (IsValid(*It)) Result.Add(*It);
	}
	Result.Sort([](const AElderlyAICharacter& A, const AElderlyAICharacter& B) { return A.GetName() < B.GetName(); });
	return Result;
}

bool AOrbitCameraController::TakeControlOfSelectedElder() { return TakeControlOfElder(SelectedElder); }

void AOrbitCameraController::EnableElderNavigationUI()
{
	if (!IsControllingElder() || !GetWorld()) return;
	bElderNavigationUI = true;
	ApplyUIInputMode();
	// Slate finishes its click reply after OnClicked; restore the requested mode afterwards.
	GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		if (IsControllingElder() && bElderNavigationUI && HasElderGameFocus()) ApplyUIInputMode();
	}));
}

bool AOrbitCameraController::TakeControlOfElder(AElderlyAICharacter* Elder)
{
	if (bChangingElderControl) return false;
	if (IsControllingElder()) return ControlledElder == Elder;
	if (!HasAuthority() || !IsLocalController() || !IsValid(Elder) || Elder->GetWorld() != GetWorld() || !GetOrbitCameraCharacter())
	{
		ElderControlMessage = TEXT("请在本地观察模式中选择一个有效老人");
		return false;
	}
	if (Elder->GetController() && !Cast<AElderlyAIController>(Elder->GetController()))
	{
		ElderControlMessage = TEXT("该老人已由其他玩家或不兼容的控制器控制");
		return false;
	}
	if (!Cast<UEnhancedInputComponent>(InputComponent) || !GetLocalPlayer() || !ElderInputMapping || !ElderMoveAction || !ElderExitAction)
	{
		ElderControlMessage = TEXT("Enhanced Input 配置不完整，未执行接管");
		return false;
	}
	TGuardValue<bool> TransitionGuard(bChangingElderControl, true);
	ObservationPawn = GetPawn();
	ObservationControlRotation = GetControlRotation();
	SuspendedAIController = Cast<AElderlyAIController>(Elder->GetController());
	bHadSuspendedMove = false;
	if (SuspendedAIController)
	{
		SuspendedAIController->SuspendForPlayerControl();
		bHadSuspendedMove = SuspendedAIController->HasSuspendedMove();
		SuspendedMove = SuspendedAIController->GetSuspendedMove();
		SuspendedAIController->UnPossess();
	}
	ControlledElder = Elder;
	Possess(Elder);
	if (GetPawn() != Elder)
	{
		ControlledElder = nullptr;
		if (IsValid(SuspendedAIController)) { SuspendedAIController->Possess(Elder); SuspendedAIController->ResumeAfterPlayerControl(); }
		if (IsValid(ObservationPawn)) Possess(ObservationPawn);
		ElderControlMessage = TEXT("接管失败，已恢复原控制关系");
		return false;
	}
	Elder->OnDestroyed.AddUniqueDynamic(this, &AOrbitCameraController::HandleControlledElderDestroyed);
	SetSelectedElder(Elder);
	bIsOrbitDragging = false;
	SetControlRotation(FRotator(-15.f, Elder->GetActorRotation().Yaw, 0.f));
	SetViewTarget(Elder);
	SetElderInputActive(true);
	ApplyOrbitDragInputMode();
	ElderControlMessage = TEXT("已接管：WASD 移动，鼠标转向，按住 Alt 操作 UI，Tab 返回观察");
	return true;
}

bool AOrbitCameraController::ReleaseElderControl()
{
	if (bChangingElderControl || !IsControllingElder()) return false;
	if (!IsValid(ObservationPawn) || (ObservationPawn->GetController() && ObservationPawn->GetController() != this))
	{
		ElderControlMessage = TEXT("原观察角色已失效或被占用，暂不能返回");
		return false;
	}
	TGuardValue<bool> TransitionGuard(bChangingElderControl, true);
	AElderlyAICharacter* Elder = ControlledElder;
	Elder->OnDestroyed.RemoveDynamic(this, &AOrbitCameraController::HandleControlledElderDestroyed);
	ControlledElder = nullptr; // Ignore EnhancedInput cancellation during mapping removal.
	SetElderInputActive(false);
	UnPossess();
	Possess(ObservationPawn);
	SetControlRotation(ObservationControlRotation);
	bIsOrbitDragging = false;
	ApplyUIInputMode();
	if (IsValid(SuspendedAIController) && !SuspendedAIController->GetPawn())
	{
		SuspendedAIController->Possess(Elder);
		SuspendedAIController->ResumeAfterPlayerControl();
	}
	else
	{
		Elder->SpawnDefaultController();
		if (AElderlyAIController* Replacement = Cast<AElderlyAIController>(Elder->GetController()))
		{
			if (bHadSuspendedMove) Replacement->MoveTo(SuspendedMove);
		}
	}
	const bool bRestored = Elder->GetController() && !Elder->IsUnderPlayerControl();
	ElderControlMessage = bRestored ? TEXT("已返回观察，老人交回 AI") : TEXT("已返回观察，但 AIController 创建失败，请检查 BP_AI 配置");
	SuspendedAIController = nullptr;
	ObservationPawn = nullptr;
	bHadSuspendedMove = false;
	return bRestored;
}

void AOrbitCameraController::HandleControlledElderDestroyed(AActor* Actor)
{
	if (Actor != ControlledElder) return;
	ControlledElder = nullptr;
	SetElderInputActive(false);
	RefreshElderCeilingVisibility();
	if (IsValid(ObservationPawn) && !ObservationPawn->GetController()) Possess(ObservationPawn);
	SetControlRotation(ObservationControlRotation);
	ApplyUIInputMode();
	SetSelectedElder(nullptr);
	if (IsValid(SuspendedAIController) && !SuspendedAIController->GetPawn()) SuspendedAIController->Destroy();
	SuspendedAIController = nullptr;
	ObservationPawn = nullptr;
	ElderControlMessage = TEXT("老人已移除，已退出接管");
}

void AOrbitCameraController::BindElderInput()
{
	if (UEnhancedInputComponent* Enhanced = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (!ElderFallAction)
		{
			ElderFallAction = NewObject<UInputAction>(this, TEXT("IA_ElderFall"));
			ElderFallAction->ValueType = EInputActionValueType::Boolean;
			ElderFallMapping = NewObject<UInputMappingContext>(this, TEXT("IMC_ElderFall"));
			ElderFallMapping->MapKey(ElderFallAction, EKeys::C);
		}
		Enhanced->BindAction(ElderFallAction, ETriggerEvent::Started, this, &AOrbitCameraController::HandleElderFall);
		if (ULocalPlayer* Local = GetLocalPlayer())
			if (auto* Subsystem = Local->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
				Subsystem->AddMappingContext(ElderFallMapping, 90);
		if (ElderMoveAction)
		{
			Enhanced->BindAction(ElderMoveAction, ETriggerEvent::Triggered, this, &AOrbitCameraController::HandleElderMove);
			Enhanced->BindAction(ElderMoveAction, ETriggerEvent::Completed, this, &AOrbitCameraController::HandleElderMoveCompleted);
			Enhanced->BindAction(ElderMoveAction, ETriggerEvent::Canceled, this, &AOrbitCameraController::HandleElderMoveCompleted);
		}
		if (ElderExitAction) Enhanced->BindAction(ElderExitAction, ETriggerEvent::Started, this, &AOrbitCameraController::HandleElderExit);
	}
}

bool AOrbitCameraController::RequestElderFall()
{
	AElderlyAICharacter* Elder = IsControllingElder() ? ControlledElder.Get() : SelectedElder.Get();
	if (!IsLocalController() || !IsValid(Elder)) return false;
	if (!Elder->bEnableFallTrigger) return false;
	if (Elder->CurrentState != EElderState::Standing && Elder->CurrentState != EElderState::Walking) return false;
	Elder->StartFalling();
	return Elder->CurrentState == EElderState::Falling;
}

void AOrbitCameraController::HandleElderFall()
{
	if (!bElderUICursorActive && HasElderGameFocus()) RequestElderFall();
}

void AOrbitCameraController::SetElderInputActive(bool bActive)
{
	ResetElderCursorInput();
	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Input = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			if (ElderInputMapping)
			{
				if (bActive) Input->AddMappingContext(ElderInputMapping, 100);
				else Input->RemoveMappingContext(ElderInputMapping);
			}
		}
	}
	if (PlayerInput) PlayerInput->FlushPressedKeys();
}

void AOrbitCameraController::HandleElderMove(const FInputActionValue& Value)
{
	if (IsControllingElder() && !bElderUICursorActive && !bElderCapturePending) ControlledElder->MoveElder(Value.Get<FVector2D>(), 0.f);
}
void AOrbitCameraController::HandleElderMoveCompleted()
{
	if (IsControllingElder()) ControlledElder->ReleaseMovementInput();
}
void AOrbitCameraController::HandleElderExit() { ReleaseElderControl(); }
