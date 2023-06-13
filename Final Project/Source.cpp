#include <iostream>         // cout, cerr
#include <cstdlib>          // EXIT_FAILURE
#include <GL/glew.h>        // GLEW library
#include <GLFW/glfw3.h>     // GLFW library
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>      // Image loading Utility functions

// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <learnOpengl/camera.h> // Camera class

#include "meshes.h"

using namespace std; // Standard namespace

/*Shader program Macro*/
#ifndef GLSL
#define GLSL(Version, Source) "#version " #Version " core \n" #Source
#endif

// Unnamed namespace
namespace
{
	const char* const WINDOW_TITLE = "Willem Kroeger - Final Project CS 330"; // Macro for window title

	// Variables for window width and height
	const int WINDOW_WIDTH = 800;
	const int WINDOW_HEIGHT = 600;

	// Stores the GL data relative to a given mesh
	struct GLMesh
	{
		GLuint vao;         // Handle for the vertex array object
		GLuint vbos[2];     // Handles for the vertex buffer objects
		GLuint nIndices;    // Number of indices of the mesh
	};

	// Main GLFW window
	GLFWwindow* gWindow = nullptr;
	// Triangle mesh data
	// Textures
	GLuint gTextureCork;
	GLuint gTextureFlame;
	GLuint gTextureSilver;
	GLuint gTextureCandle;
	GLuint gTexturePot;
	GLuint gTexturePlane;
	glm::vec2 gUVScale(1.0f, 1.0f);
	GLint gTexWrapMode = GL_REPEAT;


	// Shader program
	GLuint gSurfaceProgramId;
	GLuint gLightProgramId;

	// camera
	Camera gCamera(glm::vec3(0.0f, 0.0f, 3.0f));
	float gLastX = WINDOW_WIDTH / 2.0f;
	float gLastY = WINDOW_HEIGHT / 2.0f;
	bool gFirstMouse = true;
	bool isOrthographic = false;

	// timing
	float gDeltaTime = 0.0f; // time between current frame and last frame
	float gLastFrame = 0.0f;

	//Shape Meshes from Professor Brian
	Meshes meshes;
}

/* User-defined Function prototypes to:
 * initialize the program, set the window size,
 * redraw graphics on the window when resized,
 * and render graphics on the screen
 */
bool UInitialize(int, char* [], GLFWwindow** window);
void UResizeWindow(GLFWwindow* window, int width, int height);
void UProcessInput(GLFWwindow* window);
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos);
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
//void UCreateMesh(GLMesh &mesh);
//void UDestroyMesh(GLMesh &mesh);
bool UCreateTexture(const char* filename, GLuint& textureId);
void UDestroyTexture(GLuint textureId);
void URender();
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId);
void UDestroyShaderProgram(GLuint programId);

///////////////////////////////////////////////////////////////////////////////////////////////////////
/* Surface Vertex Shader Source Code*/
const GLchar* surfaceVertexShaderSource = GLSL(440,

	layout(location = 0) in vec3 vertexPosition; // VAP position 0 for vertex position data
layout(location = 1) in vec3 vertexNormal; // VAP position 1 for normals
layout(location = 2) in vec2 textureCoordinate;

out vec3 vertexFragmentNormal; // For outgoing normals to fragment shader
out vec3 vertexFragmentPos; // For outgoing color / pixels to fragment shader
out vec2 vertexTextureCoordinate;

//Uniform / Global variables for the  transform matrices
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;



void main()
{
	gl_Position = projection * view * model * vec4(vertexPosition, 1.0f); // Transforms vertices into clip coordinates

	vertexFragmentPos = vec3(model * vec4(vertexPosition, 1.0f)); // Gets fragment / pixel position in world space only (exclude view and projection)

	vertexFragmentNormal = mat3(transpose(inverse(model))) * vertexNormal; // get normal vectors in world space only and exclude normal translation properties
	vertexTextureCoordinate = textureCoordinate;
}
);

