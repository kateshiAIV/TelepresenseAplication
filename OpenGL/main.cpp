#define STB_IMAGE_IMPLEMENTATION
#include <GLFW/stb_image.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <windows.h>
#include <vector>
#include <iostream>
#include <Kinect.h>

struct Vertex {
    float x, y, z;
    float r, g, b;
};

float cameraDistance = 5.0f;

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    cameraDistance -= (float)yoffset * 0.5f;
    if (cameraDistance < 0.1f) cameraDistance = 0.1f;
    if (cameraDistance > 20.0f) cameraDistance = 20.0f;
}

int main() {

    IKinectSensor* sensor = nullptr;

    if (FAILED(GetDefaultKinectSensor(&sensor)) || !sensor)
    {
        std::cout << "No Kinect detected\n";
        return -1;
    }

    sensor->Open();
    std::cout << "Kinect initialized successfully!\n";

    sensor->Close();
    sensor->Release();


    if (!glfwInit()) return -1;
    GLFWwindow* window = glfwCreateWindow(800, 600, "Point Cloud", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }

    glfwMakeContextCurrent(window);
    glewInit();
    glfwSetScrollCallback(window, scroll_callback);
    glEnable(GL_DEPTH_TEST);
    glPointSize(2.0f);

    int w, h, c;
    int dw, dh, dc;
    unsigned char* colorData = stbi_load("C:/Users/kishk/source/repos/OpenGL/color.jpeg", &w, &h, &c, 3);
    unsigned char* depthData = stbi_load("C:/Users/kishk/source/repos/OpenGL/depth.png", &dw, &dh, &dc, 1);


    if (!colorData) { std::cerr << "Failed to load color image\n"; return -1; }
    if (!depthData || w != dw || h != dh) { std::cerr << "Depth image invalid\n"; return -1; }

    // Generate vertices
    std::vector<Vertex> vertices;
    float scale = 5.0f; // scale for depth
    for (int j = 0; j < h; ++j) {
        for (int i = 0; i < w; ++i) {
            float depth = depthData[j * w + i] / 255.0f * scale; // normalized depth
            float x = (i - w / 2.0f) / w * scale;
            float y = (h / 2.0f - j) / h * scale;
            float z = depth;

            int idx = (j * w + i) * 3;
            float r = colorData[idx] / 255.0f;
            float g = colorData[idx + 1] / 255.0f;
            float b = colorData[idx + 2] / 255.0f;
			// depth filtering
            if(depth >= 4 && depth<=5) vertices.push_back({ x, y, z, r, g, b });
        }
    }

    stbi_image_free(colorData);
    stbi_image_free(depthData);

    while (!glfwWindowShouldClose(window)) {
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);
        glClearColor(0.05f, 0.05f, 0.08f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluPerspective(45.0, (double)width / height, 0.1, 100.0);

        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glTranslatef(0.0f, 0.0f, -cameraDistance);

        glBegin(GL_POINTS);
        for (auto& v : vertices) {
            glColor3f(v.r, v.g, v.b);
            glVertex3f(v.x, v.y, v.z);
        }
        glEnd();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
