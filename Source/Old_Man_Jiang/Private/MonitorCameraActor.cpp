#include "MonitorCameraActor.h"
#include "ElderlyAICharacter.h"
#include "MonitorableActorInterface.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"

AMonitorCameraActor::AMonitorCameraActor()
{
	PrimaryActorTick.bCanEverTick = false;
	MonitorRoot = CreateDefaultSubobject<USceneComponent>(TEXT("MonitorRoot")); SetRootComponent(MonitorRoot);
	MonitorRange = CreateDefaultSubobject<USphereComponent>(TEXT("MonitorRange")); MonitorRange->SetupAttachment(MonitorRoot);
	MonitorRange->SetCollisionEnabled(ECollisionEnabled::QueryOnly); MonitorRange->SetCollisionResponseToAllChannels(ECR_Ignore); MonitorRange->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	MonitorRange->SetGenerateOverlapEvents(true); MonitorRange->SetHiddenInGame(true);
}
void AMonitorCameraActor::BeginPlay()
{
	Super::BeginPlay();
	MonitorRange->OnComponentBeginOverlap.AddDynamic(this, &AMonitorCameraActor::HandleMonitorBegin);
	MonitorRange->OnComponentEndOverlap.AddDynamic(this, &AMonitorCameraActor::HandleMonitorEnd);
	MonitorRange->SetSphereRadius(MonitorRangeRadius);
	// Placed elders may already overlap before BeginPlay binds the delegates.
	TArray<AActor*> InitialElders;
	MonitorRange->GetOverlappingActors(InitialElders, AElderlyAICharacter::StaticClass());
	for (AActor* Elder : InitialElders) RegisterElder(Cast<AElderlyAICharacter>(Elder));
}
void AMonitorCameraActor::HandleMonitorBegin(UPrimitiveComponent*, AActor* Other, UPrimitiveComponent*, int32, bool, const FHitResult&) { RegisterElder(Cast<AElderlyAICharacter>(Other)); }
void AMonitorCameraActor::HandleMonitorEnd(UPrimitiveComponent*, AActor* Other, UPrimitiveComponent*, int32) { UnregisterElder(Cast<AElderlyAICharacter>(Other)); }
void AMonitorCameraActor::RegisterElder(AElderlyAICharacter* Elder)
{
	if (!Elder || MonitoredElders.Contains(Elder)) return;
	MonitoredElders.Add(Elder); Elder->OnElderStateChanged.AddUniqueDynamic(this, &AMonitorCameraActor::HandleElderStateChanged);
	AppendState(Elder, IMonitorableActorInterface::Execute_GetMonitorName(Elder), Elder->CurrentState, Elder->CurrentState, true);
}
void AMonitorCameraActor::UnregisterElder(AElderlyAICharacter* Elder)
{
	if (!Elder || !MonitoredElders.Remove(Elder)) return;
	Elder->OnElderStateChanged.RemoveDynamic(this, &AMonitorCameraActor::HandleElderStateChanged);
}
void AMonitorCameraActor::HandleElderStateChanged(AActor* ElderActor, const FString& ElderName, EElderState OldState, EElderState NewState)
{
	if (OldState != NewState && MonitoredElders.Contains(Cast<AElderlyAICharacter>(ElderActor))) AppendState(ElderActor, ElderName, OldState, NewState, false);
}
void AMonitorCameraActor::AppendState(AActor* ElderActor, const FString& Name, EElderState OldState, EElderState NewState, bool bInitial)
{
	const FString State = IMonitorableActorInterface::Execute_GetMonitorStateString(ElderActor);
	CurrentDetectedStateString = FString::Printf(TEXT("老人：%s | 状态：%s"), *Name, *State);
	const FString OldText = bInitial ? TEXT("进入监控") : StaticEnum<EElderState>()->GetDisplayNameTextByValue((int64)OldState).ToString();
	const FString Line = FString::Printf(TEXT("%s | %s | %s -> %s"), *FDateTime::Now().ToString(TEXT("%H:%M:%S")), *Name, *OldText, *State);
	StateHistory.Add(Line); StateHistoryText = FString::Join(StateHistory, TEXT("\n"));
}
