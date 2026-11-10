#include "OrbitCameraController.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "InputCoreTypes.h"
#include "InputKeyEventArgs.h"
#include "OrbitCameraCharacter.h"
#include "ElderlyAICharacter.h"
#include "ElderDebugControlWidget.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "EngineUtils.h"
#include "Framework/Application/IInputProcessor.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/Application/SlateUser.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Widgets/SViewport.h"

#if !UE_BUILD_SHIPPING
namespace
{
	void RunOrbitCameraInputRouteSelfTest()
	{
		AOrbitCameraController* OrbitController = nullptr;
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			UWorld* World = Context.World();
			if (!World || !World->IsGameWorld())
			{
				continue;
			}

			for (TActorIterator<AOrbitCameraController> It(World); It; ++It)
			{
				OrbitController = *It;
				break;
			}
			if (OrbitController)
			{
				break;
			}
		}

		AOrbitCameraCharacter* OrbitCharacter = OrbitController ? Cast<AOrbitCameraCharacter>(OrbitController->GetPawn()) : nullptr;
		if (!OrbitCharacter)
		{
			UE_LOG(LogTemp, Error, TEXT("OrbitCameraInputSelfTest: no possessed orbit camera character."));
			return;
		}

		OrbitController->bRequireMouseButtonForOrbit = true;
		const float StartYaw = OrbitCharacter->TargetOrbitYaw;
		const float StartPitch = OrbitCharacter->TargetOrbitPitch;
		const float StartDistance = OrbitCharacter->TargetOrbitDistance;

		// InputKey does not pass through Slate's input pre-processor, so test the
		// same shared handler used by every real input route instead.
		OrbitController->SetOrbitDraggingForInput(true);
		const bool bDragBegan = OrbitController->bIsOrbitDragging;
		const bool bLookHandled = OrbitController->ProcessOrbitLookInput(FVector2D(40.0f, 20.0f));
		const bool bZoomHandled = OrbitController->ProcessOrbitZoomInput(2.0f);
		OrbitController->SetOrbitDraggingForInput(false);

		UE_LOG(LogTemp, Display,
			TEXT("OrbitCameraInputSelfTest: Pawn=%s DragBegan=%s DragEnded=%s RotationChanged=%s ZoomChanged=%s StartYaw=%0.2f EndYaw=%0.2f StartPitch=%0.2f EndPitch=%0.2f StartDistance=%0.2f EndDistance=%0.2f"),
			*OrbitCharacter->GetName(),
			bDragBegan ? TEXT("true") : TEXT("false"),
			OrbitController->bIsOrbitDragging ? TEXT("false") : TEXT("true"),
			bLookHandled && (!FMath::IsNearlyEqual(StartYaw, OrbitCharacter->TargetOrbitYaw) || !FMath::IsNearlyEqual(StartPitch, OrbitCharacter->TargetOrbitPitch)) ? TEXT("true") : TEXT("false"),
			bZoomHandled && !FMath::IsNearlyEqual(StartDistance, OrbitCharacter->TargetOrbitDistance) ? TEXT("true") : TEXT("false"),
			StartYaw, OrbitCharacter->TargetOrbitYaw, StartPitch, OrbitCharacter->TargetOrbitPitch, StartDistance, OrbitCharacter->TargetOrbitDistance);

		if (FParse::Param(FCommandLine::Get(), TEXT("OrbitCameraInputSelfTest")))
		{
			FPlatformMisc::RequestExit(false);
		}
	}
}
#endif

/**
 * UMG/Web widgets may mark mouse events as handled before PlayerController::InputKey.
 * This pre-processor intentionally sees the event first. Only camera mouse input is
 * consumed; ordinary left-click UI interactions are never intercepted.
 */
class FOrbitCameraInputPreProcessor final : public IInputProcessor
{
public:
	explicit FOrbitCameraInputPreProcessor(AOrbitCameraController* InController)
		: Controller(InController)
	{
	}

	virtual void Tick(const float DeltaTime, FSlateApplication& SlateApp, TSharedRef<ICursor> Cursor) override
	{
		if (AOrbitCameraController* PC = Controller.Get()) PC->UpdateElderCursorFocus();
	}

	virtual bool HandleKeyDownEvent(FSlateApplication& SlateApp, const FKeyEvent& Event) override
	{
		AOrbitCameraController* PC = Controller.Get();
		return PC && PC->HasElderGameFocus() && PC->HandleElderAltKey(Event.GetKey(), Event.IsRepeat() ? IE_Repeat : IE_Pressed, true);
	}

