// BossDiscoveryTrigger.cpp

#include "BossDiscoveryTrigger.h"

#include "BossSpawner.h"
#include "Components/BoxComponent.h"
#include "DialogueSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Portal.h"
#include "TimerManager.h"

ABossDiscoveryTrigger::ABossDiscoveryTrigger()
{
	PrimaryActorTick.bCanEverTick = false;

	Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
	RootComponent = Trigger;

	Trigger->SetBoxExtent(FVector(300.f, 300.f, 200.f));
	Trigger->SetCollisionProfileName(TEXT("Trigger"));

	// 차단벽 — 플레이어(Pawn)만 막고 나머지 채널은 무시. 시작 시 켜져 있다가
	// 접촉 후 BlockDuration 초 뒤 ReleaseBlock()에서 해제된다.
	BlockingWall = CreateDefaultSubobject<UBoxComponent>(TEXT("BlockingWall"));
	BlockingWall->SetupAttachment(RootComponent);
	BlockingWall->SetBoxExtent(FVector(100.f, 300.f, 200.f));
	BlockingWall->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	BlockingWall->SetCollisionObjectType(ECC_WorldStatic);
	BlockingWall->SetCollisionResponseToAllChannels(ECR_Ignore);
	BlockingWall->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
}

void ABossDiscoveryTrigger::BeginPlay()
{
	Super::BeginPlay();

	Trigger->OnComponentBeginOverlap.AddDynamic(this, &ABossDiscoveryTrigger::OnOverlapBegin);
}

void ABossDiscoveryTrigger::OnOverlapBegin(
	UPrimitiveComponent* OverlappedComp,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	// 플레이어만 반응
	if (OtherActor && OtherActor->ActorHasTag("Player"))
	{
		Activate();
	}
}

void ABossDiscoveryTrigger::Activate()
{
	// 1회만 발동. 차단 해제 후 재진입(또는 포탈 루프로 돌아옴)해도 같은 박스는 다시 안 터진다.
	if (bTriggered)
	{
		return;
	}
	bTriggered = true;

	// 대사 (보스 스폰 대사와 같은 DialogueSubsystem 경로 사용)
	if (!DialogueID.IsNone())
	{
		if (UGameInstance* GI = GetGameInstance())
		{
			if (UDialogueSubsystem* DS = GI->GetSubsystem<UDialogueSubsystem>())
			{
				DS->ShowDialogue(DialogueID);
			}
		}
	}

	// 울음/효과음
	if (RoarSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, RoarSound, GetActorLocation(), RoarVolume);
	}

	// 포탈 오픈 (날짜 무관, 접촉으로 오픈)
	for (APortal* Portal : PortalsToOpen)
	{
		if (Portal)
		{
			Portal->OpenByDiscovery();
		}
	}

	// 보스 스포너 활성화 — 이후(또는 즉시 겹쳐 있으면) 보스 스폰 허용
	for (ABossSpawner* Spawner : SpawnersToActivate)
	{
		if (Spawner)
		{
			Spawner->ActivateSpawn();
		}
	}

	// 차단 유지 후 해제 (BlockDuration 0이면 즉시 해제)
	if (BlockDuration > 0.f)
	{
		GetWorld()->GetTimerManager().SetTimer(
			BlockReleaseTimer,
			this,
			&ABossDiscoveryTrigger::ReleaseBlock,
			BlockDuration,
			false);
	}
	else
	{
		ReleaseBlock();
	}

	UE_LOG(LogTemp, Log,
		TEXT("[BossDiscoveryTrigger] Activated — dialogue:%s, %d portal(s), %d spawner(s), block %.1fs."),
		*DialogueID.ToString(), PortalsToOpen.Num(), SpawnersToActivate.Num(), BlockDuration);
}

void ABossDiscoveryTrigger::ReleaseBlock()
{
	// 차단벽 해제 — 플레이어가 통과해 다음 구간(포탈/보스)으로 진행할 수 있게 된다.
	if (BlockingWall)
	{
		BlockingWall->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}
