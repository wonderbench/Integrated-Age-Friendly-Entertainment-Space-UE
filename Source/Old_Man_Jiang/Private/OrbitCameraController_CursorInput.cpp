#include "OrbitCameraController.h"

#include "ElderlyAICharacter.h"
#include "Engine/GameViewportClient.h"
#include "Framework/Application/SlateApplication.h"
#include "InputKeyEventArgs.h"
#include "Slate/SGameLayerManager.h"
#include "Widgets/SViewport.h"
#include "Engine/World.h"
#include "TimerManager.h"

void AOrbitCameraController::ApplyElderCaptureOrNavigationUI()
{
	if (bElderNavigationUI) ApplyUIInputMode();
	else ApplyOrbitDragInputMode();
}

void AOrbitCameraController::RestoreElderInputAfterSightToggle()
{
	if (!IsControllingElder() || !GetWorld()) return;
	// Slate applies the button click reply after OnClicked. Restore capture afterwards,
	// but never steal focus from an Alt UI session, the editor, or another application.
	GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this]()
	{
		if (!IsControllingElder() || bElderUICursorActive || !HasElderGameFocus()) return;
		bElderCapturePending = false;
		ApplyElderCaptureOrNavigationUI();
	}));
}

bool AOrbitCameraController::InputKey(const FInputKeyEventArgs& Params)
{
	// Viewport fallback also permits deterministic input-route automation.
	if (HandleElderAltKey(Params.Key, Params.Event, HasElderGameFocus())) return true;
	return Super::InputKey(Params);
}

bool AOrbitCameraController::HasElderGameFocus() const
{
	if (!IsLocalController() || !FSlateApplication::IsInitialized() || !FSlateApplication::Get().IsActive()) return false;
	const UGameViewportClient* Viewport = GetWorld() ? GetWorld()->GetGameViewport() : nullptr;
	if (!Viewport) return false;
	const TSharedPtr<SViewport> Widget = Viewport->GetGameViewportWidget();
	const TSharedPtr<IGameLayerManager> GameLayers = Viewport->GetGameLayerManager();
	// UMG overlays are not necessarily descendants of SViewport itself.
	return (Widget && (Widget->HasKeyboardFocus() || Widget->HasFocusedDescendants()))
		|| (GameLayers && (GameLayers->AsWidget()->HasKeyboardFocus() || GameLayers->AsWidget()->HasFocusedDescendants()));
}

bool AOrbitCameraController::HandleElderAltKey(FKey Key, EInputEvent Event, bool bMayRestoreCapture)
{
	if (!IsControllingElder() || (Key != EKeys::LeftAlt && Key != EKeys::RightAlt)) return false;
	bool& Held = Key == EKeys::LeftAlt ? bLeftAltHeld : bRightAltHeld;
	if (Event == IE_Repeat) return Held;
	if (Event != IE_Pressed && Event != IE_Released) return false;
	if (Event == IE_Released && !Held) return false;
	Held = Event == IE_Pressed;
	const bool bShow = bLeftAltHeld || bRightAltHeld;
	if (bShow == bElderUICursorActive) return true;
	bElderUICursorActive = bShow;
	RotationInput = FRotator::ZeroRotator;
	ControlledElder->ReleaseMovementInput();
	if (bShow)
	{
		bElderCapturePending = false;
		ApplyUIInputMode();
	}
	else if (bMayRestoreCapture)
	{
		bElderCapturePending = false;
		ApplyElderCaptureOrNavigationUI();
	}
	else
	{
		// Never grab focus back from an editor tab or another application.
		bElderCapturePending = true;
	}
	return true;
}

void AOrbitCameraController::UpdateElderCursorFocus()
{
	if (!IsControllingElder() || (!bElderUICursorActive && !bElderCapturePending)) return;
	FSlateApplication& Slate = FSlateApplication::Get();
	if (!Slate.IsActive() || !HasElderGameFocus())
	{
		bLeftAltHeld = bRightAltHeld = bElderUICursorActive = false;
		bElderCapturePending = true;
		RotationInput = FRotator::ZeroRotator;
		return;
	}
	if (bElderCapturePending)
	{
		bElderCapturePending = false;
		// Reconcile a release missed while another application owned the keyboard.
		const FModifierKeysState Modifiers = Slate.GetModifierKeys();
		bLeftAltHeld = Modifiers.IsLeftAltDown();
		bRightAltHeld = Modifiers.IsRightAltDown();
		bElderUICursorActive = bLeftAltHeld || bRightAltHeld;
		if (bElderUICursorActive) ApplyUIInputMode();
		else ApplyElderCaptureOrNavigationUI();
	}
}

void AOrbitCameraController::ResetElderCursorInput()
{
	bLeftAltHeld = bRightAltHeld = bElderUICursorActive = bElderCapturePending = bElderNavigationUI = false;
	RotationInput = FRotator::ZeroRotator;
}
