/*
 * ================================================================
 * Proyecto de Computación Gráfica e Interacción Humano Computadora
 * ================================================================
 */

 // Macro necesaria para que stb_image.h genere la implementación
 // de sus funciones (solo debe definirse en UN archivo .cpp)
#define STB_IMAGE_IMPLEMENTATION

#include <stdio.h>
#include <string.h>
#include <cmath>
#include <vector>

// --- Bibliotecas de OpenGL ---
#include <glew.h>   // Extensiones de OpenGL (funciones modernas)
#include <glfw3.h>  // Creación de ventana y manejo de input

// --- Biblioteca de matemáticas GLM ---
// GLM proporciona tipos y funciones matemáticas compatibles con
// los tipos de GLSL (vec3, mat4, etc.)
#include <glm.hpp>
#include <gtc\matrix_transform.hpp>  // translate, rotate, scale, perspective
#include <gtc\type_ptr.hpp>          // value_ptr: convierte glm::mat4 a float*

// --- Clases propias del proyecto ---
#include "Window.h"        // Encapsula la ventana GLFW
#include "Mesh.h"          // Geometría: VAO, VBO, IBO
#include "Shader_light.h"  // Compila y enlaza shaders con soporte de luces
#include "Camera.h"        // Cámara en primera persona (WASD + ratón)
#include "Texture.h"       // Carga y bindeo de texturas
#include "Model.h"         // Carga modelos 3D (.obj) con Assimp
#include "Skybox.h"        // Cubemap para el fondo del cielo

// --- Clases de iluminación ---
#include "CommonValues.h"      // Constantes: MAX_POINT_LIGHTS, MAX_SPOT_LIGHTS
#include "DirectionalLight.h"  // Luz direccional (sol)
#include "PointLight.h"        // Luz puntual (foco omnidireccional)
#include "SpotLight.h"         // Luz focal (linterna / cono de luz)
#include "Material.h"          // Define respuesta especular de una superficie

// ============================================================
// Variables globales
// ============================================================

Window mainWindow;
std::vector<Mesh*> meshList;    // Lista de geometrías (meshes)
std::vector<Shader> shaderList; // Lista de shaders compilados
Camera camera;

Texture pisoTexture;     // Textura que se aplica al plano del piso
Model lamp_model;        // Modelo 3D de una lámpara
Skybox skybox;           // Skybox (fondo envolvente)
Material Material_opaco; // Material con bajo brillo especular

// Variables de control de tiempo para movimiento uniforme
GLfloat deltaTime = 0.0f;  // Tiempo entre frames
GLfloat lastTime = 0.0f;   // Tiempo del frame anterior
static double limitFPS = 1.0 / 60.0;  // Límite de 60 FPS

// Fuentes de luz de la escena
DirectionalLight mainLight;                  // Una sola luz direccional
PointLight pointLights[MAX_POINT_LIGHTS];    // Arreglo de luces puntuales
SpotLight spotLights[MAX_SPOT_LIGHTS];       // Arreglo de luces focales

// Rutas a los archivos de shaders (vertex y fragment)
static const char* vShader = "shaders/shader_light.vert";
static const char* fShader = "shaders/shader_light.frag";

// ============================================================
// CreateObjects: crea la geometría de la escena
// ============================================================
// Cada vértice tiene 8 componentes (stride = 8 floats):
//   x, y, z    → posición del vértice
//   s, t       → coordenadas de textura (UV)
//   nx, ny, nz → normal del vértice (esencial para iluminación)
//
// Las normales indican hacia dónde "mira" la superficie.
// El shader las usa para calcular qué tan iluminada está
// cada cara según el ángulo con la fuente de luz.
// ============================================================
void CreateObjects()
{
	// Índices: definen qué vértices forman cada triángulo.
	// Un plano se forma con 2 triángulos (6 índices, 4 vértices).
	unsigned int floorIndices[] = {
		0, 2, 1,
		1, 2, 3
	};

	// Vértices del piso: un plano horizontal en Y = 0
	// La normal apunta hacia abajo (0, -1, 0) porque la cámara
	// ve el piso desde arriba.
	GLfloat floorVertices[] = {
		//  x       y       z        s      t       nx    ny     nz
		-10.0f,  0.0f, -10.0f,   0.0f,  0.0f,   0.0f, -1.0f,  0.0f,
		 10.0f,  0.0f, -10.0f,  10.0f,  0.0f,   0.0f, -1.0f,  0.0f,
		-10.0f,  0.0f,  10.0f,   0.0f, 10.0f,   0.0f, -1.0f,  0.0f,
		 10.0f,  0.0f,  10.0f,  10.0f, 10.0f,   0.0f, -1.0f,  0.0f
	};

	// Se crea el mesh del piso y se agrega a la lista (índice 0)
	Mesh* piso = new Mesh();
	piso->CreateMesh(floorVertices, floorIndices, 32, 6);
	meshList.push_back(piso);
}