	virtual bool HandleKeyUpEvent(FSlateApplication& SlateApp, const FKeyEvent& Event) override
	{
		AOrbitCameraController* PC = Controller.Get();
		// Release must reach us even when a UMG button/text field consumes keyboard input.
		return PC && PC->HandleElderAltKey(Event.GetKey(), IE_Released, PC->HasElderGameFocus());
	}

	virtual bool HandleMouseMoveEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override
	{
		AOrbitCameraController* OrbitController = Controller.Get();
		if (OrbitController && OrbitController->IsControllingElder() && OrbitController->bElderNavigationUI && OrbitController->HasElderGameFocus()
			&& !OrbitController->bElderUICursorActive && !OrbitController->bElderCapturePending
			&& OrbitController->IsOrbitInputFromGameViewport(SlateApp, MouseEvent))
		{
			// NoCapture keeps the dashboard clickable but does not route mouse axes
			// to the pawn. Feed uncaptured motion once and let Slate update UI hover.
			const FVector2D Delta = MouseEvent.GetCursorDelta();
			OrbitController->AddYawInput(Delta.X * 0.07f);
			OrbitController->AddPitchInput(Delta.Y * 0.07f);
			return false;
		}
		if (!OrbitController || !OrbitController->IsOrbitInputFromGameViewport(SlateApp, MouseEvent) || !OrbitController->CanOrbit())
		{
			return false;
		}

		const FVector2D Delta = MouseEvent.GetCursorDelta();
		if (!Delta.IsNearlyZero())
		{
			OrbitController->RecordInputRoute(TEXT("SlatePreProcessor"), EKeys::Mouse2D, Delta.Size());
			OrbitController->ProcessOrbitLookInput(FVector2D(Delta.X, Delta.Y));
		}
		return true;
	}

	virtual bool HandleMouseButtonDownEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override
	{
		AOrbitCameraController* OrbitController = Controller.Get();
		if (!OrbitController || !OrbitController->GetOrbitCameraCharacter() || MouseEvent.GetEffectingButton() != EKeys::RightMouseButton || !OrbitController->IsOrbitInputFromGameViewport(SlateApp, MouseEvent))
		{
			return false;
		}

		OrbitController->BeginOrbitDrag();
		OrbitController->RecordInputRoute(TEXT("SlatePreProcessor"), EKeys::RightMouseButton, 1.0f);
		return OrbitController->bRequireMouseButtonForOrbit;
	}

	virtual bool HandleMouseButtonUpEvent(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) override
	{
		AOrbitCameraController* OrbitController = Controller.Get();
		if (!OrbitController || MouseEvent.GetEffectingButton() != EKeys::RightMouseButton || !OrbitController->bIsOrbitDragging)
		{
			return false;
		}

		const bool bConsume = OrbitController->bRequireMouseButtonForOrbit;
		OrbitController->EndOrbitDrag();
		return bConsume;
	}

	virtual bool HandleMouseWheelOrGestureEvent(FSlateApplication& SlateApp, const FPointerEvent& WheelEvent, const FPointerEvent* GestureEvent) override
	{
		AOrbitCameraController* OrbitController = Controller.Get();
		if (!OrbitController || !OrbitController->IsOrbitInputFromGameViewport(SlateApp, WheelEvent) || !OrbitController->CanOrbitZoom())
		{
			return false;
		}

		const float WheelDelta = WheelEvent.GetWheelDelta();
		if (!FMath::IsNearlyZero(WheelDelta))
		{
			OrbitController->RecordInputRoute(TEXT("SlatePreProcessor"), EKeys::MouseWheelAxis, WheelDelta);
			OrbitController->ProcessOrbitZoomInput(WheelDelta);
		}
		return true;
	}

	virtual const TCHAR* GetDebugName() const override
	{
		return TEXT("OrbitCameraInputPreProcessor");
	}

private:
	TWeakObjectPtr<AOrbitCameraController> Controller;
};

