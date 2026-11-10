#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "Editor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/GameViewportClient.h"
#include "EngineUtils.h"
#include "InputKeyEventArgs.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SViewport.h"
#include "Widgets/SWindow.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/StaticMeshComponent.h"
#include "ElderDebugControlWidget.h"
#include "OrbitCameraController.h"
#include "ElderlyAICharacter.h"

class FElderAltUIScenario : public IAutomationLatentCommand
{
public:
	explicit FElderAltUIScenario(FAutomationTestBase* InTest) : Test(InTest), Deadline(FPlatformTime::Seconds()+30.) {}
	bool Update() override
	{
		if (FPlatformTime::Seconds()>Deadline) { Test->AddError(TEXT("Alt UI timeout")); return true; }
		if (!World.IsValid())
		{
			for (const FWorldContext& C : GEngine->GetWorldContexts()) if (C.WorldType==EWorldType::PIE) World=C.World();
			if (!World.IsValid()) return false;
			PC=Cast<AOrbitCameraController>(World->GetFirstPlayerController());
			for (TActorIterator<AElderlyAICharacter> It(World.Get()); It; ++It) if (It->ActorHasTag(TEXT("ElderTakeoverTestA"))) Elder=*It;
			if (!PC.IsValid() || !Elder.IsValid() || !PC->TakeControlOfElder(Elder.Get())) { Test->AddError(TEXT("Missing Alt UI takeover fixture")); return true; }
			for (TObjectIterator<UElderDebugControlWidget> It; It; ++It)
				if (It->GetOwningPlayer()==PC.Get() && It->WidgetTree)
				{
					It->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
					Button=Cast<UButton>(It->WidgetTree->FindWidget(TEXT("ToggleElderSightCone")));
				}
			if (!Button.IsValid()) { Test->AddError(TEXT("Missing existing debug UI button")); return true; }
			// Real Slate routing needs the PIE window active, unlike PC-only injected input.
			FSlateApplication& Slate=FSlateApplication::Get();
			if (const TSharedPtr<SWindow> Window=Slate.FindWidgetWindow(World->GetGameViewport()->GetGameViewportWidget().ToSharedRef())) Window->BringToFront(true);
			Slate.SetUserFocusToGameViewport(0);
			Next=World->GetTimeSeconds()+.4f;
		}
		if (!PC.IsValid() || !Elder.IsValid()) { Test->AddError(TEXT("Lost Alt UI fixture")); return true; }
		if (bInjectLook) Axis();
		if (World->GetTimeSeconds()<Next) return false;
		FSlateApplication& Slate=FSlateApplication::Get();
		if (!bPrepared)
		{
			// MainUI initialization removes the debug overlay; restore that existing
			// instance only in the test, after startup, just as its UI menu does.
			for (TObjectIterator<UElderDebugControlWidget> It; It; ++It)
				if (It->GetOwningPlayer()==PC.Get())
				{
					if (!It->IsInViewport()) It->AddToViewport(90);
					It->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
				}
			Slate.SetKeyboardFocus(World->GetGameViewport()->GetGameViewportWidget(),EFocusCause::SetDirectly);
			bPrepared=true; Next=World->GetTimeSeconds()+.3f; return false;
		}
		switch (Step++)
		{
		case 0:
			UE_LOG(LogTemp,Display,TEXT("ElderAltUI fixture active=%d viewportFocus=%d buttonVisibility=%d geometry=%s"),Slate.IsActive(),World->GetGameViewport()->GetGameViewportWidget()->HasKeyboardFocus(),int32(Button->GetVisibility()),*FVector2D(Button->GetCachedGeometry().GetLocalSize()).ToString());
			Capture(TEXT("initial takeover"));
			SlateKey(EKeys::LeftAlt,true);
			Test->TestTrue(TEXT("Slate Alt opens UI"), PC->IsElderUICursorActive() && PC->bShowMouseCursor && World->GetGameViewport()->GetMouseCaptureMode()==EMouseCaptureMode::NoCapture);
			StartView=PC->GetControlRotation(); StartPosition=Elder->GetActorLocation();
			bInjectLook=true; Key(EKeys::W,true); Next=World->GetTimeSeconds()+.25f; break;
		case 1:
			bInjectLook=false; Key(EKeys::W,false);
			Test->TestTrue(TEXT("Alt suppresses movement and view"),StartView.Equals(PC->GetControlRotation(),.1f) && FVector::Dist2D(StartPosition,Elder->GetActorLocation())<1.f);
			{
				const bool Before=Elder->SightConeMesh->IsVisible();
				const FGeometry Geometry=Button->GetCachedGeometry();
				const FVector2D Position=Geometry.LocalToAbsolute(Geometry.GetLocalSize()*.5f);
				const TSet<FKey> Down={EKeys::LeftMouseButton}, Up;
				Slate.ProcessMouseMoveEvent(FPointerEvent(FSlateApplication::CursorPointerIndex,Position,Position,Up,EKeys::Invalid,0.f,FModifierKeysState()),true);
				Slate.ProcessMouseButtonDownEvent(nullptr,FPointerEvent(FSlateApplication::CursorPointerIndex,Position,Position,Down,EKeys::LeftMouseButton,0.f,FModifierKeysState()));
				Slate.ProcessMouseButtonUpEvent(FPointerEvent(FSlateApplication::CursorPointerIndex,Position,Position,Up,EKeys::LeftMouseButton,0.f,FModifierKeysState()));
				Test->TestTrue(TEXT("Actual Slate click reaches existing UMG sight button"),Before!=Elder->SightConeMesh->IsVisible());
				Slate.SetKeyboardFocus(Button->TakeWidget(),EFocusCause::SetDirectly);
				Test->TestTrue(TEXT("UMG button has focus before Alt release"),Button->HasKeyboardFocus());
			}
			Step=7; Next=World->GetTimeSeconds()+.35f; break;
		case 7:
			UE_LOG(LogTemp,Display,TEXT("ElderSightInput after-click focus=%s alt=%d cursor=%d"),Slate.GetKeyboardFocusedWidget().IsValid()?*Slate.GetKeyboardFocusedWidget()->GetTypeAsString():TEXT("None"),PC->IsElderUICursorActive(),PC->bShowMouseCursor);
			SlateKey(EKeys::LeftAlt,false); Capture(TEXT("release over focused UMG"));
			StartPosition=Elder->GetActorLocation();SlateKey(EKeys::W,true);Step=2;
			StartView=PC->GetControlRotation(); bInjectLook=true; Next=World->GetTimeSeconds()+.2f; break;
		case 2:
			bInjectLook=false; Next=World->GetTimeSeconds()+.6f; break;
		case 3:
			SlateKey(EKeys::W,false);
			Test->TestTrue(TEXT("W moves after sight button without extra viewport click"),FVector::Dist2D(StartPosition,Elder->GetActorLocation())>15.f);
			Test->TestTrue(TEXT("Look resumes without extra click"),!StartView.Equals(PC->GetControlRotation(),1.f));
			SlateKey(EKeys::LeftAlt,true);Next=World->GetTimeSeconds()+.1f;Step=8;break;
		case 8:
			Pointer=Button->GetCachedGeometry().LocalToAbsolute(Button->GetCachedGeometry().GetLocalSize()*.5f);
			Slate.ProcessMouseMoveEvent(FPointerEvent(FSlateApplication::CursorPointerIndex,Pointer,Pointer,TSet<FKey>(),EKeys::Invalid,0.f,FModifierKeysState()),true);
			Slate.ProcessMouseButtonDownEvent(nullptr,FPointerEvent(FSlateApplication::CursorPointerIndex,Pointer,Pointer,TSet<FKey>{EKeys::LeftMouseButton},EKeys::LeftMouseButton,0.f,FModifierKeysState()));
			SlateKey(EKeys::LeftAlt,false);Next=World->GetTimeSeconds()+.1f;break;
		case 9:
			Slate.ProcessMouseButtonUpEvent(FPointerEvent(FSlateApplication::CursorPointerIndex,Pointer,Pointer,TSet<FKey>(),EKeys::LeftMouseButton,0.f,FModifierKeysState()));
			Next=World->GetTimeSeconds()+.2f;break;
		case 10:
			UE_LOG(LogTemp,Display,TEXT("ElderSightInput Alt-before-mouse-up focus=%s alt=%d cursor=%d"),Slate.GetKeyboardFocusedWidget().IsValid()?*Slate.GetKeyboardFocusedWidget()->GetTypeAsString():TEXT("None"),PC->IsElderUICursorActive(),PC->bShowMouseCursor);
			StartPosition=Elder->GetActorLocation();SlateKey(EKeys::W,true);Next=World->GetTimeSeconds()+.8f;break;
		case 11:
			SlateKey(EKeys::W,false);Test->TestTrue(TEXT("W resumes when Alt is released before mouse-up"),FVector::Dist2D(StartPosition,Elder->GetActorLocation())>15.f);
			// An existing UI/PIE cursor can be available without this controller's Alt latch.
			PC->SetInputMode(FInputModeGameAndUI());PC->bShowMouseCursor=true;
			World->GetGameViewport()->SetMouseCaptureMode(EMouseCaptureMode::NoCapture);
			Step=12;Next=World->GetTimeSeconds()+.1f;break;
		case 12:
			{
				const bool Before=Elder->SightConeMesh->IsVisible();
				Pointer=Button->GetCachedGeometry().LocalToAbsolute(Button->GetCachedGeometry().GetLocalSize()*.5f);
				Slate.ProcessMouseMoveEvent(FPointerEvent(FSlateApplication::CursorPointerIndex,Pointer,Pointer,TSet<FKey>(),EKeys::Invalid,0.f,FModifierKeysState()),true);
				Slate.ProcessMouseButtonDownEvent(nullptr,FPointerEvent(FSlateApplication::CursorPointerIndex,Pointer,Pointer,TSet<FKey>{EKeys::LeftMouseButton},EKeys::LeftMouseButton,0.f,FModifierKeysState()));
				Slate.ProcessMouseButtonUpEvent(FPointerEvent(FSlateApplication::CursorPointerIndex,Pointer,Pointer,TSet<FKey>(),EKeys::LeftMouseButton,0.f,FModifierKeysState()));
				Test->TestTrue(TEXT("Direct no-Alt click toggles sight"),Before!=Elder->SightConeMesh->IsVisible());
			}
			Next=World->GetTimeSeconds()+.2f;break;
		case 13:
			UE_LOG(LogTemp,Display,TEXT("ElderSightInput direct-click focus=%s alt=%d cursor=%d"),Slate.GetKeyboardFocusedWidget().IsValid()?*Slate.GetKeyboardFocusedWidget()->GetTypeAsString():TEXT("None"),PC->IsElderUICursorActive(),PC->bShowMouseCursor);
			StartPosition=Elder->GetActorLocation();SlateKey(EKeys::W,true);Next=World->GetTimeSeconds()+.8f;break;
		case 14:
			SlateKey(EKeys::W,false);Test->TestTrue(TEXT("Direct sight click restores WASD without extra viewport click"),FVector::Dist2D(StartPosition,Elder->GetActorLocation())>15.f);
			Capture(TEXT("direct sight click"));
			Test->TestTrue(TEXT("Direct sight click returns keyboard focus to viewport"),World->GetGameViewport()->GetGameViewportWidget()->HasKeyboardFocus());
			Elder->EnterWheelchair();
			PC->SetInputMode(FInputModeGameAndUI()); PC->bShowMouseCursor=true;
			World->GetGameViewport()->SetMouseCaptureMode(EMouseCaptureMode::NoCapture);
			Step=15;Next=World->GetTimeSeconds()+.3f;break;
		case 15:
			for (TObjectIterator<UElderDebugControlWidget> It;It;++It)
				if (It->GetOwningPlayer()==PC.Get()) Button=Cast<UButton>(It->WidgetTree->FindWidget(TEXT("ElderActionStand")));
			if (!Button.IsValid()) {Test->AddError(TEXT("Missing stand button"));return true;}
			Pointer=Button->GetCachedGeometry().LocalToAbsolute(Button->GetCachedGeometry().GetLocalSize()*.5f);
			Slate.ProcessMouseMoveEvent(FPointerEvent(FSlateApplication::CursorPointerIndex,Pointer,Pointer,TSet<FKey>(),EKeys::Invalid,0.f,FModifierKeysState()),true);
			Slate.ProcessMouseButtonDownEvent(nullptr,FPointerEvent(FSlateApplication::CursorPointerIndex,Pointer,Pointer,TSet<FKey>{EKeys::LeftMouseButton},EKeys::LeftMouseButton,0.f,FModifierKeysState()));
			Slate.ProcessMouseButtonUpEvent(FPointerEvent(FSlateApplication::CursorPointerIndex,Pointer,Pointer,TSet<FKey>(),EKeys::LeftMouseButton,0.f,FModifierKeysState()));
			Test->TestTrue(TEXT("Actual stand click exits wheelchair"),Elder->CurrentState==EElderState::Standing);
			Next=World->GetTimeSeconds()+.4f;break;
		case 16:
			StartPosition=Elder->GetActorLocation();SlateKey(EKeys::W,true);Next=World->GetTimeSeconds()+1.f;break;
		case 17:
			SlateKey(EKeys::W,false);
			Test->TestTrue(TEXT("W moves after stand button without extra viewport click"),FVector::Dist2D(StartPosition,Elder->GetActorLocation())>15.f);
			Slate.SetUserFocusToGameViewport(0); // Isolate later legacy checks after a baseline failure.
			Step=4;
			Key(EKeys::LeftAlt,true); Key(EKeys::RightAlt,true); Key(EKeys::LeftAlt,false);
			Test->TestTrue(TEXT("Other held Alt keeps UI open"),PC->bShowMouseCursor && PC->IsElderUICursorActive());
			Key(EKeys::RightAlt,false); Capture(TEXT("both Alts released"));
			SlateKey(EKeys::RightAlt,true); Slate.ClearKeyboardFocus(EFocusCause::Cleared);
			SlateKey(EKeys::RightAlt,false);
			Test->TestTrue(TEXT("Release outside game does not steal keyboard focus"),!Slate.GetKeyboardFocusedWidget().IsValid() && PC->bShowMouseCursor);
			Next=World->GetTimeSeconds()+.1f; break;
		case 4:
			Slate.SetKeyboardFocus(World->GetGameViewport()->GetGameViewportWidget(),EFocusCause::SetDirectly); Next=World->GetTimeSeconds()+.1f; break;
		case 5:
			Capture(TEXT("focus returns after missed release"));
			Key(EKeys::LeftAlt,true);
			Test->TestTrue(TEXT("Return via existing UI controller entry"),PC->ReleaseElderControl());
			SlateKey(EKeys::LeftAlt,false);
			Test->TestTrue(TEXT("Stale release leaves observer UI visible"),!PC->IsControllingElder() && !PC->IsElderUICursorActive() && PC->bShowMouseCursor && World->GetGameViewport()->GetMouseCaptureMode()==EMouseCaptureMode::NoCapture);
			Test->TestTrue(TEXT("Repeated takeover"),PC->TakeControlOfElder(Elder.Get()));
			Capture(TEXT("repeated takeover"));
			Key(EKeys::Tab,true); Key(EKeys::Tab,false); Next=World->GetTimeSeconds()+.15f; break;
		case 6:
			Test->TestTrue(TEXT("Tab returns observer after Alt interaction"),!PC->IsControllingElder() && PC->bShowMouseCursor);
			UE_LOG(LogTemp,Display,TEXT("ElderAltUI: input, Slate button click, focus, dual Alt and observer checks completed")); return true;
		}
		return false;
	}
private:
	void Key(FKey K,bool Down) { PC->InputKey(FInputKeyEventArgs(nullptr,INPUTDEVICEID_NONE,K,Down?IE_Pressed:IE_Released,Down?1.f:0.f,false,FPlatformTime::Cycles64())); }
	void SlateKey(FKey K,bool Down)
	{
		const FKeyEvent Event(K,FModifierKeysState(false,false,false,false,Down && K==EKeys::LeftAlt,Down && K==EKeys::RightAlt,false,false,false),0,false,0,0);
		if (Down) FSlateApplication::Get().ProcessKeyDownEvent(Event); else FSlateApplication::Get().ProcessKeyUpEvent(Event);
	}
	void Axis() { PC->InputKey(FInputKeyEventArgs(nullptr,INPUTDEVICEID_NONE,EKeys::MouseX,8.f,World->GetDeltaSeconds(),1,FPlatformTime::Cycles64())); }
	void Capture(const TCHAR* Label) { Test->TestTrue(FString::Printf(TEXT("%s captures mouse"),Label),!PC->IsElderUICursorActive() && !PC->bShowMouseCursor && World->GetGameViewport()->GetMouseCaptureMode()==EMouseCaptureMode::CapturePermanently_IncludingInitialMouseDown); }
	FAutomationTestBase* Test;
	double Deadline;
	TWeakObjectPtr<UWorld> World;
	TWeakObjectPtr<AOrbitCameraController> PC;
	TWeakObjectPtr<AElderlyAICharacter> Elder;
	TWeakObjectPtr<UButton> Button;
	FRotator StartView;
	FVector StartPosition;
	FVector2D Pointer;
	int32 Step=0;
	float Next=0.f;
	bool bInjectLook=false;
	bool bPrepared=false;
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FElderAltUITest,"OldMan.Elder.AltUI",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FElderAltUITest::RunTest(const FString& Parameters)
{
	if (!GEditor || !GEditor->GetEditorWorldContext().World()->GetMapName().Contains(TEXT("L_ElderControlTest"))) { AddError(TEXT("Open L_ElderControlTest first")); return false; }
	// A fresh PIE window supplies a real Slate hierarchy even when a Blueprint
	// editor tab hides the embedded level viewport used by FStartPIECommand.
	FRequestPlaySessionParams SessionParams;
	GEditor->RequestPlaySession(SessionParams);
	FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<FElderAltUIScenario>(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	return true;
}
#endif