// ============================================================
// CreateShaders: compila los shaders de iluminación
// ============================================================
// Los shaders son programas que corren en la GPU:
//   - Vertex Shader: transforma posiciones de 3D a pantalla
//   - Fragment Shader: calcula el color final de cada píxel
//     usando las normales, la posición de la luz y el material
// ============================================================
void CreateShaders()
{
	Shader* shader1 = new Shader();
	shader1->CreateFromFiles(vShader, fShader);
	shaderList.push_back(*shader1);
}

// =============================================================================
//  dibuja_modelo: Funcion de emi
// =============================================================================
//  Función auxiliar para dibujar un modelo 3D en una posición y escala dadas.
//
//  Pasos internos:
//    1. Crear una matriz de mundo (world) partiendo de la identidad
//    2. Aplicar traslación → mover el modelo a (x, y, z)
//    3. Aplicar rotación   → (aquí no rota, ángulo = 0)
//    4. Aplicar escala     → redimensionar uniformemente
//    5. Enviar la matriz y el color al shader como "uniforms"
//    6. Renderizar el modelo
//
//  Nota: "uniform" es una variable que se envía desde la CPU al shader
//  en la GPU. Es constante durante todo el dibujado de un objeto.
// ============================================================================= 
void dibuja_modelo(Model model, float x, float y, float z, float escala)
{
	glm::mat4 world(1.0);  // Matriz identidad (sin transformación)
	glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f); // Color blanco (neutro)

	GLuint uniformModel = shaderList[0].GetModelLocation();
	GLuint uniformColor = shaderList[0].getColorLocation();

	world = glm::translate(glm::mat4(1.0), glm::vec3(x, y, z));
	world = glm::rotate(world, 0.0f, glm::vec3(1.0f, 0.0f, 0.0f));
	world = glm::scale(world, glm::vec3(escala, escala, escala));
	glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(world));
	glUniform3fv(uniformColor, 1, glm::value_ptr(color));
	model.RenderModel(); // Assimp maneja sus propias texturas
}