////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
/* Surface Fragment Shader Source Code*/
const GLchar* surfaceFragmentShaderSource = GLSL(440,

	in vec3 vertexFragmentNormal; // For incoming normals
in vec3 vertexFragmentPos; // For incoming fragment position
in vec2 vertexTextureCoordinate;

out vec4 fragmentColor; // For outgoing cube color to the GPU

// Uniform / Global variables for object color, light color, light position, and camera/view position
uniform vec4 objectColor;
uniform vec3 ambientColor;
uniform vec3 light1Color;
uniform vec3 light1Position;
uniform vec3 light2Color;
uniform vec3 light2Position;
uniform vec3 viewPosition;
uniform sampler2D uTexture; // Useful when working with multiple textures
uniform vec2 uvScale;
uniform bool ubHasTexture;
uniform float ambientStrength = 0.1f; // Set ambient or global lighting strength
uniform float specularIntensity1 = 0.8f;
uniform float highlightSize1 = 16.0f;
uniform float specularIntensity2 = 0.8f;
uniform float highlightSize2 = 16.0f;


void main()
{
	/*Phong lighting model calculations to generate ambient, diffuse, and specular components*/

	//Calculate Ambient lighting
	vec3 ambient = ambientStrength * ambientColor; // Generate ambient light color

	//**Calculate Diffuse lighting**
	vec3 norm = normalize(vertexFragmentNormal); // Normalize vectors to 1 unit
	vec3 light1Direction = normalize(light1Position - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube
	float impact1 = max(dot(norm, light1Direction), 0.0);// Calculate diffuse impact by generating dot product of normal and light
	vec3 diffuse1 = impact1 * light1Color * 1.0; // Generate diffuse light color
	vec3 light2Direction = normalize(light2Position - vertexFragmentPos); // Calculate distance (light direction) between light source and fragments/pixels on cube
	float impact2 = max(dot(norm, light2Direction), 0.0);// Calculate diffuse impact by generating dot product of normal and light
	vec3 diffuse2 = impact2 * light2Color * 0.1; // Generate diffuse light color

	//**Calculate Specular lighting**
	vec3 viewDir = normalize(viewPosition - vertexFragmentPos); // Calculate view direction
	vec3 reflectDir1 = reflect(-light1Direction, norm);// Calculate reflection vector
	//Calculate specular component
	float specularComponent1 = pow(max(dot(viewDir, reflectDir1), 0.0), highlightSize1);
	vec3 specular1 = specularIntensity1 * specularComponent1 * light1Color;
	vec3 reflectDir2 = reflect(-light2Direction, norm);// Calculate reflection vector
	//Calculate specular component
	float specularComponent2 = pow(max(dot(viewDir, reflectDir2), 0.0), highlightSize2);
	vec3 specular2 = specularIntensity2 * specularComponent2 * light2Color;

	//**Calculate phong result**
	//Texture holds the color to be used for all three components
	vec4 textureColor = texture(uTexture, vertexTextureCoordinate * uvScale);
	vec3 phong1;
	vec3 phong2;

	if (ubHasTexture == true)
	{
		phong1 = (ambient + diffuse1 + specular1) * textureColor.xyz;
		phong2 = (ambient + diffuse2 + specular2) * textureColor.xyz;
	}
	else
	{
		phong1 = (ambient + diffuse1 + specular1) * objectColor.xyz;
		phong2 = (ambient + diffuse2 + specular2) * objectColor.xyz;
	}

	fragmentColor = vec4(phong1 + phong2, 1.0); // Send lighting results to GPU
	//fragmentColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
}
);
/////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////

/* Light Object Shader Source Code*/
const GLchar* lightVertexShaderSource = GLSL(330,
	layout(location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;


void main()
{
	gl_Position = projection * view * model * vec4(aPos, 1.0);
}
);
/////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////
/* Light Object Shader Source Code*/
const GLchar* lightFragmentShaderSource = GLSL(330,
	out vec4 FragColor;

void main()
{
	FragColor = vec4(1.0); // set all 4 vector values to 1.0
}
);
/////////////////////////////////////////////////////////////////////////////////////////////////////////

/* Fragment Shader Source Code*/
const GLchar* fragmentShaderSource = GLSL(440,
	in vec4 vertexColor; // Variable to hold incoming color data from vertex shader
	in vec2 vertexTextureCoordinate;

out vec4 fragmentColor;
uniform vec4 uObjectColor;

uniform sampler2D uTexture;
uniform vec2 uvScale;

void main()
{
	//fragmentColor = vec4(vertexColor);
	fragmentColor = texture(uTexture, vertexTextureCoordinate * uvScale);
}
);

// Images are loaded with Y axis going down, but OpenGL's Y axis goes up, so let's flip it
void flipImageVertically(unsigned char* image, int width, int height, int channels)
{
	for (int j = 0; j < height / 2; ++j)
	{
		int index1 = j * width * channels;
		int index2 = (height - 1 - j) * width * channels;

		for (int i = width * channels; i > 0; --i)
		{
			unsigned char tmp = image[index1];
			image[index1] = image[index2];
			image[index2] = tmp;
			++index1;
			++index2;
		}
	}
}

int main(int argc, char* argv[])
{
	if (!UInitialize(argc, argv, &gWindow))
		return EXIT_FAILURE;

	// Create the mesh
	//UCreateMesh(gMesh); // Calls the function to create the Vertex Buffer Object
	meshes.CreateMeshes();

	// Create the shader program
	if (!UCreateShaderProgram(surfaceVertexShaderSource, surfaceFragmentShaderSource, gSurfaceProgramId))
		return EXIT_FAILURE;

	if (!UCreateShaderProgram(lightVertexShaderSource, lightFragmentShaderSource, gLightProgramId))
		return EXIT_FAILURE;

	// Load textures
	const char* texCork = "cork.jpg";
	const char* texPot = "pot.jpg";
	const char* texPlane = "wood.jpg";
	const char* texCandle = "brown glass.jpg";
	const char* texSilver = "silver.jpg";
	const char* texFlame = "flame.png";
	if (!UCreateTexture(texCork, gTextureCork))
	{
		cout << "Failed to load texture " << gTextureCork << endl;
		return EXIT_FAILURE;
	}
	if (!UCreateTexture(texFlame, gTextureFlame))
		{
			cout << "Failed to load texture " << gTextureFlame << endl;
			return EXIT_FAILURE;
		}
	if (!UCreateTexture(texSilver, gTextureSilver))
	{
		cout << "Failed to load texture " << gTextureSilver << endl;
		return EXIT_FAILURE;
	}
	if (!UCreateTexture(texCandle, gTextureCandle))
	{
		cout << "Failed to load texture " << gTextureCandle << endl;
		return EXIT_FAILURE;
	}
	if (!UCreateTexture(texPot, gTexturePot))
	{
		cout << "Failed to load texture " << texPot << endl;
		return EXIT_FAILURE;
	}
	if (!UCreateTexture(texPlane, gTexturePlane))
	{
		cout << "Failed to load texture " << texPlane << endl;
		return EXIT_FAILURE;
	}
	// tell opengl for each sampler to which texture unit it belongs to(only has to be done once)
		glUseProgram(gSurfaceProgramId);
	// We set the texture as texture unit 0
	glUniform1i(glGetUniformLocation(gSurfaceProgramId, "uTexture"), 0);
	glUniform2f(glGetUniformLocation(gSurfaceProgramId, "uvScale"), gUVScale.x, gUVScale.y);

	// Sets the background color of the window to black (it will be implicitely used by glClear)
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	// render loop
	// -----------
	while (!glfwWindowShouldClose(gWindow))
	{

		// per-frame timing
		// --------------------
		float currentFrame = glfwGetTime();
		gDeltaTime = currentFrame - gLastFrame;
		gLastFrame = currentFrame;

		// input
		// -----
		UProcessInput(gWindow);

		// Render this frame
		URender();

		glfwPollEvents();
	}

	// Release mesh data
	//UDestroyMesh(gMesh);
	meshes.DestroyMeshes();

	// Release textures
	UDestroyTexture(gTextureCork);
	UDestroyTexture(gTextureFlame);
	UDestroyTexture(gTextureSilver);
	UDestroyTexture(gTextureCandle);
	UDestroyTexture(gTexturePot);
	UDestroyTexture(gTexturePlane);

	// Release shader program
	UDestroyShaderProgram(gSurfaceProgramId);
	UDestroyShaderProgram(gLightProgramId);

	exit(EXIT_SUCCESS); // Terminates the program successfully
}


// Initialize GLFW, GLEW, and create a window
bool UInitialize(int argc, char* argv[], GLFWwindow** window)
{
	// GLFW: initialize and configure
	// ------------------------------
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	// GLFW: window creation
	// ---------------------
	* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);
	if (*window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return false;
	}
	glfwMakeContextCurrent(*window);
	glfwSetFramebufferSizeCallback(*window, UResizeWindow);
	glfwSetCursorPosCallback(*window, UMousePositionCallback);
	glfwSetScrollCallback(*window, UMouseScrollCallback);
	glfwSetMouseButtonCallback(*window, UMouseButtonCallback);

	// tell GLFW to capture our mouse
	glfwSetInputMode(*window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// GLEW: initialize
	// ----------------
	// Note: if using GLEW version 1.13 or earlier
	glewExperimental = GL_TRUE;
	GLenum GlewInitResult = glewInit();

	if (GLEW_OK != GlewInitResult)
	{
		std::cerr << glewGetErrorString(GlewInitResult) << std::endl;
		return false;
	}

	// Displays GPU OpenGL version
	cout << "INFO: OpenGL Version: " << glGetString(GL_VERSION) << endl;

	return true;
}


// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void UProcessInput(GLFWwindow* window)
{

	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	// Switch between orthographic and perspective views

	static bool orthoToggle = false; // Variable to track if Ortho

	if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS && !orthoToggle)
	{
		// Toggle the view mode
		isOrthographic = !isOrthographic;
		glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);  // Adjust viewport to match window size
		orthoToggle = true; // Set the flag to indicate Orthographic mode

		 // Set the camera position, target, and up vector for orthographic view
		gCamera.Position = glm::vec3(-5.0f, 0.0f, 8.0f);
		gCamera.Front = glm::vec3(0.0f, 0.0f, -1.0f);   // Set camera front vector
		gCamera.Up = glm::vec3(0.0f, 1.0f, 0.0f);       // Set camera up vector

		// Update the camera vectors
		gCamera.Right = glm::normalize(glm::cross(gCamera.Front, gCamera.Up));

		// Recalculate the view matrix using the updated camera vectors
		glm::lookAt(gCamera.Position, gCamera.Position + gCamera.Front, gCamera.Up);
	}
	if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS && orthoToggle)
	{
		isOrthographic = !isOrthographic;
		glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);  // Adjust viewport to match window size
		orthoToggle = false; // Reset Orthographic mode when 'P' key is released
	}

	//Camera Movements
	if (glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS) //Resets camera position to center
		gCamera.Position = glm::vec3(0.0f, 0.0f, 0.0f);
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		gCamera.ProcessKeyboard(FORWARD, gDeltaTime);
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		gCamera.ProcessKeyboard(BACKWARD, gDeltaTime);
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		gCamera.ProcessKeyboard(LEFT, gDeltaTime);
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		gCamera.ProcessKeyboard(RIGHT, gDeltaTime);

	float cameraOffset = gCamera.MovementSpeed * gDeltaTime;

	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
		gCamera.Position += gCamera.Up * cameraOffset;
	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		gCamera.Position -= gCamera.Up * cameraOffset;

}


// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void UResizeWindow(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}
// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void UMousePositionCallback(GLFWwindow* window, double xpos, double ypos)
{
	if (gFirstMouse)
	{
		gLastX = xpos;
		gLastY = ypos;
		gFirstMouse = false;
	}

	float xoffset = xpos - gLastX;
	float yoffset = gLastY - ypos; // reversed since y-coordinates go from bottom to top

	gLastX = xpos;
	gLastY = ypos;

	if (!isOrthographic) {
		gCamera.ProcessMouseMovement(xoffset, yoffset);
	}
}



// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void UMouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
	gCamera.ProcessMouseScroll(yoffset);
}


// glfw: handle mouse button events
// --------------------------------
void UMouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	switch (button)
	{
	case GLFW_MOUSE_BUTTON_LEFT:
	{
		if (action == GLFW_PRESS)
			cout << "Left mouse button pressed" << endl;
		else
			cout << "Left mouse button released" << endl;
	}
	break;

	case GLFW_MOUSE_BUTTON_MIDDLE:
	{
		if (action == GLFW_PRESS)
			cout << "Middle mouse button pressed" << endl;
		else
			cout << "Middle mouse button released" << endl;
	}
	break;

	case GLFW_MOUSE_BUTTON_RIGHT:
	{
		if (action == GLFW_PRESS)
			cout << "Right mouse button pressed" << endl;
		else
			cout << "Right mouse button released" << endl;
	}
	break;

	default:
		cout << "Unhandled mouse button event" << endl;
		break;
	}
}

