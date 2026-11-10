#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "Editor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"
#include "Components/StaticMeshComponent.h"
#include "Components/LightComponent.h"
#include "OrbitCameraController.h"
#include "ElderlyAICharacter.h"
#include "AIController.h"

// Tests the saved Showcase membership through real possession, without saving PIE changes.
class FElderCeilingScenario : public IAutomationLatentCommand
{
public:
	explicit FElderCeilingScenario(FAutomationTestBase* InTest) : Test(InTest), Deadline(FPlatformTime::Seconds() + 45.) {}
	bool Update() override
	{
		if (FPlatformTime::Seconds() > Deadline) { Test->AddError(TEXT("Ceiling PIE timed out")); return true; }
		if (!World.IsValid())
		{
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
				if (Context.WorldType == EWorldType::PIE && Context.World() && Context.World()->GetMapName().Contains(TEXT("Showcase"))) World = Context.World();
			if (!World.IsValid()) return false;
		}
		if (World->GetTimeSeconds() < 3.f || World->GetTimeSeconds() < Next) return false;
		// Weather and alarm lights can legitimately animate between frames. Compare
		// each batch of possession operations against this frame, not a frozen clock.
		for (auto& Pair : Lights) if (Pair.Key.IsValid()) Pair.Value = Pair.Key->Intensity;
		if (Step == 0)
		{
			PC = Cast<AOrbitCameraController>(World->GetFirstPlayerController());
			if (!PC.IsValid()) { Test->AddError(TEXT("Missing project controller")); return true; }
			Observer = PC->GetPawn();
			for (TActorIterator<AElderlyAICharacter> It(World.Get()); It; ++It) { Elder = *It; break; }
			if (!Elder.IsValid() || !Observer.IsValid()) { Test->AddError(TEXT("Missing elder/observer")); return true; }
			for (TActorIterator<AStaticMeshActor> It(World.Get()); It; ++It)
			{
				if (It->ActorHasTag(PC->ElderCeilingTag))
				{
					Ceilings.Add(*It);
					Collision.Add(It->GetStaticMeshComponent()->GetCollisionEnabled());
					ActorCollision.Add(It->GetActorEnableCollision());
				}
				else Untouched.Add(*It, It->IsHidden());
			}
			Test->TestEqual(TEXT("Exact saved ceiling group membership"), Ceilings.Num(), 115);
			for (TActorIterator<AActor> It(World.Get()); It; ++It)
			{
				TInlineComponentArray<ULightComponent*> Components(*It);
				for (ULightComponent* Light : Components) Lights.Add(Light, Light->Intensity);
			}
			Check(false, TEXT("Initial observation"));
			Test->TestFalse(TEXT("Null takeover rejected"), PC->TakeControlOfElder(nullptr));
			Check(false, TEXT("Failed takeover remains hidden"));
			Test->TestTrue(TEXT("First takeover"), PC->TakeControlOfElder(Elder.Get()));
			Check(true, TEXT("First takeover shows ceiling"));
		}
		else if (Step == 1)
		{
			Check(true, TEXT("Ceiling stays visible across frames"));
			Test->TestTrue(TEXT("Repeated takeover is idempotent"), PC->TakeControlOfElder(Elder.Get()));
			PC->SetSelectedElder(nullptr);
			Check(true, TEXT("Selection is not control owner"));
			Test->TestTrue(TEXT("Return to observation"), PC->ReleaseElderControl());
			Test->TestTrue(TEXT("Original observer restored"), PC->GetPawn() == Observer.Get());
			Check(false, TEXT("Return hides ceiling"));
			Test->TestFalse(TEXT("Repeated return rejected"), PC->ReleaseElderControl());
			Test->TestTrue(TEXT("Second takeover"), PC->TakeControlOfElder(Elder.Get()));
			Check(true, TEXT("Second takeover shows ceiling"));
		}
		else if (Step == 2)
		{
			FActorSpawnParameters Params; Params.ObjectFlags |= RF_Transient;
			AAIController* Blocker = World->SpawnActor<AAIController>(AAIController::StaticClass(), Params);
			if (!Blocker) { Test->AddError(TEXT("Failed to create return-failure fixture")); return true; }
			Blocker->Possess(Observer.Get());
			Test->TestFalse(TEXT("Occupied observer blocks return"), PC->ReleaseElderControl());
			Check(true, TEXT("Failed return keeps ceiling visible"));
			Blocker->UnPossess(); Blocker->Destroy();
			Test->TestTrue(TEXT("Return after blocker removed"), PC->ReleaseElderControl());
			Check(false, TEXT("Second return hides ceiling"));
			Test->TestTrue(TEXT("Takeover before elder destruction"), PC->TakeControlOfElder(Elder.Get()));
			UClass* ElderClass = Elder->GetClass();
			Elder->Destroy();
			Check(false, TEXT("Destroyed elder hides ceiling"));
			Test->TestTrue(TEXT("Destroyed elder restores observation"), PC->GetPawn() == Observer.Get());
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			Elder = World->SpawnActor<AElderlyAICharacter>(ElderClass, FVector(0, 0, 500), FRotator::ZeroRotator, Params);
			if (!Elder.IsValid()) { Test->AddError(TEXT("Failed to create second elder fixture")); return true; }
			Test->TestTrue(TEXT("Takeover another elder instance"), PC->TakeControlOfElder(Elder.Get()));
			Check(true, TEXT("Another elder shows same ceiling group"));
		}
		else if (Step == 3)
		{
			FActorSpawnParameters Params; Params.ObjectFlags |= RF_Transient;
			AStaticMeshActor* Added = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Params);
			if (!Added) { Test->AddError(TEXT("Failed to create runtime ceiling fixture")); return true; }
			Added->Tags.Add(PC->ElderCeilingTag); Added->SetActorHiddenInGame(true);
			PC->RefreshElderCeilingVisibility();
			Test->TestFalse(TEXT("Explicit refresh includes newly tagged mesh"), Added->IsHidden());
			Ceilings.Add(Added); Collision.Add(Added->GetStaticMeshComponent()->GetCollisionEnabled()); ActorCollision.Add(Added->GetActorEnableCollision());
			PC->UnPossess();
			Check(false, TEXT("Direct unpossess hides ceiling"));
			PC->Possess(Elder.Get());
			Check(true, TEXT("Possess restores current takeover visibility"));
			// A deleted target is skipped safely, and no editor actor is deleted.
			Ceilings.Last()->Destroy();
			PC->RefreshElderCeilingVisibility();
			Check(true, TEXT("Deleted ceiling target tolerated"));
			Observer->Destroy();
			Test->TestFalse(TEXT("Deleted observer blocks return"), PC->ReleaseElderControl());
			Check(true, TEXT("Deleted observer does not hide a still controlled elder"));
			PC->Destroy();
			Check(false, TEXT("Controller EndPlay hides ceiling"));
			Test->AddInfo(TEXT("CEILING_PASS: 115 saved members; repeated takeover/return, invalid requests, selection, elder/controller destruction; unrelated meshes, collisions and light intensities preserved."));
			return true;
		}
		++Step; Next = World->GetTimeSeconds() + .2f;
		return false;
	}
