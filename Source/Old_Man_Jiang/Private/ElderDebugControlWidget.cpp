#include "ElderDebugControlWidget.h"
#include "OrbitCameraController.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetTree.h"
#include "Components/ComboBoxString.h"
#include "ElderlyAICharacter.h"
#include "Engine/Texture2D.h"
#include "Brushes/SlateColorBrush.h"

namespace
{
	const FLinearColor GoldText(1.f, .94f, .82f, 1.f);
	const FLinearColor GoldSecondaryText(.79f, .73f, .63f, 1.f);

	FSlateBrush GoldImageBrush(const FSlateBrush& Existing, UObject* Texture, const FLinearColor& Tint)
	{
		// Keep the previous brush's desired size. PNG resolution must not change this layout.
		FSlateBrush Brush = Existing;
		Brush.SetResourceObject(Texture);
		Brush.DrawAs = ESlateBrushDrawType::Image;
		Brush.ImageType = ESlateBrushImageType::FullColor;
		Brush.Tiling = ESlateBrushTileType::NoTile;
		Brush.Mirroring = ESlateBrushMirrorType::NoMirror;
		Brush.Margin = FMargin(0.f);
		Brush.SetUVRegion(FBox2f(FVector2f::ZeroVector, FVector2f(1.f, 1.f)));
		Brush.TintColor = Tint;
		return Brush;
	}

	FButtonStyle GoldButtonStyle(const FButtonStyle& Existing, const TCHAR* TexturePath)
	{
		FButtonStyle Style = Existing;
		if (UObject* Texture = LoadObject<UObject>(nullptr, TexturePath))
		{
			Style.Normal = GoldImageBrush(Existing.Normal, Texture, FLinearColor::White);
			Style.Hovered = GoldImageBrush(Existing.Hovered, Texture, FLinearColor(1.08f, 1.08f, 1.08f, 1.f));
			Style.Pressed = GoldImageBrush(Existing.Pressed, Texture, FLinearColor(.86f, .86f, .86f, 1.f));
			Style.Disabled = GoldImageBrush(Existing.Disabled, Texture, FLinearColor(.48f, .48f, .48f, .65f));
		}
		Style.NormalForeground = GoldText;
		Style.HoveredForeground = GoldText;
		Style.PressedForeground = GoldText;
		Style.DisabledForeground = GoldSecondaryText;
		return Style;
	}

	FSlateBrush GoldRowBrush(const FSlateBrush& Existing, const FLinearColor& Color)
	{
		FSlateBrush Brush = FSlateColorBrush(Color);
		Brush.SetImageSize(Existing.GetImageSize());
		return Brush;
	}

