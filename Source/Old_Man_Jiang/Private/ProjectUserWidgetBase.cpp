#include "ProjectUserWidgetBase.h"

#include "ElderlyAICharacter.h"
#include "ElderNavigationComponent.h"
#include "ElderSplinePath.h"
#include "MonitorCameraActor.h"
#include "OrbitCameraController.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetTree.h"
#include "Styling/SlateBrush.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "UObject/UnrealType.h"

namespace
{
	const TCHAR* DashboardMaps[] = {TEXT("Showcase"), TEXT("L_ElderWellness_Main"), TEXT("L_LivingGrove_Social")};
	const TCHAR* DashboardPaths[] = {TEXT("/Game/Project_File/Maps/Showcase"), TEXT("/Game/Project_File/Maps/L_ElderWellness_Main"), TEXT("/Game/Project_File/Maps/L_LivingGrove_Social")};
	int32 DashboardLevel(const UObject* Context)
	{
		const FString Name = UGameplayStatics::GetCurrentLevelName(Context, true);
		for (int32 Index=0; Index<3; ++Index) if (Name == DashboardMaps[Index]) return Index;
		return INDEX_NONE;
	}
	bool CanControlDashboardAlarm(const AActor* Alarm)
	{
		if (!IsValid(Alarm) || !FindFProperty<FBoolProperty>(Alarm->GetClass(), TEXT("bAlarmActive"))) return false;
		const UFunction* Begin = Alarm->FindFunction(TEXT("Light_Begin"));
		const UFunction* Stop = Alarm->FindFunction(TEXT("Light_Stop"));
		return Begin && Stop && Begin->NumParms == 0 && Stop->NumParms == 0;
	}
}

void UProjectUserWidgetBase::NativeConstruct()
{
	Super::NativeConstruct();
	InitializeDashboard();
	BindTargetNavigation(true);
	RefreshTargetNavigation();

	if (AOrbitCameraController* Controller = GetProjectController())
	{
		Controller->OnSelectedElderChanged.AddUniqueDynamic(this, &UProjectUserWidgetBase::HandleSelectedElderChanged);
	}
	OnProjectWidgetReady();
	BindElderAction(GetSelectedElder());
}

void UProjectUserWidgetBase::NativeDestruct()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(DashboardAlarmTimer);
		if (DashboardSpawnHandle.IsValid()) GetWorld()->RemoveOnActorSpawnedHandler(DashboardSpawnHandle);
	}
	DashboardSpawnHandle.Reset();
	DashboardAlarms.Reset();
	if (AElderlyAICharacter* RouteElder = RouteNavigationElder.Get())
		if (RouteElder->ElderNavigation && RouteElder->ElderNavigation->GetNavigationGeneration() == RouteNavigationGeneration) RouteElder->ElderNavigation->PauseNavigation();
	RouteNavigationElder.Reset();
	BindTargetNavigation(false);
	BindElderAction(nullptr);
	if (AOrbitCameraController* Controller = GetProjectController())
	{
		Controller->OnSelectedElderChanged.RemoveDynamic(this, &UProjectUserWidgetBase::HandleSelectedElderChanged);
	}
	Super::NativeDestruct();
}

AOrbitCameraController* UProjectUserWidgetBase::GetProjectController() const
{
	return Cast<AOrbitCameraController>(GetOwningPlayer());
}

AElderlyAICharacter* UProjectUserWidgetBase::GetSelectedElder() const
{
	const AOrbitCameraController* Controller = GetProjectController();
	return Controller ? Controller->SelectedElder.Get() : nullptr;
}

bool UProjectUserWidgetBase::ToggleSelectedElderSightCone()
{
	AOrbitCameraController* Controller = GetProjectController();
	return Controller && Controller->ToggleSelectedElderSightCone();
}

void UProjectUserWidgetBase::HandleSelectedElderChanged(AElderlyAICharacter* NewSelectedElder)
{
	BindElderAction(NewSelectedElder);
	OnSelectedElderChanged(NewSelectedElder);
}

void UProjectUserWidgetBase::BindElderAction(AElderlyAICharacter* Elder)
{
	if (IsValid(ActionElder))
	{
		ActionElder->OnElderStateChanged.RemoveDynamic(this, &UProjectUserWidgetBase::HandleActionState);
		ActionElder->OnDestroyed.RemoveDynamic(this, &UProjectUserWidgetBase::HandleActionDestroyed);
	}
	ActionElder = bEnableTargetBottomNavigation ? Elder : nullptr;
	if (IsValid(ActionElder))
	{
		ActionElder->OnElderStateChanged.AddUniqueDynamic(this, &UProjectUserWidgetBase::HandleActionState);
		ActionElder->OnDestroyed.AddUniqueDynamic(this, &UProjectUserWidgetBase::HandleActionDestroyed);
	}
	RefreshElderAction();
}

