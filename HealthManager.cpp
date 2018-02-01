#include "HealthManager.h"
#include "UnrealNetwork.h"
#include "Engine.h"
#include "SuperNeonColosseumGameMode.h"
#include "BasePlayerController.h"
#include "Runtime/Engine/Classes/Engine/World.h"
//specify Engine/X.h

// Sets default values for this component's properties
UHealthManager::UHealthManager()
{
	PrimaryComponentTick.bCanEverTick = true;
	
	maxHealth		= 100.f;
	currentHealth	= maxHealth;

	isDead			= false;
	isInvincible	= false;
	overhealEnabled = false;

	owningHovercraft = dynamic_cast<AHovercraft*>(GetOwner());
}

void UHealthManager::BeginPlay()
{
	Super::BeginPlay();
}

void UHealthManager::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void UHealthManager::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Replicate to everyone
	DOREPLIFETIME(UHealthManager, maxHealth);
	DOREPLIFETIME(UHealthManager, currentHealth);
	DOREPLIFETIME(UHealthManager, overhealEnabled);
	DOREPLIFETIME(UHealthManager, isInvincible);
	DOREPLIFETIME(UHealthManager, OnHealthModified);
}

#pragma region Heal

///<summary>
/// Heals the user for a specified amount. 
/// Won't heal extra if healValue exceeds the current missing health.
///<param name='healValue'>Amount of health to be added.</param>  
///<returns>The amount that was healed.</returns>  
///</summary>
float UHealthManager::Heal(float healValue){
	if (GetOwner()->GetWorld()->GetAuthGameMode()) {
		if (!isDead && (overhealEnabled || currentHealth < maxHealth)) {
			healValue = FMath::Min(FMath::Abs(healValue), maxHealth - currentHealth);

			AddHealth(healValue);

			return healValue;
		}
	}
  return 0;
}

///<summary>
/// SERVER ONLY FUNCTION. Needed for replication.
///</summary>
bool UHealthManager::ServerHeal_Validate(float healValue) {
	return true;
}

///<summary>
/// Calls the client's Heal method. Needed for replication
///</summary>
void UHealthManager::ServerHeal_Implementation(float healValue) {
	Heal(healValue);
}

#pragma endregion Replicated Method


#pragma region TakeDamage

float UHealthManager::TakeDamage(float damageValue, AHovercraft* attacker) {

	// Only do if it is server.
	AGameModeBase* gm = GetOwner()->GetWorld()->GetAuthGameMode();
	if (gm) {

		// Show hitting hud
		if (attacker) {
			ABasePlayerController* attackerController = dynamic_cast<ABasePlayerController*>(attacker->GetController());
			if (attackerController)
				attackerController->ShowHitEvent();
		}

		// Logic for Showing attacked direction HUD
		if (attacker && owningHovercraft && owningHovercraft->GetController()) {
			dynamic_cast<ABasePlayerController*>(owningHovercraft->GetController())->ShowCharacterAttackedDirection(owningHovercraft->GetSpringArm()->GetForwardVector(), owningHovercraft->GetSpringArm()->GetComponentLocation(), attacker->GetActorLocation());
		}

		if (!isDead && !isInvincible) {
			owningHovercraft->HovercraftDamaged();

			damageValue = FMath::Min(FMath::Abs(damageValue), currentHealth);

			ReduceHealth(damageValue);

			if (isDead = owningHovercraft && currentHealth <= 0) {

				if (attacker) {
					((class ASuperNeonColosseumGameMode*)gm)->MatchRefreeComponent->OnPlayerDeath(owningHovercraft, attacker);
				}

				owningHovercraft->DestroyHovercraft();
			}
			
			return damageValue;
		}
	}
	return 0;
}

bool UHealthManager::ServerTakeDamage_Validate(float damageValue, AHovercraft* attacker)
{
	return true;
}

void UHealthManager::ServerTakeDamage_Implementation(float damageValue, AHovercraft* attacker) {
	TakeDamage(damageValue, attacker);
}

#pragma endregion Replicated Method


void UHealthManager::AddHealth(const float health) {
	currentHealth += FMath::Abs(health);
	//sets limits on what happens when a player health reaches above maximum
	if (!overhealEnabled && currentHealth > maxHealth)
		currentHealth = maxHealth;
}

void UHealthManager::ReduceHealth(const float health) {
	currentHealth -= FMath::Abs(health);
	//sets limits on what happens when a player health reaches below 0
	if (currentHealth < 0)
		currentHealth = 0;
}

float UHealthManager::GetNormalizedHealth() {
	return currentHealth / maxHealth;
}

FString UHealthManager::GetHealth() {
	return FString::SanitizeFloat(currentHealth);
}

bool UHealthManager::ToggleInvincibility() {
	return isInvincible = !isInvincible;
}

bool UHealthManager::ToggleOverhealing() {
	return overhealEnabled = !overhealEnabled;
}
