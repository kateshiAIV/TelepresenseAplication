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
#include <Kinect.h>

using namespace std;

// #include "Vertex.h"
#include "UDPSender.h"

#pragma comment(lib, "ws2_32.lib")

struct Vertex
{
    float x, y, z;
    float r, g, b;
};

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

    // create fake sphere-like cloud
    for (int i = 0; i < 500; i++)
    {
        cloud.push_back({
            (float)(rand() % 100) / 50.0f - 1.0f,
            (float)(rand() % 100) / 50.0f - 1.0f,
            (float)(rand() % 100) / 50.0f - 1.0f,
            1.0f, 0.2f, 0.8f
            });
    }

    std::cout << "sending loop...\n";

    while (true)
    {
        sendto(
            sock,
            (char*)cloud.data(),
            cloud.size() * sizeof(Vertex),
            0,
            (sockaddr*)&addr,
            sizeof(addr)
        );

        std::cout << "sent packet: " << cloud.size() << " points\n";

        Sleep(33); // ~30 FPS
    }

    closesocket(sock);
    WSACleanup();
}

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
//
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
//        if (pColorFrame) pColorFrame->Release();
//        if (pDepthFrame) pDepthFrame->Release();
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
//    return 0;
//}