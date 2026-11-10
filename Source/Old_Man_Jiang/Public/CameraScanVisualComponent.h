#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CameraScanVisualComponent.generated.h"

class UMaterialInstanceDynamic;
class UStaticMeshComponent;

/**
 * Visual-only material driver for BP_Camera's existing scan-volume mesh.
 * It contains no target queries, traces, AI, or gameplay detection state.
 */
UCLASS(ClassGroup = (Security), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class OLD_MAN_JIANG_API UCameraScanVisualComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCameraScanVisualComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scan Volume")
	bool bShowScanVolume = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scan Volume")
	FLinearColor ScanColor = FLinearColor(0.04f, 0.35f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scan Volume", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BaseOpacity = 0.28f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scan Volume", meta = (ClampMin = "0.0"))
	float EdgeIntensity = 1.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scan Volume|Animation")
	bool bAnimateScan = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scan Volume|Animation", meta = (ClampMin = "0.0"))
	float SweepSpeed = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scan Volume|Animation", meta = (ClampMin = "0.1"))
	float SweepWidth = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scan Volume|Animation", meta = (ClampMin = "0.0"))
	float SweepIntensity = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scan Volume|Depth Fade")
	bool bUseDepthFade = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scan Volume|Depth Fade", meta = (ClampMin = "1.0"))
	float DepthFadeDistance = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scan Volume|Attachment")
	bool bFollowCameraBody = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scan Volume|Attachment")
	FName CameraBodyComponentName = TEXT("StaticMesh");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scan Volume|Attachment")
	FName ScanMeshComponentName = TEXT("Cone_Brush_StaticMesh");

	UFUNCTION(BlueprintCallable, Category = "Scan Volume")
	void RefreshScanVisual();

	UFUNCTION(BlueprintCallable, Category = "Scan Volume")
	void SetScanVolumeVisible(bool bVisible);

protected:
	virtual void BeginPlay() override;

private:
	UStaticMeshComponent* ResolveStaticMeshComponent(FName ComponentName) const;
	void ApplyMaterialParameters();

	TObjectPtr<UStaticMeshComponent> ScanMesh = nullptr;
	TObjectPtr<UMaterialInstanceDynamic> DynamicMaterial = nullptr;
};
