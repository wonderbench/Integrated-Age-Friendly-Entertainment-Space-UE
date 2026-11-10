#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "ElderlyAICharacter.h"
#include "ElderlyAIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "ElderAnimInstance.h"
#include "OrbitCameraController.h"
#include "WheelchairWheelMotionComponent.h"
#include "Components/ChildActorComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimNode_SequencePlayer.h"
#include "UObject/UnrealType.h"

class FElderWheelchairSeatingScenario : public IAutomationLatentCommand
{
public:
 explicit FElderWheelchairSeatingScenario(FAutomationTestBase* T):Test(T),Deadline(FPlatformTime::Seconds()+60.){}
 bool Update() override
 {
  if(FPlatformTime::Seconds()>Deadline){Test->AddError(TEXT("Wheelchair seating timed out"));return true;}
  if(!World.IsValid())
  {
   for(const FWorldContext& C:GEngine->GetWorldContexts()) if(C.WorldType==EWorldType::PIE) World=C.World();
   if(!World.IsValid())return false;
   for(TActorIterator<AElderlyAICharacter> I(World.Get());I;++I) if(I->ActorHasTag(TEXT("ElderTakeoverTestA"))) Elder=*I;
   PC=Cast<AOrbitCameraController>(World->GetFirstPlayerController());
   if(!Elder.IsValid()||!PC.IsValid()){Test->AddError(TEXT("Open L_ElderControlTest before this test"));return true;}
   AI=Cast<AElderlyAIController>(Elder->GetController());
   Anim=Cast<UElderAnimInstance>(Elder->GetMesh()->GetAnimInstance());
   Chair=Elder->Wheelchair->GetChildActor();
   Motion=Chair.IsValid()?Chair->FindComponentByClass<UWheelchairWheelMotionComponent>():nullptr;
   if(!AI.IsValid()||!Anim.IsValid()||!Motion.IsValid()){Test->AddError(TEXT("Missing existing AI/ABP/chair wheel component"));return true;}
   Elder->ExitWheelchair(); // Establish a standing baseline even when BP BeginPlay seats the elder.
   Name=Elder->ElderName; StandingMeshLocation=Elder->GetMesh()->GetRelativeLocation();
   Elder->GetMesh()->VisibilityBasedAnimTickOption=EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
   Elder->EnterWheelchair(); Next=World->GetTimeSeconds()+5.f; return false;
  }
  if(!Elder.IsValid()||!Anim.IsValid()||!Motion.IsValid()){Test->AddError(TEXT("Lost fixture"));return true;}
  if(Step==3)Elder->MoveElder(FVector2D(0,1),Elder->GetActorRotation().Yaw);
  if(World->GetTimeSeconds()<Next)return false;
  if(Step==0)
  {
   CheckPose(); Test->TestTrue(TEXT("Chair attached to skeletal mesh"),Elder->Wheelchair->GetAttachParent()==Elder->GetMesh());
   Test->TestEqual(TEXT("Authored socket used"),Elder->Wheelchair->GetAttachSocketName(),FName(TEXT("Socket_Wheel_Chair")));
   Test->TestTrue(TEXT("Legacy offsets removed"),Elder->Wheelchair->GetRelativeTransform().Equals(Elder->WheelchairSocketOffset));
   Test->TestFalse(TEXT("Chair visible"),Chair->IsHidden());Test->TestFalse(TEXT("Attached chair collision disabled"),Chair->GetActorEnableCollision());
   Test->TestTrue(TEXT("Chair front agrees with elder"),FVector::DotProduct(Chair->GetActorRightVector(),Elder->GetActorForwardVector())>.99f);
   for(USceneComponent* C:TInlineComponentArray<USceneComponent*>(Chair.Get()))
    if(C->GetFName()==TEXT("L_BigWheel") || C->GetFName()==TEXT("L_LittleWheel"))
     Test->TestTrue(TEXT("Wheel bottoms aligned to test floor"),FMath::Abs(C->GetComponentLocation().Z-Motion->GetWheelRadiusCm(C->GetFName()))<.5f);
   Start=Elder->GetActorLocation();
   FAIMoveRequest Request(Start+FVector(150,0,0));Request.SetAcceptanceRadius(10);
   Test->TestTrue(TEXT("Seated AI MoveTo accepted"),AI->MoveTo(Request).Code==EPathFollowingRequestResult::RequestSuccessful);
   Next=World->GetTimeSeconds()+1.f;
  }
  else if(Step==1)
  {
   CheckPose();Test->TestTrue(TEXT("AI moved same seated character"),FVector::Dist2D(Start,Elder->GetActorLocation())>20.f);
   Test->TestTrue(TEXT("AI wheelchair moving state"),Elder->CurrentState==EElderState::WheelchairMoving);
   Test->TestTrue(TEXT("Attached wheels rolled"),FMath::Abs(Motion->GetWheelRollDegrees(TEXT("L_BigWheel")))>5.f);
   Next=World->GetTimeSeconds()+3.f;
  }
  else if(Step==2)
  {
   CheckPose();Test->TestTrue(TEXT("AI arrived seated idle"),Elder->CurrentState==EElderState::WheelchairIdle);
   Test->TestTrue(TEXT("Seated takeover"),PC->TakeControlOfElder(Elder.Get()));
   Start=Elder->GetActorLocation();Next=World->GetTimeSeconds()+1.f;
  }
  else if(Step==3)
  {
   CheckPose();Test->TestTrue(TEXT("Player moved seated elder"),FVector::Dist2D(Start,Elder->GetActorLocation())>20.f);
   Elder->ReleaseMovementInput();PC->ReleaseElderControl();Next=World->GetTimeSeconds()+.3f;
  }
  else if(Step==4)
  {
   CheckPose();Test->TestTrue(TEXT("Original AI restored"),Elder->GetController()==AI.Get());
   Elder->SetElderGender(EElderGender::Female);Elder->SetElderGender(EElderGender::Male);
   Test->TestEqual(TEXT("Name preserved"),Elder->ElderName,Name);
   Test->TestTrue(TEXT("Gender retains wheelchair state"),Elder->CurrentState==EElderState::WheelchairIdle);
   Anim=Cast<UElderAnimInstance>(Elder->GetMesh()->GetAnimInstance());Elder->ExitWheelchair();Next=World->GetTimeSeconds()+.4f;
  }
  else if(Step==5)
  {
   Test->TestTrue(TEXT("Exit standing"),Elder->CurrentState==EElderState::Standing);
   Test->TestFalse(TEXT("Original graph restored"),Anim->WheelchairPoseActive);
   Test->TestTrue(TEXT("Chair hidden on exit"),Chair->IsHidden());
   Test->TestTrue(TEXT("Standing mesh height restored"),Elder->GetMesh()->GetRelativeLocation().Equals(StandingMeshLocation,.01f));
   PC->TakeControlOfElder(Elder.Get());Elder->MoveElder(FVector2D(1,0),Elder->GetActorRotation().Yaw);Elder->ReleaseMovementInput();
   Next=World->GetTimeSeconds()+.6f;
  }
  else if(Step==6)
  {
   Test->TestTrue(TEXT("Standing root turn started"),Elder->ShouldPlayTurnAnimation());
   Elder->EnterWheelchair();SeatedYaw=Elder->GetActorRotation().Yaw;Next=World->GetTimeSeconds()+.6f;
  }
  else if(Step==7)
  {
   CheckPose();Test->TestTrue(TEXT("Fading root turn cannot rotate wheelchair"),FMath::Abs(FMath::FindDeltaAngleDegrees(SeatedYaw,Elder->GetActorRotation().Yaw))<.1f);
   Elder->ExitWheelchair();Next=World->GetTimeSeconds()+.5f;
  }
  else if(Step==8)
  {
   Elder->MoveElder(FVector2D(-1,0),Elder->GetActorRotation().Yaw);Elder->ReleaseMovementInput();Next=World->GetTimeSeconds()+.5f;
  }
  else if(Step==9)
  {
   Test->TestTrue(TEXT("Root turn works again after wheelchair exit"),Elder->ShouldPlayTurnAnimation());
   Elder->StopElderMovement();PC->ReleaseElderControl();return true;
  }
  ++Step;return false;
 }
private:
 void CheckPose()
 {
  Test->TestTrue(TEXT("Same animation instance retained"),Elder->GetMesh()->GetAnimInstance()==Anim.Get());
  Test->TestTrue(TEXT("Seated animation branch active"),Anim->WheelchairPoseActive);
  const UAnimSequence* ExpectedPose=Elder->CurrentState==EElderState::WheelchairMoving && Elder->WheelchairMoveAnimation ? Elder->WheelchairMoveAnimation.Get() : Elder->WheelchairIdleAnimation.Get();
  Test->TestTrue(TEXT("Configured seated animation assigned (including retargeted assets)"),ExpectedPose && Anim->WheelchairPoseAnimation==ExpectedPose);
  bool bPlaying=false;
  for(TFieldIterator<FStructProperty> I(Anim->GetClass());I;++I)
   if(I->Struct==FAnimNode_SequencePlayer::StaticStruct())
   {
    auto* P=I->ContainerPtrToValuePtr<FAnimNode_SequencePlayer>(Anim.Get());
    if(P->GetSequence()==Anim->WheelchairPoseAnimation && P->GetCachedBlendWeight()>.9f && P->GetAccumulatedTime()>0.f && P->IsLooping())bPlaying=true;
   }
  Test->TestTrue(TEXT("Actual looping sitting player evaluated"),bPlaying);
 }
 FAutomationTestBase* Test;double Deadline;int32 Step=0;float Next=0;float SeatedYaw=0;FVector Start,StandingMeshLocation;FString Name;
 TWeakObjectPtr<UWorld> World;TWeakObjectPtr<AElderlyAICharacter>Elder;TWeakObjectPtr<AElderlyAIController>AI;
 TWeakObjectPtr<AOrbitCameraController>PC;TWeakObjectPtr<UElderAnimInstance>Anim;TWeakObjectPtr<AActor>Chair;TWeakObjectPtr<UWheelchairWheelMotionComponent>Motion;
};
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FElderWheelchairSeatingTest,"OldMan.Elder.WheelchairSeating",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FElderWheelchairSeatingTest::RunTest(const FString&)
{
 ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
 ADD_LATENT_AUTOMATION_COMMAND(FElderWheelchairSeatingScenario(this));
 ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());return true;
}
#endif