void UProjectUserWidgetBase::HandleActionState(AActor*, const FString&, EElderState, EElderState)
{
	RefreshElderAction();
}

void UProjectUserWidgetBase::HandleActionDestroyed(AActor*)
{
	BindElderAction(nullptr);
}

void UProjectUserWidgetBase::RefreshElderAction()
{
	if (!bEnableTargetBottomNavigation) return;
	ElderActionMode = TEXT("Unavailable");
	FString Hint = TEXT("请先选择老人");
	bool bEnabled = false;
	if (IsValid(ActionElder))
	{
		switch (ActionElder->CurrentState)
		{
		case EElderState::Standing:
		case EElderState::Walking: ElderActionMode = TEXT("Wheelchair"); Hint = TEXT("坐入轮椅；C 键跌倒"); bEnabled = true; break;
		case EElderState::WheelchairIdle:
		case EElderState::WheelchairMoving: ElderActionMode = TEXT("Stand"); Hint = TEXT("站立"); bEnabled = true; break;
		case EElderState::Fallen: ElderActionMode = TEXT("GetUp"); bEnabled = ActionElder->GetUpAnimation != nullptr; Hint = bEnabled ? TEXT("起立") : TEXT("未配置起身动画"); break;
		case EElderState::Falling: ElderActionMode = TEXT("Busy"); Hint = TEXT("跌倒中，请稍候"); break;
		case EElderState::GettingUp: ElderActionMode = TEXT("Busy"); Hint = TEXT("起立中，请稍候"); break;
		default: Hint = TEXT("当前状态不可切换"); break;
		}
	}
	if (UButton* Button = Cast<UButton>(GetWidgetFromName(TEXT("TargetNav_2"))))
	{
		// Navigation stays selectable even when the elder action is unavailable.
		// SelectTargetNavigation guards the actual wheelchair/get-up transition.
		Button->SetIsEnabled(true);
		Button->SetToolTipText(FText::FromString(Hint));
	}
	RefreshTargetNavigation();
}

bool UProjectUserWidgetBase::TakeControlOfSelectedElder()
{
	AOrbitCameraController* Controller = GetProjectController();
	return Controller && Controller->TakeControlOfSelectedElder();
}

bool UProjectUserWidgetBase::ReleaseElderControl()
{
	AOrbitCameraController* Controller = GetProjectController();
	return Controller && Controller->ReleaseElderControl();
}

namespace
{
	const FName TargetLevelNames[] = {TEXT("Button_45"), TEXT("Button"), TEXT("Button_1")};
	const FName TargetNavigationNames[] = {TEXT("TargetNav_1"), TEXT("TargetNav_2"), TEXT("TargetNav_3"), TEXT("TargetNav_4"), TEXT("TargetNav_5")};
}

void UProjectUserWidgetBase::BindTargetNavigation(bool bBind)
{
	const FName LevelFunctions[] = {GET_FUNCTION_NAME_CHECKED(UProjectUserWidgetBase, TargetLevel0), GET_FUNCTION_NAME_CHECKED(UProjectUserWidgetBase, TargetLevel1), GET_FUNCTION_NAME_CHECKED(UProjectUserWidgetBase, TargetLevel2)};
	const FName NavFunctions[] = {GET_FUNCTION_NAME_CHECKED(UProjectUserWidgetBase, TargetNavigation0), GET_FUNCTION_NAME_CHECKED(UProjectUserWidgetBase, TargetNavigation1), GET_FUNCTION_NAME_CHECKED(UProjectUserWidgetBase, TargetNavigation2), GET_FUNCTION_NAME_CHECKED(UProjectUserWidgetBase, TargetNavigation3), GET_FUNCTION_NAME_CHECKED(UProjectUserWidgetBase, TargetNavigation4)};
	auto BindGroup = [this, bBind](const FName* Names, const FName* Functions, int32 Count, TArray<FButtonStyle>& Styles)
	{
		const bool bCache = bBind && Styles.IsEmpty();
		if (bCache) Styles.SetNum(Count);
		for (int32 Index=0; Index<Count; ++Index)
		{
			if (UButton* Button = Cast<UButton>(GetWidgetFromName(Names[Index])))
			{
				FScriptDelegate Delegate;
				Delegate.BindUFunction(this, Functions[Index]);
				if (bBind) Button->OnClicked.AddUnique(Delegate);
				else Button->OnClicked.Remove(Delegate);
				if (bCache) Styles[Index] = Button->GetStyle();
			}
		}
	};
	if (bEnableTargetLevelSelection) BindGroup(TargetLevelNames, LevelFunctions, 3, TargetLevelBaseStyles);
	if (bEnableTargetBottomNavigation) BindGroup(TargetNavigationNames, NavFunctions, 5, TargetNavigationBaseStyles);
}

