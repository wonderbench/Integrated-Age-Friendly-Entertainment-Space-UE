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
#include "OrbitCameraController.h"
#include "Components/SkeletalMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

class FWheelchairSteeringScenario : public IAutomationLatentCommand
{
public:
 explicit FWheelchairSteeringScenario(FAutomationTestBase* T):Test(T),Deadline(FPlatformTime::Seconds()+70.){}
 ~FWheelchairSteeringScenario(){if(PC.IsValid())Keys(false,false,false,false);}
 bool Update() override
 {
  if(FPlatformTime::Seconds()>Deadline){Test->AddError(FString::Printf(TEXT("WheelchairSteering timeout stage %d"),Stage));return true;}
  if(!World.IsValid())
  {
   for(const FWorldContext& C:GEngine->GetWorldContexts())if(C.WorldType==EWorldType::PIE)World=C.World();
   if(!World.IsValid())return false;
   PC=Cast<AOrbitCameraController>(World->GetFirstPlayerController());
   for(TActorIterator<AElderlyAICharacter>I(World.Get());I;++I)if(I->ActorHasTag(TEXT("ElderTakeoverTestA")))Elder=*I;
   if(!PC.IsValid()||!Elder.IsValid()){Test->AddError(TEXT("Open L_ElderControlTest"));return true;}
   Elder->EnterWheelchair();Elder->SetActorRotation(FRotator(0,37,0));
   AI=Elder->GetController();
   if(!PC->TakeControlOfElder(Elder.Get())){Test->AddError(TEXT("Takeover failed"));return true;}
   Anim=Cast<UElderAnimInstance>(Elder->GetMesh()->GetAnimInstance());
   if(!Anim.IsValid()){Test->AddError(TEXT("Existing animation instance missing"));return true;}
   Next=World->GetTimeSeconds()+.4f;return false;
  }
  const float Now=World->GetTimeSeconds();
  if(Stage==7)MinMovingSpeed=FMath::Min(MinMovingSpeed,float(Elder->GetVelocity().Size2D()));
  if(bMeasure)
  {
   const float Yaw=Elder->GetActorRotation().Yaw;
   const float Step=FMath::FindDeltaAngleDegrees(LastYaw,Yaw);TotalYaw+=Step;LastYaw=Yaw;
   Test->TestTrue(TEXT("Seated steering never enters root TIP"),!Elder->bTurningInPlace&&!Anim->RootTurnActive);
   Test->TestTrue(TEXT("No competing movement/controller rotation"),!Elder->bUseControllerRotationYaw&&!Elder->GetCharacterMovement()->bUseControllerDesiredRotation&&!Elder->GetCharacterMovement()->bOrientRotationToMovement);
   Test->TestTrue(TEXT("Steering bounded by configured rate"),FMath::Abs(Step)<=Elder->WheelchairTurnRate*World->GetDeltaSeconds()+.2f);
  }
  if(bLook)PC->InputKey(FInputKeyEventArgs(nullptr,INPUTDEVICEID_NONE,EKeys::MouseX,2.f,World->GetDeltaSeconds(),1,FPlatformTime::Cycles64()));
  if(Now<Next)return false;
  switch(Stage++)
  {
  case 0: Start=Elder->GetActorLocation();BeginMeasure();Keys(false,false,false,true);Next=Now+5.f;break;
  case 1:
   Test->TestTrue(TEXT("Held D rotates beyond a complete revolution"),TotalYaw>430.f&&TotalYaw<470.f);
   Test->TestTrue(TEXT("D alone stays in place"),FVector::Dist2D(Start,Elder->GetActorLocation())<.2f&&Elder->CurrentState==EElderState::WheelchairIdle);
   UE_LOG(LogTemp,Display,TEXT("WheelchairSteering heldD totalYaw=%.3f distance=%.3f"),TotalYaw,FVector::Dist2D(Start,Elder->GetActorLocation()));
   Keys(false,false,false,false);bMeasure=false;Next=Now+.05f;break;
  case 2: SavedYaw=Elder->GetActorRotation().Yaw;Next=Now+.6f;break;
  case 3:
   Test->TestTrue(TEXT("Release D has no tail"),Angle(SavedYaw)<.01f&&!Elder->IsActorTickEnabled());
   BeginMeasure();Keys(false,true,false,false);Next=Now+1.2f;break;
  case 4:
   Test->TestTrue(TEXT("Held A continuously turns left"),TotalYaw< -100.f&&TotalYaw> -120.f);
   Keys(false,false,false,false);bMeasure=false;Next=Now+.05f;break;
  case 5: SavedYaw=Elder->GetActorRotation().Yaw;Start=Elder->GetActorLocation();Keys(true,false,false,false);Next=Now+.7f;break;
  case 6:
   Test->TestTrue(TEXT("W travels current forward at 64"),Angle(SavedYaw)<.1f&&FVector::DotProduct(Elder->GetVelocity(),Elder->GetActorForwardVector())>63.f);
   BeginMeasure();Keys(true,false,false,true);bLook=true;Camera=Elder->PlayerCamera->GetComponentLocation();View=PC->GetControlRotation();Next=Now+.7f;break;
  case 7:
   Test->TestTrue(TEXT("WD turns while moving, without diagonal slowdown"),TotalYaw>55.f&&TotalYaw<70.f&&Elder->GetVelocity().Size2D()>63.f&&Elder->CurrentState==EElderState::WheelchairMoving);
   Test->TestTrue(TEXT("WD never clears forward speed"),MinMovingSpeed>60.f);
   Test->TestTrue(TEXT("Mouse still moves real camera"),!View.Equals(PC->GetControlRotation(),1.f)&&FVector::Dist(Camera,Elder->PlayerCamera->GetComponentLocation())>10.f);
   UE_LOG(LogTemp,Display,TEXT("WheelchairSteering WD minSpeed=%.3f yaw=%.3f"),MinMovingSpeed,TotalYaw);
   Keys(true,false,false,false);bLook=false;bMeasure=false;Next=Now+.05f;break;
  case 8: SavedYaw=Elder->GetActorRotation().Yaw;Next=Now+.5f;break;
  case 9:
   Test->TestTrue(TEXT("Release steering keeps W straight"),Angle(SavedYaw)<.01f&&Elder->GetVelocity().Size2D()>63.f);
   Keys(false,false,true,false);Next=Now+.7f;break;
  case 10:
   Test->TestTrue(TEXT("S reverses without turning 180"),Angle(SavedYaw)<.01f&&FVector::DotProduct(Elder->GetVelocity(),Elder->GetActorForwardVector())< -63.f);
   BeginMeasure();Keys(false,false,true,true);Next=Now+.7f;break;
  case 11:
   Test->TestTrue(TEXT("SD reverses and rotates right"),TotalYaw>55.f&&TotalYaw<70.f&&FVector::DotProduct(Elder->GetVelocity(),Elder->GetActorForwardVector())< -60.f);
   bMeasure=false;Keys(false,false,false,false);Next=Now+.05f;break;
  case 12: SavedYaw=Elder->GetActorRotation().Yaw;Keys(false,true,false,true);Next=Now+.5f;break;
  case 13:
   Test->TestTrue(TEXT("A+D cancel steering"),Angle(SavedYaw)<.01f&&Elder->GetVelocity().IsNearlyZero());
   Keys(false,false,false,false);Elder->WheelchairTurnRate=45.f;Elder->bTurnBeforeMove=false;BeginMeasure();Keys(false,false,false,true);Next=Now+1.f;break;
  case 14:
   Test->TestTrue(TEXT("45 degree rate and ordinary toggle independence"),TotalYaw>40.f&&TotalYaw<50.f);
   bMeasure=false;Keys(false,false,false,false);Next=Now+.05f;break;
  case 15:
   // Same release entry used by Alt/UI cancellation, then no further movement events.
   Elder->MoveElder(FVector2D(1,0),123.f);Elder->ReleaseMovementInput();SavedYaw=Elder->GetActorRotation().Yaw;Next=Now+.4f;break;
  case 16:
   Test->TestTrue(TEXT("Explicit input release clears continuous steering"),Angle(SavedYaw)<.01f&&!Elder->IsActorTickEnabled());
   Elder->MoveElder(FVector2D(1,0),0);Elder->SetSitting();SavedYaw=Elder->GetActorRotation().Yaw;Next=Now+.3f;break;
  case 17:
   Test->TestTrue(TEXT("Sitting cancels wheel steering"),Angle(SavedYaw)<.01f&&!Elder->IsActorTickEnabled());
   Elder->EnterWheelchair();Elder->MoveElder(FVector2D(1,0),0);Elder->ExitWheelchair();SavedYaw=Elder->GetActorRotation().Yaw;Next=Now+.3f;break;
  case 18:
   Test->TestTrue(TEXT("Exit wheelchair cancels steering"),Angle(SavedYaw)<.01f&&!Elder->IsActorTickEnabled());
   Elder->bTurnBeforeMove=true;Elder->EnterWheelchair();Elder->MoveElder(FVector2D(1,0),0);PC->ReleaseElderControl();SavedYaw=Elder->GetActorRotation().Yaw;Next=Now+.3f;break;
  case 19:
   Test->TestTrue(TEXT("AI authority restored, no residual turn"),Elder->GetController()==AI.Get()&&Elder->GetCharacterMovement()->bOrientRotationToMovement&&!Elder->IsActorTickEnabled()&&Angle(SavedYaw)<.01f);
   PC->TakeControlOfElder(Elder.Get());Elder->WheelchairTurnRate=90.f;Keys(true,false,false,true);Next=Now+.7f;break;
  case 20: Keys(false,false,false,true);Next=Now+.05f;break;
  case 21: Start=Elder->GetActorLocation();BeginMeasure();Next=Now+.5f;break;
  case 22:
   Test->TestTrue(TEXT("WD to D stops translation but keeps steering"),FVector::Dist2D(Start,Elder->GetActorLocation())<.2f&&TotalYaw>40.f);
   Keys(false,true,false,false);bMeasure=false;Next=Now+.05f;break;
  case 23: BeginMeasure();Next=Now+.5f;break;
  case 24:
   Test->TestTrue(TEXT("D to A reverses steering without release-all"),TotalYaw< -40.f);
   Keys(false,false,false,false);bMeasure=false;Next=Now+.05f;break;
  case 25: SavedYaw=Elder->GetActorRotation().Yaw;bLook=true;Next=Now+.5f;break;
  case 26:
   Test->TestTrue(TEXT("Mouse alone never steers wheelchair"),Angle(SavedYaw)<.01f);
   bLook=false;PC->ReleaseElderControl();
   return true;
  }
  return false;
 }
private:
 float Angle(float Yaw)const{return FMath::Abs(FMath::FindDeltaAngleDegrees(Yaw,Elder->GetActorRotation().Yaw));}
 void BeginMeasure(){bMeasure=true;TotalYaw=0;LastYaw=Elder->GetActorRotation().Yaw;}
 void Keys(bool W,bool A,bool S,bool D){const bool New[4]={W,A,S,D};const FKey K[4]={EKeys::W,EKeys::A,EKeys::S,EKeys::D};for(int32 I=0;I<4;++I)if(New[I]!=Held[I]){PC->InputKey(FInputKeyEventArgs(nullptr,INPUTDEVICEID_NONE,K[I],New[I]?IE_Pressed:IE_Released,New[I]?1.f:0.f,false,FPlatformTime::Cycles64()));Held[I]=New[I];}}
 FAutomationTestBase*Test;double Deadline;float Next=0,SavedYaw=0,LastYaw=0,TotalYaw=0,MinMovingSpeed=1000;int32 Stage=0;bool Held[4]={},bLook=false,bMeasure=false;
 FVector Start,Camera;FRotator View;
 TWeakObjectPtr<UWorld>World;TWeakObjectPtr<AOrbitCameraController>PC;TWeakObjectPtr<AElderlyAICharacter>Elder;TWeakObjectPtr<AController>AI;TWeakObjectPtr<UElderAnimInstance>Anim;
};
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWheelchairSteeringTest,"OldMan.Elder.WheelchairSteering",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FWheelchairSteeringTest::RunTest(const FString&)
{
 ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
 ADD_LATENT_AUTOMATION_COMMAND(FWheelchairSteeringScenario(this));
 ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());return true;
}
#endif
