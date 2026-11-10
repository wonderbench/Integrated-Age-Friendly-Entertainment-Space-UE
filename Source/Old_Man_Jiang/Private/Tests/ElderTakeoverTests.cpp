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
#include "ElderlyAIController.h"
#include "MonitorCameraActor.h"
#include "Navigation/PathFollowingComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Sight.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/UnrealType.h"

// Load L_ElderControlTest, then run asynchronously via Automation RunTests OldMan.Elder.Takeover.
// Real key events traverse PlayerController -> EnhancedInput -> CharacterMovement over engine frames.
class FElderTakeoverScenario : public IAutomationLatentCommand
{
public:
	explicit FElderTakeoverScenario(FAutomationTestBase* InTest) : Test(InTest), StartupDeadline(FPlatformTime::Seconds() + 30.) {}
	virtual bool Update() override
	{
		if (!World.IsValid())
		{
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				if (Context.WorldType == EWorldType::PIE && Context.World() && Context.World()->GetMapName().Contains(TEXT("L_ElderControlTest"))) World = Context.World();
			}
			if (!World.IsValid())
			{
				if (FPlatformTime::Seconds() < StartupDeadline) return false;
				Test->AddError(TEXT("PIE did not initialize L_ElderControlTest within 30 seconds.")); return true;
			}
			PC = Cast<AOrbitCameraController>(World->GetFirstPlayerController());
			for (TActorIterator<AElderlyAICharacter> It(World.Get()); It; ++It)
			{
				if (It->ActorHasTag(TEXT("ElderTakeoverTestA"))) Elder = *It;
				if (It->ActorHasTag(TEXT("ElderTakeoverTestB"))) Other = *It;
			}
			if (!PC.IsValid() || !Elder.IsValid() || !Other.IsValid()) { Test->AddError(TEXT("Missing test fixtures/controller")); return true; }
			OriginalPawn = PC->GetPawn();
			AI = Cast<AElderlyAIController>(Elder->GetController());
			if (!AI.IsValid()) { Test->AddError(TEXT("Default AI possession failed")); return true; }
			Name = Elder->ElderName;
			Animation = Elder->GetMesh()->GetAnimInstance();
			Test->TestNotNull(TEXT("ABP_AI initialized"), Animation.Get());
			Test->TestTrue(TEXT("Independent names"), Name != Other->ElderName && !Name.IsEmpty());
			Test->TestTrue(TEXT("Default standing"), Elder->CurrentState == EElderState::Standing);
			Elder->SetElderGender(EElderGender::Female);
			Elder->SetElderGender(EElderGender::Male);
			Test->TestEqual(TEXT("Gender retains name"), Elder->ElderName, Name);
			// Gender changes may intentionally refresh animation; takeover may not.
			Animation = Elder->GetMesh()->GetAnimInstance();
			FActorSpawnParameters Params; Params.ObjectFlags |= RF_Transient;
			Monitor = World->SpawnActor<AMonitorCameraActor>(AMonitorCameraActor::StaticClass(), FVector(0,0,500), FRotator::ZeroRotator, Params);
			Monitor->RegisterElder(Elder.Get()); Monitor->RegisterElder(Other.Get());
			Start = Elder->GetActorLocation();
			FAIMoveRequest Request(Start + FVector(1800, 0, 0));
			Request.SetAcceptanceRadius(30.f);
			Test->TestTrue(TEXT("Navmesh MoveTo accepted"), AI->MoveTo(Request).Code == EPathFollowingRequestResult::RequestSuccessful);
			Next = World->GetTimeSeconds() + 1.f;
			return false;
		}
		if (!PC.IsValid() || !Elder.IsValid() || !Other.IsValid()) { Test->AddError(TEXT("Fixture destroyed during test")); return true; }
		if (bExerciseLook) PC->SetControlRotation(FRotator(0, 45.f * FMath::Clamp((World->GetTimeSeconds()-LookStartTime)/.5f, 0.f, 1.f), 0));
		if (World->GetTimeSeconds() < Next) return false;
		if (CheckTurnPending)
		{
			Test->TestTrue(TEXT("Turning gates translation"), FVector::Dist2D(Start, Elder->GetActorLocation()) < 1.f);
			Test->TestTrue(TEXT("Standing during authored turn"), Elder->bTurningInPlace && Elder->CurrentState == EElderState::Standing);
			Test->TestTrue(TEXT("Turn phase advances"), Elder->TurnAnimationPhase > 0.f && Elder->TurnAnimationPhase < 1.f);
			UAnimInstance* Anim = Elder->GetMesh()->GetAnimInstance();
			const FBoolProperty* Active = FindFProperty<FBoolProperty>(Anim->GetClass(), TEXT("TurnActive"));
			Test->TestTrue(TEXT("ABP turn branch active"), Active && Active->GetPropertyValue_InContainer(Anim));
			Test->TestTrue(TEXT("Root rotation removed only from turn pose"), FMath::Abs(Elder->GetMesh()->GetSocketTransform(TEXT("root"), RTS_Component).Rotator().Yaw) < 1.f);
			CheckCone();
			CheckTurnPending = false;
			Next = World->GetTimeSeconds() + 5.5f;
			return false;
		}
		switch (Step++)
		{
		case 0:
			Test->TestTrue(TEXT("AI actually moved"), FVector::Dist2D(Start, Elder->GetActorLocation()) > 20.f);
			Test->TestTrue(TEXT("AI animation velocity input"), HasAnimationVelocity());
			HistoryCount = Monitor->StateHistory.Num();
			Test->TestTrue(TEXT("Takeover same elder"), PC->TakeControlOfElder(Elder.Get()));
			Test->TestTrue(TEXT("Exclusive player controller"), Elder->GetController() == PC.Get() && AI->GetPawn() == nullptr);
			Test->TestTrue(TEXT("AI sight suspended"), !AI->AIPerception->IsSenseEnabled(UAISense_Sight::StaticClass()));
			Test->TestTrue(TEXT("No false state/history during takeover"), Elder->CurrentState == EElderState::Walking && Monitor->StateHistory.Num() == HistoryCount);
			Test->TestTrue(TEXT("Queued AI input rejected"), AI->MoveTo(FAIMoveRequest(FVector::ZeroVector)).Code == EPathFollowingRequestResult::Failed);
			PC->SetControlRotation(FRotator::ZeroRotator);
			Next = World->GetTimeSeconds() + .3f; break;
		case 1: Start = Elder->GetActorLocation(); Key(EKeys::W, true); Next = World->GetTimeSeconds() + .6f; break;
		case 2:
			Test->TestTrue(TEXT("W mapped forward"), Elder->GetActorLocation().X > Start.X + 20.f);
			Test->TestTrue(TEXT("Player animation velocity input"), HasAnimationVelocity());
			Key(EKeys::W, false); Next = World->GetTimeSeconds() + .2f; break;
		case 3:
			Test->TestTrue(TEXT("Release W enters standing"), Elder->CurrentState == EElderState::Standing);
			Start = Elder->GetActorLocation(); Key(EKeys::S, true); CheckTurnPending = true; Next = World->GetTimeSeconds() + .8f; break;
		case 4: Test->TestTrue(TEXT("S mapped backward"), Elder->GetActorLocation().X < Start.X - 20.f); Key(EKeys::S, false); Next = World->GetTimeSeconds() + .2f; break;
		case 5: Start = Elder->GetActorLocation(); Key(EKeys::A, true); CheckTurnPending = true; Next = World->GetTimeSeconds() + .8f; break;
		case 6: Test->TestTrue(TEXT("A mapped left"), Elder->GetActorLocation().Y < Start.Y - 20.f); Key(EKeys::A, false); Next = World->GetTimeSeconds() + .2f; break;
		case 7: Start = Elder->GetActorLocation(); Key(EKeys::D, true); CheckTurnPending = true; Next = World->GetTimeSeconds() + .8f; break;
		case 8:
			Test->TestTrue(TEXT("D mapped right"), Elder->GetActorLocation().Y > Start.Y + 20.f); Key(EKeys::D, false);
			Elder->SetSitting(); Start = Elder->GetActorLocation(); Key(EKeys::W, true); Next = World->GetTimeSeconds() + .4f; break;
		case 9:
			Test->TestTrue(TEXT("Sitting blocks input"), Elder->CurrentState == EElderState::Sitting && FVector::Dist2D(Start, Elder->GetActorLocation()) < 1.f);
			Key(EKeys::W, false); Elder->StartFalling(); Next = World->GetTimeSeconds() + 1.f; break;
		case 10:
			Test->TestTrue(TEXT("Falling completes while player controlled"), Elder->CurrentState == EElderState::Fallen);
			Elder->EnterWheelchair(); Start = Elder->GetActorLocation(); Key(EKeys::W, true); Next = World->GetTimeSeconds() + 2.f; break;
		case 11:
			Test->TestTrue(TEXT("Wheelchair input moves"), Elder->CurrentState == EElderState::WheelchairMoving && FVector::Dist2D(Start, Elder->GetActorLocation()) > 20.f);
			Key(EKeys::W, false); Next = World->GetTimeSeconds() + .2f; break;
		case 12:
			Test->TestTrue(TEXT("Wheelchair release idles"), Elder->CurrentState == EElderState::WheelchairIdle);
			Elder->ExitWheelchair(); Start = Elder->GetActorLocation(); Key(EKeys::Tab, true); Next = World->GetTimeSeconds() + .8f; break;
		case 13:
			Key(EKeys::Tab, false);
			Test->TestTrue(TEXT("Tab restored original observation pawn"), PC->GetPawn() == OriginalPawn.Get() && PC->GetViewTarget() == OriginalPawn.Get());
			Test->TestTrue(TEXT("Original AI controller restored"), Elder->GetController() == AI.Get() && !AI->bSuspendedForPlayer);
			Test->TestTrue(TEXT("AI sight restored"), AI->AIPerception->IsSenseEnabled(UAISense_Sight::StaticClass()));
			Test->TestTrue(TEXT("Saved nav goal resumed"), FVector::Dist2D(Start, Elder->GetActorLocation()) > 20.f);
			Test->TestTrue(TEXT("Same AnimInstance across takeover"), Animation.Get() == Elder->GetMesh()->GetAnimInstance());
			Test->TestEqual(TEXT("Name unchanged"), Elder->ElderName, Name);
			Test->TestTrue(TEXT("Other elder not modified"), Other->CurrentState == EElderState::Standing && Other->GetController() != PC.Get());
			Test->TestTrue(TEXT("Monitor remained bound"), Monitor->StateHistory.Num() > HistoryCount);
			Test->TestTrue(TEXT("Can select a different elder"), PC->TakeControlOfElder(Other.Get()));
			Test->TestTrue(TEXT("Release second elder"), PC->ReleaseElderControl());
			AI->StopMovement(); Elder->SetStanding(); Start = Elder->GetActorLocation(); Key(EKeys::W, true); Next = World->GetTimeSeconds() + .5f; break;
		case 14:
			Key(EKeys::W, false);
			Test->TestTrue(TEXT("Player input no longer affects elder"), FVector::Dist2D(Start, Elder->GetActorLocation()) < 1.f);
			Test->TestTrue(TEXT("Retake elder"), PC->TakeControlOfElder(Elder.Get()));
			Elder->SetActorRotation(FRotator::ZeroRotator); Start = Elder->GetActorLocation();
			Key(EKeys::A, true); Next = World->GetTimeSeconds() + 1.5f; break;
		case 15:
			Test->TestTrue(TEXT("Partial left turn without translation"), Elder->bTurningInPlace && Elder->GetActorRotation().Yaw < -10.f && FVector::Dist2D(Start, Elder->GetActorLocation()) < 1.f);
			SavedTurnPhase = Elder->TurnAnimationPhase; SavedTurnCoordinate = Elder->TurnBlendPosition;
			// Change A -> S without an empty EnhancedInput frame.
			Key(EKeys::S, true); Key(EKeys::A, false); Next = World->GetTimeSeconds() + .3f; break;
		case 16:
			Test->TestTrue(TEXT("Held direction change retains clip and phase"), Elder->bTurningInPlace && Elder->TurnAnimationPhase > SavedTurnPhase && FMath::IsNearlyEqual(Elder->TurnBlendPosition, SavedTurnCoordinate));
			Test->TestTrue(TEXT("Retarget does not translate"), FVector::Dist2D(Start, Elder->GetActorLocation()) < 1.f);
			SavedTurnPhase = Elder->TurnAnimationPhase;
			bExerciseLook = true; LookStartTime = World->GetTimeSeconds(); Next = LookStartTime + .6f; break;
		case 17:
			bExerciseLook = false;
			Test->TestTrue(TEXT("Continuous look cannot restart active clip"), Elder->bTurningInPlace && Elder->TurnAnimationPhase > SavedTurnPhase && FMath::IsNearlyEqual(Elder->TurnBlendPosition, SavedTurnCoordinate));
			Test->TestTrue(TEXT("Continuous look cannot cause sliding"), FVector::Dist2D(Start, Elder->GetActorLocation()) < 1.f);
			HeldMoveDeadline = World->GetTimeSeconds() + 12.f;
			Next = World->GetTimeSeconds() + .1f; break;
		case 18:
			if (World->GetTimeSeconds()<HeldMoveDeadline && (Elder->bTurningInPlace || Elder->CurrentState!=EElderState::Walking || Elder->GetActorLocation().X>=Start.X-20.f || Elder->GetActorLocation().Y>=Start.Y-20.f || FMath::Abs(FMath::FindDeltaAngleDegrees(Elder->GetActorRotation().Yaw,-135.f))>=2.f))
			{
				--Step; Next=World->GetTimeSeconds()+.1f; break;
			}
			UE_LOG(LogTemp,Display,TEXT("HeldTurn final actorYaw=%.2f controlYaw=%.2f phase=%.3f coordinate=%.2f turning=%d state=%d location=%s start=%s"),Elder->GetActorRotation().Yaw,PC->GetControlRotation().Yaw,Elder->TurnAnimationPhase,Elder->TurnBlendPosition,Elder->bTurningInPlace,int32(Elder->CurrentState),*Elder->GetActorLocation().ToString(),*Start.ToString());
			Test->TestTrue(TEXT("Latest held heading eventually walks southwest"), !Elder->bTurningInPlace && Elder->CurrentState == EElderState::Walking && Elder->GetActorLocation().X < Start.X-20.f && Elder->GetActorLocation().Y < Start.Y-20.f && FMath::Abs(FMath::FindDeltaAngleDegrees(Elder->GetActorRotation().Yaw, -135.f)) < 2.f);
			Key(EKeys::S, false); Next = World->GetTimeSeconds() + .2f; break;
		case 19:
			Start = Elder->GetActorLocation(); Key(EKeys::W, true); Next = World->GetTimeSeconds() + .8f; break;
		case 20:
			Test->TestTrue(TEXT("Release fixture is turning"), Elder->bTurningInPlace);
			Key(EKeys::W, false); Next = World->GetTimeSeconds() + .2f; break;
		case 21:
			Test->TestTrue(TEXT("Release keeps committed turn playing"), Elder->bTurningInPlace && Elder->IsActorTickEnabled());
			Next = World->GetTimeSeconds() + 5.5f; break;
		case 22:
			Test->TestTrue(TEXT("Released turn completes without translation"), !Elder->bTurningInPlace && Elder->CurrentState==EElderState::Standing && FVector::Dist2D(Start, Elder->GetActorLocation()) < 1.f);
			Key(EKeys::D, true); Next = World->GetTimeSeconds() + .5f; break;
		case 23:
			Test->TestTrue(TEXT("Exit during turn"), Elder->bTurningInPlace && PC->ReleaseElderControl());
			Test->TestTrue(TEXT("AI restored without turn tick"), !Elder->bTurningInPlace && !Elder->IsActorTickEnabled() && Elder->GetController() == AI.Get());
			Key(EKeys::D, false);
			Test->TestTrue(TEXT("Retake after cancelled turn"), PC->TakeControlOfElder(Elder.Get()));
			AI->Destroy();
			Test->TestTrue(TEXT("Destroyed AI controller recreated"), PC->ReleaseElderControl() && IsValid(Elder->GetController()));
			Monitor->UnregisterElder(Elder.Get()); Monitor->UnregisterElder(Other.Get()); Monitor->Destroy();
			Test->TestTrue(TEXT("Takeover before target destruction"), PC->TakeControlOfElder(Other.Get()));
			Other->Destroy();
			Test->TestTrue(TEXT("Destroyed elder restores observation"), PC->GetPawn() == OriginalPawn.Get() && !PC->IsControllingElder());
			UE_LOG(LogTemp, Display, TEXT("ElderTakeoverTest: completed all runtime checks"));
			return true;
		}
		return false;
	}
