#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "Editor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "ElderlyAICharacter.h"
#include "ElderlyAIController.h"
#include "ElderNavigationComponent.h"
#include "ElderSplinePath.h"
#include "ElderAnimInstance.h"
#include "MonitorCameraActor.h"
#include "Components/SplineComponent.h"
#include "Components/SphereComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimNode_SequencePlayer.h"
#include "Animation/AnimSequence.h"
#include "UObject/UnrealType.h"

/** Frame-level continuity plus real monitor history, not just an endpoint check. */
class FElderSmoothScenario : public IAutomationLatentCommand
{
public:
	explicit FElderSmoothScenario(FAutomationTestBase* InTest) : Test(InTest), Deadline(FPlatformTime::Seconds()+120.) {}
	~FElderSmoothScenario() { FWorldDelegates::OnWorldPostActorTick.Remove(TickHandle); }
	bool Update() override
	{
		if (FPlatformTime::Seconds()>Deadline) { Test->AddError(TEXT("Smooth route timeout")); return true; }
		if (!World.IsValid())
		{
			for (const FWorldContext& C:GEngine->GetWorldContexts()) if (C.WorldType==EWorldType::PIE) World=C.World();
			if (!World.IsValid()) return false;
			for (TActorIterator<AElderlyAICharacter> It(World.Get()); It; ++It) if (It->ActorHasTag(TEXT("ElderTakeoverTestA"))) Elder=*It;
			if (!Elder.IsValid()) { Test->AddError(TEXT("Missing smooth fixture")); return true; }
			Nav=Elder->ElderNavigation;
			Elder->ExitWheelchair();
			Test->TestEqual(TEXT("BP_AI default slow gait"),Elder->GetClass()->GetDefaultObject<AElderlyAICharacter>()->DesiredMoveSpeed,64.f);
			FActorSpawnParameters P; P.ObjectFlags|=RF_Transient;
			Route=World->SpawnActor<AElderSplinePath>(AElderSplinePath::StaticClass(),Elder->GetActorLocation(),FRotator::ZeroRotator,P);
			Route->Spline->ClearSplinePoints(false);
			Route->Spline->AddSplinePoint(FVector::ZeroVector,ESplineCoordinateSpace::Local,false);
			Route->Spline->AddSplinePoint(FVector(1200,0,0),ESplineCoordinateSpace::Local,true);
			Monitor=World->SpawnActor<AMonitorCameraActor>(AMonitorCameraActor::StaticClass(),FVector(100000,0,0),FRotator::ZeroRotator,P);
			Monitor->MonitorRange->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			Monitor->RegisterElder(Elder.Get());
			TickHandle=FWorldDelegates::OnWorldPostActorTick.AddRaw(this,&FElderSmoothScenario::Sample);
			StartCase(64.f);
		}
		if (Nav->NavigationStatus==EElderNavigationStatus::Failed) { Test->AddError(Nav->NavigationMessage); return true; }
		if (bLoopCase)
		{
			if (Nav->CompletedLaps<2) return false;
			// Steering via CharacterMovement acceleration can legitimately reduce scalar
			// speed in corners. Reject discontinuities beyond its friction/acceleration
			// threshold, not normal turning; straight sections retain the 95% assertion.
			Test->TestTrue(TEXT("Loop including seam has no stops or abrupt speed reset"),Frames>100 && ZeroFrames==0 && AbruptDrops==0);
			Test->TestTrue(TEXT("Loop sampling fast enough to detect a reset"),MaxDropBudget<Elder->GetEffectiveMoveSpeed()*.5f);
			Test->TestEqual(TEXT("Loop emits only the initial Walking"),Monitor->StateHistory.Num()-HistoryStart,1);
			UE_LOG(LogTemp,Display,TEXT("ElderSmooth closed loop laps=%d frames=%d min=%.3f max=%.3f zero=%d events=%d abruptDrops=%d maxFrameDrop=%.3f maxDT=%.4f maxBudget=%.3f"),Nav->CompletedLaps,Frames,MinSpeed,MaxSpeed,ZeroFrames,Monitor->StateHistory.Num()-HistoryStart,AbruptDrops,MaxFrameDrop,MaxDeltaSeconds,MaxDropBudget);
			Nav->StopNavigation();
			Test->TestTrue(TEXT("Loop explicit stop restores physical stop and policy"),Elder->GetVelocity().Size2D()<.1f && Cast<AElderlyAIController>(Elder->GetController())->GetPathFollowingComponent()->IsStopMovementOnFinishActive());
			Monitor->Destroy(); Route->Destroy(); return true;
		}
		if (Nav->NavigationStatus!=EElderNavigationStatus::Completed) return false;
		if (FinishTime==0.f) { FinishTime=World->GetTimeSeconds(); FinishLocation=Elder->GetActorLocation(); return false; }
		if (World->GetTimeSeconds()<FinishTime+.3f) return false;
		Test->TestTrue(TEXT("Many moving frames sampled"),Frames>100);
		Test->TestTrue(TEXT("All intermediate boundaries sampled"),Boundaries>=7);
		Test->TestEqual(TEXT("No zero-speed intermediate frames"),ZeroFrames,0);
		Test->TestTrue(TEXT("Cruise speed stays above 95 percent across samples"),MinSpeed>=Elder->GetEffectiveMoveSpeed()*.95f);
		Test->TestTrue(TEXT("Speed respects the configured gait"),MaxSpeed<=Elder->GetEffectiveMoveSpeed()+1.f);
		Test->TestEqual(TEXT("Only start and final state events, no intermediate Standing"),Monitor->StateHistory.Num()-HistoryStart,2);
		Test->TestTrue(TEXT("Actual walking nodes use coordinated playback"),AnimationFrames>50 && BadAnimationFrames==0);
		Test->TestTrue(TEXT("Final state and physical stop"),Elder->CurrentState==EElderState::Standing && Elder->GetVelocity().Size2D()<.1f && FVector::Dist2D(FinishLocation,Elder->GetActorLocation())<1.f);
		Test->TestTrue(TEXT("Endpoint reached"),FVector::Dist2D(Elder->GetActorLocation(),Route->Spline->GetLocationAtSplinePoint(1,ESplineCoordinateSpace::World))<25.f);
		Test->TestTrue(TEXT("Normal stop policy restored"),Cast<AElderlyAIController>(Elder->GetController())->GetPathFollowingComponent()->IsStopMovementOnFinishActive());
		UE_LOG(LogTemp,Display,TEXT("ElderSmooth speed=%.1f frames=%d boundaries=%d min=%.3f max=%.3f zero=%d events=%d animFrames=%d badAnim=%d"),Speed,Frames,Boundaries,MinSpeed,MaxSpeed,ZeroFrames,Monitor->StateHistory.Num()-HistoryStart,AnimationFrames,BadAnimationFrames);
		if (Speed==64.f)
		{
			// Reuse the same safe corridor in the opposite direction at the old speed.
			Route->SetActorLocation(Elder->GetActorLocation());
			Route->Spline->SetLocationAtSplinePoint(1,FVector(-1200,0,0),ESplineCoordinateSpace::Local,true);
			StartCase(160.f); return false;
		}
		Route->SetActorLocation(Elder->GetActorLocation()); Route->Spline->ClearSplinePoints(false);
		for(const FVector& Point:TArray<FVector>{FVector(0,0,0),FVector(300,300,0),FVector(600,0,0),FVector(300,-300,0)}) Route->Spline->AddSplinePoint(Point,ESplineCoordinateSpace::Local,false);
		Route->Spline->SetClosedLoop(true,true); Nav->bLoopSpline=true; bLoopCase=true;
		// Validate the requested elderly gait on bends as well as straight segments.
		// The old Navigation regression separately stress-tests loops at 300 cm/s.
		StartCase(64.f); return false;
	}
private:
	void StartCase(float NewSpeed)
	{
		Speed=NewSpeed; Frames=Boundaries=ZeroFrames=AnimationFrames=BadAnimationFrames=0; MinSpeed=MAX_flt; MaxSpeed=0.f; LastDistance=0.f; FinishTime=0.f;
		HistoryStart=Monitor->StateHistory.Num();
		StartTime=World->GetTimeSeconds();
		PlaybackTimes.Reset();
		PreviousSpeed=0.f; AbruptDrops=0; MaxFrameDrop=0.f;
		MaxDeltaSeconds=MaxDropBudget=0.f;
		Elder->SetDesiredMoveSpeed(Speed); Nav->SplineSampleSpacing=150.f;
		Test->TestTrue(TEXT("Smooth route starts"),Nav->FollowSpline(Route.Get()));
	}
	void Sample(UWorld* TickWorld,ELevelTick,float DeltaSeconds)
	{
		if (TickWorld!=World.Get() || !Elder.IsValid() || Nav->NavigationStatus!=EElderNavigationStatus::Moving) return;
		const float D=Nav->CurrentSplineDistance;
		if (D!=LastDistance && LastDistance>0.f) ++Boundaries;
		LastDistance=D;
		// Exclude startup acceleration and the final goal's intentional braking.
		if (bLoopCase ? (Nav->CompletedLaps==0 && D<300.f) : (D<300.f || D>=1200.f)) return;
		const float V=Elder->GetVelocity().Size2D(); ++Frames;
		const float Drop=FMath::Max(0.f,PreviousSpeed-V);
		const auto* Movement=Elder->GetCharacterMovement();
		const float PhysicalBudget=(Movement->GroundFriction*Elder->GetEffectiveMoveSpeed()+Movement->GetMaxAcceleration())*DeltaSeconds+1.f;
		MaxDeltaSeconds=FMath::Max(MaxDeltaSeconds,DeltaSeconds); MaxDropBudget=FMath::Max(MaxDropBudget,PhysicalBudget);
		if(Drop>PhysicalBudget) ++AbruptDrops;
		MaxFrameDrop=FMath::Max(MaxFrameDrop,Drop); PreviousSpeed=V;
		MinSpeed=FMath::Min(MinSpeed,V); MaxSpeed=FMath::Max(MaxSpeed,V); if(V<1.f) ++ZeroFrames;
		if (bLoopCase && V<Speed*.95f && DipReports++<12) UE_LOG(LogTemp,Display,TEXT("ElderSmooth dip speed=%.3f target=%.1f lap=%d location=%s requested=%s decel=%d goalDistance=%.1f"),V,D,Nav->CompletedLaps,*Elder->GetActorLocation().ToString(),*Elder->GetCharacterMovement()->GetLastUpdateRequestedVelocity().ToString(),Cast<AElderlyAIController>(Elder->GetController())->GetPathFollowingComponent()->IsDecelerating(),FVector::Dist2D(Elder->GetActorLocation(),Route->Spline->GetLocationAtDistanceAlongSpline(D,ESplineCoordinateSpace::World)));
		if (UElderAnimInstance* Anim=Cast<UElderAnimInstance>(Elder->GetMesh()->GetAnimInstance()))
		{
			int32 WalkPlayers=0,AdvancedPlayers=0; const bool bHadClockSample=!PlaybackTimes.IsEmpty();
			const float ExpectedRate=Speed/160.f*Elder->WalkingAnimationRateScale;
			bool bCorrect=FMath::IsNearlyEqual(Anim->LocomotionPlayRate,ExpectedRate,.001f);
			for (TFieldIterator<FStructProperty> It(Anim->GetClass()); It; ++It)
			{
				if (It->Struct!=FAnimNode_SequencePlayer::StaticStruct()) continue;
				const auto* Player=It->ContainerPtrToValuePtr<FAnimNode_SequencePlayer>(Anim);
				const auto* Sequence=Player->GetSequence();
				if (!Sequence || !(Sequence->GetName().Contains(TEXT("Walk")) || Sequence->GetName().Contains(TEXT("Strafe")))) continue;
				++WalkPlayers;
				// Zero-weight samples retain cached inputs. Validate every player whose
				// actual clock advanced this frame, including strafes during AI turns.
				const float Clock=Player->GetAccumulatedTime();
				if(const float* Previous=PlaybackTimes.Find(Sequence); Previous && !FMath::IsNearlyEqual(*Previous,Clock,.00001f))
				{ ++AdvancedPlayers; bCorrect &= FMath::IsNearlyEqual(Player->GetPlayRate(),ExpectedRate,.001f); }
				PlaybackTimes.Add(Sequence,Clock);
			}
			++AnimationFrames; if(!bCorrect || WalkPlayers!=8 || (bHadClockSample && AdvancedPlayers==0)) ++BadAnimationFrames;
		}
	}
	FAutomationTestBase* Test; double Deadline; FDelegateHandle TickHandle;
	TWeakObjectPtr<UWorld> World; TWeakObjectPtr<AElderlyAICharacter> Elder;
	TWeakObjectPtr<UElderNavigationComponent> Nav; TWeakObjectPtr<AElderSplinePath> Route; TWeakObjectPtr<AMonitorCameraActor> Monitor;
	float Speed=0.f,MinSpeed=MAX_flt,MaxSpeed=0.f,LastDistance=0.f,FinishTime=0.f;
	int32 Frames=0,Boundaries=0,ZeroFrames=0,HistoryStart=0,AnimationFrames=0,BadAnimationFrames=0;
	FVector FinishLocation;
	bool bLoopCase=false; float StartTime=0.f;
	int32 DipReports=0;
	TMap<const UAnimSequenceBase*,float> PlaybackTimes;
	float PreviousSpeed=0.f,MaxFrameDrop=0.f; int32 AbruptDrops=0;
	float MaxDeltaSeconds=0.f,MaxDropBudget=0.f;
};
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FElderSmoothNavigationTest,"OldMan.Elder.SmoothNavigation",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FElderSmoothNavigationTest::RunTest(const FString&)
{
	if (!GEditor || !GEditor->GetEditorWorldContext().World()->GetMapName().Contains(TEXT("L_ElderControlTest"))) { AddError(TEXT("Open L_ElderControlTest first")); return false; }
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<FElderSmoothScenario>(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	return true;
}

class FElderShowcaseSmoothScenario : public IAutomationLatentCommand
{
public:
	explicit FElderShowcaseSmoothScenario(FAutomationTestBase* InTest):Test(InTest),Deadline(FPlatformTime::Seconds()+65.) {}
	~FElderShowcaseSmoothScenario() { FWorldDelegates::OnWorldPostActorTick.Remove(TickHandle); }
	bool Update() override
	{
		if(FPlatformTime::Seconds()>Deadline) { Test->AddError(TEXT("Showcase smooth timeout")); return true; }
		if(!World.IsValid())
		{
			for(const FWorldContext& C:GEngine->GetWorldContexts()) if(C.WorldType==EWorldType::PIE) World=C.World();
			if(!World.IsValid()) return false;
			for(TActorIterator<AElderlyAICharacter> It(World.Get());It;++It) if(It->ElderNavigation->bAutoStart && It->ElderNavigation->SplinePath) { Elder=*It; break; }
			if(!Elder.IsValid()) { Test->AddError(TEXT("Showcase configured elder missing")); return true; }
			Nav=Elder->ElderNavigation; Length=Nav->SplinePath->Spline->GetSplineLength();
			Elder->ExitWheelchair();
			Test->TestEqual(TEXT("Showcase saved speed 64"),Elder->DesiredMoveSpeed,64.f);
			Test->TestEqual(TEXT("Showcase physical speed synced"),Elder->GetCharacterMovement()->MaxWalkSpeed,Elder->GetEffectiveMoveSpeed());
			Elder->GetMesh()->VisibilityBasedAnimTickOption=EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
			FActorSpawnParameters P; P.ObjectFlags|=RF_Transient;
			Monitor=World->SpawnActor<AMonitorCameraActor>(AMonitorCameraActor::StaticClass(),FVector(100000,0,0),FRotator::ZeroRotator,P);
			Monitor->MonitorRange->SetCollisionEnabled(ECollisionEnabled::NoCollision); Monitor->RegisterElder(Elder.Get());
			HistoryStart=Monitor->StateHistory.Num(); ExpectedEvents=Elder->CurrentState==EElderState::Walking?1:2;
			TickHandle=FWorldDelegates::OnWorldPostActorTick.AddRaw(this,&FElderShowcaseSmoothScenario::Sample);
		}
		if(Nav->NavigationStatus==EElderNavigationStatus::Failed) { Test->AddError(Nav->NavigationMessage); return true; }
		if(Nav->NavigationStatus!=EElderNavigationStatus::Completed) return false;
		if(FinishTime==0.f) { FinishTime=World->GetTimeSeconds(); FinishLocation=Elder->GetActorLocation(); return false; }
		if(World->GetTimeSeconds()<FinishTime+.3f) return false;
		Test->TestTrue(TEXT("Showcase frame-level continuous walking"),Frames>150 && ZeroFrames==0 && MinSpeed>=Elder->GetEffectiveMoveSpeed()*.95f && MaxSpeed<=Elder->GetEffectiveMoveSpeed()+1.f);
		Test->TestEqual(TEXT("Showcase no intermediate state chatter"),Monitor->StateHistory.Num()-HistoryStart,ExpectedEvents);
		Test->TestTrue(TEXT("Showcase actual locomotion playback and coordinates"),BadAnimationFrames==0 && FootBounds.IsValid && FootBounds.GetSize().Size()>10.f);
		Test->TestTrue(TEXT("Showcase one lap reaches endpoint and stays stopped"),Nav->CompletedLaps==1 && Elder->CurrentState==EElderState::Standing && Elder->GetVelocity().Size2D()<.1f && FVector::Dist2D(FinishLocation,Elder->GetActorLocation())<1.f && FVector::Dist2D(FinishLocation,Nav->SplinePath->Spline->GetLocationAtDistanceAlongSpline(Length,ESplineCoordinateSpace::World))<25.f);
		UE_LOG(LogTemp,Display,TEXT("ElderShowcaseSmooth frames=%d min=%.3f max=%.3f zero=%d history=%d badAnim=%d footRange=%.3f end=%s"),Frames,MinSpeed,MaxSpeed,ZeroFrames,Monitor->StateHistory.Num()-HistoryStart,BadAnimationFrames,FootBounds.GetSize().Size(),*FinishLocation.ToString());
		Monitor->Destroy(); return true;
	}
private:
	void Sample(UWorld* W,ELevelTick,float)
	{
		if(W!=World.Get() || !Elder.IsValid() || Nav->NavigationStatus!=EElderNavigationStatus::Moving || Nav->CurrentSplineDistance<300.f || Nav->CurrentSplineDistance>=Length) return;
		++Frames; float V=Elder->GetVelocity().Size2D(); MinSpeed=FMath::Min(MinSpeed,V); MaxSpeed=FMath::Max(MaxSpeed,V); if(V<1.f) ++ZeroFrames;
		auto* Anim=Cast<UElderAnimInstance>(Elder->GetMesh()->GetAnimInstance());
		const FVector Local=Elder->GetActorTransform().InverseTransformVectorNoScale(Elder->GetVelocity());
		// The game-thread anim snapshot may precede this frame's movement/rotation.
		const float ExpectedRate=Elder->DesiredMoveSpeed/160.f*Elder->WalkingAnimationRateScale;
		const FVector2D Sampled=Anim?FVector2D(Anim->LocomotionBlendForward*ExpectedRate,Anim->LocomotionBlendRight*ExpectedRate):FVector2D::ZeroVector;
		const bool bCoherent=FVector2D::Distance(Sampled,FVector2D(Local))<5.f || FVector2D::Distance(Sampled,FVector2D(PreviousLocal))<5.f;
		if(!Anim || !FMath::IsNearlyEqual(Anim->LocomotionPlayRate,ExpectedRate,.001f) || !bCoherent) ++BadAnimationFrames;
		PreviousLocal=Local;
		FootBounds+=Elder->GetActorTransform().InverseTransformPosition(Elder->GetMesh()->GetSocketLocation(TEXT("foot_l")));
	}
	FAutomationTestBase* Test; double Deadline; FDelegateHandle TickHandle;
	TWeakObjectPtr<UWorld> World; TWeakObjectPtr<AElderlyAICharacter> Elder; TWeakObjectPtr<UElderNavigationComponent> Nav; TWeakObjectPtr<AMonitorCameraActor> Monitor;
	float Length=0.f,MinSpeed=MAX_flt,MaxSpeed=0.f,FinishTime=0.f; FVector FinishLocation,PreviousLocal=FVector::ZeroVector; FBox FootBounds=FBox(ForceInit);
	int32 Frames=0,ZeroFrames=0,HistoryStart=0,ExpectedEvents=0,BadAnimationFrames=0;
};
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FElderShowcaseSmoothTest,"OldMan.Showcase.SmoothNavigation",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FElderShowcaseSmoothTest::RunTest(const FString&)
{
	if(!GEditor || !GEditor->GetEditorWorldContext().World()->GetMapName().Contains(TEXT("Showcase"))) { AddError(TEXT("Open Showcase first")); return false; }
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<FElderShowcaseSmoothScenario>(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand()); return true;
}
#endif
