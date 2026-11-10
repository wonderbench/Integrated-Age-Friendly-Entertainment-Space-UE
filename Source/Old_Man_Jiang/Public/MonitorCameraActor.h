#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MonitorCameraActor.generated.h"

class USceneComponent;
class USphereComponent;
class AElderlyAICharacter;

UCLASS(Blueprintable)
class OLD_MAN_JIANG_API AMonitorCameraActor : public AActor
{
	GENERATED_BODY()
public:
	AMonitorCameraActor();
	virtual void BeginPlay() override;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Monitor") TObjectPtr<USceneComponent> MonitorRoot;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Monitor") TObjectPtr<USphereComponent> MonitorRange;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Monitor", meta=(ClampMin="1")) float MonitorRangeRadius = 1500.f;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Monitor") FString CurrentDetectedStateString;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Monitor") TArray<FString> StateHistory;
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Monitor") FString StateHistoryText;
	UFUNCTION(BlueprintCallable, Category="Monitor") void RegisterElder(AElderlyAICharacter* Elder);
	UFUNCTION(BlueprintCallable, Category="Monitor") void UnregisterElder(AElderlyAICharacter* Elder);
private:
	TSet<TWeakObjectPtr<AElderlyAICharacter>> MonitoredElders;
	UFUNCTION() void HandleMonitorBegin(UPrimitiveComponent* Overlapped, AActor* Other, UPrimitiveComponent* OtherComp, int32 Index, bool bSweep, const FHitResult& Hit);
	UFUNCTION() void HandleMonitorEnd(UPrimitiveComponent* Overlapped, AActor* Other, UPrimitiveComponent* OtherComp, int32 Index);
	UFUNCTION() void HandleElderStateChanged(AActor* ElderActor, const FString& ElderName, EElderState OldState, EElderState NewState);
	void AppendState(AActor* ElderActor, const FString& Name, EElderState OldState, EElderState NewState, bool bInitial);
};
