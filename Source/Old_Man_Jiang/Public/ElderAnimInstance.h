#pragma once
#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "ElderAnimInstance.generated.h"

/** Keeps ABP_AI's existing locomotion graph; supplies game-thread turn snapshots. */
UCLASS(Blueprintable)
class OLD_MAN_JIANG_API UElderAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
public:
	/** Read-only animation projection; EElderState on the character remains authoritative. */
	UPROPERTY(BlueprintReadOnly, Transient, Category="Elder|Wheelchair") bool WheelchairPoseActive=false;
	UPROPERTY(BlueprintReadOnly, Transient, Category="Elder|Wheelchair") TObjectPtr<UAnimSequence> WheelchairPoseAnimation;
	/** One non-looping graph player stays active through Falling and Fallen. */
	UPROPERTY(BlueprintReadOnly, Transient, Category="Elder|Fall") bool FallPoseActive=false;
	UPROPERTY(BlueprintReadOnly, Transient, Category="Elder|Fall") TObjectPtr<UAnimSequence> FallPoseAnimation;
	UPROPERTY(BlueprintReadOnly, Transient, Category="Elder|Fall") float FallPoseStartPosition=0.f;
	/** Non-looping recovery projection. CurrentState remains the authority. */
	UPROPERTY(BlueprintReadOnly, Transient, Category="Elder|Fall") bool GetUpPoseActive=false;
	UPROPERTY(BlueprintReadOnly, Transient, Category="Elder|Fall") TObjectPtr<UAnimSequence> GetUpPoseAnimation;
	UPROPERTY(BlueprintReadOnly, Transient, Category="Elder|Fall") float GetUpPoseTime=0.f;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
	virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* InProxy) override;
	void ReportTurnAfterAnimationUpdate();
	/** Original 160 cm/s gait calibration; adjust after replacing the locomotion assets. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Elder|Locomotion", meta=(ClampMin="1")) float LocomotionReferenceSpeed=160.f;
	UPROPERTY(BlueprintReadOnly, Transient, Category="Elder|Locomotion") float LocomotionPlayRate=1.f;
	UPROPERTY(BlueprintReadOnly, Transient, Category="Elder|Locomotion") float LocomotionBlendRight=0.f;
	UPROPERTY(BlueprintReadOnly, Transient, Category="Elder|Locomotion") float LocomotionBlendForward=0.f;
	UPROPERTY(BlueprintReadOnly, Transient, Category="Elder|Turning") bool RootTurnActive=false;
	UPROPERTY(BlueprintReadOnly, Transient, Category="Elder|Turning") float RootTurnCoordinate=50.f;
	UPROPERTY(BlueprintReadOnly, Transient, Category="Elder|Turning") float RootTurnPlayRate=1.f;
private:
	bool bGetUpPoseWasActive=false;
};
