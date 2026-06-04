#include "UDPReceiverActor.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Common/UdpSocketBuilder.h"
#include "Interfaces/IPv4/IPv4Endpoint.h"


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
	
	/*
	bool bCanBind = false;
	TSharedRef<FInternetAddr> LocalAddr =
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetLocalHostAddr(*GLog, bCanBind);

	if (!LocalAddr->IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid local address"));
		return;
	}

	FString IP = LocalAddr->ToString(false);

	UE_LOG(LogTemp, Warning, TEXT("Device IP: %s , %s, %s, %s, %s"), *IP, *IP, *IP, *IP,*IP);

	FIPv4Address LocalIP;
	FIPv4Address::Parse(IP, LocalIP);

	FIPv4Endpoint Endpoint(LocalIP, 5005);

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
	*/
	
	

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

// UDPReceiverActor.cpp

void AUDPReceiverActor::ReceiveUDP()
{
    if (!Socket)
        return;

    while (true)
    {
        uint32 PendingSize = 0;

        if (!Socket->HasPendingData(PendingSize))
            break;

        TArray<uint8> Buffer;
    	Buffer.SetNumUninitialized(65507);

        int32 BytesRead = 0;

        if (!Socket->Recv(Buffer.GetData(), Buffer.Num(), BytesRead))
            continue;

        if (BytesRead < sizeof(FPacketHeader))
            continue;

        FPacketHeader Header;
        FMemory::Memcpy(&Header, Buffer.GetData(), sizeof(FPacketHeader));

        const int32 PayloadSize = BytesRead - sizeof(FPacketHeader);
        const uint8* PayloadPtr = Buffer.GetData() + sizeof(FPacketHeader);

        if (Header.FrameId < CurrentFrameId)
            continue;

        if (Header.FrameId > CurrentFrameId)
        {
            PendingChunks.Empty();
            ReceivedChunkIds.Empty();

            CurrentFrameId = Header.FrameId;
            ExpectedChunkCount = Header.ChunkCount;
        }

        TArray<uint8>& ChunkData = PendingChunks.FindOrAdd(Header.ChunkId);
        ChunkData.SetNumUninitialized(PayloadSize);

        FMemory::Memcpy(
            ChunkData.GetData(),
            PayloadPtr,
            PayloadSize);

        ReceivedChunkIds.FindOrAdd(Header.FrameId)
            .Add(Header.ChunkId);

        if (ReceivedChunkIds[Header.FrameId].Num() < (int32)ExpectedChunkCount)
            continue;

        //-------------------------------------
        // Собираем полный кадр
        //-------------------------------------

        TArray<uint8> FullPayload;

        for (uint32 ChunkId = 0; ChunkId < ExpectedChunkCount; ChunkId++)
        {
            TArray<uint8>* Chunk = PendingChunks.Find(ChunkId);

            if (!Chunk)
            {
                UE_LOG(LogTemp, Warning,
                    TEXT("Missing chunk %d"), ChunkId);

                break;
            }

            FullPayload.Append(*Chunk);
        }

        const int32 VertexCount =
            FullPayload.Num() / sizeof(FVertex);

        if (VertexCount <= 0)
            continue;

        FVertex* Vertices =
            reinterpret_cast<FVertex*>(FullPayload.GetData());

        PointPositions.Reset(VertexCount);
        PointColors.Reset(VertexCount);

        for (int32 i = 0; i < VertexCount; i++)
        {
            const FVertex& V = Vertices[i];

            FVector Position(
                V.X * 900.f,
                V.Y * 900.f,
                V.Z * 50.f);

            FLinearColor Color(
                V.R,
                V.G,
                V.B,
                1.0f);

            PointPositions.Add(Position);
            PointColors.Add(Color);
        }

        //-------------------------------------
        // Niagara
        //-------------------------------------

    	
        if (NiagaraComponent && PointPositions.Num() > 0)
        {
        	
        	UE_LOG(LogTemp, Warning,
			TEXT("Received frame: %u | Points: %d"),
			CurrentFrameId,
			PointPositions.Num());
        	
            UNiagaraDataInterfaceArrayFunctionLibrary::
                SetNiagaraArrayVector(
                    NiagaraComponent,
                    TEXT("PointsPositions"),
                    PointPositions);

            UNiagaraDataInterfaceArrayFunctionLibrary::
                SetNiagaraArrayColor(
                    NiagaraComponent,
                    TEXT("PointsColors"),
                    PointColors);

            NiagaraComponent->SetVariableInt(
                TEXT("PointsSize"),
                PointPositions.Num());
        	
        	
        	
        	NiagaraComponent->ReinitializeSystem();
        }
		
    	
    	
        //-------------------------------------
        // Очистка
        //-------------------------------------

        PendingChunks.Empty();
        ReceivedChunkIds.Empty();
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





























































/*
#include "UDPReceiverActor.h"
#include "Engine/World.h"
#include "TimerManager.h"


struct FVertex
{
	int id;
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
	
	
	
	//
	//////////////////////
	bool bCanBind = false;
	TSharedRef<FInternetAddr> LocalAddr =
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetLocalHostAddr(*GLog, bCanBind);

	if (!LocalAddr->IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid local address"));
		return;
	}

	FString IP = LocalAddr->ToString(false);

	UE_LOG(LogTemp, Warning, TEXT("Device IP: %s , %s, %s, %s, %s"), *IP, *IP, *IP, *IP,*IP);

	FIPv4Address LocalIP;
	FIPv4Address::Parse(IP, LocalIP);

	FIPv4Endpoint Endpoint(LocalIP, 5005);

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
	////////////////////
	
	
	// IP ADDRESS local
	
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
*/