	void StyleElderPicker(UComboBoxString* Picker)
	{
		FComboBoxStyle Style = Picker->GetWidgetStyle();
		FComboButtonStyle& Combo = Style.ComboButtonStyle;
		Combo.ButtonStyle = GoldButtonStyle(Combo.ButtonStyle, TEXT("/Game/UI/NewGoldStyle/T_UI_ElderDropdown.T_UI_ElderDropdown"));
		// The PNG supplies the arrow. Keep Slate's original arrow size/padding, but hide its paint.
		Combo.DownArrowImage.TintColor = FLinearColor::Transparent;
		Combo.ShadowColorAndOpacity = FLinearColor::Transparent;
		Combo.MenuBorderBrush = GoldRowBrush(Combo.MenuBorderBrush, FLinearColor(.035f, .023f, .013f, .98f));
		Picker->SetWidgetStyle(Style);
		FTableRowStyle Rows = Picker->GetItemStyle();
		const FLinearColor Normal(.04f, .026f, .014f, 1.f);
		const FLinearColor Hover(.15f, .092f, .032f, 1.f);
		const FLinearColor Selected(.25f, .15f, .052f, 1.f);
		Rows.EvenRowBackgroundBrush = GoldRowBrush(Rows.EvenRowBackgroundBrush, Normal);
		Rows.OddRowBackgroundBrush = GoldRowBrush(Rows.OddRowBackgroundBrush, Normal);
		Rows.EvenRowBackgroundHoveredBrush = GoldRowBrush(Rows.EvenRowBackgroundHoveredBrush, Hover);
		Rows.OddRowBackgroundHoveredBrush = GoldRowBrush(Rows.OddRowBackgroundHoveredBrush, Hover);
		Rows.ActiveBrush = GoldRowBrush(Rows.ActiveBrush, Selected);
		Rows.InactiveBrush = GoldRowBrush(Rows.InactiveBrush, Selected);
		Rows.ActiveHoveredBrush = GoldRowBrush(Rows.ActiveHoveredBrush, Hover);
		Rows.InactiveHoveredBrush = GoldRowBrush(Rows.InactiveHoveredBrush, Hover);
		Rows.SelectorFocusedBrush = GoldRowBrush(Rows.SelectorFocusedBrush, FLinearColor(.7f, .44f, .16f, .25f));
		Rows.TextColor = GoldText;
		Rows.SelectedTextColor = GoldText;
		Picker->SetItemStyle(Rows);
		// UE exposes this construction-only property without a setter; no Slate widget exists yet.
PRAGMA_DISABLE_DEPRECATION_WARNINGS
		Picker->ForegroundColor = GoldText;
PRAGMA_ENABLE_DEPRECATION_WARNINGS
	}
}

