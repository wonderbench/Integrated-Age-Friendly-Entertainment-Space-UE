#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ElderTypes.h"
#include "MonitorableActorInterface.generated.h"

UINTERFACE(BlueprintType)
class OLD_MAN_JIANG_API UMonitorableActorInterface : public UInterface { GENERATED_BODY() };

class OLD_MAN_JIANG_API IMonitorableActorInterface
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Monitor") FString GetMonitorName() const;
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Monitor") EElderState GetMonitorState() const;
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Monitor") FString GetMonitorStateString() const;
};
