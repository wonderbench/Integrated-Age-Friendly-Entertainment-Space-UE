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
#include "GameFramework/CharacterMovementComponent.h"

// Real EnhancedInput frames, including the transient diagonal produced while changing keys.
class FElderAdjacentTurnScenario : public IAutomationLatentCommand
{
public:
	explicit FElderAdjacentTurnScenario(FAutomationTestBase* InTest) : Test(InTest), Deadline(FPlatformTime::Seconds()+30.) {}
	~FElderAdjacentTurnScenario() { if (Elder.IsValid()) Elder->GetCharacterMovement()->ProcessRootMotionPostConvertToWorld.Unbind(); }
	bool Update() override
	{
		if (!World.IsValid())
		{
			for (const FWorldContext& C : GEngine->GetWorldContexts())
				if (C.WorldType == EWorldType::PIE && C.World() && C.World()->GetMapName().Contains(TEXT("L_ElderControlTest"))) World = C.World();
			if (!World.IsValid()) { if (FPlatformTime::Seconds()<Deadline) return false; Test->AddError(TEXT("PIE startup timeout")); return true; }
			PC = Cast<AOrbitCameraController>(World->GetFirstPlayerController());
			for (TActorIterator<AElderlyAICharacter> It(World.Get()); It; ++It) if (It->ActorHasTag(TEXT("ElderTakeoverTestA"))) Elder = *It;
			if (!PC.IsValid() || !Elder.IsValid() || !PC->TakeControlOfElder(Elder.Get())) { Test->AddError(TEXT("Missing elder takeover fixture")); return true; }
			PC->SetControlRotation(FRotator::ZeroRotator);
			Elder->GetCharacterMovement()->ProcessRootMotionPostConvertToWorld.BindLambda([this](const FTransform& Delta, UCharacterMovementComponent*, float)
			{
				if (bMeasuring) RootYawTravel += FMath::Abs(Delta.Rotator().Yaw);
				return Delta; // Observe the engine's actual motion without changing it.
			});
			Next = World->GetTimeSeconds()+.3f;
		}
		if (!Elder.IsValid() || !PC.IsValid()) { Test->AddError(TEXT("Fixture destroyed")); return true; }
		const float Now = World->GetTimeSeconds();
		if (bMeasuring)
		{
			if (Elder->bTurningInPlace && (!bWasTurning || Elder->TurnAnimationPhase+.01f < LastPhase)) ++Clips;
			bWasTurning = Elder->bTurningInPlace; LastPhase = Elder->TurnAnimationPhase;
			if (Elder->bTurningInPlace) MaxTurnTranslation = FMath::Max(MaxTurnTranslation, float(FVector::Dist2D(Start, Elder->GetActorLocation())));
		}
		if (Now<Next) return false;
		if (Phase==0)
		{
			if (Case>=10) { Elder->GetCharacterMovement()->ProcessRootMotionPostConvertToWorld.Unbind(); PC->ReleaseElderControl(); UE_LOG(LogTemp, Display, TEXT("ElderAdjacentTurnTest: all cases completed")); return true; }
			Elder->StopElderMovement(); Elder->SetStanding(); Elder->SetActorLocation(InitialLocation()); Elder->SetActorRotation(FRotator(0,-90,0));
			PC->SetControlRotation(FRotator::ZeroRotator);
			// Local right, brief forward/right -> right, backward, forward/left -> left, back/left.
			Next = Now+.15f; Phase=1; return false;
		}
		if (Phase==1)
		{
			Start=Elder->GetActorLocation(); Clips=0; MaxTurnTranslation=0; LastPhase=0; RootYawTravel=0; bWasTurning=false; bMeasuring=true;
			if (Case>=5)
			{
				// Trigger exactly one 90/180-degree clip without held input. Subsequent
				// CharacterMovement steering cannot conceal an end-of-animation snap-back.
				const FVector2D Input=Case==5?FVector2D(1,0):Case==6?FVector2D(-1,0):Case==7?FVector2D(0,-1):Case==8?FVector2D(-1,1):FVector2D(1,1);
				Elder->MoveElder(Input,123.f); Elder->ReleaseMovementInput();
				Next=Now+6.2f; Phase=2; return false;
			}
			TargetKey=Case==2||Case==4?EKeys::S:Case==3?EKeys::A:EKeys::D;
			if(Case==1||Case==3)Key(EKeys::W,true);
			if(Case==4)Key(EKeys::A,true);
			Key(TargetKey,true);
			Next=Now+((Case==1 || Case==3)? .06f : .01f); Phase=2; return false;
		}
		if (Phase==2)
		{
			if (Case>=5)
			{
				Test->TestTrue(TEXT("One-shot root turn finished standing"), !Elder->bTurningInPlace && Elder->CurrentState==EElderState::Standing);
				CompletedYaw=Elder->GetActorRotation().Yaw;
				Next=Now+1.f; Phase=3; return false;
			}
			if (Case==1 || Case==3) Key(EKeys::W,false);
			Next=Now+7.f; Phase=3; return false;
		}
		Test->TestEqual(FString::Printf(TEXT("Case %d exactly one turn clip"),Case),Clips,1);
		const float LocalOffsets[10]={90,90,180,-90,-135,90,-90,180,-45,45};
		Test->TestTrue(FString::Printf(TEXT("Case %d engine consumes real root rotation"),Case),RootYawTravel > FMath::Abs(LocalOffsets[Case])-10.f);
		Test->TestTrue(FString::Printf(TEXT("Case %d no movement during turn"),Case),MaxTurnTranslation<1.f);
		const float ExpectedYaw=-90.f+LocalOffsets[Case];
		Test->TestTrue(FString::Printf(TEXT("Case %d reaches final heading and correct state"),Case),!Elder->bTurningInPlace && Elder->CurrentState==(Case>=5?EElderState::Standing:EElderState::Walking) && FMath::Abs(FMath::FindDeltaAngleDegrees(Elder->GetActorRotation().Yaw,ExpectedYaw))<2.f);
		if (Case>=5) Test->TestTrue(TEXT("Idle after root turn does not restore old transform"), FMath::Abs(FMath::FindDeltaAngleDegrees(CompletedYaw,Elder->GetActorRotation().Yaw))<.25f && FVector::Dist2D(Start,Elder->GetActorLocation())<1.f);
		UE_LOG(LogTemp,Display,TEXT("AdjacentTurn case=%d clips=%d turnTranslation=%.3f finalYaw=%.2f rootYaw=%.2f"),Case,Clips,MaxTurnTranslation,Elder->GetActorRotation().Yaw,RootYawTravel);
		bMeasuring=false; Key(TargetKey,false); Key(EKeys::A,false); Elder->StopElderMovement();
		++Case; Phase=0; Next=Now+.2f; return false;
	}
private:
	FVector InitialLocation() const { return FVector(-500,0,90.15); }
	void Key(FKey K,bool Pressed) { PC->InputKey(FInputKeyEventArgs(nullptr,INPUTDEVICEID_NONE,K,Pressed?IE_Pressed:IE_Released,Pressed?1.f:0.f,false,FPlatformTime::Cycles64())); }
	FAutomationTestBase* Test;
	double Deadline;
	TWeakObjectPtr<UWorld> World;
	TWeakObjectPtr<AOrbitCameraController> PC;
	TWeakObjectPtr<AElderlyAICharacter> Elder;
	FVector Start;
	FKey TargetKey;
	int32 Phase=0,Case=0,Clips=0;
	float Next=0,LastPhase=0,MaxTurnTranslation=0,RootYawTravel=0,CompletedYaw=0;
	bool bMeasuring=false,bWasTurning=false;
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FElderAdjacentTurnTest,"OldMan.Elder.AdjacentTurn",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FElderAdjacentTurnTest::RunTest(const FString& Parameters)
{
	if (!GEditor || !GEditor->GetEditorWorldContext().World()->GetMapName().Contains(TEXT("L_ElderControlTest"))) { AddError(TEXT("Open L_ElderControlTest first")); return false; }
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<FElderAdjacentTurnScenario>(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	return true;
}
#endif
