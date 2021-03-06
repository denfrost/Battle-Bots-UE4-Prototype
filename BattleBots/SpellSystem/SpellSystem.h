// Copyright 2015 VMR Games, Inc. All Rights Reserved.

#pragma once

#include "Character/BBotCharacter.h"
#include "Interfaces/BBotsResetInterface.h"
#include "GameFramework/Actor.h"
#include "SpellSystem.generated.h"

//class ABBotCharacter;

USTRUCT()
struct FSpellData{
  GENERATED_USTRUCT_BODY()

  UPROPERTY(EditDefaultsOnly, Category = "SpellInfo")
    FName spellName;
  UPROPERTY(EditDefaultsOnly, Category = "SpellInfo")
    FString toolTip;
  UPROPERTY(EditDefaultsOnly, Category = "SpellInfo")
    UTexture2D* icon;
  UPROPERTY(EditDefaultsOnly, Category = "Config")
    float spellDamage;
  UPROPERTY(EditDefaultsOnly, Category = "Config")
    float spellCost;
  UPROPERTY(EditDefaultsOnly, Category = "Config")
    float spellDuration;
  UPROPERTY(EditDefaultsOnly, Category = "Config")
    float spellSpeed;
  UPROPERTY(EditDefaultsOnly, Category = "Config")
    float coolDown;
  UPROPERTY(EditDefaultsOnly, Category = "Config")
    float castTime;
  UPROPERTY(EditDefaultsOnly, Category = "Config")
    bool bCastableWhileMoving;
  UPROPERTY(EditDefaultsOnly, Category = "Config")
    bool bIsPiercing;
  UPROPERTY(EditDefaultsOnly, Category = "Config")
    bool bCanStun;
  UPROPERTY(EditDefaultsOnly, Category = "Config")
    float stunChance;
  UPROPERTY(EditDefaultsOnly, Category = "Config")
    bool bKnockBack;
  UPROPERTY(EditDefaultsOnly, Category = "Config")
    float knockBackChance;
};

