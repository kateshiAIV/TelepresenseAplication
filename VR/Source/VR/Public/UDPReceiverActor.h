#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraDataInterfaceArrayFunctionLibrary.h"
#include "UDPReceiverActor.generated.h"

#pragma pack(push, 1)
struct FPacketHeader
{
    uint32 FrameId;
    uint32 ChunkId;
    uint32 ChunkCount;
    uint32 PointCount;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct FVertex
{
    float X, Y, Z;
    float R, G, B;
};
#pragma pack(pop)


#pragma pack(push, 1)
struct FVertexCompressed
{
    int16_t X, Y, Z;
    uint8_t R, G, B;
};
#pragma pack(pop)

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
    TMap<uint32, TArray<uint8>>     PendingChunks;     // chunkId -> raw bytes
    TMap<uint32, TSet<uint32>>      ReceivedChunkIds;  // frameId -> set of received chunkIds
    uint32                          CurrentFrameId = 0;
    uint32                          ExpectedChunkCount = 0;

    void ReceiveUDP();

    // Reused each frame — avoids reallocation
    TArray<FVector>      PointPositions;
    TArray<FLinearColor> PointColors;
};