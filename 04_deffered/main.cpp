#include <iostream>
#include <cassert>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <string>
#include <vector>
#include <array>

#include "ogl_resource.hpp"
#include "error_handling.hpp"
#include "window.hpp"
#include "shader.hpp"


#include "scene_definition.hpp"
#include "renderer.hpp"

#include "ogl_geometry_factory.hpp"
#include "ogl_material_factory.hpp"

#include <glm/gtx/string_cast.hpp>

// Constants
const int screenWidth = 800;
const int screenHeight = 600;

// Helper functions for shader creation
OpenGLResource createProgramFromFile(const std::string& vertexPath, const std::string& fragmentPath) {
	std::string vertexSource = loadShaderSource(vertexPath);
	std::string fragmentSource = loadShaderSource(fragmentPath);
	return createShaderProgram(vertexSource, fragmentSource);
}

bool checkProgramLinkStatus(GLuint program) {
	GLint linkStatus;
	glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
	if (linkStatus == GL_FALSE) {
		GLint logLength;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
		std::vector<GLchar> log(logLength);
		glGetProgramInfoLog(program, logLength, nullptr, log.data());
		std::cerr << "Program link error: " << log.data() << std::endl;
		return false;
	}
	return true;
}

void toggle(const std::string &aToggleName, bool &aToggleValue) {
	aToggleValue = !aToggleValue;
	std::cout << aToggleName << ": " << (aToggleValue ? "ON\n" : "OFF\n");
}

struct Config {
	int currentSceneIdx = 0;
	bool showSolid = true;
	bool showWireframe = false;
	bool useZOffset = false;
	
	// DoF parameters
	float focusDistance = 50.0f;   // Distance to focus point in world units
	float focusRange = 10.0f;      // Range around focus that's sharp
	bool debugDepth = false;       // Debug mode to visualize depth
};

GLuint dofProgram = 0;
OpenGLResource dofProgramResource;

class FullscreenQuad {
public:
	GLuint VAO;
	
	FullscreenQuad() {
		float quadVerts[] = {
			-1.0f, -1.0f, 0.0f, 0.0f,
			 3.0f, -1.0f, 2.0f, 0.0f,
			-1.0f,  3.0f, 0.0f, 2.0f,
		};
		glGenVertexArrays(1, &VAO);
		GLuint VBO;
		glGenBuffers(1, &VBO);
		glBindVertexArray(VAO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVerts), quadVerts, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	}
	
	void draw() {
		glBindVertexArray(VAO);
		glDrawArrays(GL_TRIANGLES, 0, 3);
	}
};

