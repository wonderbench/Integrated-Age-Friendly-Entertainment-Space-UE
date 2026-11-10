#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "InputKeyEventArgs.h"
#include "Camera/CameraComponent.h"
#include "OrbitCameraController.h"
#include "ElderlyAICharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

class FWheelchairCameraScenario : public IAutomationLatentCommand
{
public:
 explicit FWheelchairCameraScenario(FAutomationTestBase* T,bool Wheelchair=true):Test(T),Deadline(FPlatformTime::Seconds()+180.),bWheelchair(Wheelchair){}
 ~FWheelchairCameraScenario(){if(PC.IsValid()) ReleaseKeys();}
 bool Update() override
 {
  if(FPlatformTime::Seconds()>Deadline){Test->AddError(TEXT("Wheelchair camera timeout"));return true;}
  if(!World.IsValid())
  {
   for(const FWorldContext& C:GEngine->GetWorldContexts())if(C.WorldType==EWorldType::PIE)World=C.World();
   if(!World.IsValid())return false;
   PC=Cast<AOrbitCameraController>(World->GetFirstPlayerController());
   for(TActorIterator<AElderlyAICharacter>I(World.Get());I;++I)if(I->ActorHasTag(TEXT("ElderTakeoverTestA")))Elder=*I;
   if(!PC.IsValid()||!Elder.IsValid()){Test->AddError(TEXT("Missing L_ElderControlTest fixture"));return true;}
   if(bWheelchair)Elder->EnterWheelchair();else Elder->SetStanding();
   Elder->SetActorRotation(FRotator(0,37,0));
   AI=Elder->GetController();PC->TakeControlOfElder(Elder.Get());
   Next=World->GetTimeSeconds()+.3f;return false;
  }
  if(bLook)PC->InputKey(FInputKeyEventArgs(nullptr,INPUTDEVICEID_NONE,EKeys::MouseX,2.f,World->GetDeltaSeconds(),1,FPlatformTime::Cycles64()));
  if(Step==6)Elder->MoveElder(FVector2D(1,0),123.f);
  if(World->GetTimeSeconds()<Next)return false;
  if(Step==0){Start=Elder->GetActorLocation();View=PC->GetControlRotation();Camera=Elder->PlayerCamera->GetComponentLocation();bLook=true;Next=World->GetTimeSeconds()+.3f;++Step;}
  else if(Step==1)
  {
   bLook=false;Test->TestTrue(TEXT("Mouse changes camera control yaw"),!View.Equals(PC->GetControlRotation(),1.f));
   Test->TestTrue(TEXT("Mouse orbits real camera"),FVector::Dist(Camera,Elder->PlayerCamera->GetComponentLocation())>5.f);
   Test->TestTrue(TEXT("Idle mouse leaves chair heading and location"),FMath::Abs(FMath::FindDeltaAngleDegrees(37,Elder->GetActorRotation().Yaw))<.1f && FVector::Dist2D(Start,Elder->GetActorLocation())<1.f);
   ++Step;
  }
  else if(Step==2)
  {
   const FVector2D V=Directions[Index];
   Start=Elder->GetActorLocation();
   Expected=Elder->GetActorRotation().Yaw+FMath::RadiansToDegrees(FMath::Atan2(V.X,V.Y));
   if(V.Y>0)Key(EKeys::W,true);if(V.Y<0)Key(EKeys::S,true);if(V.X>0)Key(EKeys::D,true);if(V.X<0)Key(EKeys::A,true);
   View=PC->GetControlRotation();
   bLook=true;Next=World->GetTimeSeconds()+.07f;Step=5;
  }
  else if(Step==5)
  {
   Test->TestTrue(TEXT("No translation before hold threshold"),FVector::Dist2D(Start,Elder->GetActorLocation())<1.f);
   if(Index==0||Index==2||Index==4)ReleaseKeys();
   // Oblique root clips blend with the longer idle sample. Wait for the actual
   // clip/graph reset and steady translation, with a hard per-gesture deadline.
   SettleDeadline=World->GetTimeSeconds()+12.f;
   Next=World->GetTimeSeconds()+(bWheelchair?2.8f:5.5f);Step=3;
  }
  else if(Step==3)
  {
   const bool bTap=Index==0||Index==2||Index==4;
   if(World->GetTimeSeconds()<SettleDeadline && (Elder->bTurningInPlace || (!bTap&&Elder->GetVelocity().Size2D()<50.f)))
   {Next=World->GetTimeSeconds()+.1f;return false;}
   bLook=false;
   Test->TestTrue(TEXT("Camera changed during held keys"),!View.Equals(PC->GetControlRotation(),1.f));
   Test->TestTrue(TEXT("New gesture uses current actor-local target"),FMath::Abs(FMath::FindDeltaAngleDegrees(Expected,Elder->TargetFacingYaw))<1.f);
   Test->TestTrue(TEXT("Wheelchair reaches key heading despite orbit"),FMath::Abs(FMath::FindDeltaAngleDegrees(Expected,Elder->GetActorRotation().Yaw))<1.f);
   if(Index==0||Index==2||Index==4)
    Test->TestTrue(TEXT("Tap finishes turn without translation"),FVector::Dist2D(Start,Elder->GetActorLocation())<1.f);
   else
   {
    Test->TestTrue(TEXT("Eight directions retain equal speed"),FMath::IsNearlyEqual(Elder->GetVelocity().Size2D(),64.f,1.f));
    Test->TestTrue(TEXT("Travel matches keyboard heading"),FVector::DotProduct(Elder->GetVelocity().GetSafeNormal2D(),FRotator(0,Expected,0).Vector())>.99f);
   }
   UE_LOG(LogTemp,Display,TEXT("ActorLocal wheelchair=%d case=%d target=%.2f actual=%.2f camera=%.2f speed=%.2f"),bWheelchair,Index,Expected,Elder->GetActorRotation().Yaw,PC->GetControlRotation().Yaw,Elder->GetVelocity().Size2D());
   ReleaseKeys();Next=World->GetTimeSeconds()+.3f;Step=++Index<14?2:4;
  }
  else if(Step==4)
  {
   PC->ReleaseElderControl();Test->TestTrue(TEXT("AI restored"),Elder->GetController()==AI.Get());
   Elder->SetActorRotation(FRotator(0,-73,0));PC->TakeControlOfElder(Elder.Get());
   Elder->ExitWheelchair();Elder->MoveElder(FVector2D(0,1),58.f);
   Test->TestTrue(TEXT("Walking ignores legacy view yaw argument"),FMath::IsNearlyEqual(Elder->TargetFacingYaw,-73.f,.01f));
   Elder->StopElderMovement();
   // Keep ordinary local-strafe coverage ordinary. Wheelchair's new driving
   // contract is exercised by WheelchairSteering, not fixed-target assertions.
   if(bWheelchair)Elder->EnterWheelchair();
   Elder->MoveElder(FVector2D(0,1),145.f);
   if(!bWheelchair)Test->TestTrue(TEXT("Ordinary movement keeps actor-local heading"),FMath::IsNearlyEqual(Elder->TargetFacingYaw,-73.f,.01f));
   Elder->StopElderMovement();Elder->bTurnBeforeMove=false;Start=Elder->GetActorLocation();Expected=Elder->GetActorRotation().Yaw;
   Next=World->GetTimeSeconds()+.8f;Step=6;
  }
  else if(Step==6)
  {
   Test->TestTrue(TEXT("Local strafe without turn-before-move keeps heading"),FMath::Abs(FMath::FindDeltaAngleDegrees(Expected,Elder->GetActorRotation().Yaw))<.01f);
   Test->TestTrue(TEXT("Local strafe moves to current right"),FVector::DotProduct(Elder->GetActorLocation()-Start,Elder->GetActorRightVector())>15.f);
   Elder->StopElderMovement();Elder->bTurnBeforeMove=true;PC->ReleaseElderControl();
   Test->TestTrue(TEXT("AI rotation authority restored after strafe"),Elder->GetCharacterMovement()->bOrientRotationToMovement);return true;
  }
  return false;
 }
private:
 void Key(FKey K,bool Down){PC->InputKey(FInputKeyEventArgs(nullptr,INPUTDEVICEID_NONE,K,Down?IE_Pressed:IE_Released,Down?1.f:0.f,false,FPlatformTime::Cycles64()));}
 void ReleaseKeys(){for(FKey K:{EKeys::W,EKeys::A,EKeys::S,EKeys::D})Key(K,false);}
 FAutomationTestBase*Test;double Deadline;float Next=0,Expected=0,SettleDeadline=0;int32 Step=0,Index=0;bool bLook=false,bWheelchair;
 FVector Start,Camera;FRotator View;
 // Consecutive A,W,D,W,A,WD without resetting the actor, then all eight headings.
 const FVector2D Directions[14]={{-1,0},{0,1},{1,0},{0,1},{-1,0},{1,1},{0,1},{1,0},{0,-1},{-1,0},{1,1},{-1,1},{1,-1},{-1,-1}};
 TWeakObjectPtr<UWorld>World;TWeakObjectPtr<AOrbitCameraController>PC;TWeakObjectPtr<AElderlyAICharacter>Elder;TWeakObjectPtr<AController>AI;
};
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWheelchairCameraTest,"OldMan.Elder.WheelchairCamera",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FWheelchairCameraTest::RunTest(const FString&)
{
 ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
 ADD_LATENT_AUTOMATION_COMMAND(FWheelchairCameraScenario(this));
 ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FActorLocalWalkingTest,"OldMan.Elder.ActorLocalWalking",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FActorLocalWalkingTest::RunTest(const FString&)
{
 ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
 ADD_LATENT_AUTOMATION_COMMAND(FWheelchairCameraScenario(this,false));
 ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());return true;
}
#endif