TSharedRef<SWidget> UElderDebugControlWidget::RebuildWidget()
{
	// Build the UObject tree BEFORE Super builds its Slate tree. NativeConstruct is too late.
	if (ToggleButton) return Super::RebuildWidget();
	UCanvasPanel* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("ElderDebugCanvas"));
	WidgetTree->RootWidget = Canvas;
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	Canvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	ToggleButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ToggleElderSightCone"));
	ToggleButton->SetStyle(GoldButtonStyle(ToggleButton->GetStyle(), TEXT("/Game/UI/NewGoldStyle/T_UI_ActionBlank.T_UI_ActionBlank")));
	Label = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ToggleElderSightConeLabel"));
	Label->SetText(FText::FromString(TEXT("显示 / 隐藏老人视线范围")));
	FSlateFontInfo DebugFont = Label->GetFont();
	DebugFont.Size = 16;
	Label->SetFont(DebugFont);
	Label->SetColorAndOpacity(GoldText);
	Label->SetVisibility(ESlateVisibility::HitTestInvisible);
	ToggleButton->AddChild(Label);
	UCanvasPanelSlot* CanvasSlot = Canvas->AddChildToCanvas(ToggleButton);
	CanvasSlot->SetPosition(FVector2D(24.f, 180.f));
	CanvasSlot->SetSize(FVector2D(340.f, 42.f));
	ToggleButton->OnClicked.AddDynamic(this, &UElderDebugControlWidget::HandleToggleClicked);

	ElderPicker = WidgetTree->ConstructWidget<UComboBoxString>(UComboBoxString::StaticClass(), TEXT("ElderPicker"));
	StyleElderPicker(ElderPicker);
	UCanvasPanelSlot* PickerSlot = Canvas->AddChildToCanvas(ElderPicker);
	PickerSlot->SetPosition(FVector2D(24.f, 228.f));
	PickerSlot->SetSize(FVector2D(340.f, 36.f));
	ElderPicker->OnOpening.AddDynamic(this, &UElderDebugControlWidget::RefreshElders);
	ElderPicker->OnSelectionChanged.AddDynamic(this, &UElderDebugControlWidget::HandleSelection);
	RefreshElders();
	auto AddButton = [this, Canvas, DebugFont](const TCHAR* Name, const TCHAR* Text, float X, const TCHAR* TexturePath)
	{
		UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
		Button->SetStyle(GoldButtonStyle(Button->GetStyle(), TexturePath));
		UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>();
		TextBlock->SetText(FText::FromString(Text));
		TextBlock->SetFont(DebugFont);
		TextBlock->SetColorAndOpacity(GoldText);
		TextBlock->SetVisibility(ESlateVisibility::HitTestInvisible);
		Button->AddChild(TextBlock);
		UCanvasPanelSlot* Slot = Canvas->AddChildToCanvas(Button);
		Slot->SetPosition(FVector2D(X, 270.f));
		Slot->SetSize(FVector2D(164.f, 36.f));
		return Button;
	};
	AddButton(TEXT("TakeControl"), TEXT("接管选中老人"), 24.f, TEXT("/Game/UI/NewGoldStyle/T_UI_ActionBlank.T_UI_ActionBlank"))->OnClicked.AddDynamic(this, &UElderDebugControlWidget::HandleTakeover);
	AddButton(TEXT("ReleaseControl"), TEXT("返回观察 (Tab)"), 200.f, TEXT("/Game/UI/NewGoldStyle/T_UI_ReturnObserve.T_UI_ReturnObserve"))->OnClicked.AddDynamic(this, &UElderDebugControlWidget::HandleRelease);
	// State actions are a read-only projection of the selected elder, never a second state owner.
	auto AddStateAction = [this, Canvas](const TCHAR* Name, const TCHAR* Tooltip, float X, const TCHAR* TexturePath)
	{
		UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
		Button->SetStyle(GoldButtonStyle(Button->GetStyle(), TexturePath));
		Button->SetToolTipText(FText::FromString(Tooltip));
		Button->SetVisibility(ESlateVisibility::Collapsed);
		UCanvasPanelSlot* Slot = Canvas->AddChildToCanvas(Button);
		Slot->SetPosition(FVector2D(X, 312.f));
		Slot->SetSize(FVector2D(66.f, 66.f)); // User-requested 0.6x of the initial 110px layout/hit area.
		return Button;
	};
	WheelchairButton = AddStateAction(TEXT("ElderActionWheelchair"), TEXT("让选中老人坐入轮椅"), 24.f, TEXT("/Game/UI/NewGoldStyle/M_UI_ElderWheelchair.M_UI_ElderWheelchair"));
	FallButton = AddStateAction(TEXT("ElderActionFall"), TEXT("让选中老人跌倒并保持倒地姿势"), 160.f, TEXT("/Game/UI/NewGoldStyle/M_UI_ElderFall.M_UI_ElderFall"));
	StandButton = AddStateAction(TEXT("ElderActionStand"), TEXT("让选中老人退出轮椅并站立"), 24.f, TEXT("/Game/UI/NewGoldStyle/T_UI_ElderStand.T_UI_ElderStand"));
	WheelchairButton->OnClicked.AddDynamic(this, &UElderDebugControlWidget::HandleWheelchair);
	FallButton->OnClicked.AddDynamic(this, &UElderDebugControlWidget::HandleFall);
	StandButton->OnClicked.AddDynamic(this, &UElderDebugControlWidget::HandleStand);
	Status = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ElderControlStatus"));
	Status->SetText(FText::FromString(TEXT("先从下拉框选人；接管后 WASD 移动，按住 Alt 操作 UI，Tab 退出")));
	Status->SetAutoWrapText(true);
	Status->SetFont(DebugFont);
	Status->SetColorAndOpacity(GoldSecondaryText);
	Status->SetVisibility(ESlateVisibility::HitTestInvisible);
	UCanvasPanelSlot* StatusSlot = Canvas->AddChildToCanvas(Status);
	StatusSlot->SetPosition(FVector2D(24.f, 430.f));
	StatusSlot->SetSize(FVector2D(340.f, 70.f));
	// Keep this existing panel beside (not over) the main UI's left navigation rail.
	for (UWidget* Child : Canvas->GetAllChildren())
		if (UCanvasPanelSlot* ChildSlot = Cast<UCanvasPanelSlot>(Child->Slot))
			ChildSlot->SetPosition(ChildSlot->GetPosition() + FVector2D(120.f, 38.f));
	return Super::RebuildWidget();
}

