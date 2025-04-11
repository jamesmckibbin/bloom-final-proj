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
	float AmbientMod = 1.0;
	float DiffuseMod = 0.5;
	float SpecularMod = 0.5;
	float Shininess = 16;
}material;

struct DirLight {
	glm::vec3 direction;
	glm::vec3 ambient;
	glm::vec3 diffuse;
};

struct PointLight {
	glm::vec3 position;
	float constant;
	float linear;
	float quadratic;
	glm::vec3 ambient;
	glm::vec3 diffuse;
	float intensity;
};

// Init Assets
ew::Transform tralaTransform;
ew::Transform planeTransform;
ew::Transform cube1Transform;
ew::Transform cube2Transform;
ew::Camera camera;
ew::CameraController cameraController;

DirLight globalLight;
PointLight glowCube;
PointLight blueCube;

unsigned int postProcessFBO;
unsigned int postProcessRBO;
unsigned int colorBuffers[2];
unsigned int pingPongFBO[2];
unsigned int pingPongBuffers[2];
unsigned int dummyVAO;

float gamma = 2.2f;
float exposure = 1.0f;
int pingPongAmount = 10;
bool showBrightnessMap = false;
bool showBlurMap = false;

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
	ew::Shader unlitShader = ew::Shader("assets/unlit.vert", "assets/unlit.frag");
	ew::Shader postProcessShader = ew::Shader("assets/postprocess.vert", "assets/postprocess.frag");
	ew::Shader pingPongShader = ew::Shader("assets/pingpong.vert", "assets/pingpong.frag");

	ew::Model tralaModel = ew::Model("assets/tralalero_tralala.fbx");
	GLuint tralaTexture = ew::loadTexture("assets/tralalero_color.png");

	ew::Mesh plane = ew::Mesh(ew::createPlane(4.0f, 4.0f, 1));
	GLuint metalTexture = ew::loadTexture("assets/metal_color.png");
	GLuint metalNormal = ew::loadTexture("assets/metal_normal.png");

	ew::Mesh cube1 = ew::Mesh(ew::createCube(1.0f));
	GLuint glowTexture = ew::loadTexture("assets/glow_color.jpg");
	ew::Mesh cube2 = ew::Mesh(ew::createCube(1.0f));
	GLuint blueTexture = ew::loadTexture("assets/blue_color.jpg");

	tralaTransform.scale *= 0.1f;

	cube1Transform.position = glm::vec3(2.0f, 1.0f, 0.0f);
	cube2Transform.position = glm::vec3(-2.0f, 1.0f, 0.0f);

	camera.position = glm::vec3(0.0f, 2.0f, 4.0f);
	camera.target = glm::vec3(0.0f, 0.0f, 0.0f);
	camera.aspectRatio = (float)screenWidth / screenHeight;
	camera.fov = 60.0f;

	globalLight.direction = glm::vec3(0.0f, -1.0f, 0.0f);
	globalLight.diffuse = glm::vec3(1.0f);
	globalLight.ambient = glm::vec3(0.3, 0.4, 0.46);

	glowCube.position = cube1Transform.position;
	glowCube.diffuse = glm::vec3(1.0f, 1.0f, 0.0f);
	glowCube.intensity = 5.0f;
	glowCube.ambient = glm::vec3(0.3, 0.4, 0.46);
	glowCube.constant = 1.0f;
	glowCube.linear = 0.022f;
	glowCube.quadratic = 0.0019f;

	blueCube.position = cube2Transform.position;
	blueCube.diffuse = glm::vec3(0.0f, 0.0f, 1.0f);
	blueCube.intensity = 10.0f;
	blueCube.ambient = glm::vec3(0.3, 0.4, 0.46);
	blueCube.constant = 1.0f;
	blueCube.linear = 0.022f;
	blueCube.quadratic = 0.0019f;


	// Init Renderer
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glEnable(GL_DEPTH_TEST);

	glGenFramebuffers(2, pingPongFBO);
	glGenTextures(2, pingPongBuffers);
	for (unsigned int i = 0; i < 2; i++)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, pingPongFBO[i]);
		glBindTexture(GL_TEXTURE_2D, pingPongBuffers[i]);
		glTexImage2D(
			GL_TEXTURE_2D, 0, GL_RGBA16F, 1080, 720, 0, GL_RGBA, GL_FLOAT, NULL
		);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glFramebufferTexture2D(
			GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingPongBuffers[i], 0
		);
	}

	// Set Up Post Processing Frame Buffer
	glGenFramebuffers(1, &postProcessFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, postProcessFBO);

	glGenTextures(2, colorBuffers);
	for (unsigned int i = 0; i < 2; i++)
	{
		glBindTexture(GL_TEXTURE_2D, colorBuffers[i]);
		glTexImage2D(
			GL_TEXTURE_2D, 0, GL_RGBA16F, 1080, 720, 0, GL_RGBA, GL_FLOAT, NULL
		);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorBuffers[i], 0);
	}

	unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
	glDrawBuffers(2, attachments);

	glGenRenderbuffers(1, &postProcessRBO);
	glBindRenderbuffer(GL_RENDERBUFFER, postProcessRBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, 1080, 720);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, postProcessRBO);

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
		glBindFramebuffer(GL_FRAMEBUFFER, postProcessFBO);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);

		glBindTextureUnit(0, tralaTexture);
		litShader.use();
		litShader.setInt("_MainTex", 0);
		litShader.setVec3("_EyePos", camera.position);
		litShader.setMat4("_Model", tralaTransform.modelMatrix());
		litShader.setMat4("_ViewProjection", camera.projectionMatrix() * camera.viewMatrix());
		litShader.setFloat("_Material.AmbientMod", material.AmbientMod);
		litShader.setFloat("_Material.DiffuseMod", material.DiffuseMod);
		litShader.setFloat("_Material.SpecularMod", material.SpecularMod);
		litShader.setFloat("_Material.Shininess", material.Shininess);
		litShader.setInt("_Settings.NormalMap", false);
		litShader.setVec3("_GlobalLight.direction", globalLight.direction);
		litShader.setVec3("_GlobalLight.diffuse", globalLight.diffuse);
		litShader.setVec3("_GlobalLight.ambient", globalLight.ambient);
		litShader.setVec3("_Cube1Light.position", glowCube.position);
		litShader.setVec3("_Cube1Light.diffuse", glowCube.diffuse);
		litShader.setVec3("_Cube1Light.ambient", glowCube.ambient);
		litShader.setFloat("_Cube1Light.constant", glowCube.constant);
		litShader.setFloat("_Cube1Light.linear", glowCube.linear);
		litShader.setFloat("_Cube1Light.quadratic", glowCube.quadratic);
		litShader.setFloat("_Cube1Light.intensity", glowCube.intensity);
		litShader.setVec3("_Cube2Light.position", blueCube.position);
		litShader.setVec3("_Cube2Light.diffuse", blueCube.diffuse);
		litShader.setVec3("_Cube2Light.ambient", blueCube.ambient);
		litShader.setFloat("_Cube2Light.constant", blueCube.constant);
		litShader.setFloat("_Cube2Light.linear", blueCube.linear);
		litShader.setFloat("_Cube2Light.quadratic", blueCube.quadratic);
		litShader.setFloat("_Cube2Light.intensity", blueCube.intensity);
		tralaModel.draw();

		glBindTextureUnit(0, metalTexture);
		glBindTextureUnit(1, metalNormal);
		litShader.setInt("_MainTex", 0);
		litShader.setInt("_Settings.NormalMap", true);
		litShader.setMat4("_Model", planeTransform.modelMatrix());
		plane.draw();

		unlitShader.use();
		glBindTextureUnit(0, glowTexture);
		unlitShader.setInt("_MainTex", 0);
		unlitShader.setMat4("_Model", cube1Transform.modelMatrix());
		unlitShader.setMat4("_ViewProjection", camera.projectionMatrix() * camera.viewMatrix());
		cube1.draw();

		glBindTextureUnit(0, blueTexture);
		unlitShader.setInt("_MainTex", 0);
		unlitShader.setMat4("_Model", cube2Transform.modelMatrix());
		cube2.draw();

		// PING PONG PASS
		bool horizontal = true, first_iteration = true;
		pingPongShader.use();
		for (unsigned int i = 0; i < pingPongAmount; i++)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, pingPongFBO[horizontal]);
			pingPongShader.setInt("horizontal", horizontal);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(
				GL_TEXTURE_2D, first_iteration ? colorBuffers[1] : pingPongBuffers[!horizontal]
			);
			glBindVertexArray(dummyVAO);
			glDisable(GL_DEPTH_TEST);
			glDrawArrays(GL_TRIANGLES, 0, 3);
			horizontal = !horizontal;
			if (first_iteration)
				first_iteration = false;
		}

		// SECOND PASS
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		postProcessShader.use();
		glBindVertexArray(dummyVAO);
		glDisable(GL_DEPTH_TEST);
		if (showBrightnessMap) {
			glBindTextureUnit(0, colorBuffers[1]);
			glBindTextureUnit(1, colorBuffers[1]);
		}
		else if (showBlurMap){
			glBindTextureUnit(0, pingPongBuffers[0]);
			glBindTextureUnit(1, pingPongBuffers[0]);
		}
		else {
			glBindTextureUnit(0, colorBuffers[0]);
			glBindTextureUnit(1, pingPongBuffers[0]);
		}
		postProcessShader.setInt("_ScreenTexture", 0);
		postProcessShader.setInt("_BloomBlur", 1);
		postProcessShader.setFloat("_Exposure", exposure);
		postProcessShader.setFloat("_Gamma", gamma);
		glDrawArrays(GL_TRIANGLES, 0, 3);

		drawUI();

		glfwSwapBuffers(window);
	}

	glDeleteFramebuffers(1, &postProcessFBO);

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
	if (ImGui::CollapsingHeader("Bloom & HDR")) {
		ImGui::SliderFloat("Gamma", &gamma, 0.5f, 4.0f);
		ImGui::SliderFloat("Exposure", &exposure, 0.1f, 5.0f);
		ImGui::SliderInt("Ping Pong Amount", &pingPongAmount, 4, 50);
		ImGui::Checkbox("Show Brightness Map", &showBrightnessMap);
		ImGui::Checkbox("Show Blur Map", &showBlurMap);
	}
	if (ImGui::CollapsingHeader("Material")) {
		ImGui::SliderFloat("AmbientK", &material.AmbientMod, 0.0f, 1.0f);
		ImGui::SliderFloat("DiffuseK", &material.DiffuseMod, 0.0f, 1.0f);
		ImGui::SliderFloat("SpecularK", &material.SpecularMod, 0.0f, 1.0f);
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

