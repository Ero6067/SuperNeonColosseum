#include "BasePlayerController.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine.h"
#include "AkGameplayStatics.h"

void ABasePlayerController::PlaySound_Implementation(AActor* actor, const FString& eventName)
{
	// Play Sound
	// PostEvent(class UAkAudioEvent* in_pAkEvent, class AActor* in_pActor, bool in_stopWhenAttachedToDestroyed, FString EventName)
	UAkGameplayStatics::PostEvent(nullptr, actor, false, eventName);
}

void ABasePlayerController::ShowCharacterAttackedDirection_Implementation(FVector cameraForward, FVector cameraLocation, FVector attackerLocation) {

	FVector normalizedDelta = (cameraLocation - attackerLocation).GetSafeNormal();
	float dotProduct = FVector::DotProduct(normalizedDelta, cameraForward);
	FVector crossProduct = FVector::CrossProduct(normalizedDelta, cameraForward);

	// Assign directionId. It is decided from 1 to 8 clock wise.
	int directionId = 0;
	if (dotProduct > 0) {
		if (crossProduct.Z > 0 && dotProduct <= 0.5) {
			directionId = 3;
		}
		else if (crossProduct.Z > 0 && dotProduct > 0.5) {
			directionId = 4;
		}
		else if (crossProduct.Z < 0 && dotProduct > 0.5) {
			directionId = 5;
		}
		else if (crossProduct.Z < 0 && dotProduct <= 0.5) {
			directionId = 6;
		}

	}
	else {
		if (crossProduct.Z < 0 && dotProduct > -0.5) {
			directionId = 7;
		}
		else if (crossProduct.Z < 0 && dotProduct <= -0.5) {
			directionId = 8;
		}
		else if (crossProduct.Z > 0 && dotProduct <= -0.5) {
			directionId = 1;
		}
		else if (crossProduct.Z > 0 && dotProduct > -0.5) {
			directionId = 2;
		}
	}

	ShowCompassHUD(directionId);
}

void ABasePlayerController::ShowHitEvent_Implementation() {
	ShowHitEventOnHud();
}

void ABasePlayerController::OnDeathEvent_Implementation(int opponentPlayerSlot) {
	ShowMsgOnScreen(true, opponentPlayerSlot);
}

void ABasePlayerController::OnKillEvent_Implementation(int opponentPlayerSlot, int currentScore) {
	ShowMsgOnScreen(false, opponentPlayerSlot);
	CallPlayKillVO(currentScore);
}

void ABasePlayerController::MatchStartCountDown_Implementation() {
	GetWorldTimerManager().SetTimer(countDownTimerHandle, this, &ABasePlayerController::CountDown, 1.0f, true, 2.0f);
}

void ABasePlayerController::CountDown() {
	if (currentCountDownNum == 0) {
		OnCountDownFinished();
		GetWorldTimerManager().ClearTimer(countDownTimerHandle);

	} else {
		ShowMatchStartCountDownOnScreen(currentCountDownNum--);
	}

}
