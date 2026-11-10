#include "ElderNavigationComponent.h"
#include "ElderSplinePath.h"
#include "ElderlyAICharacter.h"
#include "ElderlyAIController.h"
#include "Components/SplineComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"

UElderNavigationComponent::UElderNavigationComponent() { PrimaryComponentTick.bCanEverTick=false; }
AElderlyAICharacter* UElderNavigationComponent::Elder() const { return Cast<AElderlyAICharacter>(GetOwner()); }
AElderlyAIController* UElderNavigationComponent::AvailableController() const
{
	const AElderlyAICharacter* Owner=Elder();
	AElderlyAIController* AI=Owner?Cast<AElderlyAIController>(Owner->GetController()):nullptr;
	return AI && !AI->bSuspendedForPlayer && Owner->CanElderMove()?AI:nullptr;
}
void UElderNavigationComponent::BeginPlay()
{
	Super::BeginPlay();
	if (AElderlyAICharacter* Owner=Elder()) Owner->OnElderStateChanged.AddUniqueDynamic(this,&UElderNavigationComponent::HandleElderStateChanged);
	NotifyAIControllerReady();
}
void UElderNavigationComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
	UnbindController(); ClearActiveReferences();
	if (AElderlyAICharacter* Owner=Elder()) Owner->OnElderStateChanged.RemoveDynamic(this,&UElderNavigationComponent::HandleElderStateChanged);
	Super::EndPlay(Reason);
}
bool UElderNavigationComponent::HasManagedNavigation() const { return NavigationStatus==EElderNavigationStatus::Moving || NavigationStatus==EElderNavigationStatus::Paused; }
bool UElderNavigationComponent::IsPausedConfiguredSpline() const { return NavigationStatus==EElderNavigationStatus::Paused && ActiveMode==EElderNavigationMode::Spline && NavigationMode==EElderNavigationMode::Spline && IsValid(ActiveSpline) && ActiveSpline==SplinePath; }
bool UElderNavigationComponent::IsContinuousSplineGoal() const
{
	return NavigationStatus==EElderNavigationStatus::Moving && ActiveMode==EElderNavigationMode::Spline
		&& IsValid(ActiveSpline) && (bLoopSpline || CurrentSplineDistance<ActiveSpline->Spline->GetSplineLength()-KINDA_SMALL_NUMBER);
}
void UElderNavigationComponent::NotifyAIControllerReady()
{
	if (!HasBegunPlay() || !GetWorld()) return;
	GetWorld()->GetTimerManager().ClearTimer(ReadyTimer);
	ReadyTimer=GetWorld()->GetTimerManager().SetTimerForNextTick(this,&UElderNavigationComponent::HandleAIReady);
}
void UElderNavigationComponent::HandleAIReady()
{
	if (!AvailableController()) return;
	if (bResumeOnAIControl && NavigationStatus==EElderNavigationStatus::Paused) { ResumeNavigation(); return; }
	if (!bAutoStartAttempted && bAutoStart && !HasManagedNavigation()) { bAutoStartAttempted=true; StartConfiguredNavigation(); }
}
bool UElderNavigationComponent::NavigateToActor(AActor* NewTarget)
{
	if (!IsValid(NewTarget) || NewTarget==GetOwner() || NewTarget->GetWorld()!=GetWorld()) { NavigationMessage=TEXT("请选择同一场景中的有效目标（不能是老人自己）"); return false; }
	TargetActor=NewTarget; NavigationMode=EElderNavigationMode::TargetActor;
	return StartConfiguredNavigation();
}
bool UElderNavigationComponent::FollowSpline(AElderSplinePath* NewPath)
{
	if (!IsValid(NewPath) || NewPath->GetWorld()!=GetWorld()) { NavigationMessage=TEXT("请选择同一场景中的有效样条路线"); return false; }
	SplinePath=NewPath; NavigationMode=EElderNavigationMode::Spline;
	return StartConfiguredNavigation();
}
bool UElderNavigationComponent::StartConfiguredNavigation()
{
	if (!AvailableController()) { NavigationMessage=TEXT("需要AI控制且老人处于可移动状态"); return false; }
	if (NavigationMode==EElderNavigationMode::TargetActor && (!IsValid(TargetActor) || TargetActor==GetOwner() || TargetActor->GetWorld()!=GetWorld())) { NavigationMessage=TEXT("未指定有效目标Actor"); return false; }
	if (NavigationMode==EElderNavigationMode::Spline && (!IsValid(SplinePath) || SplinePath->GetWorld()!=GetWorld() || SplinePath->Spline->GetNumberOfSplinePoints()<2 || SplinePath->Spline->GetSplineLength()<1.f)) { NavigationMessage=TEXT("样条路线需要至少两个不同的点"); return false; }
	if (NavigationMode==EElderNavigationMode::None) { NavigationMessage=TEXT("请选择目标导航或样条导航模式"); return false; }
	StopNavigation(); ++NavigationGeneration; bAutoStartAttempted=true;
	ActiveMode=NavigationMode; ActiveTarget=TargetActor; ActiveSpline=SplinePath;
	if (ActiveMode==EElderNavigationMode::TargetActor) ActiveTarget->OnDestroyed.AddUniqueDynamic(this,&UElderNavigationComponent::HandleRouteActorDestroyed);
	else ActiveSpline->OnDestroyed.AddUniqueDynamic(this,&UElderNavigationComponent::HandleRouteActorDestroyed);
	CurrentSplineDistance=0.f; CompletedLaps=0; bAdvancePending=false;
	NavigationStatus=EElderNavigationStatus::Moving;
	return IssueCurrentGoal();
}
void UElderNavigationComponent::UnbindController()
{
	if (BoundController.IsValid())
	{
		BoundController->GetPathFollowingComponent()->SetStopMovementOnFinish(true);
		BoundController->ReceiveMoveCompleted.RemoveDynamic(this,&UElderNavigationComponent::HandleMoveCompleted);
	}
	BoundController.Reset(); ActiveRequest=FAIRequestID::InvalidRequest;
}
void UElderNavigationComponent::ClearActiveReferences()
{
	if (IsValid(ActiveTarget)) ActiveTarget->OnDestroyed.RemoveDynamic(this,&UElderNavigationComponent::HandleRouteActorDestroyed);
	if (IsValid(ActiveSpline)) ActiveSpline->OnDestroyed.RemoveDynamic(this,&UElderNavigationComponent::HandleRouteActorDestroyed);
	ActiveTarget=nullptr; ActiveSpline=nullptr; ActiveMode=EElderNavigationMode::None;
}
void UElderNavigationComponent::StopNavigation()
{
	const bool bHadRoute=HasManagedNavigation();
	AElderlyAIController* AI=BoundController.Get();
	NavigationStatus=EElderNavigationStatus::Idle; bResumeOnAIControl=false; bAdvancePending=false;
	// Explicit stop wins over a BeginPlay/possession auto-start already queued this frame.
	bAutoStartAttempted=true;
	GetWorld()->GetTimerManager().ClearTimer(ReadyTimer);
	GetWorld()->GetTimerManager().ClearTimer(AdvanceTimer);
	UnbindController(); ClearActiveReferences();
	if (AI && AI->GetPawn()==GetOwner()) AI->StopMovement();
	if (bHadRoute && Elder() && !Elder()->IsUnderPlayerControl()) Elder()->StopElderMovement();
	NavigationMessage=TEXT("导航已停止");
}
void UElderNavigationComponent::PauseInternal(bool bForControl)
{
	if (NavigationStatus!=EElderNavigationStatus::Moving) return;
	NavigationStatus=EElderNavigationStatus::Paused; bResumeOnAIControl=bForControl;
	GetWorld()->GetTimerManager().ClearTimer(AdvanceTimer);
	AElderlyAIController* AI=BoundController.Get(); UnbindController();
	if (AI && AI->GetPawn()==GetOwner()) AI->StopMovement();
	if (!bForControl && Elder() && !Elder()->IsUnderPlayerControl()) Elder()->StopElderMovement();
	NavigationMessage=bForControl?TEXT("玩家接管，导航暂停"):TEXT("导航暂停；使用ResumeNavigation继续");
}
void UElderNavigationComponent::PauseNavigation() { PauseInternal(false); bResumeOnAIControl=false; }
void UElderNavigationComponent::SuspendForPlayerControl() { PauseInternal(true); }
bool UElderNavigationComponent::ResumeNavigation()
{
	if (NavigationStatus!=EElderNavigationStatus::Paused || !AvailableController()) return false;
	bResumeOnAIControl=false; NavigationStatus=EElderNavigationStatus::Moving;
	if (bAdvancePending) { QueueAdvance(); return true; }
	return IssueCurrentGoal();
}
bool UElderNavigationComponent::IssueCurrentGoal()
{
	AElderlyAIController* AI=AvailableController();
	if (!AI) { PauseInternal(false); return false; }
	FAIMoveRequest Request;
	if (ActiveMode==EElderNavigationMode::TargetActor && IsValid(ActiveTarget)) Request.SetGoalActor(ActiveTarget);
	else if (ActiveMode==EElderNavigationMode::Spline && IsValid(ActiveSpline) && ActiveSpline->Spline->GetNumberOfSplinePoints()>=2 && ActiveSpline->Spline->GetSplineLength()>=1.f)
	{
		CurrentSplineDistance=FMath::Min(CurrentSplineDistance,ActiveSpline->Spline->GetSplineLength());
		Request.SetGoalLocation(ActiveSpline->Spline->GetLocationAtDistanceAlongSpline(CurrentSplineDistance,ESplineCoordinateSpace::World));
	}
	else { Finish(false,TEXT("导航目标或样条已失效")); return false; }
	Request.SetUsePathfinding(true); Request.SetProjectGoalLocation(true); Request.SetAllowPartialPath(false);
	Request.SetAcceptanceRadius(FMath::Max(5.f,AcceptanceRadius));
	Request.SetReachTestIncludesAgentRadius(false); Request.SetReachTestIncludesGoalRadius(false);
	UnbindController(); BoundController=AI;
	AI->ReceiveMoveCompleted.AddUniqueDynamic(this,&UElderNavigationComponent::HandleMoveCompleted);
	bIssuingRequest=true;
	const FPathFollowingRequestResult Result=AI->MoveTo(Request);
	bIssuingRequest=false;
	if (Result.Code==EPathFollowingRequestResult::Failed) { Finish(false,TEXT("无法寻路：检查目标位置与NavMesh覆盖")); return false; }
	NavigationMessage=TEXT("导航中");
	ActiveRequest=Result.MoveId;
	if (Result.Code==EPathFollowingRequestResult::AlreadyAtGoal) QueueAdvance();
	return true;
}
void UElderNavigationComponent::HandleMoveCompleted(FAIRequestID RequestID,EPathFollowingResult::Type Result)
{
	if (bIssuingRequest || NavigationStatus!=EElderNavigationStatus::Moving || RequestID!=ActiveRequest) return;
	ActiveRequest=FAIRequestID::InvalidRequest;
	// DisableMovement can abort path following before the state dispatcher runs.
	// Preserve the same goal for an explicit resume after sitting/falling ends.
	if (Elder() && !Elder()->CanElderMove()) { PauseInternal(false); return; }
	// A real completed move can continue in the same frame without losing velocity.
	// AlreadyAtGoal still uses QueueAdvance in IssueCurrentGoal to bound recursion.
	if (Result==EPathFollowingResult::Success) AdvanceSpline();
	else Finish(false,TEXT("导航被中断或路径不可达"));
}
void UElderNavigationComponent::QueueAdvance()
{
	bAdvancePending=true;
	GetWorld()->GetTimerManager().ClearTimer(AdvanceTimer);
	// Defer even AlreadyAtGoal: duplicate points/loops must not recurse on the call stack.
	AdvanceTimer=GetWorld()->GetTimerManager().SetTimerForNextTick(this,&UElderNavigationComponent::AdvanceSpline);
}
void UElderNavigationComponent::AdvanceSpline()
{
	if (NavigationStatus!=EElderNavigationStatus::Moving) return;
	bAdvancePending=false;
	if (ActiveMode==EElderNavigationMode::TargetActor) { Finish(true,TEXT("已到达指定目标")); return; }
	if (!IsValid(ActiveSpline)) { Finish(false,TEXT("样条路线已失效")); return; }
	const float Length=ActiveSpline->Spline->GetSplineLength();
	if (Length<1.f) { Finish(false,TEXT("样条长度无效")); return; }
	if (CurrentSplineDistance>=Length-KINDA_SMALL_NUMBER)
	{
		++CompletedLaps;
		if (!bLoopSpline) { Finish(true,TEXT("样条路线已完成")); return; }
		// A closed spline's Length and zero are the same point: continue directly
		// to its next sample instead of spending an idle frame on the duplicate.
		CurrentSplineDistance=ActiveSpline->Spline->IsClosedLoop()?FMath::Min(Length,FMath::Max(25.f,SplineSampleSpacing)):0.f;
	}
	else CurrentSplineDistance=FMath::Min(Length,CurrentSplineDistance+FMath::Max(25.f,SplineSampleSpacing));
	IssueCurrentGoal();
}
void UElderNavigationComponent::Finish(bool bSuccess,const FString& Message)
{
	GetWorld()->GetTimerManager().ClearTimer(AdvanceTimer);
	NavigationStatus=bSuccess?EElderNavigationStatus::Completed:EElderNavigationStatus::Failed;
	bResumeOnAIControl=false; bAdvancePending=false;
	UnbindController(); ClearActiveReferences(); NavigationMessage=Message;
	// Terminal/failure must stop even when no path request remains between samples.
	if (Elder() && !Elder()->IsUnderPlayerControl()) Elder()->StopElderMovement();
	OnNavigationFinished.Broadcast(bSuccess,NavigationMessage);
}
void UElderNavigationComponent::HandleRouteActorDestroyed(AActor* Actor)
{
	if (!HasManagedNavigation()) return;
	AElderlyAIController* AI=BoundController.Get();
	UnbindController();
	if (AI && AI->GetPawn()==GetOwner()) AI->StopMovement();
	Finish(false,TEXT("指定的导航目标/路线已被删除"));
}
void UElderNavigationComponent::HandleElderStateChanged(AActor* Actor,const FString& Name,EElderState Previous,EElderState NewState)
{
	if (Elder() && !Elder()->CanElderMove()) PauseInternal(false);
}
