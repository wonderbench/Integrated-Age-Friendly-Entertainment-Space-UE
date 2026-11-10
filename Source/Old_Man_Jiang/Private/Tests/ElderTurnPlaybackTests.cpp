#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "Editor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "InputKeyEventArgs.h"
#include "OrbitCameraController.h"
#include "ElderlyAICharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/App.h"

// Exercise real press AND release, and observe the entire motion, not just its endpoint.
class FElderTurnPlaybackScenario : public IAutomationLatentCommand
{
public:
	explicit FElderTurnPlaybackScenario(FAutomationTestBase* InTest) : Test(InTest), Deadline(FPlatformTime::Seconds()+100.) {}
	~FElderTurnPlaybackScenario() { if (Elder.IsValid()) Elder->GetCharacterMovement()->ProcessRootMotionPostConvertToWorld.Unbind(); }
	bool Update() override
	{
		if (FPlatformTime::Seconds()>Deadline) { Test->AddError(TEXT("Playback scenario timeout")); return true; }
		if (!World.IsValid())
		{
			for (const FWorldContext& C : GEngine->GetWorldContexts()) if (C.WorldType==EWorldType::PIE && C.World()) World=C.World();
			if (!World.IsValid()) return false;
			PC=Cast<AOrbitCameraController>(World->GetFirstPlayerController());
			for (TActorIterator<AElderlyAICharacter> It(World.Get()); It; ++It) if (It->ActorHasTag(TEXT("ElderTakeoverTestA"))) Elder=*It;
			if (!PC.IsValid() || !Elder.IsValid() || !PC->TakeControlOfElder(Elder.Get())) { Test->AddError(TEXT("Missing playback fixture")); return true; }
			Elder->GetCharacterMovement()->ProcessRootMotionPostConvertToWorld.BindLambda([this](const FTransform& Delta,UCharacterMovementComponent*,float DeltaSeconds)
			{
				if(bMeasure && FMath::Abs(Delta.Rotator().Yaw)>12.f)
					UE_LOG(LogTemp,Display,TEXT("TurnPlayback largeRoot case=%d dt=%.6f worldDt=%.6f phase=%.6f stepYaw=%.3f gameTime=%.3f"),Case,DeltaSeconds,World->GetDeltaSeconds(),Elder->TurnAnimationPhase,Delta.Rotator().Yaw,World->GetTimeSeconds());
				if (bMeasure) { SignedRoot+=Delta.Rotator().Yaw; AbsoluteRoot+=FMath::Abs(Delta.Rotator().Yaw); MaxRootStep=FMath::Max(MaxRootStep,float(FMath::Abs(Delta.Rotator().Yaw))); }
				return Delta;
			});
			Next=World->GetTimeSeconds()+.3f;
		}
		if (!Elder.IsValid() || !PC.IsValid()) { Test->AddError(TEXT("Destroyed playback fixture")); return true; }
		const float Now=World->GetTimeSeconds();
		if (bMeasure)
		{
			const float Yaw=Elder->GetActorRotation().Yaw;
			MaxActorStep=FMath::Max(MaxActorStep,FMath::Abs(FMath::FindDeltaAngleDegrees(LastYaw,Yaw)));
			LastYaw=Yaw;
			if (Elder->bTurningInPlace) MaxFootTravel=FMath::Max(MaxFootTravel,float(FVector::Dist(StartFoot,Elder->GetMesh()->GetSocketTransform(TEXT("foot_l"),RTS_Component).GetLocation())));
			MaxRootPose=FMath::Max(MaxRootPose,float(FMath::Abs(Elder->GetMesh()->GetSocketTransform(TEXT("root"),RTS_Component).Rotator().Yaw)));
			if (Now-Began<.15f) EarlyYaw=FMath::Max(EarlyYaw,FMath::Abs(FMath::FindDeltaAngleDegrees(StartYaw,Yaw)));
			if (!bMiddle && Now-Began>=1.5f)
			{
				bMiddle=true; MiddleYaw=FMath::Abs(FMath::FindDeltaAngleDegrees(StartYaw,Yaw));
				Test->TestTrue(Label(TEXT("animation remains active after tap")),Elder->bTurningInPlace);
				Test->TestTrue(Label(TEXT("middle pose is a partial turn, not endpoint")),MiddleYaw>8.f && MiddleYaw<80.f);
				Test->TestTrue(Label(TEXT("actual animation clock is mid-clip")),Elder->TurnAnimationPhase>.2f && Elder->TurnAnimationPhase<.8f);
			}
		}
		if (Now<Next) return false;
		if (Stage==0)
		{
			if (Case==8) { PC->ReleaseElderControl(); return true; }
			Elder->StopElderMovement(); Elder->SetStanding();
			StartYaw=(Case%4==0)?-90.f:(Case%4==1)?0.f:(Case%4==2)?90.f:180.f;
			Elder->SetActorLocation(FVector(-500,0,90.15)); Elder->SetActorRotation(FRotator(0,StartYaw,0));
			PC->SetControlRotation(FRotator::ZeroRotator);
			Target=EKeys::D; // Actor-local right is +90 at every initial heading.
			Next=Now+.3f; Stage=1; return false;
		}
		if (Stage==1)
		{
			SignedRoot=AbsoluteRoot=MaxRootStep=MaxActorStep=EarlyYaw=MiddleYaw=MaxRootPose=MaxFootTravel=0; bMiddle=false;
			StartFoot=Elder->GetMesh()->GetSocketTransform(TEXT("foot_l"),RTS_Component).GetLocation();
			LastYaw=StartYaw; Start=Elder->GetActorLocation(); bMeasure=true; Began=Now;
			Key(true); Next=Now+(Case<4?.06f:3.5f); Stage=2; return false;
		}
		if (Stage==2) { Key(false); Next=Began+5.0f; Stage=3; return false; }
		if (Stage==3)
		{
			EndYaw=Elder->GetActorRotation().Yaw; EndPosition=Elder->GetActorLocation();
			Test->TestTrue(Label(TEXT("completed and standing")),!Elder->bTurningInPlace && Elder->CurrentState==EElderState::Standing);
			Test->TestTrue(Label(TEXT("target heading without corrective input")),FMath::Abs(FMath::FindDeltaAngleDegrees(StartYaw+90.f,EndYaw))<2.f);
			Test->TestTrue(Label(TEXT("no first-frame snap")),EarlyYaw<12.f && MaxActorStep<12.f && MaxRootStep<12.f);
			Test->TestTrue(Label(TEXT("root rotation travels forward once only")),FMath::Abs(SignedRoot-90.f)<2.f && AbsoluteRoot<94.f);
			Test->TestTrue(Label(TEXT("root bone remains extracted")),MaxRootPose<1.f);
			if (FApp::CanEverRender()) Test->TestTrue(Label(TEXT("foot pose animates independently of capsule")),MaxFootTravel>4.f);
			if (Case<4) Test->TestTrue(Label(TEXT("tap turns without walking")),FVector::Dist2D(Start,EndPosition)<1.f);
			Next=Now+1.f; Stage=4; return false;
		}
		Test->TestTrue(Label(TEXT("no return after blend to idle")),FMath::Abs(FMath::FindDeltaAngleDegrees(EndYaw,Elder->GetActorRotation().Yaw))<.25f && FVector::Dist2D(EndPosition,Elder->GetActorLocation())<.25f);
		UE_LOG(LogTemp,Display,TEXT("TurnPlayback case=%d tap=%d early=%.2f middle=%.2f maxActorStep=%.2f maxRootStep=%.2f signedRoot=%.2f absRoot=%.2f final=%.2f rootPose=%.2f footTravel=%.2f"),Case,Case<4,EarlyYaw,MiddleYaw,MaxActorStep,MaxRootStep,SignedRoot,AbsoluteRoot,Elder->GetActorRotation().Yaw,MaxRootPose,MaxFootTravel);
		bMeasure=false; ++Case; Stage=0; Next=Now+.2f; return false;
	}
private:
	FString Label(const TCHAR* What) const { return FString::Printf(TEXT("Playback case %d %s"),Case,What); }
	void Key(bool Down) { PC->InputKey(FInputKeyEventArgs(nullptr,INPUTDEVICEID_NONE,Target,Down?IE_Pressed:IE_Released,Down?1.f:0.f,false,FPlatformTime::Cycles64())); }
	FAutomationTestBase* Test; double Deadline;
	TWeakObjectPtr<UWorld> World; TWeakObjectPtr<AOrbitCameraController> PC; TWeakObjectPtr<AElderlyAICharacter> Elder;
	FKey Target; FVector Start,EndPosition,StartFoot;
	int32 Case=0,Stage=0; float Next=0,Began=0,StartYaw=0,LastYaw=0,EndYaw=0,SignedRoot=0,AbsoluteRoot=0,MaxRootStep=0,MaxActorStep=0,EarlyYaw=0,MiddleYaw=0,MaxRootPose=0,MaxFootTravel=0;
	bool bMeasure=false,bMiddle=false;
};
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FElderTurnPlaybackTest,"OldMan.Elder.TurnPlayback",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FElderTurnPlaybackTest::RunTest(const FString& Parameters)
{
	if (!GEditor || !GEditor->GetEditorWorldContext().World()->GetMapName().Contains(TEXT("L_ElderControlTest"))) { AddError(TEXT("Open L_ElderControlTest first")); return false; }
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<FElderTurnPlaybackScenario>(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	return true;
}
#endif
