#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ElderSplinePath.generated.h"

class USplineComponent;

/** World-space route, independent of the moving elder. Edit points on placed instances. */
UCLASS(Blueprintable)
class OLD_MAN_JIANG_API AElderSplinePath : public AActor
{
	GENERATED_BODY()
public:
	AElderSplinePath();
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Elder Route") TObjectPtr<USplineComponent> Spline;
};
