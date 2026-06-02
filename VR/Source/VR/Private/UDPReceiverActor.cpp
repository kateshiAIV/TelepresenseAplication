#include "UDPReceiverActor.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Common/UdpSocketBuilder.h"
#include "Interfaces/IPv4/IPv4Endpoint.h"


struct FVertex
{
	float X, Y, Z;
	float R, G, B;
};

AUDPReceiverActor::AUDPReceiverActor()
{
	PrimaryActorTick.bCanEverTick = false;
	Socket = nullptr;
}
void AUDPReceiverActor::BeginPlay()
{
    Super::BeginPlay();

    // --- Niagara setup ---
    if (PointCloudSystem)
    {
        NiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
            PointCloudSystem,
            GetRootComponent(),
            NAME_None,
            FVector::ZeroVector,
            FRotator::ZeroRotator,
            EAttachLocation::KeepRelativeOffset,
            false  // don't auto-destroy when finished
        );
    }

    // --- UDP socket (same as before) ---
    FIPv4Endpoint Endpoint(FIPv4Address(127, 0, 0, 1), 5005);
    Socket = FUdpSocketBuilder(TEXT("UDPReceiver"))
        .AsNonBlocking().AsReusable()
        .BoundToEndpoint(Endpoint)
        .WithReceiveBufferSize(2 * 1024 * 1024);

    if (Socket)
    {
        GetWorld()->GetTimerManager().SetTimer(
            TimerHandle, this,
            &AUDPReceiverActor::ReceiveUDP,
            0.01f, true);
    }
}

void AUDPReceiverActor::ReceiveUDP()
{
    if (!Socket || !NiagaraComponent) return;

    uint32 Size = 0;
    if (!Socket->HasPendingData(Size)) return;

    TArray<uint8> Buffer;
    Buffer.SetNumUninitialized(FMath::Min(Size, 65507u));

    int32 BytesRead = 0;
    Socket->Recv(Buffer.GetData(), Buffer.Num(), BytesRead);
    if (BytesRead <= 0) return;

    int32 VertexCount = BytesRead / sizeof(FVertex);
    FVertex* Verts = reinterpret_cast<FVertex*>(Buffer.GetData());

    PointPositions.Reset(VertexCount);
    PointColors.Reset(VertexCount);

    for (int32 i = 0; i < VertexCount; i++)
    {
        const FVertex& V = Verts[i];
        PointPositions.Add(FVector(V.X, V.Y, V.Z) * 100.f);
        PointColors.Add(FLinearColor(V.R, V.G, V.B, 1.f));
    }

    // Push to Niagara — these names must match your NS parameters
    UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayVector(
        NiagaraComponent, FName("PointsPosition"), PointPositions);

    UNiagaraDataInterfaceArrayFunctionLibrary::SetNiagaraArrayColor(
        NiagaraComponent, FName("PointsColors"), PointColors);
    
    
    NiagaraComponent->SetVariableInt(
    TEXT("size"),
    PointColors.Num()
);

    NiagaraComponent->ResetSystem();
}

void AUDPReceiverActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	if (Socket)
	{
		Socket->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Socket);
		Socket = nullptr;
	}

	GetWorld()->GetTimerManager().ClearTimer(TimerHandle);
}