AOrbitCameraController::AOrbitCameraController()
{
	bShowMouseCursor = true;
	// The level also contains independent CameraActors. The possessed character
	// must remain the view target, otherwise orbit math runs but is invisible.
	bAutoManageActiveCameraTarget = false;
	static ConstructorHelpers::FObjectFinder<UInputAction> Move(TEXT("/Game/Project_File/Input/IA_Move.IA_Move"));
	static ConstructorHelpers::FObjectFinder<UInputAction> Exit(TEXT("/Game/Project_File/Input/IA_ExitElderControl.IA_ExitElderControl"));
	static ConstructorHelpers::FObjectFinder<UInputMappingContext> Mapping(TEXT("/Game/Project_File/Input/IMC_ElderControl.IMC_ElderControl"));
	ElderMoveAction = Move.Object;
	ElderExitAction = Exit.Object;
	ElderInputMapping = Mapping.Object;
}

void AOrbitCameraController::BeginPlay()
{
	Super::BeginPlay();
	RefreshElderCeilingVisibility();
	ActivatePossessedOrbitCamera();
	ApplyUIInputMode();
	RegisterOrbitInputPreProcessor();
	RegisterViewportInputFallback();
	if (bShowElderDebugControl && IsLocalController())
	{
		ElderDebugControl = CreateWidget<UElderDebugControlWidget>(this, UElderDebugControlWidget::StaticClass());
		if (ElderDebugControl) ElderDebugControl->AddToViewport(90);
	}

	// Test-only switch used to capture the orbit camera without this project's
	// full-screen UMG/Web dashboard covering the scene. Normal PIE/game runs
	// never enter this branch.
	if (FParse::Param(FCommandLine::Get(), TEXT("OrbitCameraVisualTest")))
	{
		GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			UWidgetLayoutLibrary::RemoveAllWidgets(this);
		}));
	}

#if !UE_BUILD_SHIPPING
	if (FParse::Param(FCommandLine::Get(), TEXT("OrbitCameraInputSelfTest")))
	{
		GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateLambda([]()
		{
			RunOrbitCameraInputRouteSelfTest();
		}));
	}
#endif
}

void AOrbitCameraController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	ActivatePossessedOrbitCamera();
	RefreshElderCeilingVisibility();
}

void AOrbitCameraController::OnUnPossess()
{
	ResetElderCursorInput();
	Super::OnUnPossess();
	RefreshElderCeilingVisibility();
}

void AOrbitCameraController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsControllingElder() && EndPlayReason == EEndPlayReason::Destroyed) ReleaseElderControl();
	SetElderCeilingVisible(false);
	SetElderInputActive(false);
	if (ULocalPlayer* Local = GetLocalPlayer())
		if (auto* Subsystem = Local->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
			if (ElderFallMapping) Subsystem->RemoveMappingContext(ElderFallMapping);
	UnregisterOrbitInputPreProcessor();
	UnregisterViewportInputFallback();
	Super::EndPlay(EndPlayReason);
}

void AOrbitCameraController::SetupInputComponent()
{
	Super::SetupInputComponent();

	check(InputComponent);
	BindElderInput();
	InputComponent->BindAxisKey(EKeys::MouseX, this, &AOrbitCameraController::HandleInputComponentYaw);
	InputComponent->BindAxisKey(EKeys::MouseY, this, &AOrbitCameraController::HandleInputComponentPitch);
	InputComponent->BindAxisKey(EKeys::MouseWheelAxis, this, &AOrbitCameraController::HandleInputComponentZoom);
	InputComponent->BindKey(EKeys::RightMouseButton, IE_Pressed, this, &AOrbitCameraController::BeginOrbitDragFromInputComponent);
	InputComponent->BindKey(EKeys::RightMouseButton, IE_Released, this, &AOrbitCameraController::EndOrbitDragFromInputComponent);
}

bool AOrbitCameraController::CanOrbit() const
{
	const AOrbitCameraCharacter* OrbitCharacter = Cast<AOrbitCameraCharacter>(GetPawn());
	return OrbitCharacter && OrbitCharacter->bOrbitCameraEnabled && (!bRequireMouseButtonForOrbit || bIsOrbitDragging);
}

bool AOrbitCameraController::CanOrbitZoom() const
{
	const AOrbitCameraCharacter* OrbitCharacter = Cast<AOrbitCameraCharacter>(GetPawn());
	return OrbitCharacter && OrbitCharacter->bOrbitCameraEnabled;
}