private:
	void CheckCone()
	{
		const FTransform Transform = Elder->SightConeMesh->GetComponentTransform();
		const FVector Tip = Transform.TransformPosition(Elder->SightMeshTip);
		const FVector End = Transform.TransformPosition(Elder->SightMeshBaseCenter);
		Test->TestTrue(TEXT("Cone tip at head"), FVector::Dist(Tip, Elder->GetMesh()->GetSocketLocation(TEXT("head"))) < 1.f);
		Test->TestTrue(TEXT("Cone forward, follows turning elder"), FVector::DotProduct((End-Tip).GetSafeNormal(), Elder->GetActorForwardVector()) > .85f);
		Test->TestTrue(TEXT("Cone length matches sight"), FMath::Abs((End-Tip).Size()-Elder->SightRadius) < 2.f);
		Test->TestTrue(TEXT("Cone is noncolliding visualization"), Elder->SightConeMesh->GetCollisionEnabled() == ECollisionEnabled::NoCollision && !Elder->SightConeMesh->CastShadow);
	}
	void Key(FKey KeyName, bool bPressed)
	{
		PC->InputKey(FInputKeyEventArgs(nullptr, INPUTDEVICEID_NONE, KeyName, bPressed ? IE_Pressed : IE_Released, bPressed ? 1.f : 0.f, false, FPlatformTime::Cycles64()));
	}
	bool HasAnimationVelocity() const
	{
		UAnimInstance* Anim = Elder->GetMesh()->GetAnimInstance();
		if (!Anim) return false;
		for (TFieldIterator<FNumericProperty> It(Anim->GetClass()); It; ++It)
		{
			if (It->GetName().Contains(TEXT("Velocity")) && It->IsFloatingPoint() && FMath::Abs(It->GetFloatingPointPropertyValue(It->ContainerPtrToValuePtr<void>(Anim))) > 1.) return true;
		}
		return false;
	}
	FAutomationTestBase* Test;
	double StartupDeadline;
	TWeakObjectPtr<UWorld> World;
	TWeakObjectPtr<AOrbitCameraController> PC;
	TWeakObjectPtr<APawn> OriginalPawn;
	TWeakObjectPtr<AElderlyAICharacter> Elder, Other;
	TWeakObjectPtr<AElderlyAIController> AI;
	TWeakObjectPtr<AMonitorCameraActor> Monitor;
	TWeakObjectPtr<UAnimInstance> Animation;
	FVector Start;
	FString Name;
	float Next = 0.f;
	float StoppedYaw = 0.f;
	float SavedTurnPhase = 0.f, SavedTurnCoordinate = 0.f, LookStartTime = 0.f, HeldMoveDeadline = 0.f;
	bool bExerciseLook = false;
	bool CheckTurnPending = false;
	int32 Step = 0, HistoryCount = 0;
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FElderTakeoverTest, "OldMan.Elder.Takeover", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FElderTakeoverTest::RunTest(const FString& Parameters)
{
	if (!GEditor || !GEditor->GetEditorWorldContext().World()->GetMapName().Contains(TEXT("L_ElderControlTest")))
	{
		AddError(TEXT("Open L_ElderControlTest first. This test never modifies production maps."));
		return false;
	}
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<FElderTakeoverScenario>(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	return true;
}
#endif
