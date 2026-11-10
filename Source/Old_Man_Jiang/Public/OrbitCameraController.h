#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "AITypes.h"
#include "OrbitCameraController.generated.h"

struct FInputKeyEventArgs;
struct FPointerEvent;
class FSlateApplication;
class FOrbitCameraInputPreProcessor;
class UGameViewportClient;
class AOrbitCameraCharacter;
class AElderlyAICharacter;
class UElderDebugControlWidget;
class AElderlyAIController;
class UInputAction;
class UInputMappingContext;
struct FInputActionValue;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSelectedElderChanged, AElderlyAICharacter*, NewSelectedElder);

/** Routes raw mouse input to the possessed AOrbitCameraCharacter. */
UCLASS(Blueprintable)
class OLD_MAN_JIANG_API AOrbitCameraController : public APlayerController
{
	GENERATED_BODY()

public:
	AOrbitCameraController();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void SetupInputComponent() override;
	virtual bool InputKey(const FInputKeyEventArgs& Params) override;
	UFUNCTION(BlueprintPure, Category="Elder|Input")
	bool IsElderUICursorActive() const { return bElderUICursorActive; }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Orbit|Input")
	// Keep UMG fully interactive by default.  The camera only orbits while the
	// user holds the right mouse button; releasing it restores UI input.
	bool bRequireMouseButtonForOrbit = true;

	/** Scales observation-camera mouse response without changing elder look or orbit settings. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Orbit|Input", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "1.0"))
	float ObservationOrbitInputScale = 0.5f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Camera|Orbit|Input")
	bool bIsOrbitDragging = false;

	/** Current UI/trace selection. This never changes the player's possessed pawn. */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Elder|Selection")
	TObjectPtr<AElderlyAICharacter> SelectedElder;

	UPROPERTY(BlueprintAssignable, Category = "Elder|Selection")
	FOnSelectedElderChanged OnSelectedElderChanged;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Elder|UI")
	bool bShowElderDebugControl = true;

	UFUNCTION(BlueprintCallable, Category = "Elder|Selection")
	void SetSelectedElder(AElderlyAICharacter* NewSelectedElder);

	UFUNCTION(BlueprintCallable, Category = "Elder|Sight")
	bool ToggleSelectedElderSightCone();

	UFUNCTION(BlueprintCallable, Category="Elder|Control") bool TakeControlOfSelectedElder();
	UFUNCTION(BlueprintCallable, Category="Elder|Control") bool TakeControlOfElder(AElderlyAICharacter* Elder);
	UFUNCTION(BlueprintCallable, Category="Elder|Control") bool ReleaseElderControl();
	UFUNCTION(BlueprintCallable, Category="Elder|Control") bool RequestElderFall();
	UFUNCTION(BlueprintPure, Category="Elder|Control") bool IsControllingElder() const;
	/** Keep the dashboard clickable when the elder view was entered from navigation. */
	void EnableElderNavigationUI();
	/** Tag exact ceiling mesh actors, not editor-only groups or gameplay lights. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Elder|Ceiling")
	FName ElderCeilingTag = TEXT("ElderControlCeiling");
	/** Event-driven refresh; also callable after explicitly adding tagged meshes at runtime. */
	UFUNCTION(BlueprintCallable, Category="Elder|Ceiling")
	void RefreshElderCeilingVisibility();
	/** Enumerated only on UI request; never auto-selects the first actor. */
	UFUNCTION(BlueprintCallable, Category="Elder|Selection") TArray<AElderlyAICharacter*> GetAvailableElders() const;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Elder|Control") FString ElderControlMessage;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Elder|Input") TObjectPtr<UInputAction> ElderMoveAction;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Elder|Input") TObjectPtr<UInputAction> ElderExitAction;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Elder|Input") TObjectPtr<UInputMappingContext> ElderInputMapping;

	UFUNCTION(BlueprintPure, Category = "Camera|Orbit|Input")
	bool CanOrbit() const;

	/** Zoom intentionally remains available even when rotation is RMB-gated. */
	UFUNCTION(BlueprintPure, Category = "Camera|Orbit|Input")
	bool CanOrbitZoom() const;

	/** Returns the possessed orbit pawn, including its editable orbit settings. */
	UFUNCTION(BlueprintPure, Category = "Camera|Orbit", meta = (DisplayName = "Get Orbit Camera Character"))
	AOrbitCameraCharacter* GetOrbitCameraCharacter() const;

	/** True only for pointer events routed to this PIE/game viewport, never editor tabs or asset graphs. */
	bool IsOrbitInputFromGameViewport(FSlateApplication& SlateApp, const FPointerEvent& MouseEvent) const;

	/** Controller-facing entry point for changing the camera orbit centre. */
	UFUNCTION(BlueprintCallable, Category = "Camera|Orbit|Pivot", meta = (DisplayName = "Set Orbit Pivot Location"))
	void SetOrbitPivotLocation(FVector NewPivotLocation);

	UFUNCTION(BlueprintPure, Category = "Camera|Orbit|Pivot", meta = (DisplayName = "Get Orbit Pivot Location"))
	FVector GetOrbitPivotLocation() const;

	/** Shared look route used by Slate, GameViewport and InputComponent. */
	UFUNCTION(BlueprintCallable, Category = "Camera|Orbit|Input")
	bool ProcessOrbitLookInput(FVector2D LookInput);

	/** Shared zoom route used by Slate, GameViewport and InputComponent. */
	UFUNCTION(BlueprintCallable, Category = "Camera|Orbit|Input")
	bool ProcessOrbitZoomInput(float ZoomInput);

	/** C++ input-route helper; keeps RMB gating state in one place. */
	void SetOrbitDraggingForInput(bool bDragging);