bool AOrbitCameraController::ProcessOrbitLookInput(FVector2D LookInput)
{
	if (!CanOrbit() || LookInput.IsNearlyZero())
	{
		if (!bLoggedOrbitInputRejected)
		{
			const AOrbitCameraCharacter* OrbitCharacter = GetOrbitCameraCharacter();
			UE_LOG(LogTemp, Warning, TEXT("OrbitCameraInput: REJECTED CanOrbit=%s Enabled=%s RequireRMB=%s Dragging=%s Pawn=%s"),
				CanOrbit() ? TEXT("true") : TEXT("false"),
				OrbitCharacter && OrbitCharacter->bOrbitCameraEnabled ? TEXT("true") : TEXT("false"),
				bRequireMouseButtonForOrbit ? TEXT("true") : TEXT("false"),
				bIsOrbitDragging ? TEXT("true") : TEXT("false"), *GetNameSafe(GetPawn()));
			bLoggedOrbitInputRejected = true;
		}
		return false;
	}

	AOrbitCameraCharacter* OrbitCharacter = GetOrbitCameraCharacter();
	OrbitCharacter->AddOrbitInput(LookInput * FMath::Max(0.0f, ObservationOrbitInputScale));
	if (!bLoggedOrbitInputApplied)
	{
		UE_LOG(LogTemp, Display, TEXT("OrbitCameraInput: APPLIED Look=(%0.3f,%0.3f) TargetYaw=%0.3f TargetPitch=%0.3f Pawn=%s"),
			LookInput.X, LookInput.Y, OrbitCharacter->TargetOrbitYaw, OrbitCharacter->TargetOrbitPitch, *GetNameSafe(OrbitCharacter));
		bLoggedOrbitInputApplied = true;
	}
	return true;
}

bool AOrbitCameraController::ProcessOrbitZoomInput(float ZoomInput)
{
	if (!CanOrbitZoom() || FMath::IsNearlyZero(ZoomInput))
	{
		return false;
	}

	GetOrbitCameraCharacter()->AddOrbitZoomInput(ZoomInput);
	return true;
}

AOrbitCameraCharacter* AOrbitCameraController::GetOrbitCameraCharacter() const
{
	return Cast<AOrbitCameraCharacter>(GetPawn());
}

bool AOrbitCameraController::IsOrbitInputFromGameViewport(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) const
{
	UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr;
	const TSharedPtr<SViewport> ViewportWidget = Viewport ? Viewport->GetGameViewportWidget() : nullptr;
	if (!ViewportWidget.IsValid())
	{
		return false;
	}

	if (const FWidgetPath* EventPath = MouseEvent.GetEventPath())
	{
		return EventPath->ContainsWidget(ViewportWidget.Get());
	}

	const TSharedPtr<const FSlateUser> SlateUser = SlateApp.GetUser(MouseEvent);
	return SlateUser.IsValid() && SlateUser->IsWidgetUnderPointer(StaticCastSharedPtr<const SWidget>(ViewportWidget), MouseEvent.GetPointerIndex());
}

void AOrbitCameraController::SetSelectedElder(AElderlyAICharacter* NewSelectedElder)
{
	if (SelectedElder == NewSelectedElder)
	{
		return;
	}
	SelectedElder = NewSelectedElder;
	OnSelectedElderChanged.Broadcast(SelectedElder);
}

bool AOrbitCameraController::ToggleSelectedElderSightCone()
{
	if (!SelectedElder)
	{
		return false;
	}
	SelectedElder->ToggleSightCone();
	RestoreElderInputAfterSightToggle();
	return true;
}

void AOrbitCameraController::SetOrbitPivotLocation(FVector NewPivotLocation)
{
	if (AOrbitCameraCharacter* OrbitCharacter = GetOrbitCameraCharacter())
	{
		OrbitCharacter->SetOrbitPivotLocation(NewPivotLocation);
	}
}

FVector AOrbitCameraController::GetOrbitPivotLocation() const
{
	if (const AOrbitCameraCharacter* OrbitCharacter = GetOrbitCameraCharacter())
	{
		return OrbitCharacter->GetActiveOrbitPivotLocation();
	}

	return FVector::ZeroVector;
}

void AOrbitCameraController::RegisterOrbitInputPreProcessor()
{
	if (!OrbitInputPreProcessor.IsValid() && FSlateApplication::IsInitialized())
	{
		OrbitInputPreProcessor = MakeShared<FOrbitCameraInputPreProcessor>(this);
		FSlateApplication::Get().RegisterInputPreProcessor(OrbitInputPreProcessor, 0);
	}
}

