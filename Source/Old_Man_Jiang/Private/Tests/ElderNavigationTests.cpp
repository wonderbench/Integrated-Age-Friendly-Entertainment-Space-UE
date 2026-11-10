#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "Editor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/TargetPoint.h"
#include "EngineUtils.h"
#include "ElderlyAICharacter.h"
#include "ElderlyAIController.h"
#include "ElderNavigationComponent.h"
#include "ElderSplinePath.h"
#include "OrbitCameraController.h"
#include "Components/SplineComponent.h"

class FElderNavigationScenario : public IAutomationLatentCommand
{
public:
	explicit FElderNavigationScenario(FAutomationTestBase* InTest) : Test(InTest), Deadline(FPlatformTime::Seconds()+100.) {}
	bool Update() override
	{
		if (FPlatformTime::Seconds()>Deadline) { Test->AddError(FString::Printf(TEXT("Navigation timeout at stage %d"),Stage)); return true; }
		if (!World.IsValid())
		{
			for (const FWorldContext& C : GEngine->GetWorldContexts()) if (C.WorldType==EWorldType::PIE && C.World()) World=C.World();
			if (!World.IsValid()) return false;
			PC=Cast<AOrbitCameraController>(World->GetFirstPlayerController());
			for (TActorIterator<AElderlyAICharacter> It(World.Get()); It; ++It)
			{
				if (It->ActorHasTag(TEXT("ElderTakeoverTestA"))) Elder=*It;
				if (It->ActorHasTag(TEXT("ElderTakeoverTestB"))) Other=*It;
			}
			if (!PC.IsValid() || !Elder.IsValid() || !Other.IsValid()) { Test->AddError(TEXT("Missing navigation fixture")); return true; }
			Nav=Elder->ElderNavigation; OtherNav=Other->ElderNavigation;
			Name=Elder->ElderName; Origin=Elder->GetActorLocation();
			FActorSpawnParameters Params; Params.ObjectFlags|=RF_Transient;
			Target=World->SpawnActor<ATargetPoint>(ATargetPoint::StaticClass(),Origin+FVector(300,0,0),FRotator::ZeroRotator,Params);
			OtherTarget=World->SpawnActor<ATargetPoint>(ATargetPoint::StaticClass(),Other->GetActorLocation()+FVector(300,0,0),FRotator::ZeroRotator,Params);
			Route=World->SpawnActor<AElderSplinePath>(AElderSplinePath::StaticClass(),Origin,FRotator::ZeroRotator,Params);
			AutoTarget=World->SpawnActor<ATargetPoint>(ATargetPoint::StaticClass(),Origin+FVector(300,-500,0),FRotator::ZeroRotator,Params);
			const FTransform AutoTransform(FRotator::ZeroRotator,Origin+FVector(0,-500,0));
			AutoElder=World->SpawnActorDeferred<AElderlyAICharacter>(Elder->GetClass(),AutoTransform,nullptr,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
			AutoElder->SetFlags(RF_Transient);
			AutoElder->ElderNavigation->NavigationMode=EElderNavigationMode::TargetActor;
			AutoElder->ElderNavigation->TargetActor=AutoTarget.Get(); AutoElder->ElderNavigation->bAutoStart=true;
			AutoElder->FinishSpawning(AutoTransform);
			StoppedOrigin=Origin+FVector(0,-700,0);
			const FTransform StoppedTransform(FRotator::ZeroRotator,StoppedOrigin);
			StoppedAutoElder=World->SpawnActorDeferred<AElderlyAICharacter>(Elder->GetClass(),StoppedTransform,nullptr,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
			StoppedAutoElder->SetFlags(RF_Transient);
			StoppedAutoElder->ElderNavigation->NavigationMode=EElderNavigationMode::TargetActor;
			StoppedAutoElder->ElderNavigation->TargetActor=AutoTarget.Get(); StoppedAutoElder->ElderNavigation->bAutoStart=true;
			StoppedAutoElder->FinishSpawning(StoppedTransform);
			StoppedAutoElder->ElderNavigation->StopNavigation();
			Test->TestTrue(TEXT("New navigation is opt-in, existing elders idle"),!Nav->bAutoStart && Nav->NavigationStatus==EElderNavigationStatus::Idle);
			Test->TestFalse(TEXT("No mode/target cannot start"),Nav->StartConfiguredNavigation());
			Test->TestFalse(TEXT("Cannot navigate to self"),Nav->NavigateToActor(Elder.Get()));
			Test->TestTrue(TEXT("Two independently selected targets start"),Nav->NavigateToActor(Target.Get()) && OtherNav->NavigateToActor(OtherTarget.Get()));
			StepDeadline=World->GetTimeSeconds()+8.f;
		}
		if (!Elder.IsValid() || !Other.IsValid() || !PC.IsValid()) { Test->AddError(TEXT("Navigation fixture destroyed")); return true; }
		const float Now=World->GetTimeSeconds();
		if (Now<Next) return false;
		switch (Stage)
		{
		case 0:
			if (!WaitFor(Nav->NavigationStatus==EElderNavigationStatus::Completed && OtherNav->NavigationStatus==EElderNavigationStatus::Completed && AutoElder->ElderNavigation->NavigationStatus==EElderNavigationStatus::Completed,TEXT("Selected and auto-start target routes complete"))) return false;
			Test->TestTrue(TEXT("Stop in spawn frame cancels deferred auto-start"),StoppedAutoElder->ElderNavigation->NavigationStatus==EElderNavigationStatus::Idle && FVector::Dist2D(StoppedOrigin,StoppedAutoElder->GetActorLocation())<1.f);
			Test->TestTrue(TEXT("Selected targets reached independently"),FVector::Dist2D(Elder->GetActorLocation(),Target->GetActorLocation())<25.f && FVector::Dist2D(Other->GetActorLocation(),OtherTarget->GetActorLocation())<25.f);
			Test->TestTrue(TEXT("Target completion restores standing"),Elder->CurrentState==EElderState::Standing);
			Test->TestTrue(TEXT("Already-at-target accepted without recursion"),Nav->NavigateToActor(Target.Get()));
			Next=Now+.15f; break;
		case 1:
			Test->TestTrue(TEXT("Already-at-target completes"),Nav->NavigationStatus==EElderNavigationStatus::Completed);
			Target->SetActorLocation(Origin+FVector(1000,0,0));
			Test->TestTrue(TEXT("Changed selected target starts"),Nav->NavigateToActor(Target.Get()));
			Next=Now+.3f; break;
		case 2:
			Nav->PauseNavigation(); PausedLocation=Elder->GetActorLocation(); Next=Now+.3f; break;
		case 3:
			Test->TestTrue(TEXT("Manual pause stops physical motion"),Nav->NavigationStatus==EElderNavigationStatus::Paused && FVector::Dist2D(PausedLocation,Elder->GetActorLocation())<1.f);
			Test->TestTrue(TEXT("Take/release manually paused elder"),PC->TakeControlOfElder(Elder.Get()) && PC->ReleaseElderControl());
			Stage=30; Next=Now+.2f; return false;
		case 30:
			Test->TestTrue(TEXT("Manual pause stays paused after takeover"),Nav->NavigationStatus==EElderNavigationStatus::Paused && FVector::Dist2D(PausedLocation,Elder->GetActorLocation())<1.f);
			Test->TestTrue(TEXT("Manual resume accepted"),Nav->ResumeNavigation()); Stage=4; Next=Now+.3f; return false;
		case 4:
			Test->TestTrue(TEXT("Takeover pauses managed route"),PC->TakeControlOfElder(Elder.Get()) && Nav->NavigationStatus==EElderNavigationStatus::Paused);
			PausedLocation=Elder->GetActorLocation(); Next=Now+.3f; break;
		case 5:
			Test->TestTrue(TEXT("AI does not move player-controlled elder"),FVector::Dist2D(PausedLocation,Elder->GetActorLocation())<1.f);
			Test->TestFalse(TEXT("Cannot start new AI route while possessed by player"),Nav->StartConfiguredNavigation());
			// Route progress lives on the elder and also survives replacement of a suspended controller.
			for (TActorIterator<AElderlyAIController> It(World.Get()); It; ++It) if (It->bSuspendedForPlayer && !It->GetPawn()) It->Destroy();
			Test->TestTrue(TEXT("Release to replacement AI"),PC->ReleaseElderControl());
			// Travel budget follows the configured gait (now 64 instead of 160 cm/s).
			StepDeadline=Now+FVector::Dist2D(Elder->GetActorLocation(),Target->GetActorLocation())/FMath::Max(1.f,Elder->GetEffectiveMoveSpeed())+4.f; break;
		case 6:
			if (!WaitFor(Nav->NavigationStatus==EElderNavigationStatus::Completed,TEXT("Target resumes after controller recreation"))) return false;
			Test->TestEqual(TEXT("Route preserves identity"),Elder->ElderName,Name);
			Route->SetActorLocation(Elder->GetActorLocation());
			Route->Spline->ClearSplinePoints(false);
			Route->Spline->AddSplinePoint(FVector::ZeroVector,ESplineCoordinateSpace::Local,false);
			Route->Spline->AddSplinePoint(FVector(200,100,0),ESplineCoordinateSpace::Local,false);
			Route->Spline->AddSplinePoint(FVector(400,0,0),ESplineCoordinateSpace::Local,true);
			OriginalLength=Route->Spline->GetSplineLength();
			Route->Spline->AddSplinePoint(FVector(600,100,0),ESplineCoordinateSpace::Local,true);
			Test->TestTrue(TEXT("User can extend spline with another point"),Route->Spline->GetNumberOfSplinePoints()==4 && Route->Spline->GetSplineLength()>OriginalLength+100.f);
			RouteStart=Route->GetActorLocation(); Nav->SplineSampleSpacing=100.f;
			Test->TestTrue(TEXT("Editable spline route accepted"),Nav->FollowSpline(Route.Get())); Next=Now+.4f; break;
		case 7:
			UE_LOG(LogTemp,Display,TEXT("ElderNavigation before sitting status=%d distance=%.1f message=%s"),int32(Nav->NavigationStatus),Nav->CurrentSplineDistance,*Nav->NavigationMessage);
			Elder->SetSitting(); PausedLocation=Elder->GetActorLocation(); Next=Now+.3f; break;
		case 8:
			UE_LOG(LogTemp,Display,TEXT("ElderNavigation after sitting status=%d state=%d message=%s"),int32(Nav->NavigationStatus),int32(Elder->CurrentState),*Nav->NavigationMessage);
			Test->TestTrue(TEXT("Sitting pauses route without state corruption"),Nav->NavigationStatus==EElderNavigationStatus::Paused && Elder->CurrentState==EElderState::Sitting && FVector::Dist2D(PausedLocation,Elder->GetActorLocation())<1.f);
			Test->TestFalse(TEXT("Cannot resume while sitting"),Nav->ResumeNavigation());
			Elder->SetStanding(); Test->TestTrue(TEXT("Resume spline after standing"),Nav->ResumeNavigation()); StepDeadline=Now+12.f; break;
		case 9:
			if (!WaitFor(Nav->NavigationStatus==EElderNavigationStatus::Completed,TEXT("Extended spline completes"))) return false;
			Test->TestTrue(TEXT("Spline reaches appended endpoint"),FVector::Dist2D(Elder->GetActorLocation(),Route->Spline->GetLocationAtSplinePoint(3,ESplineCoordinateSpace::World))<25.f && Nav->CompletedLaps==1);
			Test->TestTrue(TEXT("Route stays in world, no actor/component ticking"),Route->GetActorLocation().Equals(RouteStart) && !Route->PrimaryActorTick.bCanEverTick && !Nav->PrimaryComponentTick.bCanEverTick);
			Route->SetActorLocation(Elder->GetActorLocation()); Route->Spline->ClearSplinePoints(false);
			Route->Spline->AddSplinePoint(FVector::ZeroVector,ESplineCoordinateSpace::Local,false);
			Route->Spline->AddSplinePoint(FVector(-150,0,0),ESplineCoordinateSpace::Local,false);
			Route->Spline->AddSplinePoint(FVector(-150,-150,0),ESplineCoordinateSpace::Local,false);
			Route->Spline->SetClosedLoop(true,true);
			Nav->bLoopSpline=true; Elder->SetDesiredMoveSpeed(300.f);
			Test->TestTrue(TEXT("Closed looping spline starts"),Nav->FollowSpline(Route.Get())); StepDeadline=Now+12.f; break;
		case 10:
			if (!WaitFor(Nav->CompletedLaps>=2,TEXT("Loop completes two laps"))) return false;
			Nav->StopNavigation(); PausedLocation=Elder->GetActorLocation(); Next=Now+.3f; break;
		case 11:
			Test->TestTrue(TEXT("Stop loop clears pending continuation"),Nav->NavigationStatus==EElderNavigationStatus::Idle && FVector::Dist2D(PausedLocation,Elder->GetActorLocation())<1.f);
			Nav->bLoopSpline=false;
			Test->TestTrue(TEXT("Route deletion fixture starts"),Nav->FollowSpline(Route.Get()));
			Route->Destroy(); Next=Now+.2f; break;
		case 12:
			Test->TestTrue(TEXT("Deleted route fails safely"),Nav->NavigationStatus==EElderNavigationStatus::Failed);
			Target->SetActorLocation(Elder->GetActorLocation()+FVector(-500,0,0));
			Test->TestTrue(TEXT("Target deletion fixture starts"),Nav->NavigateToActor(Target.Get()));
			Target->Destroy(); Next=Now+.2f; break;
		case 13:
			Test->TestTrue(TEXT("Deleted target fails safely"),Nav->NavigationStatus==EElderNavigationStatus::Failed);
			OtherTarget->SetActorLocation(FVector(100000,100000,100000));
			Nav->NavigateToActor(OtherTarget.Get()); StepDeadline=Now+4.f; break;
		case 14:
			if (!WaitFor(Nav->NavigationStatus==EElderNavigationStatus::Failed,TEXT("Unreachable target reports failure"))) return false;
			Test->TestTrue(TEXT("Other elder retains independent completed route"),OtherNav->NavigationStatus==EElderNavigationStatus::Completed && Other->CurrentState==EElderState::Standing);
			Nav->StopNavigation(); OtherTarget->Destroy();
			AutoElder->Destroy(); StoppedAutoElder->Destroy(); AutoTarget->Destroy();
			{
				FActorSpawnParameters Params; Params.ObjectFlags|=RF_Transient;
				Route=World->SpawnActor<AElderSplinePath>(AElderSplinePath::StaticClass(),Elder->GetActorLocation(),FRotator::ZeroRotator,Params);
			}
			Test->TestTrue(TEXT("Queue override fixture starts at AlreadyAtGoal"),Nav->FollowSpline(Route.Get()));
			IssueExternalMove(); break;
		case 15:
			if (!WaitFor(Cast<AElderlyAIController>(Elder->GetController())->GetMoveStatus()==EPathFollowingStatus::Idle,TEXT("External replacement move completes"))) return false;
			Test->TestTrue(TEXT("External request wins over queued/moving/paused route"),Nav->NavigationStatus==EElderNavigationStatus::Idle && FVector::Dist2D(Elder->GetActorLocation(),ExternalGoal)<35.f);
			if (++OverrideCase<3)
			{
				Route->SetActorLocation(Elder->GetActorLocation());
				Test->TestTrue(TEXT("Next override spline starts"),Nav->FollowSpline(Route.Get()));
				Next=Now+.3f; break;
			}
			Route->SetActorLocation(Elder->GetActorLocation()); Nav->FollowSpline(Route.Get());
			{
				FAIMoveRequest Invalid(FVector(100000,100000,100000)); Invalid.SetAllowPartialPath(false);
				Test->TestTrue(TEXT("Unreachable external request fails"),Cast<AElderlyAIController>(Elder->GetController())->MoveTo(Invalid).Code==EPathFollowingRequestResult::Failed);
			}
			Stage=17; Next=Now+.2f; return false;
		case 16:
			Test->TestTrue(TEXT("External override fixture is moving"),Nav->NavigationStatus==EElderNavigationStatus::Moving);
			if (OverrideCase==2) Nav->PauseNavigation();
			IssueExternalMove(); Stage=15; return false;
		case 17:
			Test->TestTrue(TEXT("Failed external request still cancels old route"),Nav->NavigationStatus==EElderNavigationStatus::Idle);
			Nav->StopNavigation(); Route->Destroy();
			UE_LOG(LogTemp,Display,TEXT("ElderNavigation: target, extension, spline, loops, pause, takeover, state gating, auto-start, deletion, failures and external override checks completed"));
			return true;
		}
		++Stage;
		return false;
	}
private:
	void IssueExternalMove()
	{
		ExternalGoal=Elder->GetActorLocation()+FVector(0,-250,0);
		FAIMoveRequest Request(ExternalGoal); Request.SetAcceptanceRadius(20.f); Request.SetReachTestIncludesAgentRadius(false); Request.SetAllowPartialPath(false);
		Test->TestTrue(TEXT("External MoveTo accepted"),Cast<AElderlyAIController>(Elder->GetController())->MoveTo(Request).Code==EPathFollowingRequestResult::RequestSuccessful);
		StepDeadline=World->GetTimeSeconds()+4.f;
	}
	bool WaitFor(bool Condition,const TCHAR* Label)
	{
		if (!Condition && World->GetTimeSeconds()<StepDeadline) return false;
		Test->TestTrue(Label,Condition);
		return true;
	}
	FAutomationTestBase* Test; double Deadline;
	TWeakObjectPtr<UWorld> World;
	TWeakObjectPtr<AElderlyAICharacter> Elder,Other,AutoElder,StoppedAutoElder;
	TWeakObjectPtr<AOrbitCameraController> PC;
	TWeakObjectPtr<UElderNavigationComponent> Nav,OtherNav;
	TWeakObjectPtr<ATargetPoint> Target,OtherTarget,AutoTarget;
	TWeakObjectPtr<AElderSplinePath> Route;
	FVector Origin,PausedLocation,RouteStart,StoppedOrigin,ExternalGoal; FString Name;
	int32 Stage=0,OverrideCase=0; float Next=0.f,StepDeadline=0.f,OriginalLength=0.f;
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FElderNavigationTest,"OldMan.Elder.Navigation",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FElderNavigationTest::RunTest(const FString& Parameters)
{
	if (!GEditor || !GEditor->GetEditorWorldContext().World()->GetMapName().Contains(TEXT("L_ElderControlTest"))) { AddError(TEXT("Open L_ElderControlTest first")); return false; }
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<FElderNavigationScenario>(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	return true;
}
#endif
