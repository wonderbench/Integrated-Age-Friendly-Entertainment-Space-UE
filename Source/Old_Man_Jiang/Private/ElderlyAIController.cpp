#include "ElderlyAIController.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISense_Sight.h"
#include "ElderlyAICharacter.h"
#include "ElderNavigationComponent.h"
#include "BrainComponent.h"
#include "Navigation/PathFollowingComponent.h"

AElderlyAIController::AElderlyAIController()
{
	AIPerception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerception"));
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	AIPerception->ConfigureSense(*SightConfig);
	AIPerception->SetDominantSense(UAISense_Sight::StaticClass());
	AIPerception->OnTargetPerceptionUpdated.AddDynamic(this, &AElderlyAIController::HandleTargetPerceptionUpdated);
	// Keep the same controller/blackboard alive during temporary player possession.
	bStopAILogicOnUnposses = false;
}
void AElderlyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	if (const AElderlyAICharacter* Elder = Cast<AElderlyAICharacter>(InPawn))
	{
		SightRadius = Elder->SightRadius; LoseSightRadius = Elder->LoseSightRadius; PeripheralVisionHalfAngleDegrees = Elder->PeripheralVisionHalfAngleDegrees;
	}
	RefreshSightConfig();
}
void AElderlyAIController::RefreshSightConfig()
{
	SightConfig->SightRadius = SightRadius;
	SightConfig->LoseSightRadius = FMath::Max(LoseSightRadius, SightRadius);
	SightConfig->PeripheralVisionAngleDegrees = PeripheralVisionHalfAngleDegrees;
	AIPerception->ConfigureSense(*SightConfig);
	AIPerception->RequestStimuliListenerUpdate();
}
void AElderlyAIController::HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (!bSuspendedForPlayer && GetPawn()) OnElderTargetPerceptionUpdated(Actor, Stimulus);
}

FPathFollowingRequestResult AElderlyAIController::MoveTo(const FAIMoveRequest& Request, FNavPathSharedPtr* OutPath)
{
	AElderlyAICharacter* Elder = Cast<AElderlyAICharacter>(GetPawn());
	if (bSuspendedForPlayer || !Elder || !Elder->CanElderMove()) return FPathFollowingRequestResult();
	// A new external command replaces the managed plan, even if its new path fails.
	// Otherwise a queued spline continuation could steal control back next frame.
	if (Elder->ElderNavigation->HasManagedNavigation() && !Elder->ElderNavigation->IsIssuingNavigationRequest()) Elder->ElderNavigation->StopNavigation();
	if (Elder->CurrentState == EElderState::Standing) Elder->StartWalking();
	else if (Elder->CurrentState == EElderState::WheelchairIdle) Elder->SetWheelchairMoving(true);
	const bool bContinuous=Elder->ElderNavigation->IsIssuingNavigationRequest() && Elder->ElderNavigation->IsContinuousSplineGoal();
	if (bContinuous && GetMoveStatus()==EPathFollowingStatus::Idle && GetPathFollowingComponent()->HasReached(Request))
	{
		// Super's immediate-finish path resets velocity even when no new path is needed.
		// Keep the component's bounded, deferred AlreadyAtGoal continuation instead.
		FPathFollowingRequestResult Reached; Reached.Code=EPathFollowingRequestResult::AlreadyAtGoal; return Reached;
	}
	bIssuingMove = true;
	GetPathFollowingComponent()->SetStopMovementOnFinish(true);
	const FPathFollowingRequestResult Result = Super::MoveTo(Request, OutPath);
	bIssuingMove = false;
	if (Result.Code == EPathFollowingRequestResult::RequestSuccessful)
	{
		LastMoveRequest = Request;
		GetPathFollowingComponent()->SetStopMovementOnFinish(!bContinuous);
	}
	else if (Result.Code!=EPathFollowingRequestResult::AlreadyAtGoal || !bContinuous) Elder->StopElderMovement();
	return Result;
}

void AElderlyAIController::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	if (!bSuspendedForPlayer && !bIssuingMove && GetMoveStatus() == EPathFollowingStatus::Idle)
	{
		if (AElderlyAICharacter* Elder = Cast<AElderlyAICharacter>(GetPawn()))
		{
			if (!Result.IsSuccess() || !Elder->CanElderMove() || !Elder->ElderNavigation->ContinuesAfter(RequestID)) Elder->StopElderMovement();
		}
	}
	Super::OnMoveCompleted(RequestID, Result);
}

void AElderlyAIController::SuspendForPlayerControl()
{
	if (bSuspendedForPlayer) return;
	bResumeMove = GetMoveStatus() != EPathFollowingStatus::Idle && LastMoveRequest.IsValid();
	bSuspendedForPlayer = true; // Guard abort delegates and subsequent queued MoveTo calls first.
	if (AElderlyAICharacter* Elder=Cast<AElderlyAICharacter>(GetPawn()))
	{
		if (Elder->ElderNavigation->HasManagedNavigation()) bResumeMove=false;
		Elder->ElderNavigation->SuspendForPlayerControl();
	}
	bResumeBrain = BrainComponent && BrainComponent->IsRunning();
	if (bResumeBrain) BrainComponent->PauseLogic(TEXT("Player takeover"));
	bSightWasEnabled = AIPerception->IsSenseEnabled(UAISense_Sight::StaticClass());
	AIPerception->SetSenseEnabled(UAISense_Sight::StaticClass(), false);
	StopMovement();
}

void AElderlyAIController::ResumeAfterPlayerControl()
{
	if (!bSuspendedForPlayer || !GetPawn()) return;
	bSuspendedForPlayer = false;
	AIPerception->SetSenseEnabled(UAISense_Sight::StaticClass(), bSightWasEnabled);
	const bool bRestoreMove = bResumeMove;
	bResumeMove = false;
	if (bResumeBrain && BrainComponent) BrainComponent->ResumeLogic(TEXT("Player released elder"));
	bResumeBrain = false;
	// A resumed brain may already have issued a replacement request.
	if (bRestoreMove && GetMoveStatus() == EPathFollowingStatus::Idle) MoveTo(LastMoveRequest);
	if (AElderlyAICharacter* Elder=Cast<AElderlyAICharacter>(GetPawn())) Elder->ElderNavigation->NotifyAIControllerReady();
}