UCLASS()
class BATTLEBOTS_API ASpellSystem : public AActor, public IBBotsResetInterface
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASpellSystem();
  
  // Called after all components have been initialized with default values
  virtual void PostInitializeComponents() override;

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

  /** Hides actor and disable collision */
  virtual void Reset() override;

  // Interface call on match reset.
  virtual void Reset_Implementation() override;

  /** An object that holds our spell configurations*/
  UPROPERTY(EditDefaultsOnly, Category = Config)
  FSpellData spellDataInfo;
  
  //TODO: no need to replicate, it doesnt work in inherited classes
  // The final processed damage post damage modifiers
  UPROPERTY(Replicated)
  float damageToDeal;

  /** A simple collision primitive to use as the root component*/
  UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Collision")
  USphereComponent* collisionComp;

  /** UStaticMeshComponent to represent the spell in the world.*/
  UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Config")
  UStaticMeshComponent* spellMesh;

  UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "PhysicsConfig")
  UProjectileMovementComponent* projectileMovementComp;

  UPROPERTY(VisibleDefaultsOnly)
  UParticleSystemComponent* particleComp;

  UPROPERTY(EditDefaultsOnly, Category = "Effects")
  UParticleSystem* spellFX;

  UPROPERTY(VisibleDefaultsOnly)
  UAudioComponent* audioComp;

  UPROPERTY(EditDefaultsOnly, Category = "Effects")
  USoundCue* explosionSound;

  // Is called when a spell collides with a player
  UFUNCTION()
  virtual void OnCollisionOverlapBegin(class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
  
  // Is called when a spell no longer collides with a player
  UFUNCTION()
  virtual void OnCollisionOverlapEnd(class AActor* OtherActor, class UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
  
  // Returns the damage event and type
  UFUNCTION(BlueprintCallable, Category = "SpellSystem")
  virtual FDamageEvent& GetDamageEvent();

  UFUNCTION(BlueprintCallable, Category = "SpellSystem")
  float GetSpellCost() const;

  UFUNCTION(BlueprintCallable, Category = "SpellSystem")
  float GetCastTime() const;

  // Clears the current unique timers such as ignite, slow, etc.
  virtual void ClearUniqueTimers();

  // Can the player cast the spell while moving?
  FORCEINLINE bool CastableWhileMoving() const { return spellDataInfo.bCastableWhileMoving; }
  FORCEINLINE bool SpellOnCD() const { return !(CDHelper < GetWorld()->GetTimeSeconds()); }

  /** Spawns a spell into the world. Manages internal spell cool down.*/
  UFUNCTION(BlueprintCallable, Category = "SpellSystem")
  void SpawnSpell(TSubclassOf<ASpellSystem> tempSpell);

  
  virtual FVector GetSpellSpawnLocation();
  // Set the spell spawn location (MouseHitLocation/CharacterLocation.
  FORCEINLINE void SetSpellSpawnLocation(FVector newSpawnLocation) {
    if (HasAuthority())
    {
      spellSpawnLocation = newSpawnLocation;
    }
  }

protected:
  // Setting a member variable was delayed due to networked serialization, thus we have to cast a tempCaster so inherited classes can get the right spellCaster.
  // Returns the current spell's caster
  FORCEINLINE virtual ABBotCharacter* GetSpellCaster() { ABBotCharacter* tempCaster = Cast<ABBotCharacter>(GetInstigator()); return tempCaster; }

  // Returns the damage type of the spell
  FORCEINLINE TSubclassOf<UDamageType> ASpellSystem::GetDamageType()
  {
    return GetDamageEvent().DamageTypeClass;
  }

  // Holds all the overlapped actors to prevent multiple calls to dealdamage
  UPROPERTY()
  TArray<AActor*> OverlappedActors;

  // The spell spawn location (MouseHitLocation/CharacterLocation.
  FVector spellSpawnLocation;

  // Holds the default dmg event and type
  FDamageEvent defaultDamageEvent;

  // Manages the spawned spell
  ASpellSystem* spellSpawner;

  // Casts the current spell
  virtual void SpawnSpell_Internal(TSubclassOf<ASpellSystem> tempSpell);

  // Returns the spawned spell with the appropriate location. AOE spells must override this method and use HITLOC instead of GetSpellCaster()->GetActorLocation()
  /* virtual ASpellSystem* GetSpawnedSpell(TSubclassOf<ASpellSystem> tempSpell, FActorSpawnParameters spawnParams, const FVector& HitLocation);*/

  // Processes final elemental damage post item dmg modifiers
  virtual float ProcessElementalDmg(float initialDamage);

  // Short-circuits if the overlapped pawn is an enemy, to prevent unnecessary computations
  bool IsEnemy(ABBotCharacter* possibleEnemy);

  // Deals damage to the actor and manages spell death. Override spell functionality, ex: Ignite, slow, etc.
  virtual void DealDamage(ABBotCharacter* enemyPlayer);

  // Process unique spell functionality such as Ignite, Slow, Heal, Knockback, etc.
  virtual void DealUniqueSpellFunctionality(ABBotCharacter* enemyPlayer);

  /* Returns the duration the unique functionality duration, ex: Ignite Duration
  This prevents the spell object from getting deleted before the ignite duration is over */
  virtual float GetFunctionalityDuration();

  /* Returns the damage pre elemental dmg processing. Used to set 
  dot dmg under aoe classes (Ignite, psn-dot, etc), such as ignite effects 
  can do more or less dmg than staying in the aoe volume.*/
  UFUNCTION()
  virtual float GetPreProcessedDotDamage();

  // Returns the final dmg to deal post dmg modifiers
  UFUNCTION()
  virtual float GetDamageToDeal();
  // Applies ProcessElementalDmg to DamageToDeal over the server
  UFUNCTION()
  virtual void SetDamageToDeal(float newDmg);

  // Destroys spell after reaching a certain range or if it collides
  virtual void DestroySpell();

  // Simulate spell explosion
  UFUNCTION(Reliable, NetMulticast)
  void SimulateExplosion();
  virtual void SimulateExplosion_Implementation();

  /* UE4 does not support multiple inheritance, 
  thus we are creating the AOETick under the spellSystem
  to be called under AOEFire/AOEIce-Tick, etc. Using an interface
  would not have solved the problem of having to implement
  the same AOETick function for every damage type.*/
  virtual void AOETick();

  // The spellDps, to be applied by deltaSeconds(Used with AOETick)
  float damagePerSecond;

private:
  // Spell cooldown helper
  UPROPERTY(Replicated)
  float CDHelper;
  
  void ProcessSpellTimers();

  // IsEnemy only runs on server authority
  UFUNCTION(Reliable, Server, WithValidation)
  void ServerIsEnemy(ABBotCharacter* possibleEnemy);
  virtual void ServerIsEnemy_Implementation(ABBotCharacter* possibleEnemy);
  virtual bool ServerIsEnemy_Validate(ABBotCharacter* possibleEnemy);
};