// ============================================================
// main: punto de entrada del programa
// ============================================================
int main()
{
	// --- 1. INICIALIZACIÓN DE VENTANA ---
	mainWindow = Window(1366, 768);
	mainWindow.Initialise();

	// --- 2. CREACIÓN DE GEOMETRÍA Y SHADERS ---
	CreateObjects();
	CreateShaders();

	// --- 3. CONFIGURACIÓN DE CÁMARA ---
	// Parámetros: posición inicial, vector "arriba" del mundo,
	// ángulo yaw (-60°), pitch (0°), velocidad de movimiento y
	// sensibilidad del ratón.
	camera = Camera(
		glm::vec3(0.0f, 2.5f, 0.0f),  // posición
		glm::vec3(0.0f, 1.0f, 0.0f),  // vector up
		-60.0f, 0.0f,                  // yaw, pitch
		0.3f, 0.5f                     // velocidad, sensibilidad
	);

	// --- 4. CARGA DE TEXTURAS ---
	// LoadTextureA() carga la textura incluyendo canal alfa (transparencia)
	pisoTexture = Texture("Textures/piso.tga");
	pisoTexture.LoadTextureA();

	// --- 5. CARGA DE MODELOS 3D ---
	// Assimp lee el archivo .obj y genera los meshes con sus texturas
	lamp_model.LoadModel("Models/redstone_lamp.obj");

	// --- 6. CONFIGURACIÓN DEL SKYBOX ---
	// Un skybox es un cubo gigante con 6 texturas (una por cara)
	// que envuelve toda la escena para simular un cielo.
	// El orden importa: derecha, izquierda, abajo, arriba, atrás, frente.
	std::vector<std::string> skyboxFaces;
	skyboxFaces.push_back("Textures/Skybox/cupertin-lake_rt.tga");
	skyboxFaces.push_back("Textures/Skybox/cupertin-lake_lf.tga");
	skyboxFaces.push_back("Textures/Skybox/cupertin-lake_dn.tga");
	skyboxFaces.push_back("Textures/Skybox/cupertin-lake_up.tga");
	skyboxFaces.push_back("Textures/Skybox/cupertin-lake_bk.tga");
	skyboxFaces.push_back("Textures/Skybox/cupertin-lake_ft.tga");
	skybox = Skybox(skyboxFaces);

	// --- 7. MATERIALES ---
	// Un material define cómo la superficie refleja la luz:
	//   - Intensidad especular (0.3): qué tan fuerte es el reflejo
	//   - Brillo/Shininess (4): qué tan concentrado está el reflejo
	// Valores bajos = superficie opaca/mate (como madera)
	// Valores altos = superficie brillante (como metal pulido)
	Material_opaco = Material(0.3f, 4);

	// ============================================================
	// 8. CONFIGURACIÓN DE LUCES
	// ============================================================

	// LUZ DIRECCIONAL (simula el sol)
	// Parámetros: color RGB, intensidad ambiental, intensidad difusa,
	//             dirección (x, y, z)
	// - Ambiental (0.3): iluminación base que llega a todas partes
	// - Difusa (0.3): iluminación que depende del ángulo superficie-luz
	mainLight = DirectionalLight(
		1.0f, 1.0f, 1.0f,   // color blanco
		0.3f, 0.8f,          // intensidad ambiental y difusa
		0.0f, -1.0f, 0.0f    // dirección: hacia -Z (hacia el fondo)
	);

	// LUZ PUNTUAL (como un foco o bombilla)
	// Emite luz en todas direcciones desde un punto.
	// La atenuación controla cómo disminuye la luz con la distancia:
	//   Atenuación = 1 / (constante + lineal*d + exponencial*d²)
	//   - constante (1.0): base, siempre presente
	//   - lineal (0.09): caída suave
	//   - exponencial (0.032): caída rápida a distancia
	unsigned int pointLightCount = 0;
	pointLights[0] = PointLight(
		1.0f, 1.0f, 1.0f,      // color blanco
		0.5f, 1.0f,             // intensidad ambiental y difusa
		0.0f, 1.0f, 0.0f,      // posición: arriba del centro
		1.0f, 0.09f, 0.032f    // atenuación: constante, lineal, exponencial
	);
	pointLightCount++;

	// LUZ FOCAL / SPOTLIGHT (como una linterna)
	// Es una luz puntual + dirección + ángulo de corte.
	// Solo ilumina dentro de un cono definido por el ángulo (5°).
	// En el render loop se actualiza para seguir a la cámara.
	unsigned int spotLightCount = 0;
	spotLights[0] = SpotLight(
		1.0f, 1.0f, 1.0f,     // color blanco
		0.0f, 2.0f,            // ambiental (0 = apagada sin cono), difusa
		0.0f, 0.0f, 0.0f,     // posición inicial (se actualiza cada frame)
		0.0f, -1.0f, 0.0f,    // dirección: hacia abajo
		1.0f, 0.0f, 0.0f,     // atenuación (sin caída por distancia)
		5.0f                   // ángulo del cono en grados
	);
	spotLightCount++;

	// --- 9. OBTENER UBICACIONES DE UNIFORMS ---
	// Los "uniforms" son variables que enviamos desde C++ al shader.
	// Primero obtenemos su ubicación (ID) para luego asignarles valor.
	GLuint uniformProjection = 0, uniformModel = 0, uniformView = 0,
		uniformEyePosition = 0, uniformSpecularIntensity = 0,
		uniformShininess = 0, uniformColor = 0;

	// --- 10. MATRIZ DE PROYECCIÓN ---
	// Transforma coordenadas 3D a coordenadas de pantalla 2D.
	// perspective(FOV, aspecto, plano_cercano, plano_lejano)
	//   - FOV 45°: campo de visión (ángulo de apertura de la cámara)
	//   - Aspecto: relación ancho/alto para evitar distorsión
	//   - 0.1 a 1000: rango de distancias visibles (near/far planes)
	glm::mat4 projection = glm::perspective(
		glm::radians(45.0f),
		(GLfloat)mainWindow.getBufferWidth() / mainWindow.getBufferHeight(),
		0.1f, 1000.0f
	);

	// ============================================================
	// 11. RENDER LOOP (bucle principal)
	// ============================================================
	// Se ejecuta cada frame hasta que el usuario cierre la ventana.
	// Cada iteración: actualiza tiempo → procesa input → dibuja escena.
	while (!mainWindow.getShouldClose())
	{
		// --- Cálculo de deltaTime ---
		// deltaTime mide el tiempo entre frames para que el movimiento
		// sea independiente de los FPS (se mueve igual a 30 o 60 FPS).
		GLfloat now = glfwGetTime();
		deltaTime = now - lastTime;
		lastTime = now;

		// --- Procesamiento de entrada ---
		glfwPollEvents();  // Captura eventos de teclado/ratón
		camera.keyControl(mainWindow.getsKeys(), 100*deltaTime);   // WASD
		camera.mouseControl(mainWindow.getXChange(), mainWindow.getYChange()); // Ratón

		// --- Limpieza de buffers ---
		// Se limpia el color (fondo negro) y el buffer de profundidad
		// (Z-buffer) para que los objetos se dibujen correctamente
		// según su distancia a la cámara.
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// --- Dibujar skybox ---
		// Se dibuja primero, antes de los objetos de la escena.
		skybox.DrawSkybox(camera.calculateViewMatrix(), projection);

		// --- Activar shader de iluminación ---
		shaderList[0].UseShader();

		// Obtener ubicaciones de los uniforms en el shader activo
		uniformModel = shaderList[0].GetModelLocation();
		uniformProjection = shaderList[0].GetProjectionLocation();
		uniformView = shaderList[0].GetViewLocation();
		uniformEyePosition = shaderList[0].GetEyePositionLocation();
		uniformColor = shaderList[0].getColorLocation();
		uniformSpecularIntensity = shaderList[0].GetSpecularIntensityLocation();
		uniformShininess = shaderList[0].GetShininessLocation();

		// --- Enviar matrices globales al shader ---
		// Projection: cómo se proyecta la escena 3D en pantalla
		glUniformMatrix4fv(uniformProjection, 1, GL_FALSE, glm::value_ptr(projection));
		// View: posición y orientación de la cámara
		glUniformMatrix4fv(uniformView, 1, GL_FALSE, glm::value_ptr(camera.calculateViewMatrix()));
		// EyePosition: posición de la cámara (necesaria para cálculo especular)
		// La reflexión especular depende del ángulo entre el ojo y la luz.
		glUniform3f(uniformEyePosition,
			camera.getCameraPosition().x,
			camera.getCameraPosition().y,
			camera.getCameraPosition().z);

		// --- Actualizar linterna (spotlight ligada a la cámara) ---
		// La linterna sigue la posición y dirección de la cámara
		// en tiempo real, simulando que el jugador la sostiene.
		glm::vec3 lowerLight = camera.getCameraPosition();
		lowerLight.y -= 0.3f;  // Ligeramente abajo para efecto realista
		spotLights[0].SetFlash(lowerLight, camera.getCameraDirection());

		// --- Enviar información de luces al shader ---
		// El shader recibe todas las fuentes de luz para calcular
		// la iluminación de cada fragmento (píxel).
		shaderList[0].SetDirectionalLight(&mainLight);
		shaderList[0].SetPointLights(pointLights, pointLightCount);
		shaderList[0].SetSpotLights(spotLights, spotLightCount);

		// ========================================================
		// DIBUJADO DE OBJETOS
		// ========================================================
		// Para cada objeto se necesita:
		//   1. Definir su matriz Model (posición, rotación, escala)
		//   2. Enviar la matriz al shader con glUniformMatrix4fv
		//   3. Asignar textura y material
		//   4. Llamar a RenderMesh() o RenderModel()

		glm::mat4 model(1.0);  // Matriz identidad (sin transformación)
		glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f); // Color blanco (neutro)

		// --- PISO ---
		// translate: lo baja 1 unidad en Y
		// scale: lo agranda 30x en X y Z para cubrir más área
		model = glm::translate(glm::mat4(1.0), glm::vec3(0.0f, -1.0f, 0.0f));
		model = glm::rotate(model, 0.0f, glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(30.0f, 1.0f, 30.0f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glUniform3fv(uniformColor, 1, glm::value_ptr(color));
		pisoTexture.UseTexture();  // Activar textura del piso
		// Aplicar material opaco: el shader usa estos valores para
		// calcular el componente especular de la iluminación.
		Material_opaco.UseMaterial(uniformSpecularIntensity, uniformShininess);
		meshList[0]->RenderMesh(); // Dibujar el piso

		// ------------------------------------------------------------------ AQUI DEFINIMOS EL MUNDO ------------------------------------------
		// --- LÁMPARA (modelo 3D) ---
		// Se coloca justo sobre el piso (x=0, y=-1, z=0) y se escala a 5%
		dibuja_modelo(lamp_model, 0.0f, -1.0f, 0.0f, 0.05);

		// --- Desactivar shader y presentar frame ---
		glUseProgram(0);          // Desenlazar el shader
		mainWindow.swapBuffers(); // Intercambiar buffers (doble buffer)
		// El doble buffer evita parpadeo: mientras uno se muestra
		// en pantalla, el otro se dibuja en memoria.
	}

	return 0;
}