int main() {
	// Initialize GLFW
	if (!glfwInit()) {
		std::cerr << "Failed to initialize GLFW" << std::endl;
		return -1;
	}

	// DoF framebuffer setup will be handled after window creation
	try {
		auto window = Window();
		MouseTracking mouseTracking;
		Config config;
		Camera camera(window.aspectRatio());
		camera.setPosition(glm::vec3(0.0f, 10.0f, 50.0f));
		camera.lookAt(glm::vec3());
		SpotLight light;
		light.setPosition(glm::vec3(25.0f, 40.0f, 30.0f));
		light.lookAt(glm::vec3());



		window.onCheckInput([&camera, &mouseTracking](GLFWwindow *aWin) {
				mouseTracking.update(aWin);
				if (glfwGetMouseButton(aWin, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
					camera.orbit(-0.4f * mouseTracking.offset(), glm::vec3());
				}
			});
		window.setKeyCallback([&config, &camera](GLFWwindow *aWin, int key, int scancode, int action, int mods) {
				if (action == GLFW_PRESS) {
					switch (key) {
					case GLFW_KEY_ENTER:
						camera.setPosition(glm::vec3(0.0f, -10.0f, -50.0f));
						camera.lookAt(glm::vec3());
						break;
					case GLFW_KEY_Q:
						config.focusDistance -= 5.0f;
						config.focusDistance = glm::max(5.0f, config.focusDistance);
						std::cout << "Focus Distance: " << config.focusDistance << std::endl;
						break;
					case GLFW_KEY_E:
						config.focusDistance += 5.0f;
						config.focusDistance = glm::min(100.0f, config.focusDistance);
						std::cout << "Focus Distance: " << config.focusDistance << std::endl;
						break;
					case GLFW_KEY_Z:
						config.focusRange -= 2.0f;
						config.focusRange = glm::max(2.0f, config.focusRange);
						std::cout << "Focus Range: " << config.focusRange << std::endl;
						break;
					case GLFW_KEY_C:
						config.focusRange += 2.0f;
						config.focusRange = glm::min(50.0f, config.focusRange);
						std::cout << "Focus Range: " << config.focusRange << std::endl;
						break;
					case GLFW_KEY_D:
						config.debugDepth = !config.debugDepth;
						std::cout << "Depth Debug: " << (config.debugDepth ? "ON" : "OFF") << std::endl;
						break;
					}
				}
			});

		std::cout << "\n=== DoF Controls ===" << std::endl;
		std::cout << "Q/E: Decrease/Increase Focus Distance" << std::endl;
		std::cout << "Z/C: Decrease/Increase Focus Range" << std::endl;
		std::cout << "D: Toggle Depth Debug" << std::endl;
		std::cout << "ENTER: Reset Camera" << std::endl;
		std::cout << "Mouse: Orbit Camera" << std::endl;

		OGLMaterialFactory materialFactory;
		materialFactory.loadShadersFromDir("./shaders/");
		materialFactory.loadTexturesFromDir("./data/textures/");

		OGLGeometryFactory geometryFactory;


		std::array<SimpleScene, 1> scenes {
			createCottageScene(materialFactory, geometryFactory),
		};

		Renderer renderer(materialFactory);
		window.onResize([&camera, &window, &renderer](int width, int height) {
				camera.setAspectRatio(window.aspectRatio());
				renderer.initialize(width, height);
			});

		dofProgramResource = createProgramFromFile(
			"shaders/passthrough.vertex.glsl",
			"shaders/dof.fragment.glsl"
		);
		dofProgram = dofProgramResource.get();
		
		// Create fullscreen quad after OpenGL is initialized
		FullscreenQuad fullscreenQuad;
		
		// Create framebuffer for compositing result (needed for DoF)
		GLuint compositingFBO, compositingColorTex;
		glGenFramebuffers(1, &compositingFBO);
		glGenTextures(1, &compositingColorTex);
		
		glBindTexture(GL_TEXTURE_2D, compositingColorTex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, window.size()[0], window.size()[1], 0, GL_RGBA, GL_FLOAT, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		
		glBindFramebuffer(GL_FRAMEBUFFER, compositingFBO);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, compositingColorTex, 0);
		
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
			std::cerr << "Compositing FBO not complete!" << std::endl;
		}
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		
		// Initialize renderer first
		renderer.initialize(window.size()[0], window.size()[1]);
		
		// Now we can safely get the textures after initialization
		GLuint colorTexture = renderer.getColorTexture();
		GLuint depthTexture = renderer.getDepthTexture();
		window.runLoop([&] {
			// 1. Shadow map pass
			renderer.shadowMapPass(scenes[config.currentSceneIdx], light);
			
			// 2. Geometry pass (fill G-buffer)
			renderer.clear();
			renderer.geometryPass(scenes[config.currentSceneIdx], camera, RenderOptions{"solid"});

			// 3. Compositing pass (combine G-buffer with lighting) -> render to texture
			glBindFramebuffer(GL_FRAMEBUFFER, compositingFBO);
			glClear(GL_COLOR_BUFFER_BIT);
			renderer.compositingPass(light);

			// 4. DoF pass (apply depth of field effect) -> render to screen
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glClear(GL_COLOR_BUFFER_BIT);
			
			glUseProgram(dofProgram);
			glUniform1f(glGetUniformLocation(dofProgram, "focus_distance"), config.focusDistance); // Use actual world units
			glUniform1f(glGetUniformLocation(dofProgram, "focus_range"), config.focusRange);
			glUniform1f(glGetUniformLocation(dofProgram, "near_plane"), camera.near());
			glUniform1f(glGetUniformLocation(dofProgram, "far_plane"), camera.far());
			glUniform1i(glGetUniformLocation(dofProgram, "debug_depth"), config.debugDepth ? 1 : 0);
			glUniform1i(glGetUniformLocation(dofProgram, "color_texture"), 0);
			glUniform1i(glGetUniformLocation(dofProgram, "depth_texture"), 1);

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, compositingColorTex); // Use composited result
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, depthTexture); // Use depth from G-buffer

			fullscreenQuad.draw();
		});
	} catch (ShaderCompilationError &exc) {
		std::cerr
			<< "Shader compilation error!\n"
			<< "Shader type: " << exc.shaderTypeName()
			<<"\nError: " << exc.what() << "\n";
		return -3;
	} catch (OpenGLError &exc) {
		std::cerr << "OpenGL error: " << exc.what() << "\n";
		return -2;
	} catch (std::exception &exc) {
		std::cerr << "Error: " << exc.what() << "\n";
		return -1;
	}

	glfwTerminate();
	return 0;
}
