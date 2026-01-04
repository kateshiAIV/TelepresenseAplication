#define STB_IMAGE_IMPLEMENTATION
#include <GLFW/stb_image.h>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <windows.h>
#include <vector>
#include <iostream>
#include <Kinect.h>
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>

using namespace cv;





//Own headers
#include "Vertex.h"
using namespace std;


float CAMERA_DISTANCE = 5.0f;
#define ASSET_PATH "C:/Users/kishk/source/repos/OpenGL/";


//логика коллбека для колесика мыши
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	CAMERA_DISTANCE -= (float)yoffset * 0.5f;
	if (CAMERA_DISTANCE < 0.1f) CAMERA_DISTANCE = 0.1f;
	if (CAMERA_DISTANCE > 20.0f) CAMERA_DISTANCE = 20.0f;
}


// Функция для отрисовки точечного облака
void drawPointCloud(const vector<Vertex>& vertices)
{
	glBegin(GL_POINTS);
	for (const auto& v : vertices)
	{
		glColor3f(v.getR(), v.getG(), v.getB());
		glVertex3f(v.getX(), v.getY(), v.getZ());
	}
	glEnd();
}

void createPointCloud(vector<Vertex>& vertices, string colorImagePath, string depthImagePath)
{
	float scale = 10.0f;
	vertices.clear();

	int cw, ch, cc;
	int dw, dh, dc;

	unsigned char* colorData = stbi_load(colorImagePath.c_str(), &cw, &ch, &cc, 3);
	unsigned char* depthData = stbi_load(depthImagePath.c_str(), &dw, &dh, &dc, 1);

	for (int i = 0; i < ch; i++)
	{
		for (int j = 0; j < cw; j++)
		{
			float depth = depthData[i * cw + j] / 255.0f * scale;
			float x = (j - cw / 2.0f) / cw * scale;
			float y = (ch / 2.0f - i) / ch * scale;
			float z = depth;

			int idx = (i * cw + j) * 3;
			float r = colorData[idx] / 255.0f;
			float g = colorData[idx + 1] / 255.0f;
			float b = colorData[idx + 2] / 255.0f;


			//if (depth >= 0.2 && depth <= 1.0) // Фильтрация по глубине
			if (true)
			{
				vertices.push_back({ x, y, z, r, g, b });
			}
		}
	}
	stbi_image_free(colorData);
	stbi_image_free(depthData);
}


int main() 
{

	// Инициализирую GLFW
	if (!glfwInit())
	{
		cout << "Failed to initialize GLFW" << endl;
		return -1;
	}
	// Инициализирую GLEW
	if (!glewInit())
	{
		cout << "Failed to initialize GLEW" << endl;
		return -1;
	}
	// Cоздаю окно GLFW
	GLFWwindow* window = glfwCreateWindow(920, 680, "Telepresense App", nullptr, nullptr);
	if (!window)
	{
		glfwTerminate();
		cout << "Failed to create GLFW window" << endl;
		return -1;
	}

	glfwMakeContextCurrent(window);
	glfwSetScrollCallback(window, scroll_callback);  // коллбек для колесика, передаю функцию scroll_callback
	glEnable(GL_DEPTH_TEST);
	glPointSize(2.0f);




	vector<Vertex> vertices;
	string colorImagePath = "C:/Users/kishk/source/repos/OpenGL/OpenGL/assets/color.jpeg";
	string depthImagePath = "C:/Users/kishk/source/repos/OpenGL/OpenGL/assets/depth.png";
	createPointCloud(vertices, colorImagePath, depthImagePath);



	while (!glfwWindowShouldClose(window))
	{
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		glViewport(0, 0, width, height);
		glClearColor(0.05f, 0.05f, 0.08f, 1.0f); //bg
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		gluPerspective(45.0, (double)width / height, 0.1, 100.0);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glTranslatef(0.0f, 0.0f, -CAMERA_DISTANCE);

		// TODO Kinnect data rendering


		createPointCloud(vertices, colorImagePath, depthImagePath);
		drawPointCloud(vertices);

		glfwSwapBuffers(window);
		glfwPollEvents();

	}
	glfwTerminate();
	return 0;
}

