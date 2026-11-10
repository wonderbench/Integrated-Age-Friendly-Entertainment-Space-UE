#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "InputKeyEventArgs.h"
#include "ElderlyAICharacter.h"
#include "ElderlyAIController.h"
#include "ElderAnimInstance.h"
#include "MonitorCameraActor.h"
#include "OrbitCameraController.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Navigation/PathFollowingComponent.h"

// Real input frames and frame-by-frame continuity, not only final heading checks.
class FWalkingSteeringScenario : public IAutomationLatentCommand
{
public:
 explicit FWalkingSteeringScenario(FAutomationTestBase* T):Test(T),Deadline(FPlatformTime::Seconds()+85.){}
 ~FWalkingSteeringScenario(){if(PC.IsValid())Keys(false,false,false,false);}
 bool Update() override
 {
  if(FPlatformTime::Seconds()>Deadline){Test->AddError(FString::Printf(TEXT("WalkingSteering timeout stage %d"),Stage));return true;}
  if(!World.IsValid())
  {
   for(const FWorldContext& C:GEngine->GetWorldContexts())if(C.WorldType==EWorldType::PIE)World=C.World();
   if(!World.IsValid())return false;
   PC=Cast<AOrbitCameraController>(World->GetFirstPlayerController());
   for(TActorIterator<AElderlyAICharacter>I(World.Get());I;++I)if(I->ActorHasTag(TEXT("ElderTakeoverTestA")))Elder=*I;
   if(!PC.IsValid()||!Elder.IsValid()){Test->AddError(TEXT("Open L_ElderControlTest"));return true;}
   Elder->SetStanding();Elder->SetActorRotation(FRotator::ZeroRotator);
   AI=Cast<AElderlyAIController>(Elder->GetController());
   if(!AI.IsValid()||!PC->TakeControlOfElder(Elder.Get())){Test->AddError(TEXT("Takeover failed"));return true;}
   Anim=Cast<UElderAnimInstance>(Elder->GetMesh()->GetAnimInstance());
   if(!Anim.IsValid()){Test->AddError(TEXT("Existing ElderAnimInstance missing"));return true;}
   FActorSpawnParameters P;P.ObjectFlags|=RF_Transient;
   Monitor=World->SpawnActor<AMonitorCameraActor>(Elder->GetActorLocation(),FRotator::ZeroRotator,P);
   Monitor->RegisterElder(Elder.Get());
   Next=World->GetTimeSeconds()+.4f;return false;
  }
  const float Now=World->GetTimeSeconds();
  if(bMeasure)
  {
   MinSpeed=FMath::Min(MinSpeed,float(Elder->GetVelocity().Size2D()));
   const float YawStep=FMath::Abs(FMath::FindDeltaAngleDegrees(LastYaw,Elder->GetActorRotation().Yaw));
   MaxStep=FMath::Max(MaxStep,YawStep);
   Test->TestTrue(TEXT("Moving turn never enters stationary TIP"),!Elder->bTurningInPlace&&!Anim->RootTurnActive);
   Test->TestTrue(TEXT("Moving turn remains Walking"),Elder->CurrentState==EElderState::Walking);
   Test->TestTrue(TEXT("Moving yaw is bounded per frame"),YawStep<=Elder->MaxMovingTurnRate*FMath::Min(World->GetDeltaSeconds(),.05f)+.15f);
   Test->TestTrue(TEXT("Locomotion still sees velocity"),Anim->LocomotionBlendForward>20.f);
   LastYaw=Elder->GetActorRotation().Yaw;
  }
  if(Now<Next)return false;
  switch(Stage++)
  {
  case 0: Keys(true,false,false,false);Next=Now+.65f;break;
  case 1:
   Test->TestTrue(TEXT("W reaches forward walking"),Elder->CurrentState==EElderState::Walking&&Elder->GetVelocity().X>60.f);
   Hist=Monitor->StateHistory.Num();bMeasure=true;LastYaw=Elder->GetActorRotation().Yaw;Start=Elder->GetActorLocation();
   Expected=LastYaw+45.f;Keys(true,false,false,true);PC->SetControlRotation(FRotator(0,143,0));Next=Now+.2f;break;
  case 2:
   Test->TestTrue(TEXT("WD moves before finishing its turn"),FVector::Dist2D(Start,Elder->GetActorLocation())>5.f&&Elder->GetActorRotation().Yaw>1.f&&Elder->GetActorRotation().Yaw<40.f);
   Test->TestTrue(TEXT("WD is local 45, not camera heading"),FMath::Abs(FMath::FindDeltaAngleDegrees(Expected,Elder->TargetFacingYaw))<.2f);
   Next=Now+3.f;break;
  case 3:
   CheckHeading(TEXT("WD settles northeast"));
   Elder->TurnSpeed=2.f;Expected=Elder->GetActorRotation().Yaw-45.f;Keys(true,true,false,false);Next=Now+4.f;break;
  case 4:
   CheckHeading(TEXT("WA settles local left-forward"));
   Elder->TurnSpeed=5.f;Keys(true,false,false,false);Next=Now+.12f;break;
  case 5: Keys(true,false,false,true);Next=Now+.15f;break;
  case 6: Keys(true,true,false,false);Next=Now+.15f;break;
  case 7: Expected=Elder->GetActorRotation().Yaw+135.f;Keys(false,false,true,true);Next=Now+.15f;break;
  case 8:
   Test->TestTrue(TEXT("Rapid SD uses current local direction"),FMath::Abs(FMath::FindDeltaAngleDegrees(Expected,Elder->TargetFacingYaw))<5.f);
   Test->TestTrue(TEXT("No turn-induced monitor transitions"),Monitor->StateHistory.Num()==Hist);
   Test->TestTrue(TEXT("No speed clearing during WD/WA/SD"),MinSpeed>50.f);
   UE_LOG(LogTemp,Display,TEXT("WalkingSteering continuous minSpeed=%.3f maxYawStep=%.3f historyDelta=%d"),MinSpeed,MaxStep,Monitor->StateHistory.Num()-Hist);
   bMeasure=false;Expected=Elder->TargetFacingYaw;ReleaseYaw=Elder->GetActorRotation().Yaw;Keys(false,false,false,false);Next=Now+.05f;break;
  case 9: Start=Elder->GetActorLocation();Next=Now+3.f;break;
  case 10:
   CheckHeading(TEXT("Released input completes smooth tail"));
   Test->TestTrue(TEXT("Released tail rotates but does not translate"),FMath::Abs(FMath::FindDeltaAngleDegrees(ReleaseYaw,Elder->GetActorRotation().Yaw))>10.f&&FVector::Dist2D(Start,Elder->GetActorLocation())<.2f&&Elder->CurrentState==EElderState::Standing);
   Expected=Elder->GetActorRotation().Yaw+90.f;Start=Elder->GetActorLocation();Keys(false,false,false,true);Next=Now+.06f;break;
  case 11: Keys(false,false,false,false);Next=Now+4.5f;break;
  case 12:
   CheckHeading(TEXT("Idle tap D still completes root turn"));
   Test->TestTrue(TEXT("Idle tap does not walk"),FVector::Dist2D(Start,Elder->GetActorLocation())<1.f);
   Expected=Elder->GetActorRotation().Yaw;Keys(true,false,false,false);Next=Now+.6f;break;
  case 13:
   CheckHeading(TEXT("W follows latest 90-degree turned actor"));
   Test->TestTrue(TEXT("Velocity follows current forward"),FVector::DotProduct(Elder->GetVelocity().GetSafeNormal2D(),Elder->GetActorForwardVector())>.99f);
   Keys(true,false,false,true);Next=Now+.15f;break;
  case 14:
   Keys(false,false,false,false);Elder->SetSitting();ReleaseYaw=Elder->GetActorRotation().Yaw;Next=Now+.3f;break;
  case 15:
   Test->TestTrue(TEXT("Sitting cancels smooth turn"),FMath::Abs(FMath::FindDeltaAngleDegrees(ReleaseYaw,Elder->GetActorRotation().Yaw))<.01f&&!Elder->IsActorTickEnabled());
   Elder->SetStanding();Keys(true,false,false,false);Next=Now+.6f;break;
  case 16: Keys(true,false,false,true);Next=Now+.15f;break;
  case 17:
   Keys(false,false,false,false);Elder->EnterWheelchair();ReleaseYaw=Elder->GetActorRotation().Yaw;Next=Now+.3f;break;
  case 18:
   Test->TestTrue(TEXT("Wheelchair mode cancels walking steering"),FMath::Abs(FMath::FindDeltaAngleDegrees(ReleaseYaw,Elder->GetActorRotation().Yaw))<.01f&&!Elder->IsActorTickEnabled());
   Elder->ExitWheelchair();Keys(true,false,false,false);Next=Now+.6f;break;
  case 19: Keys(true,false,false,true);Next=Now+.15f;break;
  case 20:
   Keys(false,false,false,false);PC->ReleaseElderControl();
   Test->TestTrue(TEXT("AI restored after active smooth turn"),Elder->GetController()==AI.Get()&&Elder->GetCharacterMovement()->bOrientRotationToMovement&&!Elder->IsActorTickEnabled());
   Goal=Elder->GetActorLocation()+FVector(150,0,0);{FAIMoveRequest R(Goal);R.SetAcceptanceRadius(10.f);R.SetReachTestIncludesAgentRadius(false);R.SetReachTestIncludesGoalRadius(false);Test->TestTrue(TEXT("Standing AI MoveTo accepted"),AI->MoveTo(R).Code==EPathFollowingRequestResult::RequestSuccessful);}
   Next=Now+4.f;break;
  case 21:
   UE_LOG(LogTemp,Display,TEXT("WalkingSteering AI goalDistance=%.3f state=%d pathStatus=%d"),FVector::Dist2D(Goal,Elder->GetActorLocation()),int32(Elder->CurrentState),int32(AI->GetMoveStatus()));
   Test->TestTrue(TEXT("AI navigation arrives after steering takeover"),FVector::Dist2D(Goal,Elder->GetActorLocation())<25.f&&Elder->CurrentState==EElderState::Standing);
   PC->TakeControlOfElder(Elder.Get());Keys(true,false,false,false);Next=Now+.6f;break;
  case 22: Keys(true,false,false,true);Next=Now+.15f;break;
  case 23: Keys(false,false,false,false);Next=Now+.05f;break;
  case 24:
   Start=Elder->GetActorLocation();Expected=Elder->GetActorRotation().Yaw+90.f;
   Keys(false,false,false,true);Next=Now+.06f;break;
  case 25: Keys(false,false,false,false);Next=Now+.4f;break;
  case 26:
   Test->TestTrue(TEXT("Fresh idle tap during release tail uses real root TIP"),Elder->bTurningInPlace&&Anim->RootTurnActive);
   Next=Now+4.f;break;
  case 27:
   CheckHeading(TEXT("Fresh tail-time tap reaches local right"));
   Test->TestTrue(TEXT("Fresh tail-time tap does not translate"),FVector::Dist2D(Start,Elder->GetActorLocation())<1.f);
   PC->ReleaseElderControl();return true;
  }
  return false;
 }
private:
 void CheckHeading(const TCHAR* Label){Test->TestTrue(Label,FMath::Abs(FMath::FindDeltaAngleDegrees(Expected,Elder->GetActorRotation().Yaw))<.2f);}
 void Key(FKey K,bool Down){PC->InputKey(FInputKeyEventArgs(nullptr,INPUTDEVICEID_NONE,K,Down?IE_Pressed:IE_Released,Down?1.f:0.f,false,FPlatformTime::Cycles64()));}
 void Keys(bool W,bool A,bool S,bool D){const bool New[4]={W,A,S,D};const FKey K[4]={EKeys::W,EKeys::A,EKeys::S,EKeys::D};for(int32 I=0;I<4;++I)if(New[I]!=Held[I]){Key(K[I],New[I]);Held[I]=New[I];}}
 FAutomationTestBase* Test;double Deadline;float Next=0,Expected=0,LastYaw=0,ReleaseYaw=0,MinSpeed=1000,MaxStep=0;int32 Stage=0,Hist=0;bool bMeasure=false,Held[4]={};
 FVector Start,Goal;
 TWeakObjectPtr<UWorld> World;TWeakObjectPtr<AOrbitCameraController> PC;TWeakObjectPtr<AElderlyAICharacter> Elder;TWeakObjectPtr<AElderlyAIController> AI;TWeakObjectPtr<UElderAnimInstance> Anim;TWeakObjectPtr<AMonitorCameraActor> Monitor;
};
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWalkingSteeringTest,"OldMan.Elder.WalkingSteering",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FWalkingSteeringTest::RunTest(const FString&)
{
 ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
 ADD_LATENT_AUTOMATION_COMMAND(FWalkingSteeringScenario(this));
 ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());return true;
}
#endif
