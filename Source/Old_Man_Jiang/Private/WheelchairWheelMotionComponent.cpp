#include "WheelchairWheelMotionComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Actor.h"

UWheelchairWheelMotionComponent::UWheelchairWheelMotionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
	for (FName Name : {FName(TEXT("L_BigWheel")), FName(TEXT("R_BigWheel")), FName(TEXT("L_LittleWheel")), FName(TEXT("R_LittleWheel"))})
	{
		FWheelchairWheelConfig& Wheel = Wheels.AddDefaulted_GetRef();
		Wheel.PivotName = Name;
	}
}

void UWheelchairWheelMotionComponent::BeginPlay()
{
	Super::BeginPlay();
	TInlineComponentArray<USceneComponent*> Components(GetOwner());
	TSet<FName> Seen;
	for (const FWheelchairWheelConfig& Config : Wheels)
	{
		if (Config.PivotName.IsNone() || Seen.Contains(Config.PivotName)) continue;
		Seen.Add(Config.PivotName);
		USceneComponent* Pivot = nullptr;
		for (USceneComponent* Component : Components)
			if (Component->GetFName() == Config.PivotName) { Pivot = Component; break; }
		if (!Pivot) { UE_LOG(LogTemp, Warning, TEXT("Wheelchair %s: missing wheel pivot %s"), *GetOwner()->GetName(), *Config.PivotName.ToString()); continue; }
		// Measure in the pivot's unscaled space; current model axle is local X.
		FBox Bounds(ForceInit);
		TArray<USceneComponent*> Children;
		Pivot->GetChildrenComponents(true, Children);
		for (USceneComponent* Child : Children)
			if (UStaticMeshComponent* Mesh = Cast<UStaticMeshComponent>(Child); Mesh && Mesh->GetStaticMesh())
			{
				const FTransform Relative = Mesh->GetComponentTransform().GetRelativeTransform(Pivot->GetComponentTransform());
				Bounds += Mesh->GetStaticMesh()->GetBoundingBox().TransformBy(Relative);
			}
		const float Radius = Config.RadiusOverrideCm > 0.f ? Config.RadiusOverrideCm : (Bounds.IsValid ? FMath::Max(Bounds.GetExtent().Y, Bounds.GetExtent().Z) : 0.f);
		if (!FMath::IsFinite(Radius) || Radius <= UE_SMALL_NUMBER)
		{
			UE_LOG(LogTemp, Warning, TEXT("Wheelchair %s: no valid radius for %s"), *GetOwner()->GetName(), *Config.PivotName.ToString());
			continue;
		}
		FWheelRuntime& Wheel = RuntimeWheels.AddDefaulted_GetRef();
		Wheel.Pivot = Pivot;
		Wheel.Parent = Pivot->GetAttachParent();
		Wheel.RestRotation = Pivot->GetRelativeRotation().Quaternion();
		Wheel.RadiusCm = Radius;
	}
	ResetWheelMotion();
}

FVector UWheelchairWheelMotionComponent::GetRollingTangent(const FWheelRuntime& Wheel) const
{
	// Positive rotation about +X rolls toward -Y. Exclude the accumulated spin.
	FQuat RestWorld = Wheel.RestRotation;
	if (const USceneComponent* Parent = Wheel.Pivot->GetAttachParent()) RestWorld = Parent->GetComponentQuat() * RestWorld;
	return RestWorld.RotateVector(FVector(0, -1, 0));
}

void UWheelchairWheelMotionComponent::SampleWheels()
{
	for (FWheelRuntime& Wheel : RuntimeWheels)
		if (Wheel.Pivot.IsValid())
		{
			Wheel.PreviousLocation = Wheel.Pivot->GetComponentLocation();
			Wheel.PreviousTangent = GetRollingTangent(Wheel);
			Wheel.Parent = Wheel.Pivot->GetAttachParent();
		}
	PreviousOwnerTransform = GetOwner()->GetActorTransform();
	bHavePreviousSample = true;
}

void UWheelchairWheelMotionComponent::ResetWheelMotion(bool bResetRotation)
{
	if (bResetRotation)
		for (FWheelRuntime& Wheel : RuntimeWheels)
			if (Wheel.Pivot.IsValid())
			{
				Wheel.RollDegrees = 0.f;
				Wheel.Pivot->SetRelativeRotation(Wheel.RestRotation);
			}
	bHavePreviousSample = false;
}

void UWheelchairWheelMotionComponent::SetWheelMotionEnabled(bool bEnabled)
{
	bMotionEnabled = bEnabled;
	ResetWheelMotion();
}

void UWheelchairWheelMotionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	AActor* Owner = GetOwner();
	if (!Owner) return;
	// Hidden attached chairs follow walking elders too: discard that travel history.
	if (!bMotionEnabled || Owner->IsHidden()) { bHavePreviousSample = false; return; }
	if (!bHavePreviousSample) { SampleWheels(); return; }
	const FTransform Current = Owner->GetActorTransform();
	if (FVector::Dist(Current.GetLocation(), PreviousOwnerTransform.GetLocation()) > FMath::Max(1.f, TeleportDistanceCm)
		|| Current.GetRotation().AngularDistance(PreviousOwnerTransform.GetRotation()) > FMath::DegreesToRadians(120.f)
		|| !Current.GetScale3D().Equals(PreviousOwnerTransform.GetScale3D(), .0001f))
	{
		SampleWheels(); return;
	}
	for (FWheelRuntime& Wheel : RuntimeWheels)
	{
		USceneComponent* Pivot = Wheel.Pivot.Get();
		if (!Pivot || Wheel.Parent.Get() != Pivot->GetAttachParent()) continue;
		const FVector Tangent = GetRollingTangent(Wheel);
		const FVector MidTangent = (Wheel.PreviousTangent + Tangent).GetSafeNormal();
		const FVector Scale = Pivot->GetComponentScale().GetAbs();
		const float Radius = Wheel.RadiusCm * FMath::Max(Scale.Y, Scale.Z);
		if (Radius <= UE_SMALL_NUMBER) continue;
		const double Distance = FVector::DotProduct(Pivot->GetComponentLocation() - Wheel.PreviousLocation, MidTangent);
		Wheel.RollDegrees = FMath::UnwindDegrees(Wheel.RollDegrees + FMath::RadiansToDegrees(Distance / Radius));
		Pivot->SetRelativeRotation(Wheel.RestRotation * FQuat(FVector::ForwardVector, FMath::DegreesToRadians(Wheel.RollDegrees)));
	}
	SampleWheels();
}

float UWheelchairWheelMotionComponent::GetWheelRadiusCm(FName PivotName) const
{
	for (const FWheelRuntime& Wheel : RuntimeWheels) if (Wheel.Pivot.IsValid() && Wheel.Pivot->GetFName() == PivotName) return Wheel.RadiusCm;
	return 0.f;
}

float UWheelchairWheelMotionComponent::GetWheelRollDegrees(FName PivotName) const
{
	for (const FWheelRuntime& Wheel : RuntimeWheels) if (Wheel.Pivot.IsValid() && Wheel.Pivot->GetFName() == PivotName) return Wheel.RollDegrees;
	return 0.f;
}
