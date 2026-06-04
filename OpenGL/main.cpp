#define STB_IMAGE_IMPLEMENTATION

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <winsock2.h>
#include <ws2tcpip.h>


#include <GLFW/stb_image.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <iostream>
#include <fstream>
#include <Kinect.h>

using namespace std;

//#include "Vertex.h" // выключить в дебаг моде
#include "UDPSender.h"
#include <chrono>

#pragma comment(lib, "ws2_32.lib")

#pragma pack(push, 1)
struct Vertex
{
    float x, y, z;
    float r, g, b;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct VertexCompressed
{
    int16_t x, y, z;
    uint8_t r, g, b;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct PacketHeader
{
    uint32_t frameId;
    uint32_t chunkId;
    uint32_t chunkCount;
    uint32_t pointCount;
};
#pragma pack(pop)

static void generateCloud(std::vector<Vertex>& cloud, int count)
{
    cloud.clear();
    cloud.reserve(count);

    for (int i = 0; i < count; i++)
    {
        cloud.push_back({
            (float(rand()) / RAND_MAX) * 2.0f - 1.0f,
            (float(rand()) / RAND_MAX) * 2.0f - 1.0f,
            (float(rand()) / RAND_MAX) * 2.0f - 1.0f,
            float(rand()) / RAND_MAX,
            float(rand()) / RAND_MAX,
            float(rand()) / RAND_MAX
            });
    }
}


void generateCloudFromImages(
    std::vector<Vertex>& cloud,
    const char* colorFile,
    const char* depthFile)
{
    int colorW, colorH, colorC;
    unsigned char* colorImg =
        stbi_load(colorFile, &colorW, &colorH, &colorC, 3);

    int depthW, depthH, depthC;
    unsigned char* depthImg =
        stbi_load(depthFile, &depthW, &depthH, &depthC, 1);

    if (!colorImg || !depthImg)
    {
        std::cout << "Failed to load images\n";
        return;
    }

    if (colorW != depthW || colorH != depthH)
    {
        std::cout << "Image sizes do not match\n";
        return;
    }

    cloud.clear();
    cloud.reserve(colorW * colorH);

    const float depthScale = 5.0f;

    for (int y = 0; y < depthH; y+=2)
    {
        for (int x = 0; x < depthW; x+=2)
        {

          

            int idxDepth = y * depthW + x;

            unsigned char depthValue = depthImg[idxDepth];

            if (depthValue == 0)
                continue;

            float z = (depthValue / 255.0f) * depthScale;

            float px = (float)x / depthW - 0.5f;
            float py = -(float)y / depthH + 0.5f;

            int idxColor = (y * colorW + x) * 3;

            float r = colorImg[idxColor + 0] / 255.0f;
            float g = colorImg[idxColor + 1] / 255.0f;
            float b = colorImg[idxColor + 2] / 255.0f;

            cloud.push_back({
                px,
                py,
                z,
                r,
                g,
                b
                });

        }
    }

    stbi_image_free(colorImg);
    stbi_image_free(depthImg);

    std::cout << "Loaded point cloud: "
        << cloud.size()
        << " points\n";
}

void generateCompressedCloudFromImages(
    std::vector<VertexCompressed>& cloud,
    const char* colorFile,
    const char* depthFile)
{
    int colorW, colorH, colorC;
    unsigned char* colorImg =
        stbi_load(colorFile, &colorW, &colorH, &colorC, 3);

    int depthW, depthH, depthC;
    unsigned char* depthImg =
        stbi_load(depthFile, &depthW, &depthH, &depthC, 1);

    if (!colorImg || !depthImg)
    {
        std::cout << "Failed to load images\n";
        return;
    }

    if (colorW != depthW || colorH != depthH)
    {
        std::cout << "Image sizes do not match\n";
        return;
    }

    cloud.clear();
    cloud.reserve(colorW * colorH);

    const float depthScale = 5.0f;

    for (int y = 0; y < depthH; y += 2)
    {
        for (int x = 0; x < depthW; x += 2)
        {
            int idxDepth = y * depthW + x;
            unsigned char depthValue = depthImg[idxDepth];

            if (depthValue == 0)
                continue;

            float z = (depthValue / 255.0f) * depthScale;

            float px = (float)x / depthW - 0.5f;
            float py = -(float)y / depthH + 0.5f;

            int idxColor = (y * colorW + x) * 3;

            float r = colorImg[idxColor + 0] / 255.0f;
            float g = colorImg[idxColor + 1] / 255.0f;
            float b = colorImg[idxColor + 2] / 255.0f;

            const float depthScale = 5.0f;

            int16_t cX = (int16_t)(px * 32767.0f);
            int16_t cY = (int16_t)(py * 32767.0f);
            int16_t cZ = (int16_t)((z / depthScale) * 32767.0f);

            uint8_t cR = (uint8_t)(r * 255.0f);
            uint8_t cG = (uint8_t)(g * 255.0f);
            uint8_t cB = (uint8_t)(b * 255.0f);

			cloud.push_back({
				cX,
				cY,
				cZ,
				cR,
				cG,
				cB
				});


        }
    }

    stbi_image_free(colorImg);
    stbi_image_free(depthImg);

    std::cout << "Loaded compressed point cloud: "
        << cloud.size()
        << " points\n";
}



int main()
{
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(5005);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    std::vector<Vertex> cloud;
    std::vector<VertexCompressed> cloudCompressed;

    const int MAX_UDP_SIZE = 1024;

    uint32_t frameId = 0;

    std::cout << "UDP sender started...\n";


    while (true)
    {

        generateCompressedCloudFromImages(
            cloudCompressed,
            "assets/color.jpeg",
            "assets/depth.png");

        const int stride = sizeof(VertexCompressed);

        const int payloadLimit =
            ((MAX_UDP_SIZE - sizeof(PacketHeader)) / stride) * stride;

        int totalSize = cloudCompressed.size() * sizeof(VertexCompressed);
        int chunkCount = (totalSize + payloadLimit - 1) / payloadLimit;

        uint64_t FrameBytesSent = 0;

        for (int chunkId = 0; chunkId < chunkCount; chunkId++)
        {
            PacketHeader header;
            header.frameId = frameId;
            header.chunkId = chunkId;
            header.chunkCount = chunkCount;
            header.pointCount = (uint32_t)cloudCompressed.size();

            int offset = chunkId * payloadLimit;
            int chunkSize = min(payloadLimit, totalSize - offset);

            std::vector<char> packet(sizeof(PacketHeader) + chunkSize);


            memcpy(packet.data(), &header, sizeof(PacketHeader));
            memcpy(packet.data() + sizeof(PacketHeader),
                ((char*)cloudCompressed.data()) + offset,
                chunkSize);

            int sent = sendto(
                sock,
                packet.data(),
                (int)packet.size(),
                0,
                (sockaddr*)&addr,
                sizeof(addr));

            if (sent > 0)
            {
                FrameBytesSent += sent;
            }
        }

        std::cout
            << "frame " << frameId
			<< "compressed cloud with " << cloudCompressed.size() << " points, "
            << " sent (" << chunkCount << " chunks, "
            << FrameBytesSent << " bytes)"
            << std::endl;

        frameId++;

        Sleep(33); // ~30 FPS
    }

    closesocket(sock);
    WSACleanup();
}



//
//
//IKinectSensor* pSensor = nullptr;
//IColorFrameReader* pColorReader = nullptr;
//IDepthFrameReader* pDepthReader = nullptr;
//ICoordinateMapper* pMapper = nullptr;
//
//IColorFrameSource* pColorSource = nullptr;
//IDepthFrameSource* pDepthSource = nullptr;
//
//float CAMERA_DISTANCE = 5.0f;
//
////колесико для зума
//void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
//{
//    CAMERA_DISTANCE -= (float)yoffset * 0.5f;
//    if (CAMERA_DISTANCE < 0.1f) CAMERA_DISTANCE = 0.1f;
//    if (CAMERA_DISTANCE > 20.0f) CAMERA_DISTANCE = 20.0f;
//}
//
//// точечное облако
//void drawPointCloud(const vector<Vertex>& vertices)
//{
//    glBegin(GL_POINTS);
//    for (const auto& v : vertices)
//    {
//        glColor3f(v.getR(), v.getG(), v.getB());
//        glVertex3f(v.getX(), v.getY(), v.getZ());
//    }
//    glEnd();
//}
//
//// создание point cloud
//void createPointCloud(vector<Vertex>& vertices, IColorFrame* pColorFrame, IDepthFrame* pDepthFrame, ICoordinateMapper* pMapper)
//{
//    vertices.clear();
//    if (!pColorFrame || !pDepthFrame || !pMapper)
//        return;
//
//    // размеры кадра цвета
//    int cw = 0, ch = 0;
//    IFrameDescription* pColorDesc = nullptr;
//    pColorFrame->get_FrameDescription(&pColorDesc);
//    pColorDesc->get_Width(&cw);
//    pColorDesc->get_Height(&ch);
//    pColorDesc->Release();
//
//    vector<BYTE> colorBuffer(cw * ch * 4); // RGBA
//    if (FAILED(pColorFrame->CopyConvertedFrameDataToArray(cw * ch * 4, colorBuffer.data(), ColorImageFormat_Rgba)))
//        return;
//
//    // размеры кадра глубины
//    int dw = 0, dh = 0;
//    IFrameDescription* pDepthDesc = nullptr;
//    pDepthFrame->get_FrameDescription(&pDepthDesc);
//    pDepthDesc->get_Width(&dw);
//    pDepthDesc->get_Height(&dh);
//    pDepthDesc->Release();
//
//    vector<UINT16> depthBuffer(dw * dh);
//    pDepthFrame->CopyFrameDataToArray(dw * dh, depthBuffer.data());
//
//    // сопоставление depth -> color
//    vector<ColorSpacePoint> colorPoints(dw * dh);
//    if (FAILED(pMapper->MapDepthFrameToColorSpace(dw * dh, depthBuffer.data(), dw * dh, colorPoints.data())))
//        return;
//
//    // построение точек
//    for (int y = 0; y < dh; y++)
//    {
//        for (int x = 0; x < dw; x++)
//        {
//            int idxDepth = y * dw + x;
//            UINT16 depthValue = depthBuffer[idxDepth];
//            if (depthValue == 0) continue;
//
//            ColorSpacePoint cp = colorPoints[idxDepth];
//            int cx = static_cast<int>(cp.X + 0.5f);
//            int cy = static_cast<int>(cp.Y + 0.5f);
//
//            if (cx < 0 || cx >= cw || cy < 0 || cy >= ch) continue;
//
//            int idxColor = (cy * cw + cx) * 4;
//            float r = colorBuffer[idxColor] / 255.0f;
//            float g = colorBuffer[idxColor + 1] / 255.0f;
//            float b = colorBuffer[idxColor + 2] / 255.0f;
//
//            float z = depthValue / 500.0f; // mm -> meters
//            z = -z;
//		     // масштабирование
//            float px = (x - dw / 2.0f) / dw * 5.0f;
//            float py = (dh / 2.0f - y) / dh * 5.0f;
//
//            vertices.push_back({ px, py, z, r, g, b });
//        }
//    }
//}
//
//// инициализация Kinect
//bool initKinect()
//{
//    if (FAILED(GetDefaultKinectSensor(&pSensor)) || !pSensor)
//    {
//        cout << "Failed to get Kinect sensor" << endl;
//        return false;
//    }
//
//    if (FAILED(pSensor->Open()))
//    {
//        cout << "Failed to open Kinect sensor" << endl;
//        return false;
//    }
//
//    if (FAILED(pSensor->get_ColorFrameSource(&pColorSource)) || !pColorSource)
//    {
//        cout << "Failed to get color source" << endl;
//        return false;
//    }
//
//    if (FAILED(pColorSource->OpenReader(&pColorReader)) || !pColorReader)
//    {
//        cout << "Failed to open color reader" << endl;
//        return false;
//    }
//
//    if (FAILED(pSensor->get_DepthFrameSource(&pDepthSource)) || !pDepthSource)
//    {
//        cout << "Failed to get depth source" << endl;
//        return false;
//    }
//
//    if (FAILED(pDepthSource->OpenReader(&pDepthReader)) || !pDepthReader)
//    {
//        cout << "Failed to open depth reader" << endl;
//        return false;
//    }
//
//    if (FAILED(pSensor->get_CoordinateMapper(&pMapper)) || !pMapper)
//    {
//        cout << "Failed to get coordinate mapper" << endl;
//        return false;
//    }
//
//    cout << "Kinect initialized successfully" << endl;
//    return true;
//}
//
//// --- main
//int main()
//{
//
//    WSADATA wsa;
//    WSAStartup(MAKEWORD(2, 2), &wsa);
//
//    SOCKET sock = socket(AF_INET, SOCK_DGRAM, 0);
//
//    sockaddr_in addr{};
//    addr.sin_family = AF_INET;
//    addr.sin_port = htons(5005);
//
//    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
//
//    if (!glfwInit())
//    {
//        cout << "Failed to initialize GLFW" << endl;
//        return -1;
//    }
//
//    GLFWwindow* window = glfwCreateWindow(920, 680, "Telepresence App", nullptr, nullptr);
//    if (!window) { glfwTerminate(); return -1; }
//
//    glfwMakeContextCurrent(window);
//    if (glewInit() != GLEW_OK) { cout << "Failed to initialize GLEW" << endl; return -1; }
//
//    if (!initKinect()) { cout << "Kinect init failed" << endl; return -1; }
//
//    glfwSetScrollCallback(window, scroll_callback);
//
//    glEnable(GL_DEPTH_TEST);
//    glPointSize(2.0f);
//
//    vector<Vertex> vertices;
//
//    while (!glfwWindowShouldClose(window))
//    {
//        int width, height;
//        glfwGetFramebufferSize(window, &width, &height);
//
//        glViewport(0, 0, width, height);
//        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//
//        glMatrixMode(GL_PROJECTION);
//        glLoadIdentity();
//        gluPerspective(45.0, (double)width / height, 0.1, 100.0);
//
//        glMatrixMode(GL_MODELVIEW);
//        glLoadIdentity();
//        glTranslatef(0, 0, -CAMERA_DISTANCE);
//
//        // --- Acquire frames
//        IColorFrame* pColorFrame = nullptr;
//        IDepthFrame* pDepthFrame = nullptr;
//        HRESULT hrColor = pColorReader->AcquireLatestFrame(&pColorFrame);
//        HRESULT hrDepth = pDepthReader->AcquireLatestFrame(&pDepthFrame);
//
//        if (SUCCEEDED(hrColor) && SUCCEEDED(hrDepth))
//        {
//            createPointCloud(vertices, pColorFrame, pDepthFrame, pMapper);
//        }
//
//        drawPointCloud(vertices);
//
//        const int MAX_PACKET = 60000; // безопасно < 65k
//
//        int totalSize = vertices.size() * sizeof(Vertex);
//        char* data = (char*)vertices.data();
//
//        int offset = 0;
//
//        while (offset < totalSize)
//        {
//            int chunkSize = min(MAX_PACKET, totalSize - offset);
//
//            sendto(
//                sock,
//                data + offset,
//                chunkSize,
//                0,
//                (sockaddr*)&addr,
//                sizeof(addr)
//            );
//
//            offset += chunkSize;
//        }
//
//            
//        std::cout << "sent packet: " << vertices.size() << " points\n";
//            
//        //Sleep(33); 
//
//
//        if (pColorFrame) pColorFrame->Release();
//        if (pDepthFrame) pDepthFrame->Release();
//
//
//
//        glfwSwapBuffers(window);
//        glfwPollEvents();
//    }
//
//    glfwTerminate();
//
//    if (pColorReader) pColorReader->Release();
//    if (pColorSource) pColorSource->Release();
//    if (pDepthReader) pDepthReader->Release();
//    if (pDepthSource) pDepthSource->Release();
//    if (pMapper) pMapper->Release();
//
//    if (pSensor)
//    {
//        pSensor->Close();
//        pSensor->Release();
//    }
//
//
//    closesocket(sock);
//    WSACleanup();
//    return 0;
//}