// Functioned called to render a frame
void URender()
{
	GLint modelLoc;
	GLint viewLoc;
	GLint projLoc;
	GLint viewPosLoc;
	GLint ambStrLoc;
	GLint ambColLoc;
	GLint light1ColLoc;
	GLint light1PosLoc;
	GLint light2ColLoc;
	GLint light2PosLoc;
	GLint objColLoc;
	GLint specInt1Loc;
	GLint highlghtSz1Loc;
	GLint specInt2Loc;
	GLint highlghtSz2Loc;
	GLint uHasTextureLoc;
	bool ubHasTextureVal;
	glm::mat4 scale;
	glm::mat4 rotation;
	glm::mat4 translation;
	glm::mat4 model;
	GLint objectColorLoc;

	// Enable z-depth
	glEnable(GL_DEPTH_TEST);

	// Clear the frame and z buffers
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// camera/view transformation
	glm::mat4 view = gCamera.GetViewMatrix();

	// Retrieve the projection matrix based on the current view mode
	glm::mat4 projection;
	if (isOrthographic)
	{
		projection = glm::ortho(-5.0f, 5.0f, -5.0f, 5.0f, 0.1f, 100.0f);
	}
	else
	{
		projection = glm::perspective(glm::radians(gCamera.Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
	}

	// Set the shader to be used
	glUseProgram(gSurfaceProgramId);

	// Retrieves and passes transform matrices to the Shader program
	modelLoc = glGetUniformLocation(gSurfaceProgramId, "model");
	viewLoc = glGetUniformLocation(gSurfaceProgramId, "view");
	projLoc = glGetUniformLocation(gSurfaceProgramId, "projection");
	viewPosLoc = glGetUniformLocation(gSurfaceProgramId, "viewPosition");
	ambStrLoc = glGetUniformLocation(gSurfaceProgramId, "ambientStrength");
	ambColLoc = glGetUniformLocation(gSurfaceProgramId, "ambientColor");
	light1ColLoc = glGetUniformLocation(gSurfaceProgramId, "light1Color");
	light1PosLoc = glGetUniformLocation(gSurfaceProgramId, "light1Position");
	light2ColLoc = glGetUniformLocation(gSurfaceProgramId, "light2Color");
	light2PosLoc = glGetUniformLocation(gSurfaceProgramId, "light2Position");
	objColLoc = glGetUniformLocation(gSurfaceProgramId, "objectColor");
	specInt1Loc = glGetUniformLocation(gSurfaceProgramId, "specularIntensity1");
	highlghtSz1Loc = glGetUniformLocation(gSurfaceProgramId, "highlightSize1");
	specInt2Loc = glGetUniformLocation(gSurfaceProgramId, "specularIntensity2");
	highlghtSz2Loc = glGetUniformLocation(gSurfaceProgramId, "highlightSize2");
	uHasTextureLoc = glGetUniformLocation(gSurfaceProgramId, "ubHasTexture");

	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

	//set ambient lighting strength
	glUniform1f(ambStrLoc, 2.0f);
	//set ambient color
	glUniform3f(ambColLoc, 0.1f, 0.1f, 0.1f);
	glUniform3f(light1ColLoc, 0.8f, 0.5f, 0.2f);
	glUniform3f(light1PosLoc, -4.0f, -0.2f, 1.0f);
	glUniform3f(light2ColLoc, 0.0f, 0.0f, 0.8f);
	glUniform3f(light2PosLoc, 0.0f, -0.2f, -2.0f);


	//set specular intensity
	glUniform1f(specInt1Loc, .8f);
	glUniform1f(specInt2Loc, .4f);
	
	
	//set specular highlight size
	glUniform1f(highlghtSz1Loc, 2.0f);
	glUniform1f(highlghtSz2Loc, 2.0f);


	GLint UVScaleLoc = glGetUniformLocation(gSurfaceProgramId, "uvScale");
	glUniform2fv(UVScaleLoc, 1, glm::value_ptr(gUVScale));


	ubHasTextureVal = true;
	glUniform1i(uHasTextureLoc, ubHasTextureVal);

	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(meshes.gPlaneMesh.vao); // Plane mesh

	// bind textures on corresponding texture units
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gTexturePlane);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(9.0f, 1.0f, 3.0f));
	// 2. Rotate the object
	rotation = glm::rotate(0.0f, glm::vec3(1.0, 0.0f, 0.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(0.0f, -0.5f, 0.0f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// Draws the triangles
	glDrawElements(GL_TRIANGLES, meshes.gPlaneMesh.nIndices, GL_UNSIGNED_INT, 0);

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);

	/*
	* This is the flowerpot without flowers inside it
	*/

	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(meshes.gCylinderMesh.vao); //Base cylinder for flowerpot

	// bind textures on corresponding texture units
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gTexturePot);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(0.5f, 0.8f, 0.5f));
	// 2. Rotate the object
	rotation = glm::rotate(0.0f, glm::vec3(1.0, 1.0f, 1.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(-7.0f, -0.49f, -1.0f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// Draws the triangles
	glDrawArrays(GL_TRIANGLE_FAN, 0, 36);		//bottom
	//glDrawArrays(GL_TRIANGLE_FAN, 36, 36);		//top
	glDrawArrays(GL_TRIANGLE_STRIP, 72, 146);	//sides

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);

	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(meshes.gTorusMesh.vao); //1st smaller torus

	// bind textures on corresponding texture units
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gTexturePot);


	// 1. Scales the object
	scale = glm::scale(glm::vec3(0.45f, 0.45f, 0.3f));
	// 2. Rotate the object
	rotation = glm::rotate(glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(-7.0f, 0.15f, -1.0f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// Draws the triangles
	glDrawArrays(GL_TRIANGLES, 0, meshes.gTorusMesh.nVertices);

	// Deactivate the Vertex Array Objects
	glBindVertexArray(0);

	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(meshes.gTorusMesh.vao); //2nd Larger Torus

	// bind textures on corresponding texture units
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gTexturePot);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(0.5f, 0.5f, 0.7f));
	// 2. Rotate the object
	rotation = glm::rotate(glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(-7.0f, -0.25f, -1.0f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// Draws the triangles
	glDrawArrays(GL_TRIANGLES, 0, meshes.gTorusMesh.nVertices);

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);

	/*
	End flowerpot without flowers
	*/

	/*
	Flower pot with flowers in photo
	*/

	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(meshes.gCylinderMesh.vao); //Base cylinder for flowerpot

	// bind textures on corresponding texture units
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gTexturePot);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(0.3f, 0.9f, 0.3f));
	// 2. Rotate the object
	rotation = glm::rotate(0.0f, glm::vec3(1.0, 1.0f, 1.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(-8.0f, -0.49f, 1.0f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// Draws the triangles
	glDrawArrays(GL_TRIANGLE_FAN, 0, 36);		//bottom
	//glDrawArrays(GL_TRIANGLE_FAN, 36, 36);		//top
	glDrawArrays(GL_TRIANGLE_STRIP, 72, 146);	//sides

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);

	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(meshes.gTorusMesh.vao); //1st smaller torus

	// bind textures on corresponding texture units
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gTexturePot);


	// 1. Scales the object
	scale = glm::scale(glm::vec3(0.3f, 0.3f, 0.3f));
	// 2. Rotate the object
	rotation = glm::rotate(glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(-8.0f, 0.10f, 1.0f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// Draws the triangles
	glDrawArrays(GL_TRIANGLES, 0, meshes.gTorusMesh.nVertices);

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);

	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(meshes.gTorusMesh.vao); //2nd Larger Torus

	// bind textures on corresponding texture units
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gTexturePot);


	// 1. Scales the object
	scale = glm::scale(glm::vec3(0.4f, 0.4f, 0.5f));
	// 2. Rotate the object
	rotation = glm::rotate(glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(-8.0f, -0.2f, 1.0f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// Draws the triangles
	glDrawArrays(GL_TRIANGLES, 0, meshes.gTorusMesh.nVertices);

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);

	/*
	End flowerpot with flowers in photo
	*/

	/*
	Green labeled candle emitting light
	*/

	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(meshes.gCylinderMesh.vao); //Base cylinder for flowerpot

	// bind textures on corresponding texture units
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gTextureCandle);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(0.3f, 0.3f, 0.3f));
	// 2. Rotate the object
	rotation = glm::rotate(0.0f, glm::vec3(1.0, 1.0f, 1.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(-4.0f, -0.49f, 1.0f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// Draws the triangles
	glDrawArrays(GL_TRIANGLE_FAN, 0, 36);		//bottom
	//glDrawArrays(GL_TRIANGLE_FAN, 36, 36);		//top
	glDrawArrays(GL_TRIANGLE_STRIP, 72, 146);	//sides

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);

	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(meshes.gCylinderMesh.vao); //Base cylinder for flowerpot

	// bind textures on corresponding texture units
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gTextureCandle);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(0.25f, 0.35f, 0.25f));
	// 2. Rotate the object
	rotation = glm::rotate(0.0f, glm::vec3(1.0, 1.0f, 1.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(-4.0f, -0.49f, 1.0f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// Draws the triangles
	glDrawArrays(GL_TRIANGLE_FAN, 0, 36);		//bottom
	glDrawArrays(GL_TRIANGLE_FAN, 36, 36);		//top
	glDrawArrays(GL_TRIANGLE_STRIP, 72, 146);	//sides

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);

	//Candle Wick

	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(meshes.gConeMesh.vao);

		// bind textures on corresponding texture units
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gTextureFlame);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(0.05f, 0.2f, 0.05f));
	// 2. Rotate the object
	rotation = glm::rotate(0.0f, glm::vec3(1.0, 1.0f, 1.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(-4.0f, -0.2f, 1.0f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// Draws the triangles
	glDrawArrays(GL_TRIANGLE_FAN, 0, 36);		//bottom
	glDrawArrays(GL_TRIANGLE_STRIP, 36, 108);	//sides

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);

	/*
	End candle with green label
	*/

	/*
	Candle with white label
	*/

	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(meshes.gCylinderMesh.vao); //Base cylinder for flowerpot

	// bind textures on corresponding texture units
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gTextureCandle);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(0.3f, 0.3f, 0.3f));
	// 2. Rotate the object
	rotation = glm::rotate(0.0f, glm::vec3(1.0, 1.0f, 1.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(0.0f, -0.49f, -2.0f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// Draws the triangles
	glDrawArrays(GL_TRIANGLE_FAN, 0, 36);		//bottom
	//glDrawArrays(GL_TRIANGLE_FAN, 36, 36);		//top
	glDrawArrays(GL_TRIANGLE_STRIP, 72, 146);	//sides

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);

	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(meshes.gCylinderMesh.vao); //Base cylinder for flowerpot

	// bind textures on corresponding texture units
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gTextureCandle);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(0.25f, 0.35f, 0.25f));
	// 2. Rotate the object
	rotation = glm::rotate(0.0f, glm::vec3(1.0, 1.0f, 1.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(0.0f, -0.49f, -2.0f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// Draws the triangles
	glDrawArrays(GL_TRIANGLE_FAN, 0, 36);		//bottom
	glDrawArrays(GL_TRIANGLE_FAN, 36, 36);		//top
	glDrawArrays(GL_TRIANGLE_STRIP, 72, 146);	//sides

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);

	//Candle Wick

// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(meshes.gConeMesh.vao);

	// bind textures on corresponding texture units
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gTextureFlame);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(0.05f, 0.2f, 0.05f));
	// 2. Rotate the object
	rotation = glm::rotate(0.0f, glm::vec3(1.0, 1.0f, 1.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(0.0f, -0.2f, -2.0f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;

	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// Draws the triangles
	glDrawArrays(GL_TRIANGLE_FAN, 0, 36);		//bottom
	glDrawArrays(GL_TRIANGLE_STRIP, 36, 108);	//sides

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);

	/*
	End candle with white label
	*/

	/*
	Candle lid
	*/

	// Set the shader to be used
	glUseProgram(gSurfaceProgramId);

	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(meshes.gCylinderMesh.vao); 

	// bind textures on corresponding texture units
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gTextureSilver);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(0.25f, 0.1f, 0.25f));
	// 2. Rotate the object
	rotation = glm::rotate(0.0f, glm::vec3(1.0, 1.0f, 1.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(-4.0f, -0.49f, -1.0f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	// Draws the triangles
	glDrawArrays(GL_TRIANGLE_FAN, 0, 36);		//bottom
	glDrawArrays(GL_TRIANGLE_FAN, 36, 36);		//top
	glDrawArrays(GL_TRIANGLE_STRIP, 72, 146);	//sides

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);

	/*
	Cork Coasters
	*/

	// Set the shader to be used
	glUseProgram(gSurfaceProgramId);

	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(meshes.gBoxMesh.vao);

	// bind textures on corresponding texture units
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gTextureCork);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(0.5f, 0.1f, 0.5f));
	// 2. Rotate the object
	rotation = glm::rotate(15.0f, glm::vec3(0.0, 1.0f, 0.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(1.0f, -0.45f, 1.0f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glDrawElements(GL_TRIANGLES, meshes.gBoxMesh.nIndices, GL_UNSIGNED_INT, (void*)0);

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);

	// Set the shader to be used
	glUseProgram(gSurfaceProgramId);

	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(meshes.gBoxMesh.vao);

	// bind textures on corresponding texture units
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gTextureCork);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(0.5f, 0.1f, 0.5f));
	// 2. Rotate the object
	rotation = glm::rotate(10.0f, glm::vec3(0.0, 1.0f, 0.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(1.0f, -0.35f, 1.0f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glDrawElements(GL_TRIANGLES, meshes.gBoxMesh.nIndices, GL_UNSIGNED_INT, (void*)0);

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);

	// Set the shader to be used
	glUseProgram(gSurfaceProgramId);

	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(meshes.gBoxMesh.vao);

	// bind textures on corresponding texture units
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gTextureCork);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(0.5f, 0.1f, 0.5f));
	// 2. Rotate the object
	rotation = glm::rotate(15.0f, glm::vec3(0.0, 1.0f, 0.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(1.0f, -0.25f, 1.0f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glDrawElements(GL_TRIANGLES, meshes.gBoxMesh.nIndices, GL_UNSIGNED_INT, (void*)0);

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);

	// Set the shader to be used
	glUseProgram(gSurfaceProgramId);

	// Activate the VBOs contained within the mesh's VAO
	glBindVertexArray(meshes.gBoxMesh.vao);

	// bind textures on corresponding texture units
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gTextureCork);

	// 1. Scales the object
	scale = glm::scale(glm::vec3(0.5f, 0.1f, 0.5f));
	// 2. Rotate the object
	rotation = glm::rotate(20.0f, glm::vec3(0.0, 1.0f, 0.0f));
	// 3. Position the object
	translation = glm::translate(glm::vec3(1.0f, -0.15f, 1.0f));
	// Model matrix: transformations are applied right-to-left order
	model = translation * rotation * scale;
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glDrawElements(GL_TRIANGLES, meshes.gBoxMesh.nIndices, GL_UNSIGNED_INT, (void*)0);

	// Deactivate the Vertex Array Object
	glBindVertexArray(0);


	// glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
	glfwSwapBuffers(gWindow);    // Flips the the back buffer with the front buffer every frame.
}


// Implements the UCreateMesh function
/*void UCreateMesh(GLMesh &mesh)
{
	// Position and Color data
	GLfloat verts[] = {
		// Vertex Positions    // Colors (r,g,b,a)
		 0.5f,  0.5f, 0.0f,   1.0f, 0.0f, 0.0f, 1.0f, // Top Right Vertex 0
		 0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f, 1.0f, // Bottom Right Vertex 1
		-0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f, 1.0f, // Bottom Left Vertex 2
		-0.5f,  0.5f, 0.0f,   1.0f, 0.0f, 1.0f, 1.0f, // Top Left Vertex 3

		 0.5f, -0.5f, -1.0f,  0.5f, 0.5f, 1.0f, 1.0f, // 4 br  right
		 0.5f,  0.5f, -1.0f,  1.0f, 1.0f, 0.5f, 1.0f, //  5 tl  right
		-0.5f,  0.5f, -1.0f,  0.2f, 0.2f, 0.5f, 1.0f, //  6 tl  top
		-0.5f, -0.5f, -1.0f,  1.0f, 0.0f, 1.0f, 1.0f  //  7 bl back
	};

	// Index data to share position data
	GLushort indices[] = {
		0, 1, 3,  // Triangle 1
		1, 2, 3,   // Triangle 2
		0, 1, 4,  // Triangle 3
		0, 4, 5,  // Triangle 4
		0, 5, 6, // Triangle 5
		0, 3, 6,  // Triangle 6
		4, 5, 6, // Triangle 7
		4, 6, 7, // Triangle 8
		2, 3, 6, // Triangle 9
		2, 6, 7, // Triangle 10
		1, 4, 7, // Triangle 11
		1, 2, 7 // Triangle 12
	};

	const GLuint floatsPerVertex = 3;
	const GLuint floatsPerColor = 4;

	glGenVertexArrays(1, &mesh.vao); // we can also generate multiple VAOs or buffers at the same time
	glBindVertexArray(mesh.vao);

	// Create 2 buffers: first one for the vertex data; second one for the indices
	glGenBuffers(2, mesh.vbos);
	glBindBuffer(GL_ARRAY_BUFFER, mesh.vbos[0]); // Activates the buffer
	glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW); // Sends vertex or coordinate data to the GPU

	mesh.nIndices = sizeof(indices) / sizeof(indices[0]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.vbos[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// Strides between vertex coordinates is 6 (x, y, z, r, g, b, a). A tightly packed stride is 0.
	GLint stride = sizeof(float) * (floatsPerVertex + floatsPerColor);// The number of floats before each

	// Create Vertex Attribute Pointers
	glVertexAttribPointer(0, floatsPerVertex, GL_FLOAT, GL_FALSE, stride, 0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, floatsPerColor, GL_FLOAT, GL_FALSE, stride, (char*)(sizeof(float) * floatsPerVertex));
	glEnableVertexAttribArray(1);
}*/


//void UDestroyMesh(GLMesh &mesh)
//{
//	glDeleteVertexArrays(1, &mesh.vao);
//	glDeleteBuffers(2, mesh.vbos);
//}


// Implements the UCreateShaders function
bool UCreateShaderProgram(const char* vtxShaderSource, const char* fragShaderSource, GLuint& programId)
{
	// Compilation and linkage error reporting
	int success = 0;
	char infoLog[512];

	// Create a Shader program object.
	programId = glCreateProgram();

	// Create the vertex and fragment shader objects
	GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);

	// Retrive the shader source
	glShaderSource(vertexShaderId, 1, &vtxShaderSource, NULL);
	glShaderSource(fragmentShaderId, 1, &fragShaderSource, NULL);

	// Compile the vertex shader, and print compilation errors (if any)
	glCompileShader(vertexShaderId); // compile the vertex shader
	// check for shader compile errors
	glGetShaderiv(vertexShaderId, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertexShaderId, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;

		return false;
	}

	glCompileShader(fragmentShaderId); // compile the fragment shader
	// check for shader compile errors
	glGetShaderiv(fragmentShaderId, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragmentShaderId, sizeof(infoLog), NULL, infoLog);
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;

		return false;
	}

	// Attached compiled shaders to the shader program
	glAttachShader(programId, vertexShaderId);
	glAttachShader(programId, fragmentShaderId);

	glLinkProgram(programId);   // links the shader program
	// check for linking errors
	glGetProgramiv(programId, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(programId, sizeof(infoLog), NULL, infoLog);
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;

		return false;
	}

	glUseProgram(programId);    // Uses the shader program

	return true;
}

/*Generate and load the texture*/
bool UCreateTexture(const char* filename, GLuint& textureId)
{
	int width, height, channels;
	unsigned char* image = stbi_load(filename, &width, &height, &channels, 0);
	if (image)
	{
		flipImageVertically(image, width, height, channels);

		glGenTextures(1, &textureId);
		glBindTexture(GL_TEXTURE_2D, textureId);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_MIRRORED_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_MIRRORED_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		if (channels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		else if (channels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			cout << "Not implemented to handle image with " << channels << " channels" << endl;
			return false;
		}

		glGenerateMipmap(GL_TEXTURE_2D);

		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		return true;
	}

	// Error loading the image
	return false;
}


void UDestroyTexture(GLuint textureId)
{
	glGenTextures(1, &textureId);
}

void UDestroyShaderProgram(GLuint programId)
{
	glDeleteProgram(programId);
}