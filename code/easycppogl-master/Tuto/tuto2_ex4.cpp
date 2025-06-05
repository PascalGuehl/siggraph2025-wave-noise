// tuto2_ex1.cpp : utilisation de VAO/VBO et shaders.
//

#include <iostream>
#include <string>
#include <stdlib.h>

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#include <gl_eigen.h>
using namespace EZCOGL;

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

// resolution de la fenêtre d'affichage
int W = 600;
int H = 500;

// pour le pointeur de la souris
double			g_PreviousXPosition;
double			g_PreviousYPosition;
bool			g_LeftButtonPressed;
bool			g_RightButtonPressed;
double			PosX = 0.0;
double			PosY = 0.0;

// objet défini par tableau de sommets et tableau d'indices de sommets
//////////////////////////////////////////////////
float *vertices;		// les sommets
unsigned int *index;	// les indices de sommets
int ns, ni;				// nombre de sommets et nombre d'indices

// GL ERROR CHECK
///////////////////////////////////////////////
int CheckGLError(char *file, int line)
{
	GLenum glErr;
	int    retCode = 0;

	glErr = glGetError();
	while (glErr != GL_NO_ERROR) {
		std::string glErrStr = "";
		switch (glErr)
		{
		case GL_INVALID_ENUM: glErrStr = "INVALID_ENUM"; break;
		case GL_INVALID_VALUE: glErrStr = "INVALID_VALUE"; break;
		case GL_INVALID_OPERATION: glErrStr = "INVALID_OPERATION"; break;
		case GL_STACK_OVERFLOW: glErrStr = "STACK_OVERFLOW"; break;
		case GL_STACK_UNDERFLOW: glErrStr = "STACK_UNDERFLOW"; break;
		case GL_OUT_OF_MEMORY: glErrStr = "OUT_OF_MEMORY"; break;
		case GL_INVALID_FRAMEBUFFER_OPERATION: glErrStr = "INVALID_FRAMEBUFFER_OPERATION"; break;
		}
		std::cout<<"GL Error #"<< glErr<<"( "<< glErrStr<<" ) in File "<< file<<" at line: "<<line<<"\n";
		retCode = 1;
		glErr = glGetError();
	}
	return retCode;
}
#define CHECK_GL_ERROR() CheckGLError(__FILE__, __LINE__)

// creation et chargement d'un objet dans le GPU 
///////////////////////////////////////////////

void createcircle(float rayon, int nb)
{
	vertices = (float *)malloc(sizeof(float) * (2 * (nb + 1)+1));
	vertices[0] = 0.0; vertices[1] = 0.0;
	for (int i = 0; i <= nb; i++)
	{
		vertices[2 * (i+1)] = rayon*(float)cos((double)i / (double)nb*M_PI*2.0);
		vertices[2 * (i+1) + 1] = rayon*(float)sin((double)i / (double)nb*M_PI*2.0);
	}
	index = (unsigned int *)malloc(sizeof(unsigned int) * 3 * (nb+1));
	for (int i = 0; i<=nb; i++)
	{
		index[3*i] = 0;  index[3 * i+1] = i; index[3 * i + 2] = i + 1;
	}
	ns = nb + 2;
	ni = 3 * (nb+1);
}

// chargement de l'objet dans la mémoire GPU: utilisation de buffers
////////////////////////////////////////////////////////////////////
// identifiant des buffers opengl
unsigned int vao_id; 
unsigned int vbo_id[3];
void loadglobject()
{
	// Definition d'un objet geometrique en dur
	createcircle(1.0, 32);

	// Creation d'un seul VAO car 1 seul objet
	glGenVertexArrays(1, &vao_id);
	glBindVertexArray(vao_id);

	// Chargement en RAM GPU de l'objet avec 3 VBO
	glGenBuffers(2, vbo_id);

	glBindBuffer(GL_ARRAY_BUFFER, vbo_id[0]);
	glBufferData(GL_ARRAY_BUFFER, 3 * ns * sizeof(float), vertices, GL_STATIC_DRAW);
	CHECK_GL_ERROR();
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo_id[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, ni * sizeof(unsigned int), index, GL_STATIC_DRAW);
	CHECK_GL_ERROR();
}

// CREATION des SHADERS OpenGl
/////////////////////////////////////////////////
unsigned int vshader; // identifiant du vertex shader
static const std::string vertexshader = R"(
	#version 460

	in  vec2 vpos;
	out vec3 mycol;

	uniform mat4 trans;

	void main(void) {
	    gl_Position = trans*vec4(vpos.x, vpos.y, 0.0,1.0);
	    mycol=vec3(1,1,1);
	}
)";