private:
	void Check(bool bVisible, const TCHAR* Phase)
	{
		for (int32 Index = 0; Index < Ceilings.Num(); ++Index)
		{
			AStaticMeshActor* Actor = Ceilings[Index].Get();
			if (!IsValid(Actor)) continue;
			Test->TestEqual(FString::Printf(TEXT("%s: %s hidden"), Phase, *Actor->GetName()), Actor->IsHidden(), !bVisible);
			Test->TestEqual(TEXT("Actor collision preserved"), Actor->GetActorEnableCollision(), ActorCollision[Index]);
			Test->TestEqual(TEXT("Component collision preserved"), Actor->GetStaticMeshComponent()->GetCollisionEnabled(), Collision[Index]);
			Test->TestTrue(TEXT("Mesh visible flag preserved"), Actor->GetStaticMeshComponent()->IsVisible());
			Test->TestFalse(TEXT("Mesh hidden flag preserved"), Actor->GetStaticMeshComponent()->bHiddenInGame);
		}
		for (const auto& Pair : Untouched) if (Pair.Key.IsValid()) Test->TestEqual(TEXT("Unrelated mesh hidden state preserved"), Pair.Key->IsHidden(), Pair.Value);
		for (const auto& Pair : Lights) if (Pair.Key.IsValid()) Test->TestEqual(TEXT("Light intensity preserved"), Pair.Key->Intensity, Pair.Value);
	}
	FAutomationTestBase* Test;
	double Deadline;
	TWeakObjectPtr<UWorld> World;
	TWeakObjectPtr<AOrbitCameraController> PC;
	TWeakObjectPtr<APawn> Observer;
	TWeakObjectPtr<AElderlyAICharacter> Elder;
	TArray<TWeakObjectPtr<AStaticMeshActor>> Ceilings;
	TArray<ECollisionEnabled::Type> Collision;
	TArray<bool> ActorCollision;
	TMap<TWeakObjectPtr<AStaticMeshActor>, bool> Untouched;
	TMap<TWeakObjectPtr<ULightComponent>, float> Lights;
	int32 Step = 0;
	float Next = 0.f;
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FElderCeilingTest, "OldMan.Showcase.CeilingVisibility", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FElderCeilingTest::RunTest(const FString& Parameters)
{
	if (!GEditor || GEditor->PlayWorld || !GEditor->GetEditorWorldContext().World()->GetMapName().Contains(TEXT("Showcase")))
	{
		AddError(TEXT("Open Showcase with PIE stopped first.")); return false;
	}
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	FAutomationTestFramework::Get().EnqueueLatentCommand(MakeShared<FElderCeilingScenario>(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	return true;
}
#endif
