#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateTypes.h"
#include "ElderTypes.h"
#include "ProjectUserWidgetBase.generated.h"

class AOrbitCameraController;
class AElderlyAICharacter;

/**
 * Shared native parent for the project's UBP_* widgets.
 * Keeps project access points in C++ while every existing Widget Blueprint
 * retains its own layout, animations, bindings, and event graph.
 */
UCLASS(Blueprintable)
class OLD_MAN_JIANG_API UProjectUserWidgetBase : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Presentation only. Existing map/weather/elder state owners are unchanged. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Project UI|Navigation")
	bool bEnableTargetLevelSelection = false;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Project UI|Navigation")
	bool bEnableTargetBottomNavigation = false;
	UPROPERTY(BlueprintReadOnly, Transient, Category="Project UI|Navigation")
	int32 SelectedLevelIndex = INDEX_NONE;
	UPROPERTY(BlueprintReadOnly, Transient, Category="Project UI|Navigation")
	int32 SelectedNavigationIndex = INDEX_NONE;
	/** Read-only presentation: Wheelchair, Stand, GetUp, Busy or Unavailable. */
	UPROPERTY(BlueprintReadOnly, Transient, Category="Project UI|Navigation")
	FName ElderActionMode = TEXT("Unavailable");
	UFUNCTION(BlueprintCallable, Category="Project UI|Navigation") void SelectTargetLevel(int32 Index);
	UFUNCTION(BlueprintCallable, Category="Project UI|Navigation") void SelectTargetNavigation(int32 Index);
	UFUNCTION(BlueprintImplementableEvent, Category="Project UI|Navigation") void OnTargetLevelSelected(int32 Index);
	UFUNCTION(BlueprintImplementableEvent, Category="Project UI|Navigation") void OnTargetNavigationSelected(int32 Index);
	UFUNCTION(BlueprintPure, Category = "Project UI|Controller")
	AOrbitCameraController* GetProjectController() const;

	UFUNCTION(BlueprintPure, Category = "Project UI|Elder")
	AElderlyAICharacter* GetSelectedElder() const;

	UFUNCTION(BlueprintCallable, Category = "Project UI|Elder")
	bool ToggleSelectedElderSightCone();

	UFUNCTION(BlueprintCallable, Category = "Project UI|Elder") bool TakeControlOfSelectedElder();
	UFUNCTION(BlueprintCallable, Category = "Project UI|Elder") bool ReleaseElderControl();

	UFUNCTION(BlueprintImplementableEvent, Category = "Project UI|Lifecycle")
	void OnProjectWidgetReady();

	UFUNCTION(BlueprintImplementableEvent, Category = "Project UI|Elder")
	void OnSelectedElderChanged(AElderlyAICharacter* NewSelectedElder);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	FTimerHandle DashboardAlarmTimer;
	FDelegateHandle DashboardSpawnHandle;
	TArray<TWeakObjectPtr<AActor>> DashboardAlarms;
	void InitializeDashboard();
	void RefreshDashboardAlarm();
	void HandleDashboardActorSpawned(AActor* Actor);
	void InitializeDashboardAlarmButton();
	UFUNCTION() void ToggleDashboardAlarm();
	TWeakObjectPtr<AElderlyAICharacter> RouteNavigationElder;
	uint64 RouteNavigationGeneration = 0;
	void BindElderAction(AElderlyAICharacter* Elder);
	void RefreshElderAction();
	UPROPERTY(Transient) TObjectPtr<AElderlyAICharacter> ActionElder;
	UFUNCTION() void HandleActionState(AActor* Elder, const FString& Name, EElderState Previous, EElderState Current);
	UFUNCTION() void HandleActionDestroyed(AActor* Elder);
	void BindTargetNavigation(bool bBind);
	void RefreshTargetNavigation();
	UPROPERTY(Transient) TArray<FButtonStyle> TargetLevelBaseStyles;
	UPROPERTY(Transient) TArray<FButtonStyle> TargetNavigationBaseStyles;
	UFUNCTION() void TargetLevel0();
	UFUNCTION() void TargetLevel1();
	UFUNCTION() void TargetLevel2();
	UFUNCTION() void TargetNavigation0();
	UFUNCTION() void TargetNavigation1();
	UFUNCTION() void TargetNavigation2();
	UFUNCTION() void TargetNavigation3();
	UFUNCTION() void TargetNavigation4();
	UFUNCTION()
	void HandleSelectedElderChanged(AElderlyAICharacter* NewSelectedElder);
};
