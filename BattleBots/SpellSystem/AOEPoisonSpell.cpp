// // Copyright 2015 VMR Games, Inc. All Rights Reserved.

#include "BattleBots.h"
#include "AOEPoisonSpell.h"




AAOEPoisonSpell::AAOEPoisonSpell()
  :Super()
{
  // Set to true to start destruction timer at instantiation.
  spellDataInfo.bIsPiercing = true;

  AoeTickInterval = 0.2;

  collisionComp->InitSphereRadius(200.f);
  collisionComp->SetCollisionProfileName(FName(TEXT("OverlapAll")));
}

void AAOEPoisonSpell::PostInitializeComponents()
{
  Super::PostInitializeComponents();

  if (HasAuthority())
  {
    // Sets the dmg done per tick
    SetDamageToDeal(ProcessElementalDmg(GetPreProcessedDotDamage()));
  }
}

void AAOEPoisonSpell::BeginPlay()
{
  Super::BeginPlay();

  if (HasAuthority())
  {
    // Enables AOE Tick on spawn
    GetWorldTimerManager().SetTimer(AOETickHandler, this, &AAOEPoisonSpell::AOETick, AoeTickInterval, true);
  }
}

void AAOEPoisonSpell::OnCollisionOverlapBegin(class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
  // Because the damage is managed in AOETick, we need to override this method to prevent double processing
  ABBotCharacter* enemyPlayer = Cast<ABBotCharacter>(OtherActor);

  if (IsEnemy(enemyPlayer)) {
    if (!OverlappedActors.Contains(enemyPlayer))
    {
      OverlappedActors.AddUnique(enemyPlayer);
    }
  }
}

void AAOEPoisonSpell::DestroySpell()
{
  /* The spell gets automatically destroyed after spellDuration. */
  GetWorldTimerManager().ClearTimer(AOETickHandler);
}

void AAOEPoisonSpell::SimulateExplosion_Implementation()
{
  SetActorEnableCollision(false);
  SetActorHiddenInGame(true);
}

FVector AAOEPoisonSpell::GetSpellSpawnLocation()
{
  return spellSpawnLocation;
}

void AAOEPoisonSpell::DealDamage(ABBotCharacter* enemyPlayer)
{
  if (HasAuthority())
  {
    // Deal damage only on the server
    UGameplayStatics::ApplyDamage(enemyPlayer, GetDamageToDeal(), GetInstigatorController(), this, GetDamageEvent().DamageTypeClass);
    
    // Apply damage while enemy is in the volume, then apply a poison dot.
    DealUniqueSpellFunctionality(enemyPlayer);
  }
}

float AAOEPoisonSpell::GetPreProcessedDotDamage()
{
  return damagePerSecond * AoeTickInterval;
}
