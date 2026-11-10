#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AITypes.h"
#include "Navigation/PathFollowingComponent.h"
#include "ElderTypes.h"
#include "ElderNavigationComponent.generated.h"

class AElderSplinePath;
class AElderlyAICharacter;
class AElderlyAIController;

UENUM(BlueprintType)
enum class EElderNavigationMode : uint8 { None, TargetActor, Spline };
UENUM(BlueprintType)
enum class EElderNavigationStatus : uint8 { Idle, Moving, Paused, Completed, Failed };
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FElderNavigationFinished, bool, bSuccess, const FString&, Message);

/** Per-elder route configuration/progress. All movement is issued through its existing AIController. */
UCLASS(ClassGroup=(AI), meta=(BlueprintSpawnableComponent))
class OLD_MAN_JIANG_API UElderNavigationComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UElderNavigationComponent();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Elder|AI Navigation") EElderNavigationMode NavigationMode=EElderNavigationMode::None;
	/** Opt-in: existing elders remain idle until explicitly configured. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Elder|AI Navigation") bool bAutoStart=false;
	/** Pick a TargetPoint or other actor in this level using the Details eyedropper. */
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Elder|AI Navigation") TObjectPtr<AActor> TargetActor;
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="Elder|AI Navigation") TObjectPtr<AElderSplinePath> SplinePath;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Elder|AI Navigation", meta=(ClampMin="5",Units="cm")) float AcceptanceRadius=20.f;
	/** NavMesh waypoints sampled along the curve; smaller spacing follows bends more closely. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Elder|AI Navigation", meta=(ClampMin="25",Units="cm")) float SplineSampleSpacing=150.f;
	/** Repeat the route; use a closed spline for a continuous loop without a return leg. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Elder|AI Navigation") bool bLoopSpline=false;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Elder|AI Navigation") EElderNavigationStatus NavigationStatus=EElderNavigationStatus::Idle;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Elder|AI Navigation") float CurrentSplineDistance=0.f;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Elder|AI Navigation") int32 CompletedLaps=0;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Elder|AI Navigation") FString NavigationMessage;
	UPROPERTY(BlueprintAssignable, Category="Elder|AI Navigation") FElderNavigationFinished OnNavigationFinished;
	UFUNCTION(BlueprintCallable, Category="Elder|AI Navigation", meta=(ReturnDisplayName="Started")) bool StartConfiguredNavigation();
	UFUNCTION(BlueprintCallable, Category="Elder|AI Navigation") bool NavigateToActor(AActor* NewTarget);
	UFUNCTION(BlueprintCallable, Category="Elder|AI Navigation") bool FollowSpline(AElderSplinePath* NewPath);
	UFUNCTION(BlueprintCallable, Category="Elder|AI Navigation") void PauseNavigation();
	UFUNCTION(BlueprintCallable, Category="Elder|AI Navigation") bool ResumeNavigation();
	UFUNCTION(BlueprintCallable, Category="Elder|AI Navigation") void StopNavigation();
	bool HasManagedNavigation() const;
	bool IsPausedConfiguredSpline() const;
	uint64 GetNavigationGeneration() const { return NavigationGeneration; }
	bool IsIssuingNavigationRequest() const { return bIssuingRequest; }
	bool IsContinuousSplineGoal() const;
	bool ContinuesAfter(FAIRequestID RequestID) const { return RequestID==ActiveRequest && IsContinuousSplineGoal(); }
	void SuspendForPlayerControl();
	void NotifyAIControllerReady();
private:
	TWeakObjectPtr<AElderlyAIController> BoundController;
	UPROPERTY(Transient) TObjectPtr<AActor> ActiveTarget;
	UPROPERTY(Transient) TObjectPtr<AElderSplinePath> ActiveSpline;
	EElderNavigationMode ActiveMode=EElderNavigationMode::None;
	FAIRequestID ActiveRequest=FAIRequestID::InvalidRequest;
	FTimerHandle AdvanceTimer, ReadyTimer;
	bool bIssuingRequest=false;
	bool bResumeOnAIControl=false;
	bool bAutoStartAttempted=false;
	bool bAdvancePending=false;
	uint64 NavigationGeneration=0;
	AElderlyAICharacter* Elder() const;
	AElderlyAIController* AvailableController() const;
	void HandleAIReady();
	void UnbindController();
	void ClearActiveReferences();
	void PauseInternal(bool bForControl);
	bool IssueCurrentGoal();
	void QueueAdvance();
	void AdvanceSpline();
	void Finish(bool bSuccess, const FString& Message);
	UFUNCTION() void HandleMoveCompleted(FAIRequestID RequestID, EPathFollowingResult::Type Result);
	UFUNCTION() void HandleRouteActorDestroyed(AActor* Actor);
	UFUNCTION() void HandleElderStateChanged(AActor* Actor, const FString& Name, EElderState Previous, EElderState NewState);
};
