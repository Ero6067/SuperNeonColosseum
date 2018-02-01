#include "SoundManager.h"
#include "SuperNeonColosseumGameInstance.h"

class USuperNeonColosseumGameInstance;

// Sets default values
ASoundManager::ASoundManager()
{
}

// Called when the game starts or when spawned
void ASoundManager::BeginPlay()
{
	Super::BeginPlay();
	dynamic_cast<USuperNeonColosseumGameInstance*>(GetGameInstance())->SoundManager = this;
}

void ASoundManager::MulticastPlaySoundAtLocation_Implementation(const FVector location, const FRotator orientation, const FString& eventName)
{
	// (class UAkAudioEvent* AkEvent, FVector Location, FRotator Orientation, const FString& EventName, UObject* WorldContextObject );
	UAkGameplayStatics::PostEventAtLocation(nullptr, location, orientation, eventName, GetWorld());

}

bool ASoundManager::MulticastPlaySoundAtLocation_Validate(const FVector location, const FRotator orientation, const FString& eventName)
{
	return true;
}

void ASoundManager::RequestPlaySoundForAPlayer_Implementation(ABasePlayerController* playerController, AActor* hovercraft, const FString& eventName)
{
	playerController->PlaySound(hovercraft, eventName);
}

bool ASoundManager::RequestPlaySoundForAPlayer_Validate(ABasePlayerController* playerController, AActor* hovercraft, const FString& eventName)
{
	return true;
}
