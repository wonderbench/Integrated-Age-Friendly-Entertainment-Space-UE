#include "ElderAnimInstance.h"
#include "ElderlyAICharacter.h"
#include "AnimNodes/AnimNode_BlendSpacePlayer.h"
#include "UObject/UnrealType.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimSequence.h"

class FElderAnimInstanceProxy final : public FAnimInstanceProxy
{
public:
	explicit FElderAnimInstanceProxy(UAnimInstance* Instance) : FAnimInstanceProxy(Instance) {}
	virtual void PostUpdate(UAnimInstance* Instance) const override
	{
		FAnimInstanceProxy::PostUpdate(Instance);
		CastChecked<UElderAnimInstance>(Instance)->ReportTurnAfterAnimationUpdate();
	}
};

FAnimInstanceProxy* UElderAnimInstance::CreateAnimInstanceProxy() { return new FElderAnimInstanceProxy(this); }
void UElderAnimInstance::DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy) { delete InProxy; }

void UElderAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	const AElderlyAICharacter* Elder=Cast<AElderlyAICharacter>(TryGetPawnOwner());
	const bool bWheelchair=Elder && (Elder->CurrentState==EElderState::WheelchairIdle || Elder->CurrentState==EElderState::WheelchairMoving);
	// Keep a valid outgoing sequence throughout the blend back to locomotion.
	WheelchairPoseAnimation=Elder ?
		(Elder->CurrentState==EElderState::WheelchairMoving && Elder->WheelchairMoveAnimation ? Elder->WheelchairMoveAnimation : Elder->WheelchairIdleAnimation) : nullptr;
	WheelchairPoseActive=bWheelchair && WheelchairPoseAnimation!=nullptr;
	// Keep the sequence valid during blend-out. StartPosition is consumed only
	// on player initialization: direct Fallen enters at the end; Falling at zero.
	FallPoseAnimation=Elder ? Elder->FallAnimation : nullptr;
	FallPoseActive=Elder && FallPoseAnimation && (Elder->CurrentState==EElderState::Falling || Elder->CurrentState==EElderState::Fallen || Elder->CurrentState==EElderState::GettingUp);
	FallPoseStartPosition=Elder && Elder->CurrentState!=EElderState::Falling && FallPoseAnimation ? FallPoseAnimation->GetPlayLength() : 0.f;
	GetUpPoseAnimation=Elder ? Elder->GetUpAnimation : nullptr;
	GetUpPoseActive=Elder && GetUpPoseAnimation && Elder->CurrentState==EElderState::GettingUp;
	if (GetUpPoseActive)
	{
		// The graph evaluates this explicit clock, so every separate get-up
		// activation begins at frame zero even when the node ran before.
		GetUpPoseTime=bGetUpPoseWasActive
			? FMath::Min(GetUpPoseTime+DeltaSeconds, GetUpPoseAnimation->GetPlayLength())
			: 0.f;
	}
	// Preserve the outgoing pose while the graph blends back to locomotion.
	// The next activation resets the clock in the branch above.
	bGetUpPoseWasActive=GetUpPoseActive;
	// Faster cadence with a shorter blended stride for ordinary walking.
	// Raw local velocity stays in ABP_AI; TIP, idle and seated players are untouched.
	LocomotionPlayRate=Elder?FMath::Max(0.f,Elder->DesiredMoveSpeed)/FMath::Max(1.f,LocomotionReferenceSpeed):1.f;
	if (Elder && !bWheelchair) LocomotionPlayRate *= FMath::Clamp(Elder->WalkingAnimationRateScale, 0.1f, 3.f);
	const FVector LocalVelocity=Elder?Elder->GetActorTransform().InverseTransformVectorNoScale(Elder->GetVelocity()):FVector::ZeroVector;
	LocomotionBlendRight=LocomotionPlayRate>KINDA_SMALL_NUMBER?LocalVelocity.Y/LocomotionPlayRate:0.f;
	LocomotionBlendForward=LocomotionPlayRate>KINDA_SMALL_NUMBER?LocalVelocity.X/LocomotionPlayRate:0.f;
	RootTurnActive=Elder && Elder->ShouldPlayTurnAnimation();
	if (Elder)
	{
		RootTurnCoordinate=Elder->TurnBlendPosition;
		RootTurnPlayRate=Elder->GetActiveTurnPlayRate();
	}
	if (!RootTurnActive && Elder)
	{
		// An unvisited player retains its last cached weight. Clear the observation
		// before graph update: if the outgoing child is still evaluated, UE writes
		// this frame's actual weight; otherwise post-update observes zero.
		for (TFieldIterator<FStructProperty> It(GetClass()); It; ++It)
		{
			if (It->Struct!=FAnimNode_BlendSpacePlayer::StaticStruct()) continue;
			auto* Player=It->ContainerPtrToValuePtr<FAnimNode_BlendSpacePlayer>(this);
			if (Player->GetBlendSpace()==Elder->TurnInPlaceBlendSpace) Player->ClearCachedBlendWeight();
		}
	}
}

void UElderAnimInstance::ReportTurnAfterAnimationUpdate()
{
	AElderlyAICharacter* Elder=Cast<AElderlyAICharacter>(TryGetPawnOwner());
	if (!Elder) return;
	// Proxy PostUpdate is on the game thread after parallel update completes,
	// even when bone evaluation is skipped because the mesh is not rendered.
	// Read the actual player's normalized clock; never drive it backwards from C++.
	for (TFieldIterator<FStructProperty> It(GetClass()); It; ++It)
	{
		if (It->Struct!=FAnimNode_BlendSpacePlayer::StaticStruct()) continue;
		const auto* Player=It->ContainerPtrToValuePtr<FAnimNode_BlendSpacePlayer>(this);
		if (Player->GetBlendSpace()==Elder->TurnInPlaceBlendSpace)
		{
			// A false selector is insufficient while the outgoing player still has
			// blend weight. Only a fully inactive child can be reinitialized on entry.
			if (!RootTurnActive && Player->GetCachedBlendWeight()<=KINDA_SMALL_NUMBER)
				Elder->ReportTurnGraphInactive();
			else if (RootTurnActive && Elder->ShouldPlayTurnAnimation())
				Elder->ReportTurnPlaybackProgress(Player->GetAccumulatedTime());
			break;
		}
	}
}
