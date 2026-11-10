#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "ElderlyAIController.generated.h"

class UAIPerceptionComponent;
class UAISenseConfig_Sight;

/** Dedicated controller for elderly actors. Perception is intentionally here, not in the player character. */
UCLASS(Blueprintable)
class OLD_MAN_JIANG_API AElderlyAIController : public AAIController
{
	GENERATED_BODY()
public:
	AElderlyAIController();
	virtual void OnPossess(APawn* InPawn) override;
	virtual FPathFollowingRequestResult MoveTo(const FAIMoveRequest& MoveRequest, FNavPathSharedPtr* OutPath = nullptr) override;
	virtual void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result) override;
	UFUNCTION(BlueprintCallable, Category="Elder|Control") void SuspendForPlayerControl();
	UFUNCTION(BlueprintCallable, Category="Elder|Control") void ResumeAfterPlayerControl();
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Elder|Control") bool bSuspendedForPlayer = false;
	bool HasSuspendedMove() const { return bResumeMove; }
	const FAIMoveRequest& GetSuspendedMove() const { return LastMoveRequest; }

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Elder|Perception") TObjectPtr<UAIPerceptionComponent> AIPerception;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Elder|Perception") TObjectPtr<UAISenseConfig_Sight> SightConfig;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Elder|Perception", meta=(ClampMin="0")) float SightRadius = 1500.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Elder|Perception", meta=(ClampMin="0")) float LoseSightRadius = 1800.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Elder|Perception", meta=(ClampMin="0", ClampMax="180")) float PeripheralVisionHalfAngleDegrees = 55.f;
	UFUNCTION(BlueprintCallable, Category="Elder|Perception") void RefreshSightConfig();
	UFUNCTION(BlueprintImplementableEvent, Category="Elder|Perception") void OnElderTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);
private:
	FAIMoveRequest LastMoveRequest;
	bool bResumeMove = false;
	bool bResumeBrain = false;
	bool bSightWasEnabled = true;
	bool bIssuingMove = false;
	UFUNCTION() void HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);
};
