#include "OrbitCameraCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "Components/BoxComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Engine/TriggerBox.h"
#include "EngineUtils.h"
#include "GameFramework/SpringArmComponent.h"
#include "HAL/IConsoleManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

#if !UE_BUILD_SHIPPING
namespace
{
	void RunOrbitCameraSelfTest()
	{
		AOrbitCameraCharacter* OrbitCharacter = nullptr;
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			UWorld* World = Context.World();
			if (!World || !World->IsGameWorld())
			{
				continue;
			}

			for (TActorIterator<AOrbitCameraCharacter> It(World); It; ++It)
			{
				OrbitCharacter = *It;
				break;
			}
			if (OrbitCharacter)
			{
				break;
			}
		}

		if (!OrbitCharacter)
		{
			UE_LOG(LogTemp, Error, TEXT("OrbitCameraSelfTest: no orbit camera character in a game world."));
			return;
		}

		const FVector StartLocation = OrbitCharacter->Camera->GetComponentLocation();
		OrbitCharacter->AddOrbitInput(FVector2D(180.0f, 10000.0f));
		OrbitCharacter->AddOrbitZoomInput(10000.0f);
		const float MinTargetDistance = OrbitCharacter->TargetOrbitDistance;
		OrbitCharacter->UpdateOrbitCamera(1.0f);
		OrbitCharacter->SpringArm->TickComponent(1.0f, LEVELTICK_All, nullptr);
		const FVector MinPitchLocation = OrbitCharacter->Camera->GetComponentLocation();
		const float MinPitch = OrbitCharacter->CurrentOrbitPitch;
		const float MinDistance = FVector::Distance(MinPitchLocation, OrbitCharacter->GetActiveOrbitPivotLocation());

		OrbitCharacter->AddOrbitInput(FVector2D(0.0f, -10000.0f));
		OrbitCharacter->AddOrbitZoomInput(-10000.0f);
		const float MaxTargetDistance = OrbitCharacter->TargetOrbitDistance;
		OrbitCharacter->UpdateOrbitCamera(1.0f);
		OrbitCharacter->SpringArm->TickComponent(1.0f, LEVELTICK_All, nullptr);
		const FVector MaxPitchLocation = OrbitCharacter->Camera->GetComponentLocation();
		const float MaxPitch = OrbitCharacter->CurrentOrbitPitch;
		const float MaxDistance = FVector::Distance(MaxPitchLocation, OrbitCharacter->GetActiveOrbitPivotLocation());

		UE_LOG(LogTemp, Display,
			TEXT("OrbitCameraSelfTest: LocationMoved=%s Start=%s MinPitchLocation=%s MaxPitchLocation=%s MinPitch=%0.2f MaxPitch=%0.2f MinTargetDistance=%0.2f MaxTargetDistance=%0.2f MinDistance=%0.2f MaxDistance=%0.2f TargetPitchRange=[%0.2f,%0.2f] TargetDistanceRange=[%0.2f,%0.2f]"),
			StartLocation.Equals(MinPitchLocation, 1.0f) ? TEXT("false") : TEXT("true"),
			*StartLocation.ToCompactString(), *MinPitchLocation.ToCompactString(), *MaxPitchLocation.ToCompactString(),
			MinPitch, MaxPitch, MinTargetDistance, MaxTargetDistance, MinDistance, MaxDistance,
			OrbitCharacter->MinOrbitPitch, OrbitCharacter->MaxOrbitPitch,
			OrbitCharacter->MinOrbitDistance, OrbitCharacter->MaxOrbitDistance);

		if (FParse::Param(FCommandLine::Get(), TEXT("OrbitCameraSelfTest")))
		{
			FPlatformMisc::RequestExit(false);
		}
	}

	FAutoConsoleCommand OrbitCameraSelfTestCommand(
		TEXT("OrbitCamera.SelfTest"),
		TEXT("Runs a non-shipping orbit camera movement and clamp self-test in the active game world."),
		FConsoleCommandDelegate::CreateStatic(&RunOrbitCameraSelfTest));
}
#endif

