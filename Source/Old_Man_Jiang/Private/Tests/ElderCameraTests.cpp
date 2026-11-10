#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "Editor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/GameViewportClient.h"
#include "EngineUtils.h"
#include "InputKeyEventArgs.h"
#include "Camera/CameraComponent.h"
#include "Camera/PlayerCameraManager.h"
#include "OrbitCameraController.h"
#include "ElderlyAICharacter.h"

// Real input-component events, not SetControlRotation pretending to test mouse input.
class FElderCameraScenario : public IAutomationLatentCommand
{
public:
	explicit FElderCameraScenario(FAutomationTestBase* InTest) : Test(InTest), Deadline(FPlatformTime::Seconds()+25.) {}
	bool Update() override
	{
		if (FPlatformTime::Seconds()>Deadline) { Test->AddError(TEXT("Elder camera scenario timeout")); return true; }
		if (!World.IsValid())
		{
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
				if (Context.WorldType==EWorldType::PIE && Context.World()) World=Context.World();
			if (!World.IsValid()) return false;
			PC=Cast<AOrbitCameraController>(World->GetFirstPlayerController());
			for (TActorIterator<AElderlyAICharacter> It(World.Get()); It; ++It)
				if (It->ActorHasTag(TEXT("ElderTakeoverTestA"))) Elder=*It;
			if (!PC.IsValid() || !Elder.IsValid()) { Test->AddError(TEXT("Missing elder camera fixture")); return true; }
			OriginalPawn=PC->GetPawn(); OriginalView=PC->GetControlRotation();
			// The user may configure BP_AI to start seated; this suite covers walking mode.
			Elder->SetStanding();
			if (!PC->TakeControlOfElder(Elder.Get())) { Test->AddError(TEXT("Camera fixture takeover failed")); return true; }
			StartLocation=Elder->GetActorLocation(); StartBody=Elder->GetActorRotation();
			Next=World->GetTimeSeconds()+.3f;
		}
		if (!PC.IsValid() || !Elder.IsValid()) { Test->AddError(TEXT("Destroyed elder camera fixture")); return true; }
		if (bInjectLook)
		{
			Axis(EKeys::MouseX, LookSign*8.f);
			Axis(EKeys::MouseY, LookSign*4.f);
		}
		if (World->GetTimeSeconds()<Next) return false;
		switch (Step++)
		{
		case 0:
			CheckCapture(TEXT("takeover"));
			StartView=PC->GetControlRotation(); bInjectLook=true;
			Next=World->GetTimeSeconds()+.18f; break;
		case 1:
			bInjectLook=false; Next=World->GetTimeSeconds()+.15f; break;
		case 2:
			CheckLook(TEXT("mouse look without RMB"));
			Key(EKeys::RightMouseButton,true); Next=World->GetTimeSeconds()+.08f; break;
		case 3:
			Key(EKeys::RightMouseButton,false); Next=World->GetTimeSeconds()+.15f; break;
		case 4:
			CheckCapture(TEXT("after RMB release"));
			StartView=PC->GetControlRotation(); LookSign=-1.f; bInjectLook=true;
			Next=World->GetTimeSeconds()+.18f; break;
		case 5:
			bInjectLook=false; Next=World->GetTimeSeconds()+.15f; break;
		case 6:
			CheckLook(TEXT("mouse look after RMB click"));
			Test->TestTrue(TEXT("Looking does not rotate or move elderly body"),StartBody.Equals(Elder->GetActorRotation(),.1f) && FVector::Dist2D(StartLocation,Elder->GetActorLocation())<1.f && Elder->CurrentState==EElderState::Standing && !Elder->bTurningInPlace);
			PC->SetIgnoreLookInput(true); StartView=PC->GetControlRotation(); bInjectLook=true;
			Next=World->GetTimeSeconds()+.15f; break;
		case 7:
			bInjectLook=false; Next=World->GetTimeSeconds()+.15f; break;
		case 8:
			Test->TestTrue(TEXT("Explicit look-input suppression respected"),StartView.Equals(PC->GetControlRotation(),.1f));
			PC->SetIgnoreLookInput(false);
			Test->TestTrue(TEXT("Release elder control"),PC->ReleaseElderControl());
			Next=World->GetTimeSeconds()+.2f; break;
		case 9:
			Test->TestTrue(TEXT("Observation pawn and heading restored"),PC->GetPawn()==OriginalPawn.Get() && OriginalView.Equals(PC->GetControlRotation(),.1f));
			Test->TestTrue(TEXT("Observation cursor restored"),PC->bShowMouseCursor);
			Key(EKeys::RightMouseButton,true); Next=World->GetTimeSeconds()+.15f; break;
		case 10:
			Test->TestTrue(TEXT("Observation RMB drag still starts"),PC->bIsOrbitDragging && !PC->bShowMouseCursor);
			Key(EKeys::RightMouseButton,false); Next=World->GetTimeSeconds()+.15f; break;
		case 11:
			Test->TestTrue(TEXT("Observation RMB release still restores UI"),!PC->bIsOrbitDragging && PC->bShowMouseCursor && World->GetGameViewport()->GetMouseCaptureMode()==EMouseCaptureMode::NoCapture);
			UE_LOG(LogTemp,Display,TEXT("ElderCamera: all mouse/camera/capture/observation assertions completed"));
			return true;
		}
		return false;
	}
private:
	void Axis(FKey KeyName,float Value) { PC->InputKey(FInputKeyEventArgs(nullptr,INPUTDEVICEID_NONE,KeyName,Value,World->GetDeltaSeconds(),1,FPlatformTime::Cycles64())); }
	void Key(FKey KeyName,bool Down) { PC->InputKey(FInputKeyEventArgs(nullptr,INPUTDEVICEID_NONE,KeyName,Down?IE_Pressed:IE_Released,Down?1.f:0.f,false,FPlatformTime::Cycles64())); }
	void CheckCapture(const TCHAR* Label)
	{
		UGameViewportClient* Viewport=World->GetGameViewport();
		Test->TestTrue(FString::Printf(TEXT("%s retains game mouse capture"),Label),!PC->bShowMouseCursor && Viewport && Viewport->GetMouseCaptureMode()==EMouseCaptureMode::CapturePermanently_IncludingInitialMouseDown && Viewport->GetMouseLockMode()==EMouseLockMode::LockAlways);
		UE_LOG(LogTemp,Display,TEXT("ElderCamera %s cursor=%d capture=%d"),Label,PC->bShowMouseCursor,Viewport?int32(Viewport->GetMouseCaptureMode()):-1);
	}
	void CheckLook(const TCHAR* Label)
	{
		const FRotator View=PC->GetControlRotation();
		Test->TestTrue(FString::Printf(TEXT("%s changes yaw and pitch"),Label),FMath::Abs(FMath::FindDeltaAngleDegrees(StartView.Yaw,View.Yaw))>1.f && FMath::Abs(FMath::FindDeltaAngleDegrees(StartView.Pitch,View.Pitch))>1.f);
		Test->TestTrue(FString::Printf(TEXT("%s reaches actual camera"),Label),Elder->PlayerCamera->IsActive() && Elder->PlayerCamera->GetComponentRotation().Equals(View,.2f) && PC->GetViewTarget()==Elder.Get() && PC->PlayerCameraManager->GetCameraRotation().Equals(View,.2f));
		UE_LOG(LogTemp,Display,TEXT("ElderCamera %s control=%s camera=%s"),Label,*View.ToString(),*Elder->PlayerCamera->GetComponentRotation().ToString());
	}
	FAutomationTestBase* Test;
	double Deadline;
	TWeakObjectPtr<UWorld> World;
	TWeakObjectPtr<AOrbitCameraController> PC;
	TWeakObjectPtr<AElderlyAICharacter> Elder;
	TWeakObjectPtr<APawn> OriginalPawn;
	FVector StartLocation;
	FRotator StartBody,OriginalView,StartView;
	int32 Step=0;
	float Next=0.f,LookSign=1.f;
	bool bInjectLook=false;
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FElderCameraTest,"OldMan.Elder.CameraLook",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FElderCameraTest::RunTest(const FString& Parameters)
{
	if (!GEditor || !GEditor->GetEditorWorldContext().World()->GetMapName().Contains(TEXT("L_ElderControlTest"))) { AddError(TEXT("Open L_ElderControlTest first")); return false; }
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<FElderCameraScenario>(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	return true;
}
#endif
