#include <stdio.h>
#include <math.h>

#include <ew/external/glad.h>
#include <ew/shader.h>
#include <ew/model.h>
#include <ew/procGen.h>
#include <ew/camera.h>
#include <ew/transform.h>
#include <ew/cameraController.h>
#include <ew/texture.h>

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <iostream>

void framebufferSizeCallback(GLFWwindow* window, int width, int height);
GLFWwindow* initWindow(const char* title, int width, int height);
void drawUI();

//Global state
int screenWidth = 1080;
int screenHeight = 720;
float prevFrameTime;
float deltaTime;

struct Material {
	float Ka = 1.0;
	float Kd = 0.5;
	float Ks = 0.5;
	float Shininess = 128;
}material;

// Init Assets
ew::Transform tralaTransform;
ew::Transform planeTransform;
ew::Camera camera;
ew::CameraController cameraController;

unsigned int ppFBO;
unsigned int ppRBO;
unsigned int ppTex;
unsigned int depTex;
unsigned int dummyVAO;

void resetCamera(ew::Camera* camera, ew::CameraController* controller) {
	camera->position = glm::vec3(0, 0, 5.0f);
	camera->target = glm::vec3(0);
	controller->yaw = controller->pitch = 0;
}

int main() {
	// Init Window
	GLFWwindow* window = initWindow("Beautiful Awesome OpenGL", screenWidth, screenHeight);
	glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

	ew::Shader litShader = ew::Shader("assets/lit.vert", "assets/lit.frag");
	ew::Shader ppShader = ew::Shader("assets/pp.vert", "assets/pp.frag");

	ew::Model tralaModel = ew::Model("assets/tralalero_tralala.fbx");
	GLuint tralaTexture = ew::loadTexture("assets/tralalero_color.png");
	ew::Mesh plane = ew::Mesh(ew::createPlane(4.0f, 4.0f, 1));
	GLuint glowTexture = ew::loadTexture("assets/plane_color.jpg");

	tralaTransform.scale *= 0.1f;

	camera.position = glm::vec3(0.0f, 2.0f, 4.0f);
	camera.target = glm::vec3(0.0f, 0.0f, 0.0f);
	camera.aspectRatio = (float)screenWidth / screenHeight;
	camera.fov = 90.0f;

	// Init Renderer
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glEnable(GL_DEPTH_TEST);

	// Set Up Post Processing Frame Buffer
	glGenFramebuffers(1, &ppFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, ppFBO);

	glGenTextures(1, &ppTex);
	glBindTexture(GL_TEXTURE_2D, ppTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1080, 720, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ppTex, 0);

	glGenRenderbuffers(1, &ppRBO);
	glBindRenderbuffer(GL_RENDERBUFFER, ppRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, 1080, 720);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, ppRBO);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glCreateVertexArrays(1, &dummyVAO);

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		float time = (float)glfwGetTime();
		deltaTime = time - prevFrameTime;
		prevFrameTime = time;

		//RENDER
		tralaTransform.rotation = glm::rotate(tralaTransform.rotation, deltaTime, glm::vec3(0.0, 1.0, 0.0));

		cameraController.move(window, &camera, deltaTime);

		// FIRST PASS
		glBindFramebuffer(GL_FRAMEBUFFER, ppFBO);
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);

		glBindTextureUnit(0, tralaTexture);
		litShader.use();
		litShader.setInt("_MainTex", 0);
		litShader.setVec3("_EyePos", camera.position);
		litShader.setMat4("_Model", tralaTransform.modelMatrix());
		litShader.setMat4("_ViewProjection", camera.projectionMatrix() * camera.viewMatrix());
		litShader.setFloat("_Material.Ka", material.Ka);
		litShader.setFloat("_Material.Kd", material.Kd);
		litShader.setFloat("_Material.Ks", material.Ks);
		litShader.setFloat("_Material.Shininess", material.Shininess);
		litShader.setInt("_Settings.NormalMap", false);
		tralaModel.draw();

		glBindTextureUnit(0, glowTexture);
		litShader.setInt("_MainTex", 0);
		litShader.setInt("_Settings.NormalMap", false);
		litShader.setMat4("_Model", planeTransform.modelMatrix());
		plane.draw();

		// SECOND PASS
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		ppShader.use();
		glBindVertexArray(dummyVAO);
		glDisable(GL_DEPTH_TEST);
		glBindTextureUnit(0, ppTex);
		ppShader.setInt("_ScreenTexture", 0);
		glDrawArrays(GL_TRIANGLES, 0, 3);

		drawUI();

		glfwSwapBuffers(window);
	}

	glDeleteFramebuffers(1, &ppFBO);

	printf("Shutting down...");
}

void drawUI() {
	ImGui_ImplGlfw_NewFrame();
	ImGui_ImplOpenGL3_NewFrame();
	ImGui::NewFrame();

	ImGui::Begin("Settings");
	if (ImGui::Button("Reset Camera")) {
		resetCamera(&camera, &cameraController);
	}
	if (ImGui::CollapsingHeader("Material")) {
		ImGui::SliderFloat("AmbientK", &material.Ka, 0.0f, 1.0f);
		ImGui::SliderFloat("DiffuseK", &material.Kd, 0.0f, 1.0f);
		ImGui::SliderFloat("SpecularK", &material.Ks, 0.0f, 1.0f);
		ImGui::SliderFloat("Shininess", &material.Shininess, 2.0f, 1024.0f);
	}
	ImGui::End();

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
	screenWidth = width;
	screenHeight = height;
}

GLFWwindow* initWindow(const char* title, int width, int height) {
	printf("Initializing...");
	if (!glfwInit()) {
		printf("GLFW failed to init!");
		return nullptr;
	}

	GLFWwindow* window = glfwCreateWindow(width, height, title, NULL, NULL);
	if (window == NULL) {
		printf("GLFW failed to create window");
		return nullptr;
	}
	glfwMakeContextCurrent(window);

	if (!gladLoadGL(glfwGetProcAddress)) {
		printf("GLAD Failed to load GL headers");
		return nullptr;
	}

	//Initialize ImGUI
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init();

	return window;
}