AOrbitCameraCharacter::AOrbitCameraCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	// APlayerController uses this component when this Character is its view target.
	bFindCameraComponentWhenViewTarget = true;

	CameraOrbitPivot = CreateDefaultSubobject<USceneComponent>(TEXT("CameraOrbitPivot"));
	CameraOrbitPivot->SetupAttachment(GetRootComponent());
	CameraOrbitPivot->SetRelativeRotation(FRotator(-30.0f, 0.0f, 0.0f));

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(CameraOrbitPivot);
	SpringArm->TargetArmLength = OrbitDistance;
	SpringArm->bUsePawnControlRotation = false;
	// The arm must inherit the pivot's transform.  Disabling pitch/yaw here
	// leaves the arm fixed in world space, which turns this into a look camera
	// instead of a real orbit camera.
	SpringArm->bInheritPitch = true;
	SpringArm->bInheritYaw = true;
	SpringArm->bInheritRoll = false;
	SpringArm->bDoCollisionTest = false;
	SpringArm->bEnableCameraLag = false;
	SpringArm->bEnableCameraRotationLag = false;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = false;
	Camera->SetActive(true);
}

void AOrbitCameraCharacter::BeginPlay()
{
	const float AuthoredOrbitDistance = OrbitDistance;
	Super::BeginPlay();
	for (TActorIterator<ATriggerBox> It(GetWorld()); It; ++It)
	{
		if (It->ActorHasTag(TEXT("OrbitCameraBounds")))
		{
			if (const UBoxComponent* Box = Cast<UBoxComponent>(It->GetCollisionComponent()))
			{
				OrbitCameraBounds = Box->Bounds.GetBox();
				break;
			}
		}
	}
	if (OrbitCameraBounds.IsValid && !bUseActorLocationAsPivot)
	{
		OrbitDistance = AuthoredOrbitDistance;
	}
	InitializeOrbitCamera();

#if !UE_BUILD_SHIPPING
	if (FParse::Param(FCommandLine::Get(), TEXT("OrbitCameraSelfTest")))
	{
		GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateLambda([]()
		{
			RunOrbitCameraSelfTest();
		}));
	}
#endif
}

void AOrbitCameraCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateOrbitCamera(DeltaSeconds);

	if (bDebugOrbitPivot)
	{
		const FVector Pivot = GetActiveOrbitPivotLocation();
		DrawDebugSphere(GetWorld(), Pivot, 40.0f, 16, FColor::Cyan, false, 0.0f, 0, 2.0f);
		DrawDebugLine(GetWorld(), Pivot, Camera->GetComponentLocation(), FColor::Cyan, false, 0.0f, 0, 1.0f);
	}
}

void AOrbitCameraCharacter::InitializeOrbitCamera()
{
	Camera->Activate(true);

	const FRotator InitialRotation = CameraOrbitPivot->GetComponentRotation();
	const FVector InitialCameraLocation = GetActorLocation();
	CurrentOrbitYaw = InitialRotation.Yaw;
	TargetOrbitYaw = CurrentOrbitYaw;
	CurrentOrbitPitch = FMath::Clamp(InitialRotation.Pitch, MinOrbitPitch, MaxOrbitPitch);
	TargetOrbitPitch = CurrentOrbitPitch;

	if (!OrbitCameraBounds.IsValid && SpringArm->TargetArmLength > KINDA_SMALL_NUMBER)
	{
		OrbitDistance = SpringArm->TargetArmLength;
	}
	OrbitDistance = FMath::Clamp(OrbitDistance, MinOrbitDistance, MaxOrbitDistance);
	CurrentOrbitDistance = OrbitDistance;
	TargetOrbitDistance = OrbitDistance;

	if (bUseActorLocationAsPivot)
	{
		if (!bOrbitInitialPoseApplied)
		{
			// Keep the spawn transform as the camera position, not the pivot. A
			// spring arm places its camera behind the pivot, so move the Actor to
			// the matching centre once and retain the current visible start view.
			const FVector InitialPivotLocation = InitialCameraLocation + InitialRotation.Vector() * CurrentOrbitDistance;
			SetActorLocation(InitialPivotLocation, false, nullptr, ETeleportType::TeleportPhysics);
			bOrbitInitialPoseApplied = true;
		}
		OrbitPivotLocation = GetActorLocation();
	}

	CameraOrbitPivot->SetWorldLocation(GetActiveOrbitPivotLocation());
	CameraOrbitPivot->SetWorldRotation(FRotator(CurrentOrbitPitch, CurrentOrbitYaw, 0.0f));
	SpringArm->TargetArmLength = GetBoundedOrbitDistance(CurrentOrbitDistance, CameraOrbitPivot->GetComponentRotation());
}

void AOrbitCameraCharacter::AddOrbitInput(FVector2D LookInput)
{
	if (!bOrbitCameraEnabled)
	{
		return;
	}

	TargetOrbitYaw += LookInput.X * OrbitYawSpeed;
	TargetOrbitPitch = FMath::Clamp(TargetOrbitPitch - LookInput.Y * OrbitPitchSpeed, MinOrbitPitch, MaxOrbitPitch);
}