void UProjectUserWidgetBase::RefreshTargetNavigation()
{
	auto Refresh = [this](const FName* Names, int32 Selected, const TArray<FButtonStyle>& Styles)
	{
		for (int32 Index=0; Index<Styles.Num(); ++Index)
		{
			if (UButton* Button = Cast<UButton>(GetWidgetFromName(Names[Index])))
			{
				FButtonStyle Style = Styles[Index];
				// Authored Pressed brush is the supplied highlight. No tint, scale or new border.
				if (Index == Selected)
				{
					Style.SetNormal(Style.Pressed);
					Style.SetHovered(Style.Pressed);
				}
				Button->SetStyle(Style);
			}
		}
	};
	if (bEnableTargetLevelSelection) Refresh(TargetLevelNames, SelectedLevelIndex, TargetLevelBaseStyles);
	if (bEnableTargetBottomNavigation)
	{
		Refresh(TargetNavigationNames, SelectedNavigationIndex, TargetNavigationBaseStyles);
		for (int32 Index = 0; Index < 5; ++Index)
		{
			if (UTextBlock* Label = Cast<UTextBlock>(GetWidgetFromName(*FString::Printf(TEXT("TargetNavLabel_%d"), Index + 1))))
			{
				const FLinearColor Color = Index == SelectedNavigationIndex
					? FLinearColor(1.0f, 0.82f, 0.50f, 1.0f)
					: FLinearColor(0.75f, 0.62f, 0.45f, 1.0f);
				Label->SetColorAndOpacity(FSlateColor(Color));
			}
		}
	}
}

void UProjectUserWidgetBase::SelectTargetLevel(int32 Index)
{
	if (!bEnableTargetLevelSelection || Index<0 || Index>=3) return;
	SelectedLevelIndex=Index;
	RefreshTargetNavigation();
	OnTargetLevelSelected(Index);
	if (DashboardLevel(this) != Index && GetWorld() && GetWorld()->IsGameWorld())
		UGameplayStatics::OpenLevel(this, FName(DashboardPaths[Index]));
}

void UProjectUserWidgetBase::InitializeDashboard()
{
	const int32 Level = DashboardLevel(this);
	if (Level == INDEX_NONE) return;
	if (bEnableTargetLevelSelection)
	{
		SelectedLevelIndex = Level;
		const TCHAR* Names[] = {TEXT("起居适老"), TEXT("娱乐康养"), TEXT("生态康养")};
		for (int32 Index=0; Index<3; ++Index)
			if (UTextBlock* Text = Cast<UTextBlock>(GetWidgetFromName(*FString::Printf(TEXT("TXT_Level%d"), Index+1)))) Text->SetText(FText::FromString(Names[Index]));
	}
	if (!GetWidgetFromName(TEXT("Target_OverviewValue0"))) return;
	InitializeDashboardAlarmButton();
	const int32 Sensors[] = {3,4,7};
	const int32 Health[] = {3,5,5};
	const auto SetText = [this](const TCHAR* Name, const FString& Value)
	{
		if (UTextBlock* Text = Cast<UTextBlock>(GetWidgetFromName(Name))) Text->SetText(FText::FromString(Value));
	};
	SetText(TEXT("Target_OverviewValue0"), TEXT("1 人"));
	SetText(TEXT("Target_OverviewValue1"), FString::Printf(TEXT("%d 台"), 4 + Sensors[Level] + Health[Level]));
	SetText(TEXT("Target_OverviewValue2"), TEXT("0 场"));
	SetText(TEXT("Target_OverviewNote2"), TEXT("参与 0 人次"));
	SetText(TEXT("Target_DevicesValue0"), TEXT("4 / 4"));
	SetText(TEXT("Target_DevicesValue1"), FString::Printf(TEXT("%d / %d"), Sensors[Level], Sensors[Level]));
	SetText(TEXT("Target_DevicesValue2"), FString::Printf(TEXT("%d / %d"), Health[Level], Health[Level]));
	DashboardAlarms.Reset();
	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AActor> It(World); It; ++It) HandleDashboardActorSpawned(*It);
		DashboardSpawnHandle = World->AddOnActorSpawnedHandler(FOnActorSpawned::FDelegate::CreateUObject(this, &UProjectUserWidgetBase::HandleDashboardActorSpawned));
		RefreshDashboardAlarm();
		World->GetTimerManager().SetTimer(DashboardAlarmTimer, this, &UProjectUserWidgetBase::RefreshDashboardAlarm, 0.25f, true);
	}
}

