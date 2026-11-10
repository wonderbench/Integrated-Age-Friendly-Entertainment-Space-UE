#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Types/SlateEnums.h"
#include "ElderTypes.h"
#include "ElderDebugControlWidget.generated.h"

class UButton;
class UTextBlock;
class UComboBoxString;
class AElderlyAICharacter;

/** Small opt-in test control; it only forwards to the selected elder on BP_Controller. */
UCLASS()
class OLD_MAN_JIANG_API UElderDebugControlWidget : public UUserWidget
{
	GENERATED_BODY()
protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
private:
	UPROPERTY() TObjectPtr<UButton> ToggleButton;
	UPROPERTY() TObjectPtr<UTextBlock> Label;
	UPROPERTY() TObjectPtr<UComboBoxString> ElderPicker;
	UPROPERTY() TObjectPtr<UTextBlock> Status;
	UPROPERTY() TObjectPtr<UButton> WheelchairButton;
	UPROPERTY() TObjectPtr<UButton> FallButton;
	UPROPERTY() TObjectPtr<UButton> StandButton;
	TWeakObjectPtr<AElderlyAICharacter> ObservedElder;
	void UnbindObservedElder();
	void RefreshStateActions();
	UFUNCTION() void HandleSelectedElderChanged(AElderlyAICharacter* Elder);
	UFUNCTION() void HandleElderStateChanged(AActor* Elder, const FString& Name, EElderState Previous, EElderState Current);
	UFUNCTION() void HandleElderDestroyed(AActor* Elder);
	UFUNCTION() void HandleWheelchair();
	UFUNCTION() void HandleFall();
	UFUNCTION() void HandleStand();
	TMap<FString, TWeakObjectPtr<AElderlyAICharacter>> Options;
	bool bRefreshing = false;
	UFUNCTION() void RefreshElders();
	UFUNCTION() void HandleSelection(FString Item, ESelectInfo::Type SelectionType);
	UFUNCTION() void HandleTakeover();
	UFUNCTION() void HandleRelease();
	UFUNCTION() void HandlePawnChanged(APawn* OldPawn, APawn* NewPawn);
	UFUNCTION() void HandleToggleClicked();
};