private:
	bool HandleElderAltKey(FKey Key, EInputEvent Event, bool bMayRestoreCapture);
	bool HasElderGameFocus() const;
	void UpdateElderCursorFocus();
	void ResetElderCursorInput();
	void RestoreElderInputAfterSightToggle();
	bool bLeftAltHeld = false;
	bool bRightAltHeld = false;
	bool bElderUICursorActive = false;
	bool bElderCapturePending = false;
	bool bElderNavigationUI = false;
	void ApplyElderCaptureOrNavigationUI();
	void SetElderCeilingVisible(bool bVisible);
	void BindElderInput();
	void HandleElderFall();
	UPROPERTY(Transient) TObjectPtr<UInputAction> ElderFallAction;
	UPROPERTY(Transient) TObjectPtr<UInputMappingContext> ElderFallMapping;
	void SetElderInputActive(bool bActive);
	void HandleElderMove(const FInputActionValue& Value);
	void HandleElderMoveCompleted();
	void HandleElderExit();
	UFUNCTION() void HandleControlledElderDestroyed(AActor* Actor);
	UPROPERTY(Transient) TObjectPtr<APawn> ObservationPawn;
	UPROPERTY(Transient) TObjectPtr<AElderlyAICharacter> ControlledElder;
	UPROPERTY(Transient) TObjectPtr<AElderlyAIController> SuspendedAIController;
	FAIMoveRequest SuspendedMove;
	bool bHadSuspendedMove = false;
	bool bChangingElderControl = false;
	FRotator ObservationControlRotation;
	friend class FOrbitCameraInputPreProcessor;

	void RegisterOrbitInputPreProcessor();
	void UnregisterOrbitInputPreProcessor();
	void RegisterViewportInputFallback();
	void UnregisterViewportInputFallback();
	bool HandleViewportInputKey(FInputKeyEventArgs& Params);
	bool HandleViewportInputAxis(FInputKeyEventArgs& Params, float& Delta, float& DeltaTime, int32& NumSamples);
	void RecordInputRoute(const TCHAR* Route, const FKey& Key, float Value);
	void ApplyUIInputMode();
	void ApplyOrbitDragInputMode();
	void ActivatePossessedOrbitCamera();
	void HandleInputComponentYaw(float AxisValue);
	void HandleInputComponentPitch(float AxisValue);
	void HandleInputComponentZoom(float AxisValue);
	void BeginOrbitDragFromInputComponent();
	void EndOrbitDragFromInputComponent();
	void HandleOrbitYaw(float AxisValue);
	void HandleOrbitPitch(float AxisValue);
	void HandleOrbitZoom(float AxisValue);
	void BeginOrbitDrag();
	void EndOrbitDrag();

	TSharedPtr<FOrbitCameraInputPreProcessor> OrbitInputPreProcessor;
	TWeakObjectPtr<UGameViewportClient> RegisteredGameViewport;
	bool bOwnsViewportKeyFallback = false;
	bool bOwnsViewportAxisFallback = false;
	bool bLoggedSlateInput = false;
	bool bLoggedViewportInput = false;
	bool bLoggedComponentInput = false;
	bool bLoggedOrbitInputApplied = false;
	bool bLoggedOrbitInputRejected = false;
	TObjectPtr<UElderDebugControlWidget> ElderDebugControl;
};