void UProjectUserWidgetBase::InitializeDashboardAlarmButton()
{
	if (!WidgetTree || GetWidgetFromName(TEXT("DashboardAlarmButton"))) return;
	UCanvasPanel* Canvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!Canvas) return;
	UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("DashboardAlarmButton"));
	UTextBlock* Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DashboardAlarmLabel"));
	if (!Button || !Label) return;
	FButtonStyle Style = Button->GetStyle();
	Style.SetNormal(FSlateNoResource());
	Style.SetHovered(FSlateNoResource());
	Style.SetPressed(FSlateNoResource());
	Style.SetDisabled(FSlateNoResource());
	Button->SetStyle(Style);
	if (FBoolProperty* Focusable = FindFProperty<FBoolProperty>(UButton::StaticClass(), TEXT("IsFocusable")))
		Focusable->SetPropertyValue_InContainer(Button, false);
	Button->SetToolTipText(FText::FromString(TEXT("随机开启一盏警报灯；再次点击解除警报")));
	Button->OnClicked.AddUniqueDynamic(this, &UProjectUserWidgetBase::ToggleDashboardAlarm);
	Label->SetText(FText::FromString(TEXT("警报")));
	Label->SetJustification(ETextJustify::Center);
	Label->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UTextBlock* Reference = Cast<UTextBlock>(GetWidgetFromName(TEXT("Target_DevicesLabel0"))))
		Label->SetFont(Reference->GetFont());
	Button->AddChild(Label);
	if (UCanvasPanelSlot* AlarmSlot = Canvas->AddChildToCanvas(Button))
	{
		AlarmSlot->SetPosition(FVector2D(350.0f, 678.0f));
		AlarmSlot->SetSize(FVector2D(100.0f, 32.0f));
		AlarmSlot->SetZOrder(10);
	}
}

void UProjectUserWidgetBase::ToggleDashboardAlarm()
{
	TArray<AActor*> Available;
	TArray<AActor*> ActiveAlarms;
	for (const TWeakObjectPtr<AActor>& Ref : DashboardAlarms)
	{
		AActor* Alarm = Ref.Get();
		if (!CanControlDashboardAlarm(Alarm)) continue;
		const FBoolProperty* State = FindFProperty<FBoolProperty>(Alarm->GetClass(), TEXT("bAlarmActive"));
		Available.Add(Alarm);
		if (State->GetPropertyValue_InContainer(Alarm)) ActiveAlarms.Add(Alarm);
	}
	const bool bStopping = !ActiveAlarms.IsEmpty();
	const TArray<AActor*>& Candidates = bStopping ? ActiveAlarms : Available;
	AActor* Target = Candidates.IsEmpty() ? nullptr : Candidates[FMath::RandRange(0, Candidates.Num() - 1)];
	if (Target)
	{
		if (UFunction* Function = Target->FindFunction(bStopping ? TEXT("Light_Stop") : TEXT("Light_Begin")))
			Target->ProcessEvent(Function, nullptr);
	}
	RefreshDashboardAlarm();
}

void UProjectUserWidgetBase::HandleDashboardActorSpawned(AActor* Actor)
{
	if (!IsValid(Actor)) return;
	for (UClass* Class = Actor->GetClass(); Class; Class = Class->GetSuperClass())
		if (Class->GetPathName() == TEXT("/Game/Project_File/BP/Actor/BP_alarm.BP_alarm_C") ||
			Class->GetPathName() == TEXT("/Game/Project_File/BP/BP_alarm.BP_alarm_C"))
		{ DashboardAlarms.AddUnique(Actor); return; }
}

