#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"
#include "UDPReceiverActor.generated.h"

UCLASS()
class VR_API AUDPReceiverActor : public AActor
{
    GENERATED_BODY()
public:
    AUDPReceiverActor();

    // Drop a Niagara System asset in here from the editor
    UPROPERTY(EditAnywhere, Category="Niagara")
    UNiagaraSystem* PointCloudSystem;

    UPROPERTY(VisibleAnywhere, Category="Niagara")
    UNiagaraComponent* NiagaraComponent;

private:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    FSocket* Socket;
    FTimerHandle TimerHandle;

    void ReceiveUDP();

    // Reused each frame — avoids reallocation
    TArray<FVector>      PointPositions;
    TArray<FLinearColor> PointColors;
};