void UElderDebugControlWidget::RefreshElders()
{
	AOrbitCameraController* Controller = Cast<AOrbitCameraController>(GetOwningPlayer());
	if (!Controller || !ElderPicker) return;
	TGuardValue<bool> RefreshGuard(bRefreshing, true);
	ElderPicker->ClearOptions();
	Options.Reset();
	const FString Prompt = TEXT("请选择老人");
	ElderPicker->AddOption(Prompt);
	FString Selected = Prompt;
	for (AElderlyAICharacter* Elder : Controller->GetAvailableElders())
	{
		const FString Option = FString::Printf(TEXT("%s [%s]"), *Elder->ElderName, *Elder->GetName());
		Options.Add(Option, Elder);
		ElderPicker->AddOption(Option);
		if (Elder == Controller->SelectedElder) Selected = Option;
	}
	ElderPicker->SetSelectedOption(Selected);
}

void UElderDebugControlWidget::HandleSelection(FString Item, ESelectInfo::Type SelectionType)
{
	if (bRefreshing) return;
	if (AOrbitCameraController* Controller = Cast<AOrbitCameraController>(GetOwningPlayer()))
	{
		const TWeakObjectPtr<AElderlyAICharacter>* Elder = Options.Find(Item);
		Controller->SetSelectedElder(Elder ? Elder->Get() : nullptr);
	}
}

void UElderDebugControlWidget::HandleTakeover()
{
	if (AOrbitCameraController* Controller = Cast<AOrbitCameraController>(GetOwningPlayer()))
	{
		Controller->TakeControlOfSelectedElder();
		Status->SetText(FText::FromString(Controller->ElderControlMessage));
	}
}

void UElderDebugControlWidget::HandleRelease()
{
	if (AOrbitCameraController* Controller = Cast<AOrbitCameraController>(GetOwningPlayer()))
	{
		Controller->ReleaseElderControl();
		Status->SetText(FText::FromString(Controller->ElderControlMessage));
	}
}
void UElderDebugControlWidget::HandleToggleClicked()
{
	if (AOrbitCameraController* Controller = Cast<AOrbitCameraController>(GetOwningPlayer())) Controller->ToggleSelectedElderSightCone();
}

void UElderDebugControlWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (AOrbitCameraController* Controller = Cast<AOrbitCameraController>(GetOwningPlayer()))
	{
		Controller->OnPossessedPawnChanged.AddUniqueDynamic(this, &UElderDebugControlWidget::HandlePawnChanged);
		Controller->OnSelectedElderChanged.AddUniqueDynamic(this, &UElderDebugControlWidget::HandleSelectedElderChanged);
		HandleSelectedElderChanged(Controller->SelectedElder);
	}
}

void UElderDebugControlWidget::NativeDestruct()
{
	if (AOrbitCameraController* Controller = Cast<AOrbitCameraController>(GetOwningPlayer()))
	{
		Controller->OnPossessedPawnChanged.RemoveDynamic(this, &UElderDebugControlWidget::HandlePawnChanged);
		Controller->OnSelectedElderChanged.RemoveDynamic(this, &UElderDebugControlWidget::HandleSelectedElderChanged);
	}
	UnbindObservedElder();
	Super::NativeDestruct();
}

void UElderDebugControlWidget::UnbindObservedElder()
{
	if (AElderlyAICharacter* Elder = ObservedElder.Get())
	{
		Elder->OnElderStateChanged.RemoveDynamic(this, &UElderDebugControlWidget::HandleElderStateChanged);
		Elder->OnDestroyed.RemoveDynamic(this, &UElderDebugControlWidget::HandleElderDestroyed);
	}
	ObservedElder.Reset();
}