void AOrbitCameraCharacter::AddOrbitZoomInput(float ZoomInput)
{
	if (!bOrbitCameraEnabled || FMath::IsNearlyZero(ZoomInput))
	{
		return;
	}

	TargetOrbitDistance = FMath::Clamp(TargetOrbitDistance - ZoomInput * OrbitZoomSpeed, MinOrbitDistance, MaxOrbitDistance);
}

void AOrbitCameraCharacter::SetOrbitPivotLocation(FVector NewPivotLocation)
{
	bUseActorLocationAsPivot = false;
	OrbitPivotLocation = NewPivotLocation;
	CameraOrbitPivot->SetWorldLocation(GetActiveOrbitPivotLocation());
	SpringArm->TargetArmLength = GetBoundedOrbitDistance(CurrentOrbitDistance, CameraOrbitPivot->GetComponentRotation());
}

void AOrbitCameraCharacter::SetOrbitEnabled(bool bEnabled)
{
	bOrbitCameraEnabled = bEnabled;
}

void AOrbitCameraCharacter::UpdateOrbitCamera(float DeltaSeconds)
{
	if (bUseActorLocationAsPivot)
	{
		OrbitPivotLocation = GetActorLocation();
	}

	TargetOrbitPitch = FMath::Clamp(TargetOrbitPitch, MinOrbitPitch, MaxOrbitPitch);
	TargetOrbitDistance = FMath::Clamp(TargetOrbitDistance, MinOrbitDistance, MaxOrbitDistance);
	CurrentOrbitYaw = FMath::FInterpTo(CurrentOrbitYaw, TargetOrbitYaw, DeltaSeconds, OrbitInterpSpeed);
	CurrentOrbitPitch = FMath::FInterpTo(CurrentOrbitPitch, TargetOrbitPitch, DeltaSeconds, OrbitInterpSpeed);
	CurrentOrbitDistance = FMath::FInterpTo(CurrentOrbitDistance, TargetOrbitDistance, DeltaSeconds, OrbitZoomInterpSpeed);

	CameraOrbitPivot->SetWorldLocation(GetActiveOrbitPivotLocation());
	CameraOrbitPivot->SetWorldRotation(FRotator(CurrentOrbitPitch, CurrentOrbitYaw, 0.0f));
	SpringArm->TargetArmLength = GetBoundedOrbitDistance(CurrentOrbitDistance, CameraOrbitPivot->GetComponentRotation());
}

float AOrbitCameraCharacter::GetBoundedOrbitDistance(float DesiredDistance, const FRotator& OrbitRotation) const
{
	if (!OrbitCameraBounds.IsValid) return DesiredDistance;
	const FVector Pivot = GetActiveOrbitPivotLocation();
	const FVector Direction = -OrbitRotation.Vector();
	double Limit = DesiredDistance;
	if (Direction.X > KINDA_SMALL_NUMBER) Limit = FMath::Min(Limit, (OrbitCameraBounds.Max.X - Pivot.X) / Direction.X);
	else if (Direction.X < -KINDA_SMALL_NUMBER) Limit = FMath::Min(Limit, (OrbitCameraBounds.Min.X - Pivot.X) / Direction.X);
	if (Direction.Y > KINDA_SMALL_NUMBER) Limit = FMath::Min(Limit, (OrbitCameraBounds.Max.Y - Pivot.Y) / Direction.Y);
	else if (Direction.Y < -KINDA_SMALL_NUMBER) Limit = FMath::Min(Limit, (OrbitCameraBounds.Min.Y - Pivot.Y) / Direction.Y);
	return static_cast<float>(FMath::Max(0.0, Limit));
}

FVector AOrbitCameraCharacter::GetActiveOrbitPivotLocation() const
{
	FVector Pivot = bUseActorLocationAsPivot ? GetActorLocation() : OrbitPivotLocation;
	if (OrbitCameraBounds.IsValid)
	{
		const double MarginX = FMath::Min(100.0, (OrbitCameraBounds.Max.X - OrbitCameraBounds.Min.X) * 0.25);
		const double MarginY = FMath::Min(100.0, (OrbitCameraBounds.Max.Y - OrbitCameraBounds.Min.Y) * 0.25);
		Pivot.X = FMath::Clamp(Pivot.X, OrbitCameraBounds.Min.X + MarginX, OrbitCameraBounds.Max.X - MarginX);
		Pivot.Y = FMath::Clamp(Pivot.Y, OrbitCameraBounds.Min.Y + MarginY, OrbitCameraBounds.Max.Y - MarginY);
	}
	return Pivot;
}
