#include "UDPReceiverActor.h"
#include "Engine/World.h"
#include "TimerManager.h"


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
	
	
	
	
	// IP ADDRESS
	bool bCanBind = false;
	TSharedRef<FInternetAddr> LocalAddr =
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetLocalHostAddr(*GLog, bCanBind);

	if (LocalAddr->IsValid())
	{
		FString IP = LocalAddr->ToString(false);
		UE_LOG(LogTemp, Warning, TEXT("Device IP: %s"), *IP);

		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green, IP);
		}
	}
	////////////////////////////////
	

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

		if (BytesRead <= 0) return;

		int32 VertexSize = sizeof(FVertex);
		int32 VertexCount = BytesRead / VertexSize;

		UE_LOG(LogTemp, Warning, TEXT("Received %d vertices"), VertexCount);

		FVertex* Vertices = reinterpret_cast<FVertex*>(Buffer.GetData());

		for (int32 i = 0; i < VertexCount; i++)
		{
			const FVertex& V = Vertices[i];

			// Position
			FVector Pos(V.X, V.Y, V.Z);

			// Scale to Unreal units (important!)
			Pos *= 100.0f;

			// Optional: adjust coordinate system if needed
			// Swap(Pos.Y, Pos.Z);

			// Color
			FColor Color(
				(uint8)(FMath::Clamp(V.R, 0.f, 1.f) * 255),
				(uint8)(FMath::Clamp(V.G, 0.f, 1.f) * 255),
				(uint8)(FMath::Clamp(V.B, 0.f, 1.f) * 255),
				255
			);

			DrawDebugPoint(GetWorld(), Pos, 6.0f, Color, false, 0.5f);
		}
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