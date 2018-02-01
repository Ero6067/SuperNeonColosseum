#include "Weapon.h"

AWeapon::AWeapon()
{
	PrimaryActorTick.bCanEverTick = true;
	
	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(RootComponent);

	weaponName = "BaseWeapon";
	travelDistance = 100;
	hitDamage = 10;
	cooldown = 1;
}
// Called when the game starts or when spawned
void AWeapon::BeginPlay()
{
	Super::BeginPlay();

}
// Called every frame
void AWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}