unsigned int fshader; // identifiant du fragment shader
static const std::string  fragshader = R"(
	#version 460

	in vec3 mycol;
	out vec3 gl_FragColor;

	void main(void) {
	      gl_FragColor = mycol;  
	}
)";

// initialisation et compilation des deux shaders sur GPU
unsigned int gpuprog; // identifiant du programme GPU composé des deux shaders
void createglshaders()
{
	int i;
	int compiled = 0;
	char buffer[1024];

	// initialisation du vertex shader
	vshader = glCreateShader(GL_VERTEX_SHADER);
	const char* csrc = vertexshader.c_str();
	glShaderSource(vshader, 1, &csrc, nullptr);
	glCompileShader(vshader);
	CHECK_GL_ERROR();
	// on verifie le statut et les erreurs
	glGetShaderiv(vshader, GL_COMPILE_STATUS, &compiled);
	CHECK_GL_ERROR();
	if (compiled) { std::cout<<"Vertex Shader compilation successful!\n"; }
	else
	{
		std::cout<<"Shader compile error:\n";
		glGetShaderInfoLog(vshader, 1024, &i, buffer);
		std::cout<<buffer<<"\n";
		abort();
	}

	// initialisation du fragment shader
	fshader = glCreateShader(GL_FRAGMENT_SHADER);
	csrc = fragshader.c_str();
	glShaderSource(fshader, 1, &csrc, nullptr);
	glCompileShader(fshader);
	CHECK_GL_ERROR();
	// on verifie le statut et les erreurs
	glGetShaderiv(fshader, GL_COMPILE_STATUS, &compiled);
	CHECK_GL_ERROR();
	if (compiled) { std::cout<<"Fragment Shader compilation successful!\n"; }
	else
	{
		std::cout<<"Shader compile error:\n";
		glGetShaderInfoLog(fshader, 1024, &i, buffer);
		std::cout << buffer << "\n";
		abort();
	}

	// assemblage du programme par edition de lien
	gpuprog = glCreateProgram();
	glAttachShader(gpuprog, vshader);
	glAttachShader(gpuprog, fshader);
	glLinkProgram(gpuprog);
	CHECK_GL_ERROR();
	// on verifie le statut et les erreurs
	glGetProgramiv(gpuprog, GL_LINK_STATUS, &compiled);
	CHECK_GL_ERROR();
	if (compiled) { std::cout<<"Link successful!\n"; }
	else
	{
		std::cout<<"Link error:\n";
		glGetProgramInfoLog(gpuprog, 1024, &i, buffer);
		std::cout << buffer << "\n";
		abort();
	}

	// tout est bon, alors : activer le programme!
	glUseProgram(gpuprog);
	CHECK_GL_ERROR();
}

// Donner des valeurs aux variables attributs des shaders
///////////////////////////////////////////////
int ivpos; // pointeur sur la variable "vpos" du vertex shader
int itrans; // pointeur sur la variable uniforme "trans" du vertex shader
void setshadervariables()
{
	glBindVertexArray(vao_id);

	// recupérer le "pointeur" (en fait, la position) de la variable "vpos" du vertex shader
	ivpos = glGetAttribLocation(gpuprog, "vpos");
	// associer le VBO à cette variable pour ce VAO
	glBindBuffer(GL_ARRAY_BUFFER, vbo_id[0]);
	glVertexAttribPointer(ivpos, 2, GL_FLOAT, false, 0, NULL);
	CHECK_GL_ERROR();
	// activer le VBO
	glEnableVertexAttribArray(ivpos);
	CHECK_GL_ERROR();

	itrans = glGetUniformLocation(gpuprog, "trans");
}

float alpha = M_PI / 4.0;
float scale[] = { 0.5, 0.5 };
// OPENGL DISPLAY
void display()
{
	// effacer l'écran
	glClear(GL_COLOR_BUFFER_BIT);

	// initialiser le viewport
	glViewport(0, 0, W, H);

	// definir la matrice de transformation
	float matdata[16] = { scale[0]*cos(alpha), -scale[1]*sin(alpha), 0.0, 0.0 ,
		scale[0] *sin(alpha), scale[1] *cos(alpha), 0.0, 0.0 ,
		0.0, 0.0, 1.0, 0.0 ,
		PosX, PosY, 0.0, 1.0 };
	glUniformMatrix4fv(itrans, 1, false, matdata);
	CHECK_GL_ERROR();
	
	// dessiner la figure
	glFrontFace(GL_CW);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glDisable(GL_CULL_FACE);
	// faire le dessin vectoriel: suite de sommets liés topologiquement
	glBindVertexArray(vao_id); 
	glDrawElements(GL_TRIANGLES, ni, GL_UNSIGNED_INT, NULL);
	glBindVertexArray(0);
	CHECK_GL_ERROR();
}