void UProjectUserWidgetBase::RefreshDashboardAlarm()
{
	DashboardAlarms.RemoveAll([](const TWeakObjectPtr<AActor>& Ref) { return !Ref.IsValid(); });
	int32 Active = 0;
	for (const TWeakObjectPtr<AActor>& Ref : DashboardAlarms)
		if (const AActor* Alarm = Ref.Get())
			if (const FBoolProperty* Property = FindFProperty<FBoolProperty>(Alarm->GetClass(), TEXT("bAlarmActive")))
				Active += Property->GetPropertyValue_InContainer(Alarm) ? 1 : 0;
	if (UTextBlock* Text = Cast<UTextBlock>(GetWidgetFromName(TEXT("Target_OverviewValue3")))) Text->SetText(FText::FromString(Active ? TEXT("注意") : TEXT("安全")));
	if (UTextBlock* Text = Cast<UTextBlock>(GetWidgetFromName(TEXT("Target_OverviewNote3")))) Text->SetText(FText::FromString(TEXT("本地警报灯状态")));
	if (UTextBlock* Text = Cast<UTextBlock>(GetWidgetFromName(TEXT("Target_DevicesValue3")))) Text->SetText(FText::FromString(FString::Printf(TEXT("%d 条"), Active)));
	const bool bCanControl = DashboardAlarms.ContainsByPredicate([](const TWeakObjectPtr<AActor>& Ref) { return CanControlDashboardAlarm(Ref.Get()); });
	if (UButton* Button = Cast<UButton>(GetWidgetFromName(TEXT("DashboardAlarmButton")))) Button->SetIsEnabled(bCanControl);
	if (UTextBlock* Label = Cast<UTextBlock>(GetWidgetFromName(TEXT("DashboardAlarmLabel"))))
		Label->SetColorAndOpacity(FSlateColor(Active ? FLinearColor(1.f, 0.76f, 0.27f, 1.f) : FLinearColor(0.75f, 0.62f, 0.45f, 1.f)));
}
void UProjectUserWidgetBase::SelectTargetNavigation(int32 Index)
{
	if (!bEnableTargetBottomNavigation || Index<0 || Index>=5) return;
	// A second click on the active route pauses the same managed request.
	// Keep the owner and generation so a later click can resume its progress.
	if (Index == 2 && SelectedNavigationIndex == 2)
	{
		if (AElderlyAICharacter* RouteElder = RouteNavigationElder.Get())
			if (RouteElder->ElderNavigation && RouteElder->ElderNavigation->GetNavigationGeneration() == RouteNavigationGeneration && RouteElder->ElderNavigation->NavigationStatus == EElderNavigationStatus::Moving) RouteElder->ElderNavigation->PauseNavigation();
		SelectedNavigationIndex = INDEX_NONE;
		RefreshTargetNavigation();
		OnTargetNavigationSelected(Index);
		return;
	}
	if (SelectedNavigationIndex == 2 && Index != 2)
	{
		if (AElderlyAICharacter* RouteElder = RouteNavigationElder.Get())
			if (RouteElder->ElderNavigation && RouteElder->ElderNavigation->GetNavigationGeneration() == RouteNavigationGeneration) RouteElder->ElderNavigation->PauseNavigation();
	}
	// The monitor tab is reconciled with the resulting scan visibility below.
	SelectedNavigationIndex = Index;
	RefreshTargetNavigation();

	// A single elder is unambiguous in the migrated scenes. Preserve an explicit
	// selection and require the existing selection UI when several elders exist.
	if ((Index == 0 || Index == 1 || Index == 2 || Index == 4) && !GetSelectedElder())
	{
		if (AOrbitCameraController* Controller = GetProjectController())
		{
			const TArray<AElderlyAICharacter*> Elders = Controller->GetAvailableElders();
			if (Elders.Num() == 1) Controller->SetSelectedElder(Elders[0]);
		}
	}

	// The first button switches between the observation pawn (god view) and
	// third-person control of the selected elder.
	if (Index == 0)
	{
		AOrbitCameraController* Controller = GetProjectController();
		if (Controller)
		{
			if (Controller->IsControllingElder()) Controller->ReleaseElderControl();
			else if (Controller->TakeControlOfSelectedElder()) Controller->EnableElderNavigationUI();
		}
		OnTargetNavigationSelected(Index);
		return;
	}

	// The second button reuses the selected elder's established state owner.
	// Transitions remain owned by the elder; repeated clicks cannot skip animations.
	if (Index == 1)
	{
		AElderlyAICharacter* Elder = GetSelectedElder();
		if (Elder)
		{
			if (Elder->CurrentState == EElderState::Fallen) Elder->StartGettingUp();
			else if (Elder->CurrentState == EElderState::Standing || Elder->CurrentState == EElderState::Walking) Elder->EnterWheelchair();
			else if (Elder->CurrentState == EElderState::WheelchairIdle || Elder->CurrentState == EElderState::WheelchairMoving) Elder->ExitWheelchair();
		}
		OnTargetNavigationSelected(Index);
		return;
	}

	// Start the selected elder's authored spline route. Player control must be
	// returned to the existing AI controller before it can issue movement.
	if (Index == 2)
	{
		AElderlyAICharacter* Elder = GetSelectedElder();
		if (AElderlyAICharacter* PriorElder = RouteNavigationElder.Get())
			if (PriorElder != Elder && PriorElder->ElderNavigation && PriorElder->ElderNavigation->GetNavigationGeneration() == RouteNavigationGeneration) PriorElder->ElderNavigation->PauseNavigation();
		UElderNavigationComponent* RouteComponent = Elder ? Elder->ElderNavigation : nullptr;
		if (RouteComponent && RouteComponent->NavigationMode == EElderNavigationMode::Spline && IsValid(RouteComponent->SplinePath))
		{
			AOrbitCameraController* Controller = GetProjectController();
			if (Controller && Controller->GetPawn() == Elder) Controller->ReleaseElderControl();
			if (!Elder->IsUnderPlayerControl())
			{
				if (Elder->CanElderMove())
				{
					const bool bResume = RouteComponent->IsPausedConfiguredSpline();
					const uint64 ExpectedGeneration = RouteComponent->GetNavigationGeneration() + (bResume ? 0 : 1);
					const bool bStarted = bResume ? RouteComponent->ResumeNavigation() : RouteComponent->StartConfiguredNavigation();
					if (bStarted && RouteComponent->GetNavigationGeneration() == ExpectedGeneration)
					{
						RouteNavigationElder = Elder;
						RouteNavigationGeneration = ExpectedGeneration;
					}
				}
			}
		}
		OnTargetNavigationSelected(Index);
		return;
	}

	// Toggle the already-authored wireframe visualization on monitor cameras.
	// This changes only display, not the cameras' detection or overlap behavior.
	if (Index == 3)
	{
		TArray<UStaticMeshComponent*> ScanMeshes;
		if (UWorld* World = GetWorld())
		{
			for (TActorIterator<AMonitorCameraActor> It(World); It; ++It)
			{
				TInlineComponentArray<UStaticMeshComponent*> Components(*It);
				for (UStaticMeshComponent* Mesh : Components)
					if (Mesh && Mesh->GetFName() == TEXT("SM_Camera_WireFrame")) ScanMeshes.Add(Mesh);
			}
		}
		const bool bShow = !ScanMeshes.ContainsByPredicate([](const UStaticMeshComponent* Mesh) { return Mesh->IsVisible(); });
		for (UStaticMeshComponent* Mesh : ScanMeshes)
		{
			Mesh->SetHiddenInGame(false);
			Mesh->SetVisibility(bShow, true);
		}
		SelectedNavigationIndex = bShow && !ScanMeshes.IsEmpty() ? Index : INDEX_NONE;
		RefreshTargetNavigation();
		OnTargetNavigationSelected(Index);
		return;
	}

	// The fifth button still toggles the existing selected-elder sight visualizer.
	// Its cone visibility no longer controls the navigation highlight.
	if (Index == 4)
	{
		ToggleSelectedElderSightCone();
		OnTargetNavigationSelected(Index);
		return;
	}

	OnTargetNavigationSelected(Index);
}
void UProjectUserWidgetBase::TargetLevel0() { SelectTargetLevel(0); }
void UProjectUserWidgetBase::TargetLevel1() { SelectTargetLevel(1); }
void UProjectUserWidgetBase::TargetLevel2() { SelectTargetLevel(2); }
void UProjectUserWidgetBase::TargetNavigation0() { SelectTargetNavigation(0); }
void UProjectUserWidgetBase::TargetNavigation1() { SelectTargetNavigation(1); }
void UProjectUserWidgetBase::TargetNavigation2() { SelectTargetNavigation(2); }
void UProjectUserWidgetBase::TargetNavigation3() { SelectTargetNavigation(3); }
void UProjectUserWidgetBase::TargetNavigation4() { SelectTargetNavigation(4); }
