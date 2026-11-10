#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/GameViewportClient.h"
#include "EngineUtils.h"
#include "InputKeyEventArgs.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SViewport.h"
#include "Widgets/SWindow.h"
#include "OrbitCameraController.h"
#include "ElderlyAICharacter.h"

class FFallKeyScenario : public IAutomationLatentCommand
{
public:
	explicit FFallKeyScenario(FAutomationTestBase* T): Test(T), Deadline(FPlatformTime::Seconds()+20.) {}
	bool Update() override
	{
		if (FPlatformTime::Seconds()>Deadline) { Test->AddError(TEXT("Fall input timeout")); return true; }
		if (!World.IsValid())
		{
			for (const FWorldContext& C:GEngine->GetWorldContexts()) if(C.WorldType==EWorldType::PIE) World=C.World();
			if(!World.IsValid()) return false;
			PC=Cast<AOrbitCameraController>(World->GetFirstPlayerController());
			for(TActorIterator<AElderlyAICharacter> I(World.Get());I;++I) { Elder=*I;break; }
			if(!PC.IsValid() || !Elder.IsValid()) { Test->AddError(TEXT("Missing Showcase elder/controller")); return true; }
			Elder->SetStanding(); PC->TakeControlOfElder(Elder.Get());
			auto& Slate=FSlateApplication::Get();
			if(auto Window=Slate.FindWidgetWindow(World->GetGameViewport()->GetGameViewportWidget().ToSharedRef())) Window->BringToFront(true);
			Slate.SetUserFocusToGameViewport(0);
			Next=World->GetTimeSeconds()+1.f;return false;
		}
		if(World->GetTimeSeconds()<Next)return false;
		if(Step++==0)
		{
			auto& Slate=FSlateApplication::Get();
			Slate.SetKeyboardFocus(World->GetGameViewport()->GetGameViewportWidget(),EFocusCause::SetDirectly);
			UE_LOG(LogTemp,Display,TEXT("FallKey fixture active=%d focus=%d state=%d controlling=%d cursorMode=%d"),Slate.IsActive(),World->GetGameViewport()->GetGameViewportWidget()->HasKeyboardFocus(),int32(Elder->CurrentState),PC->IsControllingElder(),PC->IsElderUICursorActive());
			SlateKey(EKeys::C, true);
			Next=World->GetTimeSeconds()+.15f;return false;
		}
		SlateKey(EKeys::C, false);
		UE_LOG(LogTemp,Display,TEXT("FallKey resulting state=%d"),int32(Elder->CurrentState));
		Test->TestTrue(TEXT("C key travels through Enhanced Input into Falling"),Elder->CurrentState==EElderState::Falling);
		PC->ReleaseElderControl();return true;
	}
private:
	void SlateKey(FKey Key, bool bDown)
	{
		const FKeyEvent Event(Key, FModifierKeysState(), 0, false, 0, 0);
		if (bDown) FSlateApplication::Get().ProcessKeyDownEvent(Event);
		else FSlateApplication::Get().ProcessKeyUpEvent(Event);
	}
	FAutomationTestBase* Test;double Deadline;float Next=0;int32 Step=0;
	TWeakObjectPtr<UWorld> World;TWeakObjectPtr<AOrbitCameraController> PC;TWeakObjectPtr<AElderlyAICharacter> Elder;
};
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFallKeyTest,"OldMan.Showcase.FallKey",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FFallKeyTest::RunTest(const FString&)
{
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FFallKeyScenario(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());return true;
}
#endif