// MANAGE EVENTS
// si la fenetre est redimensionnee
void resize(GLFWwindow* window, int width, int height)
{
	W = width;
	H = height;
}
// si une touche du clavier est pressee
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_UP && (action == GLFW_PRESS || action == GLFW_REPEAT))
	{
		std::cout<<"KEY UP\n";
	}
	if (key == GLFW_KEY_DOWN && (action == GLFW_PRESS || action == GLFW_REPEAT))
	{
		std::cout << "KEY DOWN\n";
	}
	if (key == GLFW_KEY_LEFT && (action == GLFW_PRESS || action == GLFW_REPEAT))
	{
		std::cout << "KEY LEFT\n";
	}
	if (key == GLFW_KEY_RIGHT && (action == GLFW_PRESS || action == GLFW_REPEAT))
	{
		std::cout << "KEY RIGHT\n";
	}

	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, GL_TRUE);
	}
}
// gestion evenements de la souris: boutons
void mouseButton_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (ImGui::GetIO().WantCaptureMouse || ImGui::IsMouseHoveringAnyWindow())
	{
		return;
	}
	if (button == GLFW_MOUSE_BUTTON_LEFT)
	{
		if (action == GLFW_PRESS)
		{
			g_LeftButtonPressed = true;
		}
		else if (action == GLFW_RELEASE)
		{
			g_LeftButtonPressed = false;
		}
	}

	if (button == GLFW_MOUSE_BUTTON_RIGHT)
	{
		if (action == GLFW_PRESS)
		{
			g_RightButtonPressed = true;
		}
		else if (action == GLFW_RELEASE)
		{
			g_RightButtonPressed = false;
		}
	}
}
// gestion evenements de la souris: mouvement
void mouseMove_callback(GLFWwindow* window, double x, double y)
{
	if (ImGui::GetIO().WantCaptureMouse || ImGui::IsMouseHoveringAnyWindow())
	{
		return;
	}
	if (g_LeftButtonPressed)
	{
		double speed = 0.005;
		PosX += speed*(x - g_PreviousXPosition);
		PosY -= speed*(y - g_PreviousYPosition);
	}

	g_PreviousXPosition = x;
	g_PreviousYPosition = y;
}

// ouverture de la fenetre et initialisation d'opengl
int main()
{
	GLFWwindow* window;

	/* Initialize the library */
	if (!glfwInit())
		return -1;


	/* Create a windowed mode window and its OpenGL context */
	window = glfwCreateWindow(W, H, "OpenGL Window", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}

	/* Make the window's context current */
	glfwMakeContextCurrent(window);
	/* set up all call back functions */
	glfwSetFramebufferSizeCallback(window, resize);
	glfwSetKeyCallback(window, key_callback);
	glfwSetCursorPosCallback(window, mouseMove_callback);
	glfwSetMouseButtonCallback(window, mouseButton_callback);

	bool err = gl3wInit() != 0;
	if (err)
	{
		// Problem: gl3wInit failed, something is seriously wrong. 
		fprintf(stderr, "Failed to initialize OpenGL loader!");
	}
	std::cout << "Vendor: "<<glGetString(GL_VENDOR)<<"\n";
	std::cout << "Renderer: "<< glGetString(GL_RENDERER) << "\n";
	std::cout << "Version: "<< glGetString(GL_VERSION) << "\n";
	std::cout << "GLSL: "<< glGetString(GL_SHADING_LANGUAGE_VERSION) << "\n";

	const char* glsl_version = "#version 330";
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO &io = ImGui::GetIO();
	// Setup Platform/Renderer bindings
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init(glsl_version);
	// Setup Dear ImGui style
	ImGui::StyleColorsDark();

	// creation du cercle dans des tableaux sur CPU
	//createcircle(0.25, 8);
	// chargement des tableaux dans la mémoire GPU
	loadglobject();
	// creation des vertex et fragment shaders
	createglshaders();
	// attribuer des valeurs aux variables des shaders
	setshadervariables();

	/* Loop until the user closes the window */
	while (!glfwWindowShouldClose(window))
	{
		// feed inputs to dear imgui, start new frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		/* Render here */
		display();

		// render GUI
		ImGui::Begin("GUI window");
		ImGui::SliderFloat("rotation", &alpha, 0, 2.0 * M_PI);
		ImGui::SliderFloat2("scale", scale, 0.1, 2.0);
		ImGui::End();

		// Render dear imgui into screen
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		/* Swap front and back buffers */
		glfwSwapBuffers(window);

		/* Wait for events or Poll for events */
		glfwWaitEvents();
		//glfwPollEvents();
	}
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	glfwTerminate();
	return 0;
}