void AOrbitCameraController::RegisterViewportInputFallback()
{
	UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr;
	if (!Viewport)
	{
		return;
	}

	if (!Viewport->OnOverrideInputKey().IsBound())
	{
		Viewport->OnOverrideInputKey().BindUObject(this, &AOrbitCameraController::HandleViewportInputKey);
		bOwnsViewportKeyFallback = true;
	}
	if (!Viewport->OnOverrideInputAxis().IsBound())
	{
		Viewport->OnOverrideInputAxis().BindUObject(this, &AOrbitCameraController::HandleViewportInputAxis);
		bOwnsViewportAxisFallback = true;
	}
	RegisteredGameViewport = Viewport;
}

void AOrbitCameraController::UnregisterViewportInputFallback()
{
	if (RegisteredGameViewport.IsValid())
	{
		if (bOwnsViewportKeyFallback && RegisteredGameViewport->OnOverrideInputKey().IsBoundToObject(this))
		{
			RegisteredGameViewport->OnOverrideInputKey().Unbind();
		}
		if (bOwnsViewportAxisFallback && RegisteredGameViewport->OnOverrideInputAxis().IsBoundToObject(this))
		{
			RegisteredGameViewport->OnOverrideInputAxis().Unbind();
		}
	}
	RegisteredGameViewport.Reset();
	bOwnsViewportKeyFallback = false;
	bOwnsViewportAxisFallback = false;
}

bool AOrbitCameraController::HandleViewportInputKey(FInputKeyEventArgs& Params)
{
	if (!GetOrbitCameraCharacter() || Params.Key != EKeys::RightMouseButton)
	{
		return false;
	}

	if (Params.Event == IE_Pressed)
	{
		BeginOrbitDrag();
		RecordInputRoute(TEXT("GameViewport"), Params.Key, 1.0f);
	}
	else if (Params.Event == IE_Released)
	{
		EndOrbitDrag();
		RecordInputRoute(TEXT("GameViewport"), Params.Key, 0.0f);
	}
	return bRequireMouseButtonForOrbit;
}

bool AOrbitCameraController::HandleViewportInputAxis(FInputKeyEventArgs& Params, float& Delta, float& DeltaTime, int32& NumSamples)
{
	if (Params.Key == EKeys::MouseX && CanOrbit())
	{
		ProcessOrbitLookInput(FVector2D(Delta, 0.0f));
		RecordInputRoute(TEXT("GameViewport"), Params.Key, Delta);
		return true;
	}
	if (Params.Key == EKeys::MouseY && CanOrbit())
	{
		ProcessOrbitLookInput(FVector2D(0.0f, Delta));
		RecordInputRoute(TEXT("GameViewport"), Params.Key, Delta);
		return true;
	}
	if (Params.Key == EKeys::MouseWheelAxis && CanOrbitZoom())
	{
		ProcessOrbitZoomInput(Delta);
		RecordInputRoute(TEXT("GameViewport"), Params.Key, Delta);
		return true;
	}
	return false;
}

void AOrbitCameraController::RecordInputRoute(const TCHAR* Route, const FKey& Key, float Value)
{
	bool* bAlreadyLogged = nullptr;
	if (FCString::Strcmp(Route, TEXT("SlatePreProcessor")) == 0)
	{
		bAlreadyLogged = &bLoggedSlateInput;
	}
	else if (FCString::Strcmp(Route, TEXT("GameViewport")) == 0)
	{
		bAlreadyLogged = &bLoggedViewportInput;
	}
	else
	{
		bAlreadyLogged = &bLoggedComponentInput;
	}

	if (!*bAlreadyLogged)
	{
		UE_LOG(LogTemp, Display, TEXT("OrbitCameraInput: INPUT_ROUTE_FROM_%s Key=%s Value=%0.3f Pawn=%s"), Route, *Key.ToString(), Value, *GetNameSafe(GetPawn()));
		*bAlreadyLogged = true;
	}
}

void AOrbitCameraController::UnregisterOrbitInputPreProcessor()
{
	if (OrbitInputPreProcessor.IsValid() && FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().UnregisterInputPreProcessor(OrbitInputPreProcessor);
	}
	OrbitInputPreProcessor.Reset();
}

