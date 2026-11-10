#pragma once

#include "CoreMinimal.h"
#include "ElderTypes.generated.h"

UENUM(BlueprintType)
enum class EElderGender : uint8 { Male UMETA(DisplayName="Male"), Female UMETA(DisplayName="Female") };
UENUM(BlueprintType)
enum class EElderState : uint8 { Standing, Sitting, Walking, Falling, Fallen, WheelchairIdle, WheelchairMoving, GettingUp };
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnElderStateChanged, AActor*, ElderActor, const FString&, ElderName, EElderState, PreviousState, EElderState, NewState);
