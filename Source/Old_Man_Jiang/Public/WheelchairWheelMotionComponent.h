#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WheelchairWheelMotionComponent.generated.h"

class USceneComponent;

USTRUCT(BlueprintType)
struct FWheelchairWheelConfig
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wheel") FName PivotName;
	/** Zero measures the meshes below the pivot. Positive values are unscaled cm. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wheel", meta=(ClampMin="0")) float RadiusOverrideCm = 0.f;
};

/** Visual rolling for BP_WheelChair's existing X-axis wheel pivots. Does not move the chair. */
UCLASS(ClassGroup=(Elder), meta=(BlueprintSpawnableComponent))
class OLD_MAN_JIANG_API UWheelchairWheelMotionComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UWheelchairWheelMotionComponent();
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Wheelchair|Wheels") TArray<FWheelchairWheelConfig> Wheels;
	/** Large discontinuities are relocation, not travelled distance. Call ResetWheelMotion after smaller teleports. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Wheelchair|Wheels", meta=(ClampMin="1")) float TeleportDistanceCm = 200.f;
	UFUNCTION(BlueprintCallable, Category="Wheelchair|Wheels") void SetWheelMotionEnabled(bool bEnabled);
	UFUNCTION(BlueprintCallable, Category="Wheelchair|Wheels") void ResetWheelMotion(bool bResetRotation = false);
	UFUNCTION(BlueprintPure, Category="Wheelchair|Wheels") float GetWheelRadiusCm(FName PivotName) const;
	UFUNCTION(BlueprintPure, Category="Wheelchair|Wheels") float GetWheelRollDegrees(FName PivotName) const;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
protected:
	virtual void BeginPlay() override;
private:
	struct FWheelRuntime
	{
		TWeakObjectPtr<USceneComponent> Pivot;
		TWeakObjectPtr<USceneComponent> Parent;
		FQuat RestRotation = FQuat::Identity;
		FVector PreviousLocation = FVector::ZeroVector;
		FVector PreviousTangent = FVector::ZeroVector;
		float RadiusCm = 0.f;
		float RollDegrees = 0.f;
	};
	TArray<FWheelRuntime> RuntimeWheels;
	FTransform PreviousOwnerTransform;
	bool bMotionEnabled = true;
	bool bHavePreviousSample = false;
	void SampleWheels();
	FVector GetRollingTangent(const FWheelRuntime& Wheel) const;
};
