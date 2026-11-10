#include "CameraScanVisualComponent.h"

#include "Components/StaticMeshComponent.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"

UCameraScanVisualComponent::UCameraScanVisualComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCameraScanVisualComponent::BeginPlay()
{
	Super::BeginPlay();

	ScanMesh = ResolveStaticMeshComponent(ScanMeshComponentName);
	UStaticMeshComponent* CameraBody = ResolveStaticMeshComponent(CameraBodyComponentName);
	if (!ScanMesh)
	{
		return;
	}

	// The artist-authored relative transform stays intact; only its parent changes at runtime.
	if (bFollowCameraBody && CameraBody && CameraBody != ScanMesh)
	{
		ScanMesh->AttachToComponent(CameraBody, FAttachmentTransformRules::KeepRelativeTransform);
	}

	ScanMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ScanMesh->SetCastShadow(false);
	ScanMesh->SetVisibility(bShowScanVolume, true);
	DynamicMaterial = ScanMesh->CreateDynamicMaterialInstance(0);
	ApplyMaterialParameters();
}

void UCameraScanVisualComponent::RefreshScanVisual()
{
	if (!ScanMesh)
	{
		ScanMesh = ResolveStaticMeshComponent(ScanMeshComponentName);
	}

	if (ScanMesh && !DynamicMaterial)
	{
		DynamicMaterial = ScanMesh->CreateDynamicMaterialInstance(0);
	}

	if (ScanMesh)
	{
		ScanMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ScanMesh->SetCastShadow(false);
		ScanMesh->SetVisibility(bShowScanVolume, true);
	}

	ApplyMaterialParameters();
}

void UCameraScanVisualComponent::SetScanVolumeVisible(const bool bVisible)
{
	bShowScanVolume = bVisible;
	if (ScanMesh)
	{
		ScanMesh->SetVisibility(bShowScanVolume, true);
	}
}

UStaticMeshComponent* UCameraScanVisualComponent::ResolveStaticMeshComponent(const FName ComponentName) const
{
	if (!GetOwner())
	{
		return nullptr;
	}

	TInlineComponentArray<UStaticMeshComponent*> Components;
	GetOwner()->GetComponents(Components);
	for (UStaticMeshComponent* Component : Components)
	{
		if (Component && Component->GetFName() == ComponentName)
		{
			return Component;
		}
	}

	return nullptr;
}

void UCameraScanVisualComponent::ApplyMaterialParameters()
{
	if (!DynamicMaterial)
	{
		return;
	}

	DynamicMaterial->SetVectorParameterValue(TEXT("ScanColor"), ScanColor);
	DynamicMaterial->SetScalarParameterValue(TEXT("BaseOpacity"), BaseOpacity);
	DynamicMaterial->SetScalarParameterValue(TEXT("EdgeIntensity"), EdgeIntensity);
	DynamicMaterial->SetScalarParameterValue(TEXT("SweepSpeed"), SweepSpeed);
	DynamicMaterial->SetScalarParameterValue(TEXT("SweepWidth"), SweepWidth);
	DynamicMaterial->SetScalarParameterValue(TEXT("SweepIntensity"), bAnimateScan ? SweepIntensity : 0.0f);
	DynamicMaterial->SetScalarParameterValue(TEXT("UseDepthFade"), bUseDepthFade ? 1.0f : 0.0f);
	DynamicMaterial->SetScalarParameterValue(TEXT("DepthFadeDistance"), DepthFadeDistance);
}