void UElderDebugControlWidget::HandleSelectedElderChanged(AElderlyAICharacter* Elder)
{
	UnbindObservedElder();
	if (IsValid(Elder))
	{
		ObservedElder = Elder;
		Elder->OnElderStateChanged.AddUniqueDynamic(this, &UElderDebugControlWidget::HandleElderStateChanged);
		Elder->OnDestroyed.AddUniqueDynamic(this, &UElderDebugControlWidget::HandleElderDestroyed);
	}
	RefreshElders();
	RefreshStateActions();
}

void UElderDebugControlWidget::HandleElderStateChanged(AActor* Elder, const FString& Name, EElderState Previous, EElderState Current)
{
	RefreshStateActions();
}

void UElderDebugControlWidget::HandleElderDestroyed(AActor* Elder)
{
	UnbindObservedElder();
	RefreshStateActions();
}

void UElderDebugControlWidget::RefreshStateActions()
{
	const AElderlyAICharacter* Elder = ObservedElder.Get();
	const bool bStanding = Elder && Elder->CurrentState == EElderState::Standing;
	const bool bWheelchair = Elder && (Elder->CurrentState == EElderState::WheelchairIdle || Elder->CurrentState == EElderState::WheelchairMoving);
	if (WheelchairButton) WheelchairButton->SetVisibility(bStanding ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	if (FallButton) FallButton->SetVisibility(bStanding && Elder->bEnableFallTrigger ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	const bool bFalling = Elder && Elder->CurrentState == EElderState::Falling;
	const bool bFallen = Elder && Elder->CurrentState == EElderState::Fallen;
	const bool bGettingUp = Elder && Elder->CurrentState == EElderState::GettingUp;
	if (StandButton)
	{
		StandButton->SetVisibility((bWheelchair || bFalling || bFallen || bGettingUp) ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		StandButton->SetIsEnabled(bWheelchair || (bFallen && Elder->GetUpAnimation));
		const TCHAR* Tip = bFalling ? TEXT("跌倒动作进行中") : bGettingUp ? TEXT("正在起身") : bFallen ?
			(Elder->GetUpAnimation ? TEXT("让选中老人起身") : TEXT("站立：缺少起身动画")) : TEXT("让选中老人退出轮椅并站立");
		StandButton->SetToolTipText(FText::FromString(Tip));
	}
}

void UElderDebugControlWidget::HandleWheelchair()
{
	if (AOrbitCameraController* Controller = Cast<AOrbitCameraController>(GetOwningPlayer()))
		if (AElderlyAICharacter* Elder = Controller->SelectedElder; IsValid(Elder) && Elder->CurrentState == EElderState::Standing)
			Elder->EnterWheelchair();
}

void UElderDebugControlWidget::HandleFall()
{
	if (AOrbitCameraController* Controller = Cast<AOrbitCameraController>(GetOwningPlayer()))
		if (AElderlyAICharacter* Elder = Controller->SelectedElder; IsValid(Elder) && Elder->bEnableFallTrigger && Elder->CurrentState == EElderState::Standing)
			Elder->StartFalling();
}

void UElderDebugControlWidget::HandleStand()
{
	if (AOrbitCameraController* Controller = Cast<AOrbitCameraController>(GetOwningPlayer()))
		if (AElderlyAICharacter* Elder = Controller->SelectedElder; IsValid(Elder))
		{
			if (Elder->CurrentState == EElderState::WheelchairIdle || Elder->CurrentState == EElderState::WheelchairMoving) Elder->ExitWheelchair();
			else if (Elder->CurrentState == EElderState::Fallen) Elder->StartGettingUp();
		}
}

void UElderDebugControlWidget::HandlePawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	if (!Status) return;
	const bool bElder = Cast<AElderlyAICharacter>(NewPawn) != nullptr;
	Status->SetText(FText::FromString(bElder ? TEXT("已接管：WASD 移动，鼠标转向，按住 Alt 操作 UI，Tab 返回观察") : TEXT("观察模式：选择老人后可接管")));
}
