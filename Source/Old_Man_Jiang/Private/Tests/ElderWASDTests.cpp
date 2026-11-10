#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "Editor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "InputKeyEventArgs.h"
#include "EnhancedPlayerInput.h"
#include "InputAction.h"
#include "OrbitCameraController.h"
#include "ElderlyAICharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

class FElderWASDScenario : public IAutomationLatentCommand
{
public:
	explicit FElderWASDScenario(FAutomationTestBase* T) : Test(T), Deadline(FPlatformTime::Seconds()+240.) {}
	bool Update() override
	{
		if(FPlatformTime::Seconds()>Deadline) { Test->AddError(TEXT("WASD timeout")); return true; }
		if(!World.IsValid())
		{
			for(const FWorldContext& C:GEngine->GetWorldContexts()) if(C.WorldType==EWorldType::PIE) World=C.World();
			if(!World.IsValid()) return false;
			PC=Cast<AOrbitCameraController>(World->GetFirstPlayerController());
			for(TActorIterator<AElderlyAICharacter> It(World.Get());It;++It) if(It->ActorHasTag(TEXT("ElderTakeoverTestA"))) Elder=*It;
			if(!PC.IsValid() || !Elder.IsValid() || !PC->TakeControlOfElder(Elder.Get())) { Test->AddError(TEXT("WASD missing fixture")); return true; }
			Test->TestTrue(TEXT("Saved move action uses Axis2D and additive mapping"),PC->ElderMoveAction->ValueType==EInputActionValueType::Axis2D && PC->ElderMoveAction->AccumulationBehavior==EInputActionAccumulationBehavior::Cumulative);
			PC->SetIgnoreLookInput(true);
			Next=World->GetTimeSeconds()+.4f;
		}
		if(!PC.IsValid() || !Elder.IsValid()) { Test->AddError(TEXT("WASD lost fixture")); return true; }
		const float Now=World->GetTimeSeconds();
		if(bMeasure && Elder->bTurningInPlace) Test->TestTrue(Label(TEXT("turn has zero translation")),Elder->GetVelocity().Size2D()<1.f);
		if(Now<Next) return false;
		switch(Stage)
		{
		case 0:
			if(Case==19)
			{
				Elder->StopElderMovement(); Elder->SetActorRotation(FRotator(0,37,0));
				Elder->MoveElder(FVector2D(1,1),37.f); Elder->SetSitting();
				Test->TestTrue(TEXT("Disabled state clears pending manual intent"),Elder->MoveInput.IsNearlyZero() && !Elder->bTurningInPlace && !Elder->IsActorTickEnabled());
				Elder->SetStanding(); Elder->MoveElder(FVector2D(1,1),37.f);
				PC->ReleaseElderControl();
				Test->TestTrue(TEXT("Return to AI clears pending manual intent"),Elder->MoveInput.IsNearlyZero() && !Elder->bTurningInPlace && !Elder->IsActorTickEnabled() && Elder->GetCharacterMovement()->bOrientRotationToMovement);
				PC->SetIgnoreLookInput(false); UE_LOG(LogTemp,Display,TEXT("ElderWASD: all 19 tap/hold/order/release cases and lifecycle checks completed")); return true;
			}
			ReleaseKeys(); Elder->StopElderMovement(); Elder->SetStanding();
			Input=Directions[Case<6?Case:Case<14?Case-6:(Case<16 || Case==18)?2:0];
			Expected=37.f+FMath::RadiansToDegrees(FMath::Atan2(Input.X,Input.Y));
			PC->SetControlRotation(FRotator(0,37,0));
			Elder->SetActorLocation(FVector(-500,0,90.15));
			Elder->SetActorRotation(FRotator(0,Case==18?37.f:Case>=16?Expected:Expected-90.f,0));
			Next=Now+.3f; Stage=1; break;
		case 1:
			Start=Elder->GetActorLocation(); bMeasure=true; Started=Now;
			Expected=Elder->GetActorRotation().Yaw+FMath::RadiansToDegrees(FMath::Atan2(Input.X,Input.Y));
			if(Case==14) Key(EKeys::W,true); else if(Case==15 || Case==18) Key(EKeys::D,true); else PressDirection(Input);
			Next=Now+(Case==18?.45f:.055f); Stage=2; break;
		case 2:
			if(Case==14 || Case==15 || Case==18) { if(Case==18) Test->TestTrue(TEXT("Turn is already playing before retarget"),Elder->bTurningInPlace); Expected=Elder->GetActorRotation().Yaw+45.f; Key(Case==14?EKeys::D:EKeys::W,true); Next=Now+.055f; Stage=3; break; }
			Stage=3; Next=Now; break;
		case 3:
			Test->TestTrue(Label(TEXT("normalized composed vector")),Elder->MoveInput.Equals(Input.GetSafeNormal(),.01f));
			Test->TestTrue(Label(TEXT("actor-local target")),FMath::Abs(FMath::FindDeltaAngleDegrees(Elder->TargetFacingYaw,Expected))<3.f);
			Expected=Elder->TargetFacingYaw; // Later endpoint checks use the actual input-frame sample.
			if(Case<6 || Case==16 || Case==18) ReleaseKeys();
			if(Case==17) { Test->TestTrue(Label(TEXT("aligned hold does not walk before threshold")),FVector::Dist2D(Start,Elder->GetActorLocation())<.1f); }
			Next=Now+.05f; Stage=4; break;
		case 4:
			if(Elder->bTurningInPlace || FMath::Abs(FMath::FindDeltaAngleDegrees(Elder->GetActorRotation().Yaw,Expected))>2.f) { Next=Now+.05f; break; }
			if(Now-Started<.35f) { Next=Now+.05f; break; }
			Sample=Elder->GetActorLocation(); Next=Now+.5f; Stage=5; break;
		case 5:
			if(Case<6 || Case==16 || Case==18) Test->TestTrue(Label(TEXT("tap completes facing without walking")),FVector::Dist2D(Start,Elder->GetActorLocation())<1.f && Elder->CurrentState==EElderState::Standing);
			else
			{
				const FVector Delta=Elder->GetActorLocation()-Sample;
				const FVector Forward=FRotator(0,Expected,0).Vector();
				Test->TestTrue(Label(TEXT("hold walks in target direction")),FVector::DotProduct(Delta,Forward)>15.f && FMath::Abs(FVector::DotProduct(Delta,FVector::CrossProduct(Forward,FVector::UpVector)))<2.f);
				Test->TestTrue(Label(TEXT("diagonal speed equals cardinal limit")),FMath::Abs(Elder->GetVelocity().Size2D()-64.f)<2.f);
			}
			if(Case==14 || Case==15)
			{
				Expected=Elder->GetActorRotation().Yaw+(Case==14?0.f:90.f); Key(Case==14?EKeys::D:EKeys::W,false);
				Next=Now+.06f; Stage=6; break;
			}
			Stage=8; Next=Now; break;
		case 6:
			Test->TestTrue(Label(TEXT("partial release updates target")),FMath::Abs(FMath::FindDeltaAngleDegrees(Elder->TargetFacingYaw,Expected))<.5f);
			Next=Now+.05f; Stage=7; break;
		case 7:
			if(Elder->bTurningInPlace || FMath::Abs(FMath::FindDeltaAngleDegrees(Elder->GetActorRotation().Yaw,Expected))>2.f || Elder->GetVelocity().Size2D()<50.f) { Next=Now+.05f; break; }
			Stage=8; Next=Now; break;
		case 8:
			UE_LOG(LogTemp,Display,TEXT("ElderWASD case=%d target=%.2f yaw=%.2f speed=%.2f input=%s"),Case,Expected,Elder->GetActorRotation().Yaw,Elder->GetVelocity().Size2D(),*Elder->MoveInput.ToString());
			ReleaseKeys(); bMeasure=false; ++Case; Stage=0; Next=Now+.25f; break;
		}
		return false;
	}
private:
	FString Label(const TCHAR* S) const { return FString::Printf(TEXT("WASD case %d %s"),Case,S); }
	void Key(FKey K,bool Down) { PC->InputKey(FInputKeyEventArgs(nullptr,INPUTDEVICEID_NONE,K,Down?IE_Pressed:IE_Released,Down?1.f:0.f,false,FPlatformTime::Cycles64())); }
	void ReleaseKeys() { for(FKey K:{EKeys::W,EKeys::A,EKeys::S,EKeys::D}) Key(K,false); }
	void PressDirection(FVector2D V) { if(V.Y>0) Key(EKeys::W,true); if(V.Y<0) Key(EKeys::S,true); if(V.X>0) Key(EKeys::D,true); if(V.X<0) Key(EKeys::A,true); }
	const FVector2D Directions[8]={{0,1},{1,0},{1,1},{-1,1},{1,-1},{-1,-1},{0,-1},{-1,0}};
	FAutomationTestBase* Test; double Deadline;
	TWeakObjectPtr<UWorld> World; TWeakObjectPtr<AOrbitCameraController> PC; TWeakObjectPtr<AElderlyAICharacter> Elder;
	int32 Case=0,Stage=0; float Next=0,Started=0,Expected=0; bool bMeasure=false;
	FVector2D Input; FVector Start,Sample;
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FElderWASDTest,"OldMan.Elder.WASD",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FElderWASDTest::RunTest(const FString& Parameters)
{
	if(!GEditor || !GEditor->GetEditorWorldContext().World()->GetMapName().Contains(TEXT("L_ElderControlTest"))) { AddError(TEXT("Open L_ElderControlTest first")); return false; }
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<FElderWASDScenario>(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand()); return true;
}

// Compare the actual EnhancedInput result, not an assumption about the enum name.
class FElderMappingScenario : public IAutomationLatentCommand
{
public:
	explicit FElderMappingScenario(FAutomationTestBase* T):Test(T),Deadline(FPlatformTime::Seconds()+20.) {}
	~FElderMappingScenario() { if(Action.IsValid()) Action->AccumulationBehavior=EInputActionAccumulationBehavior::Cumulative; }
	bool Update() override
	{
		if(FPlatformTime::Seconds()>Deadline) { Test->AddError(TEXT("Mapping test timeout")); return true; }
		if(!PC.IsValid())
		{
			UWorld* W=nullptr; for(const FWorldContext& C:GEngine->GetWorldContexts()) if(C.WorldType==EWorldType::PIE) W=C.World();
			if(!W) return false;
			PC=Cast<AOrbitCameraController>(W->GetFirstPlayerController());
			AElderlyAICharacter* E=nullptr; for(TActorIterator<AElderlyAICharacter> It(W);It;++It) { E=*It; break; }
			if(!PC.IsValid() || !E || !PC->TakeControlOfElder(E)) { Test->AddError(TEXT("Mapping fixture missing")); return true; }
			Action=PC->ElderMoveAction; Next=W->GetTimeSeconds()+.3f;
		}
		const float Now=PC->GetWorld()->GetTimeSeconds(); if(Now<Next) return false;
		UEnhancedPlayerInput* EnhancedInput=Cast<UEnhancedPlayerInput>(PC->PlayerInput);
		if(!EnhancedInput) { Test->AddError(TEXT("EnhancedPlayerInput missing")); return true; }
		switch(Step++)
		{
		case 0: Action->AccumulationBehavior=EInputActionAccumulationBehavior::TakeHighestAbsoluteValue; Key(EKeys::W,true); Key(EKeys::D,true); break;
		case 1:
			UE_LOG(LogTemp,Display,TEXT("ElderMapping baseline Highest WD=%s"),*EnhancedInput->GetActionValue(Action.Get()).Get<FVector2D>().ToString());
			Key(EKeys::W,false); Key(EKeys::D,false); Action->AccumulationBehavior=EInputActionAccumulationBehavior::Cumulative; break;
		case 2: Key(EKeys::W,true); Key(EKeys::D,true); break;
		case 3:
			Test->TestTrue(TEXT("W+D accumulates both axes"),EnhancedInput->GetActionValue(Action.Get()).Get<FVector2D>().Equals(FVector2D(1,1),.01f));
			UE_LOG(LogTemp,Display,TEXT("ElderMapping Cumulative WD=%s"),*EnhancedInput->GetActionValue(Action.Get()).Get<FVector2D>().ToString());
			Key(EKeys::D,false); Key(EKeys::S,true); break;
		case 4:
			Test->TestTrue(TEXT("W+S cancels"),EnhancedInput->GetActionValue(Action.Get()).Get<FVector2D>().IsNearlyZero());
			Key(EKeys::W,false); Key(EKeys::S,false); Key(EKeys::A,true); Key(EKeys::D,true); break;
		case 5:
			Test->TestTrue(TEXT("A+D cancels"),EnhancedInput->GetActionValue(Action.Get()).Get<FVector2D>().IsNearlyZero());
			Key(EKeys::A,false); Key(EKeys::D,false); PC->ReleaseElderControl(); return true;
		}
		Next=Now+.12f; return false;
	}
private:
	void Key(FKey K,bool Down) { PC->InputKey(FInputKeyEventArgs(nullptr,INPUTDEVICEID_NONE,K,Down?IE_Pressed:IE_Released,Down?1.f:0.f,false,FPlatformTime::Cycles64())); }
	FAutomationTestBase* Test; double Deadline; TWeakObjectPtr<AOrbitCameraController> PC; TWeakObjectPtr<UInputAction> Action; int32 Step=0; float Next=0;
};
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FElderMappingTest,"OldMan.Elder.InputMapping",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FElderMappingTest::RunTest(const FString& Parameters)
{
	if(!GEditor || !GEditor->GetEditorWorldContext().World()->GetMapName().Contains(TEXT("L_ElderControlTest"))) { AddError(TEXT("Open L_ElderControlTest first")); return false; }
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<FElderMappingScenario>(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand()); return true;
}
#endif