void AOrbitCameraController::ApplyUIInputMode()
{
	FInputModeGameAndUI InputMode;
	InputMode.SetHideCursorDuringCapture(false);
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	SetInputMode(InputMode);
	if (UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
	{
		Viewport->SetMouseCaptureMode(EMouseCaptureMode::NoCapture);
		Viewport->SetMouseLockMode(EMouseLockMode::DoNotLock);
	}
	bShowMouseCursor = true;
}

void AOrbitCameraController::ApplyOrbitDragInputMode()
{
	FInputModeGameOnly InputMode;
	InputMode.SetConsumeCaptureMouseDown(false);
	SetInputMode(InputMode);
	if (UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr)
	{
		Viewport->SetMouseCaptureMode(EMouseCaptureMode::CapturePermanently_IncludingInitialMouseDown);
		Viewport->SetMouseLockMode(EMouseLockMode::LockAlways);
	}
	bShowMouseCursor = false;
}

void AOrbitCameraController::ActivatePossessedOrbitCamera()
{
	if (AOrbitCameraCharacter* OrbitCharacter = GetOrbitCameraCharacter())
	{
		SetViewTarget(OrbitCharacter);
		UE_LOG(LogTemp, Display, TEXT("OrbitCamera: ActiveViewTarget=%s"), *GetNameSafe(GetViewTarget()));
	}
}

void AOrbitCameraController::HandleInputComponentYaw(float AxisValue)
{
	if (IsControllingElder() && bElderNavigationUI) return; // Slate supplies this mouse axis in NoCapture mode.
	if (!FMath::IsNearlyZero(AxisValue))
	{
		RecordInputRoute(TEXT("InputComponent"), EKeys::MouseX, AxisValue);
	}
	HandleOrbitYaw(AxisValue);
}

void AOrbitCameraController::HandleInputComponentPitch(float AxisValue)
{
	if (IsControllingElder() && bElderNavigationUI) return; // Do not count the same mouse movement twice.
	if (!FMath::IsNearlyZero(AxisValue))
	{
		RecordInputRoute(TEXT("InputComponent"), EKeys::MouseY, AxisValue);
	}
	HandleOrbitPitch(AxisValue);
}

void AOrbitCameraController::HandleInputComponentZoom(float AxisValue)
{
	if (!FMath::IsNearlyZero(AxisValue))
	{
		RecordInputRoute(TEXT("InputComponent"), EKeys::MouseWheelAxis, AxisValue);
	}
	HandleOrbitZoom(AxisValue);
}

void AOrbitCameraController::BeginOrbitDragFromInputComponent()
{
	RecordInputRoute(TEXT("InputComponent"), EKeys::RightMouseButton, 1.0f);
	BeginOrbitDrag();
}

void AOrbitCameraController::EndOrbitDragFromInputComponent()
{
	RecordInputRoute(TEXT("InputComponent"), EKeys::RightMouseButton, 0.0f);
	EndOrbitDrag();
}

void AOrbitCameraController::HandleOrbitYaw(float AxisValue)
{
	if (IsControllingElder()) { if (!bElderUICursorActive && !bElderCapturePending) AddYawInput(AxisValue); return; }
	ProcessOrbitLookInput(FVector2D(AxisValue, 0.0f));
}

void AOrbitCameraController::HandleOrbitPitch(float AxisValue)
{
	if (IsControllingElder()) { if (!bElderUICursorActive && !bElderCapturePending) AddPitchInput(-AxisValue); return; }
	ProcessOrbitLookInput(FVector2D(0.0f, AxisValue));
}

void AOrbitCameraController::HandleOrbitZoom(float AxisValue)
{
	ProcessOrbitZoomInput(AxisValue);
}

void AOrbitCameraController::BeginOrbitDrag()
{
	if (GetOrbitCameraCharacter() && bRequireMouseButtonForOrbit)
	{
		bIsOrbitDragging = true;
		ApplyOrbitDragInputMode();
	}
}

void AOrbitCameraController::EndOrbitDrag()
{
	// RMB drag belongs only to the observation pawn. Its release also reaches
	// this InputComponent while possessing an elder; do not switch that pawn's
	// captured mouse-look back to UI/NoCapture (BeginOrbitDrag already gates it).
	if (GetOrbitCameraCharacter() && bRequireMouseButtonForOrbit)
	{
		bIsOrbitDragging = false;
		ApplyUIInputMode();
	}
}

void AOrbitCameraController::SetOrbitDraggingForInput(bool bDragging)
{
	if (!bRequireMouseButtonForOrbit)
	{
		return;
	}

	bIsOrbitDragging = bDragging;
}
