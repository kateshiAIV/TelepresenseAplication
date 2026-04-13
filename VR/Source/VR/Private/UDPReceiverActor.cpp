#include "UDPReceiverActor.h"
#include "Engine/World.h"
#include "TimerManager.h"

AUDPReceiverActor::AUDPReceiverActor()
{
	PrimaryActorTick.bCanEverTick = false;
	Socket = nullptr;
}
void AUDPReceiverActor::BeginPlay()
{
	Super::BeginPlay();

	FIPv4Endpoint Endpoint(FIPv4Address(127, 0, 0, 1), 5005);

	Socket = FUdpSocketBuilder(TEXT("UDPReceiver"))
		.AsNonBlocking()
		.AsReusable()
		.BoundToEndpoint(Endpoint)
		.WithReceiveBufferSize(2 * 1024 * 1024);

	if (!Socket)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create socket"));
		return;
	}

	GetWorld()->GetTimerManager().SetTimer(
		TimerHandle,
		this,
		&AUDPReceiverActor::ReceiveUDP,
		0.01f,
		true
	);

	UE_LOG(LogTemp, Warning, TEXT("UDP Receiver started on 127.0.0.1:5005"));
}


void AUDPReceiverActor::ReceiveUDP()
{
	if (!Socket) return;

	uint32 Size = 0;

	while (Socket->HasPendingData(Size))
	{
		TArray<uint8> Buffer;
		Buffer.SetNumUninitialized(FMath::Min(Size, 65507u));

		int32 BytesRead = 0;
		Socket->Recv(Buffer.GetData(), Buffer.Num(), BytesRead);

		UE_LOG(LogTemp, Warning, TEXT("Received packet: %d bytes"), BytesRead);

		// Debug first bytes
		FString Debug;
		for (int i = 0; i < FMath::Min(BytesRead, 20); i++)
		{
			Debug += FString::Printf(TEXT("%d "), Buffer[i]);
		}

		UE_LOG(LogTemp, Warning, TEXT("Data: %s"), *Debug);
	}
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