#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Tests/AutomationEditorCommon.h"
#include "Editor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Components/SceneComponent.h"
#include "WheelchairWheelMotionComponent.h"

class FWheelchairRollingScenario : public IAutomationLatentCommand
{
public:
	explicit FWheelchairRollingScenario(FAutomationTestBase* InTest) : Test(InTest), Deadline(FPlatformTime::Seconds() + 90.) {}
	bool Update() override
	{
		if (FPlatformTime::Seconds() > Deadline) { Test->AddError(TEXT("Wheel rolling PIE timed out")); return true; }
		if (!World.IsValid())
		{
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
				if (Context.WorldType == EWorldType::PIE) World = Context.World();
			if (!World.IsValid()) return false;
		}
		if (World->GetTimeSeconds() < Next) return false;
		if (Step == 0)
		{
			UClass* Class = LoadClass<AActor>(nullptr, TEXT("/Game/Project_File/BP/Actor/BP_WheelChair.BP_WheelChair_C"));
			FActorSpawnParameters Params;
			Params.ObjectFlags |= RF_Transient;
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			Chair = World->SpawnActor<AActor>(Class, FVector(0, 0, 1500), FRotator::ZeroRotator, Params);
			Other = World->SpawnActor<AActor>(Class, FVector(300, 0, 1500), FRotator::ZeroRotator, Params);
			if (!Chair.IsValid() || !Other.IsValid()) { Test->AddError(TEXT("Cannot spawn saved wheelchair BP")); return true; }
			Motion = Chair->FindComponentByClass<UWheelchairWheelMotionComponent>();
			OtherMotion = Other->FindComponentByClass<UWheelchairWheelMotionComponent>();
			if (!Motion.IsValid() || !OtherMotion.IsValid()) { Test->AddError(TEXT("Saved wheelchair missing wheel motion component")); return true; }
			for (FName Name : Names) Test->TestTrue(*FString::Printf(TEXT("Measured radius for %s"), *Name.ToString()), Motion->GetWheelRadiusCm(Name) > 10.f);
			Test->TestTrue(TEXT("Big model radius 23.167cm"), FMath::IsNearlyEqual(Motion->GetWheelRadiusCm(Names[0]), 23.1672f, .01f));
			Test->TestTrue(TEXT("Small model radius 11.200cm"), FMath::IsNearlyEqual(Motion->GetWheelRadiusCm(Names[2]), 11.2002f, .01f));
			Distance = Motion->GetWheelRadiusCm(Names[0]) * PI / 2.f;
			TInlineComponentArray<USceneComponent*> Components(Chair.Get());
			for (USceneComponent* Component : Components) if (Component->GetFName() == TEXT("Body")) Body = Component;
			if (!Body.IsValid()) { Test->AddError(TEXT("Missing Body")); return true; }
			BodyRest = Body->GetRelativeTransform();
		}
		else if (Step == 1) Move(FVector(0, Distance, 0));
		else if (Step == 2)
		{
			for (FName Name : Names) Near(Name, -FMath::RadiansToDegrees(Distance / Motion->GetWheelRadiusCm(Name)), TEXT("Distance/radius rolling"));
			Move(FVector(0, -Distance, 0));
		}
		else if (Step == 3) { AllZero(TEXT("Reverse cancels forward")); }
		else if (Step == 4) { AllZero(TEXT("Stationary wheels stop")); Move(FVector(0, 0, 50)); }
		else if (Step == 5) { AllZero(TEXT("Vertical movement does not roll")); Move(FVector(0, 1000, 0)); }
		else if (Step == 6) { AllZero(TEXT("Teleport does not spin")); Chair->AddActorWorldRotation(FRotator(0, 5, 0)); }
		else if (Step == 7)
		{
			Test->TestTrue(TEXT("Pivot turn drives big wheels in opposite directions"), Motion->GetWheelRollDegrees(Names[0]) * Motion->GetWheelRollDegrees(Names[1]) < 0.f);
			for (FName Name : Names) Before.Add(Name, Motion->GetWheelRollDegrees(Name));
			Motion->SetWheelMotionEnabled(false); Move(FVector(0, Distance, 0));
		}
		else if (Step == 8)
		{
			for (FName Name : Names) Near(Name, Before[Name], TEXT("Disabled"));
			Motion->SetWheelMotionEnabled(true);
		}
		else if (Step == 9) { Chair->SetActorHiddenInGame(true); Move(FVector(0, Distance, 0)); }
		else if (Step == 10) { Chair->SetActorHiddenInGame(false); }
		else if (Step == 11)
		{
			for (FName Name : Names) Near(Name, Before[Name], TEXT("Hidden travel discarded"));
			Chair->SetActorRotation(FRotator::ZeroRotator);
			Chair->SetActorScale3D(FVector(2));
			Motion->ResetWheelMotion(true);
		}
		else if (Step == 12) Move(FVector(0, 2 * Distance, 0));
		else if (Step == 13)
		{
			Near(Names[0], -90, TEXT("Uniform scale doubles circumference"));
			Near(Names[1], -90, TEXT("Both scaled big wheels"));
			for (FName Name : Names) Test->TestEqual(TEXT("Other chair independent"), OtherMotion->GetWheelRollDegrees(Name), 0.f);
			Test->TestTrue(TEXT("Body relative transform preserved"), Body->GetRelativeTransform().Equals(BodyRest));
			// ChildActor-style attachment: the wheel owner has no velocity of its own.
			Chair->AttachToActor(Other.Get(), FAttachmentTransformRules::KeepWorldTransform);
			Motion->ResetWheelMotion(true);
		}
		else if (Step == 14) Other->AddActorWorldOffset(FVector(0, 2 * Distance, 0));
		else if (Step == 15)
		{
			Near(Names[0], -90, TEXT("Attached chair follows actual world travel"));
			Near(Names[1], -90, TEXT("Attached chair second wheel"));
			Chair->Destroy(); Other->Destroy();
			return true;
		}
		++Step; Next = World->GetTimeSeconds() + .15f;
		return false;
	}
private:
	void Move(FVector Offset) { Chair->AddActorWorldOffset(Offset); }
	void Near(FName Name, float Expected, const TCHAR* Label)
	{
		const float Actual = Motion->GetWheelRollDegrees(Name);
		Test->TestTrue(*FString::Printf(TEXT("%s %s actual=%.4f expected=%.4f"), Label, *Name.ToString(), Actual, Expected), FMath::Abs(FMath::FindDeltaAngleDegrees(Actual, Expected)) < .1f);
	}
	void AllZero(const TCHAR* Label) { for (FName Name : Names) Near(Name, 0, Label); }
	FAutomationTestBase* Test;
	TWeakObjectPtr<UWorld> World;
	TWeakObjectPtr<AActor> Chair, Other;
	TWeakObjectPtr<USceneComponent> Body;
	TWeakObjectPtr<UWheelchairWheelMotionComponent> Motion, OtherMotion;
	TArray<FName> Names = {TEXT("L_BigWheel"), TEXT("R_BigWheel"), TEXT("L_LittleWheel"), TEXT("R_LittleWheel")};
	TMap<FName, float> Before;
	FTransform BodyRest;
	double Deadline;
	float Next = 2.f, Distance = 0.f;
	int32 Step = 0;
};

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FWheelchairWheelMotionTest, "OldMan.Wheelchair.Rolling", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWheelchairWheelMotionTest::RunTest(const FString&)
{
	if (GEditor->PlayWorld) { AddError(TEXT("Stop PIE before running wheel test")); return false; }
	ADD_LATENT_AUTOMATION_COMMAND(FStartPIECommand(false));
	ADD_LATENT_AUTOMATION_COMMAND(FWheelchairRollingScenario(this));
	ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
	return true;
}
#endif
