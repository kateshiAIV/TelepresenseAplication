#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Networking.h"
#include "Sockets.h"

#include "UDPReceiverActor.generated.h"

UCLASS()
class VR_API AUDPReceiverActor : public AActor
{
	GENERATED_BODY()

public:
	AUDPReceiverActor();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	FSocket* Socket;

	void ReceiveUDP();

	FTimerHandle TimerHandle;
};