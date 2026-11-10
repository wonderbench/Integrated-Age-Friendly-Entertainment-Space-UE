#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "OrbitCameraCharacter.generated.h"

class UCameraComponent;
class USceneComponent;
class USpringArmComponent;

/**
 * Reusable orbit-camera base. BP_Character_Base inherits this class so the
 * existing Blueprint remains the project Pawn and all orbit settings stay
 * instance-editable in its Details panel.
 */
UCLASS(Blueprintable)
class OLD_MAN_JIANG_API AOrbitCameraCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AOrbitCameraCharacter();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera|Orbit|Components")
	TObjectPtr<USceneComponent> CameraOrbitPivot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera|Orbit|Components")
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera|Orbit|Components")
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Orbit|Pivot", meta = (MakeEditWidget = "true"))
	FVector OrbitPivotLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Orbit|Pivot")
	bool bUseActorLocationAsPivot = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Orbit|Rotation")
	float TargetOrbitYaw = 0.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Camera|Orbit|Rotation")
	float CurrentOrbitYaw = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Orbit|Rotation")
	float TargetOrbitPitch = -30.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Camera|Orbit|Rotation")
	float CurrentOrbitPitch = -30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Orbit|Rotation", meta = (ClampMin = "0.01"))
	float OrbitYawSpeed = 0.38f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Orbit|Rotation", meta = (ClampMin = "0.01"))
	float OrbitPitchSpeed = 0.28f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Orbit|Rotation", meta = (ClampMin = "0.01"))
	float MinOrbitPitch = -80.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Orbit|Rotation", meta = (ClampMin = "-89.0", ClampMax = "89.0"))
	float MaxOrbitPitch = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Orbit|Smoothing", meta = (ClampMin = "0.01"))
	// Fast settle: removes the long inertial tail while retaining a small,
	// comfortable amount of interpolation.
	float OrbitInterpSpeed = 18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Orbit|Distance", meta = (ClampMin = "0.0"))
	float OrbitDistance = 2500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Orbit|Distance", meta = (ClampMin = "0.0"))
	float TargetOrbitDistance = 2500.0f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Camera|Orbit|Distance")
	float CurrentOrbitDistance = 2500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Orbit|Distance", meta = (ClampMin = "0.0"))
	float MinOrbitDistance = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Orbit|Distance", meta = (ClampMin = "0.0"))
	float MaxOrbitDistance = 6000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Orbit|Distance", meta = (ClampMin = "0.01"))
	float OrbitZoomSpeed = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Orbit|Smoothing", meta = (ClampMin = "0.01"))
	float OrbitZoomInterpSpeed = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Orbit|Input")
	bool bOrbitCameraEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Orbit|Debug")
	bool bDebugOrbitPivot = false;

	UFUNCTION(BlueprintCallable, Category = "Camera|Orbit")
	void AddOrbitInput(FVector2D LookInput);

	UFUNCTION(BlueprintCallable, Category = "Camera|Orbit")
	void AddOrbitZoomInput(float ZoomInput);

	UFUNCTION(BlueprintCallable, Category = "Camera|Orbit")
	void SetOrbitPivotLocation(FVector NewPivotLocation);

	UFUNCTION(BlueprintCallable, Category = "Camera|Orbit")
	void SetOrbitEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "Camera|Orbit")
	void InitializeOrbitCamera();

	UFUNCTION(BlueprintCallable, Category = "Camera|Orbit")
	void UpdateOrbitCamera(float DeltaSeconds);

	UFUNCTION(BlueprintPure, Category = "Camera|Orbit")
	FVector GetActiveOrbitPivotLocation() const;

private:
	// Optional per-level, non-colliding TriggerBox tagged OrbitCameraBounds.
	// Only the horizontal footprint is used; vertical movement remains free.
	FBox OrbitCameraBounds = FBox(ForceInit);
	float GetBoundedOrbitDistance(float DesiredDistance, const FRotator& OrbitRotation) const;
	// A PlayerStart describes the initial camera pose. It is converted once to
	// the corresponding orbit-centre location during initialization.
	bool bOrbitInitialPoseApplied = false;
};
