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
Camera camera;       // modo 1 — 3ra persona (sigue al avatar)
Camera aerialCamera; // modo 2 — aérea cenital
Camera freeCamera;   // modo 3 — libre (primera persona)
Camera poi1Camera;   // modo 4 — galería de bustos: frente
Camera poi2Camera;   // modo 5 — galería de bustos: lateral
Camera poi3Camera;   // modo 6 — galería de bustos: elevada
int cameraMode = 0;  // 0-5 según tecla presionada.

// Avatar (Joker como personaje principal.
glm::vec3 avatarPos(0.0f, -1.0f, 0.0f); // posición en el mundo (Y=-1 = nivel del suelo)
float     avatarYaw = -90.0f;            // dirección que mira (°), -90 = hacia -Z
static const float AVATAR_SPEED      = 30.0f; // unidades/segundo
static const float AVATAR_TURN_SPEED = 0.3f;  // grados por unidad de mouse

Texture pisoTexture;     // Textura que se aplica al plano del piso
Model lamp_model;        // Modelo 3D de una lámpara

// Ladrones Fantasma
Model Joker_M;
Model Ladrones_M;

// Escenario
Model CentralBuilding_M;
Model SteampunkHouse_M;
Model SteampunkHouse2_M;
Model SteampunkPostOffice_M;
Model SteampunkProp_M;
Model BazaarSteampunk_M;
Model TimePortal_M;
Model Subway_M;
Model Pilar_M;
Model Cervantes_M;
Model Poe_M;
Model Shakespeare_M;

Skybox skybox;           // Skybox (fondo envolvente)
Material Material_opaco; // Material con bajo brillo especular

// ---- Tren ----
Model Train_M;
Model TrainWheels_M;
Model TrainBars_M;
Texture trackTexture;   // tile de vías
Texture humoTexture;    // partícula de humo

// Circuito del tren: rectángulo a ±280 unidades del centro
// El tren recorre 4 segmentos: +Z, -X, -Z, +X
static const float TRACK_R  = 280.0f;  // radio del circuito
static const float TRAIN_SPEED = 25.0f; // unidades/segundo
static const float PERIMETER  = 8.0f * TRACK_R; // 4 lados * 2*280

float trainT = 0.0f;  // distancia recorrida acumulada (wraps en PERIMETER)
float wheelAngle = 0.0f;  // ángulo de giro ruedas
float barsAngle  = 0.0f;  // ángulo manivelas

// Partículas de humo
struct SmokeParticle {
    glm::vec3 pos;
    float life;    // 0=vivo, 1=muerto
    float size;
    float alpha;
};
static const int MAX_SMOKE = 20;
SmokeParticle smoke[MAX_SMOKE];
float smokeTimer = 0.0f;

// Variables de control de tiempo para movimiento uniforme
GLfloat deltaTime = 0.0f;  // Tiempo entre frames
GLfloat lastTime = 0.0f;   // Tiempo del frame anterior
static double limitFPS = 1.0 / 60.0;  // Límite de 60 FPS

// Ciclo día/noche: duración total en segundos (máximo 120 según lineamientos)
static const float CYCLE_DURATION = 60.0f;
float cycleElapsed = 0.0f;  // tiempo acumulado del ciclo (0 = mediodía)
bool  cycleRunning = false; // inicia pausado; tecla 7 lo activa una vez

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
		 10.0f,  0.0f, -10.0f,   1.0f,  0.0f,   0.0f, -1.0f,  0.0f,
		-10.0f,  0.0f,  10.0f,   0.0f,  1.0f,   0.0f, -1.0f,  0.0f,
		 10.0f,  0.0f,  10.0f,   1.0f,  1.0f,   0.0f, -1.0f,  0.0f
	};

	// Se crea el mesh del piso y se agrega a la lista (índice 0)
	Mesh* piso = new Mesh();
	piso->CreateMesh(floorVertices, floorIndices, 32, 6);
	meshList.push_back(piso);

	// Quad cuadrado unitario para tiles de tracks (índice 1)
	// -0.5 a 0.5 en X y Z, se escala uniformemente al render
	GLfloat tileVertices[] = {
		-0.5f, 0.0f, -0.5f,  0.0f, 0.0f,  0.0f, 1.0f, 0.0f,
		 0.5f, 0.0f, -0.5f,  1.0f, 0.0f,  0.0f, 1.0f, 0.0f,
		-0.5f, 0.0f,  0.5f,  0.0f, 1.0f,  0.0f, 1.0f, 0.0f,
		 0.5f, 0.0f,  0.5f,  1.0f, 1.0f,  0.0f, 1.0f, 0.0f
	};
	Mesh* trackTile = new Mesh();
	trackTile->CreateMesh(tileVertices, floorIndices, 32, 6);
	meshList.push_back(trackTile); // índice 1
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

	// Cámara aérea (tecla 2)
	aerialCamera = Camera(
		glm::vec3(0.0f, 700.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f),
		-90.0f, -89.0f,
		1.0f, 0.3f
	);

	// Cámara libre / primera persona (tecla 3)
	freeCamera = Camera(
		glm::vec3(0.0f, 2.5f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f),
		-60.0f, 0.0f,
		1.0f, 0.5f
	);

	// Cámaras fijas — tercer recorrido: galería de bustos (teclas 4-6)
	// Los bustos están en X≈30-46, Y≈7-10, Z≈-193 a -200
	poi1Camera = Camera(glm::vec3(0,0,0), glm::vec3(0,1,0), 0, 0, 0, 0);
	poi1Camera.setPositionAndLookAt(
		glm::vec3(38.0f,  8.0f, -155.0f),  // frente a la galería
		glm::vec3(38.0f,  8.0f, -200.0f)
	);
	poi2Camera = Camera(glm::vec3(0,0,0), glm::vec3(0,1,0), 0, 0, 0, 0);
	poi2Camera.setPositionAndLookAt(
		glm::vec3( 5.0f,  8.0f, -197.0f),  // lateral izquierdo
		glm::vec3(46.0f,  8.0f, -200.0f)
	);
	poi3Camera = Camera(glm::vec3(0,0,0), glm::vec3(0,1,0), 0, 0, 0, 0);
	poi3Camera.setPositionAndLookAt(
		glm::vec3(20.0f, 30.0f, -155.0f),  // vista elevada 3/4
		glm::vec3(40.0f,  5.0f, -200.0f)
	);

	// --- 4. CARGA DE TEXTURAS ---
	// LoadTextureA() carga la textura incluyendo canal alfa (transparencia)
	pisoTexture = Texture("Textures/Suelocyberpunk.jpg");
	pisoTexture.LoadTexture();

	// --- 5. CARGA DE MODELOS 3D ---
	// Assimp lee el archivo .obj/.fbx y genera los meshes con sus texturas
	lamp_model = Model();
	lamp_model.LoadModel("Models/redstone_lamp.obj");

	// Ladrones Fantasma
	Joker_M    = Model(); Joker_M.LoadModel("Models/LadronesFantasma/Joker.glb");
	Ladrones_M = Model(); Ladrones_M.LoadModel("Models/LadronesFantasma/Ladrones.glb");

	// Escenario
	CentralBuilding_M = Model();
	CentralBuilding_M.LoadModel("Models/escenario/centralBuilding.glb");
	SteampunkHouse_M = Model();
	SteampunkHouse_M.LoadModel("Models/escenario/steampunk_house.glb");
	SteampunkHouse2_M = Model();
	SteampunkHouse2_M.LoadModel("Models/escenario/steampunk_house2.glb");
	SteampunkPostOffice_M = Model();
	SteampunkPostOffice_M.LoadModel("Models/escenario/steampunk_post_office.glb");
	SteampunkProp_M = Model();
	SteampunkProp_M.LoadModel("Models/escenario/steampunk_prop.glb");
	BazaarSteampunk_M = Model();
	BazaarSteampunk_M.LoadModel("Models/escenario/bazaar_steampunk.glb");
	TimePortal_M = Model();
	TimePortal_M.LoadModel("Models/escenario/time_portal_steampunk.glb");
	Pilar_M = Model();
	Pilar_M.LoadModel("Models/bustos/pilar.glb");
	Cervantes_M = Model();
	Cervantes_M.LoadModel("Models/bustos/cervantes_statue.glb");
	Poe_M = Model();
	Poe_M.LoadModel("Models/bustos/poe_statue.glb");
	Shakespeare_M = Model();
	Shakespeare_M.LoadModel("Models/bustos/william_shakespeare_statue.glb");

	// Tren
	Subway_M = Model(); Subway_M.LoadModel("Models/LadronesFantasma/subway.glb");
	Train_M      = Model(); Train_M.LoadModel("Models/Escenario/Train/train.glb");
	TrainWheels_M = Model(); TrainWheels_M.LoadModel("Models/Escenario/Train/wheels.glb");
	TrainBars_M   = Model(); TrainBars_M.LoadModel("Models/Escenario/Train/bars.glb");
	trackTexture  = Texture("Textures/train_tracks.png"); trackTexture.LoadTextureA();
	humoTexture   = Texture("Textures/Humo.png");         humoTexture.LoadTextureA();

	// Inicializar partículas de humo (todas muertas al inicio)
	for (int i = 0; i < MAX_SMOKE; i++) { smoke[i].life = 1.0f; }

	// --- 6. CONFIGURACIÓN DEL SKYBOX ---
	// Un skybox es un cubo gigante con 6 texturas (una por cara)
	// que envuelve toda la escena para simular un cielo.
	// El orden importa: derecha, izquierda, abajo, arriba, atrás, frente.
	std::vector<std::string> skyboxFaces;
	skyboxFaces.push_back("Textures/new_Skybox/miramar_rt.tga");
	skyboxFaces.push_back("Textures/new_Skybox/miramar_lf.tga");
	skyboxFaces.push_back("Textures/new_Skybox/miramar_dn.tga");
	skyboxFaces.push_back("Textures/new_Skybox/miramar_up.tga");
	skyboxFaces.push_back("Textures/new_Skybox/miramar_bk.tga");
	skyboxFaces.push_back("Textures/new_Skybox/miramar_ft.tga");
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

	// LUMINARIAS PUNTUALES — se prenden de noche automáticamente
	// Posicionadas cerca de la lámpara y puntos clave del escenario.
	// Atenuación suave (0.014, 0.0007) para iluminar un radio amplio.
	unsigned int pointLightCount = 0;

	// Luminaria 1 — junto a la lámpara redstone (izquierda del centro)
	pointLights[0] = PointLight(
		1.0f, 0.85f, 0.5f,     // color ámbar/cálido (como farola)
		0.3f, 1.5f,             // intensidad ambiental y difusa
		-20.0f, 4.0f, 0.0f,    // posición: encima del lamp_model
		1.0f, 0.014f, 0.0007f  // atenuación: rango amplio
	);
	pointLightCount++;

	// Luminaria 2 — zona del Bazaar
	pointLights[1] = PointLight(
		1.0f, 0.85f, 0.5f,
		0.3f, 1.5f,
		80.0f, 4.0f, 40.0f,    // encima del bazaar
		1.0f, 0.014f, 0.0007f
	);
	pointLightCount++;

	// Luminaria 3 — entrada al Central Building
	pointLights[2] = PointLight(
		1.0f, 0.85f, 0.5f,
		0.3f, 1.5f,
		0.0f, 4.0f, -15.0f,    // frente al edificio central
		1.0f, 0.014f, 0.0007f
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
		uniformShininess = 0, uniformColor = 0, uniformAlpha = 0, uniformNoLighting = 0;

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
		glfwPollEvents();
		bool* keys = mainWindow.getsKeys();

		// Cambio de cámara con teclas 1-6 (detección de flanco)
		static bool keyPrev[6] = {};
		int camKeys[6] = { GLFW_KEY_1, GLFW_KEY_2, GLFW_KEY_3,
		                   GLFW_KEY_4, GLFW_KEY_5, GLFW_KEY_6 };
		for (int i = 0; i < 6; i++) {
			if (keys[camKeys[i]] && !keyPrev[i]) cameraMode = i;
			keyPrev[i] = keys[camKeys[i]];
		}

		// Seleccionar cámara activa
		Camera* camPtrs[6] = { &camera, &aerialCamera, &freeCamera,
		                       &poi1Camera, &poi2Camera, &poi3Camera };
		Camera* activeCamera = camPtrs[cameraMode];

		if (cameraMode == 0) {
			// --- MODO 1: 3ra persona — avatar con WASD, giro con mouse ---
			avatarYaw += mainWindow.getXChange() * AVATAR_TURN_SPEED;
			mainWindow.getYChange();

			float yawRad = glm::radians(avatarYaw);
			glm::vec3 avFwd(  cosf(yawRad), 0.0f,  sinf(yawRad));
			glm::vec3 avRight(-sinf(yawRad), 0.0f,  cosf(yawRad));
			float spd = AVATAR_SPEED * deltaTime;
			if (keys[GLFW_KEY_W]) avatarPos += avFwd   * spd;
			if (keys[GLFW_KEY_S]) avatarPos -= avFwd   * spd;
			if (keys[GLFW_KEY_A]) avatarPos -= avRight * spd;
			if (keys[GLFW_KEY_D]) avatarPos += avRight * spd;

			glm::vec3 camPos = avatarPos - avFwd * 8.0f + glm::vec3(0.0f, 4.0f, 0.0f);
			camera.setPositionAndLookAt(camPos, avatarPos + glm::vec3(0.0f, 2.0f, 0.0f));

		} else if (cameraMode == 1) {
			// --- MODO 2: aérea — WASD en plano XZ, mouse bloqueado ---
			aerialCamera.keyControlAerial(keys, 100 * deltaTime);
			mainWindow.getXChange();
			mainWindow.getYChange();

		} else if (cameraMode == 2) {
			// --- MODO 3: libre (primera persona) — WASD + mouse completo ---
			freeCamera.keyControl(keys, 100 * deltaTime);
			freeCamera.mouseControl(mainWindow.getXChange(), mainWindow.getYChange());

		} else {
			// --- MODOS 4-6: cámaras fijas del tercer recorrido ---
			mainWindow.getXChange();
			mainWindow.getYChange();
		}

		// Toggle del spotlight con tecla F
		static bool fKeyPrev = false;
		static bool spotlightOn = true;
		if (keys[GLFW_KEY_F] && !fKeyPrev) spotlightOn = !spotlightOn;
		fKeyPrev = keys[GLFW_KEY_F];

		// ============================================================
		// CICLO DÍA / NOCHE  (tecla 7 = reproducir una vez desde mediodía)
		// cycleTime: 0.0 = mediodía, 0.5 = medianoche, 1.0 = mediodía otra vez
		// ============================================================
		static bool key7Prev = false;
		if (keys[GLFW_KEY_7] && !key7Prev) {
			cycleElapsed = 0.0f;
			cycleRunning = true;
		}
		key7Prev = keys[GLFW_KEY_7];

		if (cycleRunning) {
			cycleElapsed += deltaTime;
			if (cycleElapsed >= CYCLE_DURATION) {
				cycleElapsed = CYCLE_DURATION; // congela en mediodía al terminar
				cycleRunning = false;
			}
		}

		float cycleTime = cycleElapsed / CYCLE_DURATION;
		float sunAngle  = cycleTime * 2.0f * 3.14159265f;

		// dayFactor: 1 = pleno día, 0 = plena noche (suavizado con coseno)
		float dayFactor = glm::max(0.0f, cosf(sunAngle));

		// Dirección del sol: gira en el plano Y-Z
		float sunDirX =  0.3f;
		float sunDirY = -cosf(sunAngle); // -1 al mediodía, +1 a medianoche
		float sunDirZ =  sinf(sunAngle);

		// Color de la luz según la fase del día
		glm::vec3 dayColor  (1.00f, 0.95f, 0.80f); // blanco cálido
		glm::vec3 dawnColor (1.00f, 0.45f, 0.10f); // naranja amanecer/atardecer
		glm::vec3 nightColor(0.02f, 0.02f, 0.10f); // azul oscuro noche

		glm::vec3 sunColor;
		if (dayFactor > 0.3f)
			sunColor = glm::mix(dawnColor, dayColor,  (dayFactor - 0.3f) / 0.7f);
		else if (dayFactor > 0.0f)
			sunColor = glm::mix(nightColor, dawnColor, dayFactor / 0.3f);
		else
			sunColor = nightColor;

		// Reconstruir la luz direccional con los valores del ciclo
		mainLight = DirectionalLight(
			sunColor.r, sunColor.g, sunColor.b,
			0.05f + dayFactor * 0.35f, // ambientIntensity: baja de noche
			dayFactor * 0.8f,          // diffuseIntensity: 0 de noche
			sunDirX, sunDirY, sunDirZ
		);

		// Luminarias puntuales: las 3 se encienden de noche (dayFactor < 0.25)
		pointLightCount = (dayFactor < 0.25f) ? 3 : 0;

		// Tinte del skybox según fase del día
		glm::vec3 skyTintColor = glm::max(sunColor, glm::vec3(0.04f, 0.04f, 0.12f));
		skybox.SetTint(skyTintColor.r, skyTintColor.g, skyTintColor.b);

		// --- Limpieza de buffers ---
		// Se limpia el color (fondo negro) y el buffer de profundidad
		// (Z-buffer) para que los objetos se dibujen correctamente
		// según su distancia a la cámara.
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// --- Dibujar skybox ---
		skybox.DrawSkybox(activeCamera->calculateViewMatrix(), projection);

		// --- Activar shader de iluminación ---
		shaderList[0].UseShader();

		// Obtener ubicaciones de los uniforms en el shader activo
		uniformModel = shaderList[0].GetModelLocation();
		uniformProjection = shaderList[0].GetProjectionLocation();
		uniformView = shaderList[0].GetViewLocation();
		uniformEyePosition = shaderList[0].GetEyePositionLocation();
		uniformColor = shaderList[0].getColorLocation();
		uniformAlpha = shaderList[0].getAlphaLocation();
		uniformNoLighting = shaderList[0].getNoLightingLocation();
		glUniform1f(uniformAlpha, 1.0f);    // alpha por defecto = opaco
		glUniform1i(uniformNoLighting, 0);  // iluminación activa por defecto
		uniformSpecularIntensity = shaderList[0].GetSpecularIntensityLocation();
		uniformShininess = shaderList[0].GetShininessLocation();

		// --- Enviar matrices globales al shader ---
		// Projection: cómo se proyecta la escena 3D en pantalla
		glUniformMatrix4fv(uniformProjection, 1, GL_FALSE, glm::value_ptr(projection));
		// View: posición y orientación de la cámara activa
		glUniformMatrix4fv(uniformView, 1, GL_FALSE, glm::value_ptr(activeCamera->calculateViewMatrix()));
		glUniform3f(uniformEyePosition,
			activeCamera->getCameraPosition().x,
			activeCamera->getCameraPosition().y,
			activeCamera->getCameraPosition().z);

		// --- Actualizar linterna (spotlight ligada a la cámara) ---
		// La linterna sigue la posición y dirección de la cámara
		// en tiempo real, simulando que el jugador la sostiene.
		glm::vec3 lowerLight = activeCamera->getCameraPosition();
		lowerLight.y -= 0.3f;
		spotLights[0].SetFlash(lowerLight, activeCamera->getCameraDirection());

		// --- Enviar información de luces al shader ---
		// El shader recibe todas las fuentes de luz para calcular
		// la iluminación de cada fragmento (píxel).
		shaderList[0].SetDirectionalLight(&mainLight);
		shaderList[0].SetPointLights(pointLights, pointLightCount);
		shaderList[0].SetSpotLights(spotLights, spotlightOn ? spotLightCount : 0);

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
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(-20.0f, -1.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.05f, 0.05f, 0.05f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glUniform3fv(uniformColor, 1, glm::value_ptr(color));
		Material_opaco.UseMaterial(uniformSpecularIntensity, uniformShininess);
		lamp_model.RenderModel();

		// --- JOKER (avatar del jugador) ---
		model = glm::mat4(1.0);
		model = glm::translate(model, avatarPos);
		model = glm::rotate(model, glm::radians(-(avatarYaw+90)), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(0.01f, 0.01f, 0.01f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glUniform3fv(uniformColor, 1, glm::value_ptr(color));
		Material_opaco.UseMaterial(uniformSpecularIntensity, uniformShininess);
		glDisable(GL_CULL_FACE);
		Joker_M.RenderModel();
		glEnable(GL_CULL_FACE);

		// --- LADRONES FANTASMA — afuera del subway ---
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(230.0f, -1.0f, -10.0f));
		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.09f, 0.09f, 0.09f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glUniform3fv(uniformColor, 1, glm::value_ptr(color));
		Material_opaco.UseMaterial(uniformSpecularIntensity, uniformShininess);
		glDisable(GL_CULL_FACE);
		Ladrones_M.RenderModel();
		glEnable(GL_CULL_FACE);

		// --- CENTRAL BUILDING ---
		model = glm::mat4(1.0);
		// Central Building — centrado, movido hacia atrás en Z
		model = glm::translate(model, glm::vec3(0.0f, -1.0f, -30.0f));
		model = glm::scale(model, glm::vec3(7.5f, 7.5f, 7.5f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glUniform3fv(uniformColor, 1, glm::value_ptr(color));
		Material_opaco.UseMaterial(uniformSpecularIntensity, uniformShininess);
		glDisable(GL_CULL_FACE);
		CentralBuilding_M.RenderModel();
		glEnable(GL_CULL_FACE);

		// Steampunk House — periferia derecha delantera
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(220.0f, -1.0f, 70.0f));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(25.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(19.5f, 19.5f, 19.5f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glDisable(GL_CULL_FACE);
		SteampunkHouse_M.RenderModel();
		glEnable(GL_CULL_FACE);

		// Steampunk House — periferia derecha trasera
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(250.0f, -1.0f, -170.0f));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(-35.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(19.5f, 19.5f, 19.5f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glDisable(GL_CULL_FACE);
		SteampunkHouse_M.RenderModel();
		glEnable(GL_CULL_FACE);

		// Steampunk House 2 — periferia izquierda delantera
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(-230.0f, 7.0f, 50.0f));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(-20.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(15.0f, 15.0f, 15.0f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glDisable(GL_CULL_FACE);
		SteampunkHouse2_M.RenderModel();
		glEnable(GL_CULL_FACE);

		// Steampunk House 2 — periferia izquierda trasera
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(-260.0f, 7.0f, -160.0f));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(40.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(15.0f, 15.0f, 15.0f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glDisable(GL_CULL_FACE);
		SteampunkHouse2_M.RenderModel();
		glEnable(GL_CULL_FACE);

		// Steampunk House — periferia frente derecha
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(130.0f, -1.0f, 230.0f));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(55.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(19.5f, 19.5f, 19.5f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glDisable(GL_CULL_FACE);
		SteampunkHouse_M.RenderModel();
		glEnable(GL_CULL_FACE);

		// Steampunk House — periferia atrás centro
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(50.0f, -1.0f, -245.0f));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(-15.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(19.5f, 19.5f, 19.5f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glDisable(GL_CULL_FACE);
		SteampunkHouse_M.RenderModel();
		glEnable(GL_CULL_FACE);

		// Steampunk House 2 — periferia frente izquierda
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(-130.0f, 7.0f, 225.0f));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(-50.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(15.0f, 15.0f, 15.0f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glDisable(GL_CULL_FACE);
		SteampunkHouse2_M.RenderModel();
		glEnable(GL_CULL_FACE);

		// Steampunk House 2 — periferia atrás izquierda
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(-240.0f, 7.0f, -230.0f));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(20.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(15.0f, 15.0f, 15.0f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glDisable(GL_CULL_FACE);
		SteampunkHouse2_M.RenderModel();
		glEnable(GL_CULL_FACE);

		// Bazaar — más alejado del centro
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(80.0f, -1.0f, 40.0f));
		model = glm::rotate(model, glm::radians(15.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.095f, 0.095f, 0.095f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glDisable(GL_CULL_FACE);
		BazaarSteampunk_M.RenderModel();
		glEnable(GL_CULL_FACE);

		// Time Portal — lado izquierdo
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(-80.0f, -1.0f, 60.0f));
		model = glm::rotate(model, glm::radians(-30.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::scale(model, glm::vec3(4.5f, -4.5f, 4.5f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glDisable(GL_CULL_FACE);
		TimePortal_M.RenderModel();
		glEnable(GL_CULL_FACE);

		// Time Portal — lado derecho lejos
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(-200.0f, -1.0f, -160.0f));
		model = glm::rotate(model, glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::scale(model, glm::vec3(4.5f, -4.5f, 4.5f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glDisable(GL_CULL_FACE);
		TimePortal_M.RenderModel();
		glEnable(GL_CULL_FACE);

		// ---- Galería de bustos — fila en X, fondo del mapa Z=-200 ----
		// Separación 8 unidades en X, base X=30

		// Pilar 1  (X=30)
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(30.0f, -1.0f, -200.0f));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(1.5f, 1.5f, 1.5f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glDisable(GL_CULL_FACE);
		Pilar_M.RenderModel();
		glEnable(GL_CULL_FACE);

		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(30.0f, 10.0f, -193.0f));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(1.2f, 1.2f, 1.2f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glDisable(GL_CULL_FACE);
		Cervantes_M.RenderModel();
		glEnable(GL_CULL_FACE);

		// Pilar 2  (X=38)
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(38.0f, -1.0f, -200.0f));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(1.5f, 1.5f, 1.5f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glDisable(GL_CULL_FACE);
		Pilar_M.RenderModel();
		glEnable(GL_CULL_FACE);

		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(38.0f, 10.0f, -200.0f));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(4.0f, 4.0f, 4.0f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glDisable(GL_CULL_FACE);
		Poe_M.RenderModel();
		glEnable(GL_CULL_FACE);

		// Pilar 3  (X=46)
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(46.0f, -1.0f, -200.0f));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(1.5f, 1.5f, 1.5f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glDisable(GL_CULL_FACE);
		Pilar_M.RenderModel();
		glEnable(GL_CULL_FACE);

		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(46.0f, 7.0f, -200.0f));
		model = glm::scale(model, glm::vec3(12.0f, 12.0f, 12.0f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glDisable(GL_CULL_FACE);
		Shakespeare_M.RenderModel();
		glEnable(GL_CULL_FACE);

		// Pilar 4 — vacío  (X=54)
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(54.0f, -1.0f, -200.0f));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(1.5f, 1.5f, 1.5f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glDisable(GL_CULL_FACE);
		Pilar_M.RenderModel();
		glEnable(GL_CULL_FACE);

		// Pilar 5 — vacío  (X=62)
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(62.0f, -1.0f, -200.0f));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(1.5f, 1.5f, 1.5f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glDisable(GL_CULL_FACE);
		Pilar_M.RenderModel();
		glEnable(GL_CULL_FACE);

		// Pilar 6 — vacío  (X=70)
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(70.0f, -1.0f, -200.0f));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(1.5f, 1.5f, 1.5f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glDisable(GL_CULL_FACE);
		Pilar_M.RenderModel();
		glEnable(GL_CULL_FACE);

		// Steampunk Prop — cerca del centro
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(0.0f, -1.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.015f, 0.015f, 0.015f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glDisable(GL_CULL_FACE);
		SteampunkProp_M.RenderModel();
		glEnable(GL_CULL_FACE);

		// Steampunk Post Office — esquina derecha trasera
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(100.0f, -1.0f, -60.0f));
		model = glm::scale(model, glm::vec3(7.5f, 7.5f, 7.5f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glDisable(GL_CULL_FACE);
		SteampunkPostOffice_M.RenderModel();
		glEnable(GL_CULL_FACE);

		// Steampunk Post Office espejo — misma posición, espejo en Z
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(100.0f, -1.0f, -60.0f));
		model = glm::scale(model, glm::vec3(7.5f, 7.5f, -7.5f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glDisable(GL_CULL_FACE);
		SteampunkPostOffice_M.RenderModel();
		glEnable(GL_CULL_FACE);

		// Subway — cerca de la casa derecha delantera (220, -1, 70)
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(210.0f, -3.5f, 90.0f));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(30.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(5.0f, 5.0f, 5.0f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glDisable(GL_CULL_FACE);
		Subway_M.RenderModel();
		glEnable(GL_CULL_FACE);

		// ================================================================
		// TREN — animación y render
		// ================================================================
		{
			const float tileSize   = 20.0f;                 // debe coincidir con tracks
			const float cornerSize = 80.0f;                 // zona de curva (4 * tileSize)
			const float sideLen    = 2.0f * TRACK_R;
			const float straightEnd = TRACK_R - cornerSize;

			trainT += TRAIN_SPEED * deltaTime;
			if (trainT >= PERIMETER) trainT -= PERIMETER;
			barsAngle += TRAIN_SPEED * deltaTime * 3.0f;

			// ── Calcular posición y dirección continua sobre el circuito ──
			// El circuito tiene 4 segmentos rectos + 4 curvas de esquina.
			// Para el yaw continuo, muestreamos la posición un instante
			// adelante y calculamos atan2 de la diferencia.
			auto getCircuitPos = [&](float dist) -> glm::vec3 {
				float d = fmodf(dist, PERIMETER);
				if (d < 0) d += PERIMETER;

				// Longitud de cada segmento recto (entre esquinas)
				float straightLen = 2.0f * straightEnd;
				// Longitud de cada cuarto de arco
				float curveLen = (3.14159265f * 0.5f) * cornerSize;

				// El circuito recorre en este orden:
				//  Seg0 recto: X=+R, Z de -straightEnd → +straightEnd  (+Z)
				//  Curva NE: centro (+straightEnd, +straightEnd), arco 0°→90° (gira a -X)
				//  Seg1 recto: Z=+R, X de +straightEnd → -straightEnd  (-X)
				//  Curva NW: centro (-straightEnd, +straightEnd), arco 90°→180°
				//  Seg2 recto: X=-R, Z de +straightEnd → -straightEnd  (-Z)
				//  Curva SW: centro (-straightEnd, -straightEnd), arco 180°→270°
				//  Seg3 recto: Z=-R, X de -straightEnd → +straightEnd  (+X)
				//  Curva SE: centro (+straightEnd, -straightEnd), arco 270°→360°

				// Seg 0 recto (+Z)
				if (d < straightLen) {
					float frac = d / straightLen;
					return glm::vec3(TRACK_R, -1.0f, -straightEnd + frac * straightLen);
				}
				d -= straightLen;
				// Curva NE: centro (straightEnd, straightEnd), entra por X=+R Z=+straightEnd, sale por Z=+R X=+straightEnd
				if (d < curveLen) {
					float a = glm::radians(d / curveLen * 90.0f); // 0→90°
					return glm::vec3(straightEnd + cornerSize * cosf(a), -1.0f, straightEnd + cornerSize * sinf(a));
				}
				d -= curveLen;
				// Seg 1 recto (-X)
				if (d < straightLen) {
					float frac = d / straightLen;
					return glm::vec3(straightEnd - frac * straightLen, -1.0f, TRACK_R);
				}
				d -= straightLen;
				// Curva NW: centro (-straightEnd, straightEnd)
				if (d < curveLen) {
					float a = glm::radians(90.0f + d / curveLen * 90.0f); // 90→180°
					return glm::vec3(-straightEnd + cornerSize * cosf(a), -1.0f, straightEnd + cornerSize * sinf(a));
				}
				d -= curveLen;
				// Seg 2 recto (-Z)
				if (d < straightLen) {
					float frac = d / straightLen;
					return glm::vec3(-TRACK_R, -1.0f, straightEnd - frac * straightLen);
				}
				d -= straightLen;
				// Curva SW: centro (-straightEnd, -straightEnd)
				if (d < curveLen) {
					float a = glm::radians(180.0f + d / curveLen * 90.0f); // 180→270°
					return glm::vec3(-straightEnd + cornerSize * cosf(a), -1.0f, -straightEnd + cornerSize * sinf(a));
				}
				d -= curveLen;
				// Seg 3 recto (+X)
				if (d < straightLen) {
					float frac = d / straightLen;
					return glm::vec3(-straightEnd + frac * straightLen, -1.0f, -TRACK_R);
				}
				d -= straightLen;
				// Curva SE: centro (straightEnd, -straightEnd)
				if (d < curveLen) {
					float a = glm::radians(270.0f + d / curveLen * 90.0f); // 270→360°
					return glm::vec3(straightEnd + cornerSize * cosf(a), -1.0f, -straightEnd + cornerSize * sinf(a));
				}
				return glm::vec3(TRACK_R, -1.0f, -straightEnd);
			};

			glm::vec3 trainPos  = getCircuitPos(trainT);
			glm::vec3 trainPos2 = getCircuitPos(trainT + 0.5f); // muestra adelante para dirección
			glm::vec3 trainDir  = glm::normalize(trainPos2 - trainPos);

			// Yaw continuo: atan2 del vector dirección en plano XZ
			float modelYaw = glm::degrees(atan2f(trainDir.x, trainDir.z)) + 270.0f;

			// Frente del tren en coordenadas mundo (para humo)
			glm::vec3 trainFront = trainPos + trainDir * 15.0f;

			// --- Cuerpo del tren ---
			model = glm::mat4(1.0f);
			model = glm::translate(model, trainPos);
			model = glm::rotate(model, glm::radians(modelYaw), glm::vec3(0.0f, 1.0f, 0.0f));
			model = glm::scale(model, glm::vec3(20.0f, 20.0f, 20.0f));
			glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
			glDisable(GL_CULL_FACE);
			Train_M.RenderModel();
			glEnable(GL_CULL_FACE);

			// --- Ruedas (adelantadas en dirección de marcha, sin inclinación) ---
			glm::vec3 wheelsPos = trainPos + trainDir * 18.0f + glm::vec3(0.0f, 3.0f, 0.0f);
			model = glm::mat4(1.0f);
			model = glm::translate(model, wheelsPos);
			model = glm::rotate(model, glm::radians(modelYaw), glm::vec3(0.0f, 1.0f, 0.0f));
			model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
			model = glm::rotate(model, glm::radians(-10.0f), glm::vec3(0.0f, 0.0f, 1.0f));
			model = glm::scale(model, glm::vec3(20.0f, 20.0f, 20.0f));
			glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
			glDisable(GL_CULL_FACE);
			TrainWheels_M.RenderModel();
			glEnable(GL_CULL_FACE);

			// --- Barras: óvalo aplastado en el eje local del tren ---
			// El offset se aplica en espacio local (perpendicular a la dirección)
			float barsRad = glm::radians(barsAngle);
			glm::vec3 localRight = glm::normalize(glm::cross(trainDir, glm::vec3(0,1,0)));
			float barsOffsetSide = cosf(barsRad) * 1.2f;
			float barsOffsetUp   = sinf(barsRad) * 0.3f;
			glm::vec3 barsPos = trainPos
				+ localRight * barsOffsetSide
				+ glm::vec3(0.0f, 8.0f + barsOffsetUp, 0.0f); // +8 para subirlas
			model = glm::mat4(1.0f);
			model = glm::translate(model, barsPos);
			model = glm::rotate(model, glm::radians(modelYaw), glm::vec3(0.0f, 1.0f, 0.0f));
			model = glm::scale(model, glm::vec3(20.0f, 20.0f, 20.0f));
			glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
			glDisable(GL_CULL_FACE);
			TrainBars_M.RenderModel();
			glEnable(GL_CULL_FACE);

			// ================================================================
			// HUMO — partículas billboard con alpha real
			// ================================================================
			smokeTimer += deltaTime;
			if (smokeTimer >= 0.6f) {
				smokeTimer = 0.0f;
				for (int i = 0; i < MAX_SMOKE; i++) {
					if (smoke[i].life >= 1.0f) {
						// Spawn en el frente del tren, elevado
						smoke[i].pos   = trainFront + glm::vec3(0.0f, 25.0f, 0.0f);
						smoke[i].life  = 0.0f;
						smoke[i].size  = 2.5f;
						smoke[i].alpha = 1.0f;
						break;
					}
				}
			}

			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glDepthMask(GL_FALSE);
			glUniform1i(uniformNoLighting, 1); // humo sin iluminación
			humoTexture.UseTexture();

			glm::mat4 viewMat = activeCamera->calculateViewMatrix();
			glm::vec3 camRight(viewMat[0][0], viewMat[1][0], viewMat[2][0]);
			glm::vec3 camUp   (viewMat[0][1], viewMat[1][1], viewMat[2][1]);

			for (int i = 0; i < MAX_SMOKE; i++) {
				if (smoke[i].life >= 1.0f) continue;
				smoke[i].life  += deltaTime * 0.3f;
				smoke[i].pos.y += deltaTime * 18.0f;
				smoke[i].size   = 2.5f + smoke[i].life * 10.0f;
				smoke[i].alpha  = 1.0f - smoke[i].life;

				float s = smoke[i].size;
				glm::vec3 p = smoke[i].pos;
				glm::vec3 v0 = p - camRight*s - camUp*s;
				glm::vec3 v1 = p + camRight*s - camUp*s;
				glm::vec3 v2 = p + camRight*s + camUp*s;
				glm::vec3 v3 = p - camRight*s + camUp*s;

				GLfloat smokeVerts[] = {
					v0.x,v0.y,v0.z, 0.0f,0.0f, 0.0f,1.0f,0.0f,
					v1.x,v1.y,v1.z, 1.0f,0.0f, 0.0f,1.0f,0.0f,
					v2.x,v2.y,v2.z, 1.0f,1.0f, 0.0f,1.0f,0.0f,
					v3.x,v3.y,v3.z, 0.0f,1.0f, 0.0f,1.0f,0.0f
				};
				unsigned int smokeIdx[] = {0,1,2, 0,2,3};
				Mesh* smokeQuad = new Mesh();
				smokeQuad->CreateMesh(smokeVerts, smokeIdx, 32, 6);

				glUniform1f(uniformAlpha, smoke[i].alpha);  // alpha real via uniform
				glUniform3fv(uniformColor, 1, glm::value_ptr(color));
				model = glm::mat4(1.0f);
				glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
				smokeQuad->RenderMesh();
				delete smokeQuad;
			}
			glUniform1f(uniformAlpha, 1.0f);
			glUniform1i(uniformNoLighting, 0); // restaurar iluminación
			glDepthMask(GL_TRUE);
			glDisable(GL_BLEND);
		}

		// ================================================================
		// TRAIN TRACKS — tiles rodeando el perímetro con curvas suaves
		// ================================================================
		{
			glUniform1i(uniformNoLighting, 1);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glDepthMask(GL_FALSE);
			trackTexture.UseTexture();
			glUniform3fv(uniformColor, 1, glm::value_ptr(color));
			const float tileSize    = 30.0f;   // más anchas
			const float r           = TRACK_R;
			const float cornerSize  = 90.0f;   // radio del arco de esquina
			const float straightEnd = r - cornerSize;

			// drawTile: posición central del tile + yaw tangente al recorrido
			auto drawTile = [&](float tx, float tz, float yawDeg) {
				model = glm::mat4(1.0f);
				model = glm::translate(model, glm::vec3(tx, -0.99f, tz));
				model = glm::rotate(model, glm::radians(yawDeg), glm::vec3(0.0f,1.0f,0.0f));
				model = glm::scale(model, glm::vec3(tileSize, 1.0f, tileSize));
				glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
				meshList[1]->RenderMesh();
			};

			float halfTile = tileSize * 0.5f;
			// Lados rectos
			for (float z = -straightEnd; z < straightEnd; z += tileSize)
				drawTile( r, z + halfTile, 90.0f);
			for (float z = -straightEnd; z < straightEnd; z += tileSize)
				drawTile(-r, z + halfTile, 90.0f);
			for (float x = -straightEnd; x < straightEnd; x += tileSize)
				drawTile(x + halfTile,  r, 0.0f);
			for (float x = -straightEnd; x < straightEnd; x += tileSize)
				drawTile(x + halfTile, -r, 0.0f);

			// Curvas — posición sobre el arco con yaw tangente correcto
			// Para un arco con ángulo paramétrico 'a' (radianes):
			//   pos = centro + cornerSize * (cos(a), sin(a))
			//   tangente = perpendicular al radio = yaw derivado de la dirección de avance
			const int CURVE_STEPS = 12;
			const float eps = 0.01f; // offset pequeño para calcular tangente

			// Helper: tangente del arco en el punto 'a' = atan2 de la diferencia entre dos puntos cercanos
			// Para cada esquina se define su centro y se samplea a±eps para obtener la dirección real

			// Esquina NE: centro (straightEnd, straightEnd), arco 0→90°
			for (int s = 0; s < CURVE_STEPS; s++) {
				float a   = glm::radians((s + 0.5f) / CURVE_STEPS * 90.0f);
				float cx  =  straightEnd + cornerSize * cosf(a);
				float cz  =  straightEnd + cornerSize * sinf(a);
				float cx2 =  straightEnd + cornerSize * cosf(a + eps);
				float cz2 =  straightEnd + cornerSize * sinf(a + eps);
				float yaw = glm::degrees(atan2f(cx2 - cx, cz2 - cz)) + 90.0f;
				drawTile(cx, cz, yaw);
			}
			// Esquina NW: centro (-straightEnd, straightEnd), arco 90→180°
			for (int s = 0; s < CURVE_STEPS; s++) {
				float a   = glm::radians(90.0f + (s + 0.5f) / CURVE_STEPS * 90.0f);
				float cx  = -straightEnd + cornerSize * cosf(a);
				float cz  =  straightEnd + cornerSize * sinf(a);
				float cx2 = -straightEnd + cornerSize * cosf(a + eps);
				float cz2 =  straightEnd + cornerSize * sinf(a + eps);
				float yaw = glm::degrees(atan2f(cx2 - cx, cz2 - cz)) + 90.0f;
				drawTile(cx, cz, yaw);
			}
			// Esquina SW: centro (-straightEnd, -straightEnd), arco 180→270°
			for (int s = 0; s < CURVE_STEPS; s++) {
				float a   = glm::radians(180.0f + (s + 0.5f) / CURVE_STEPS * 90.0f);
				float cx  = -straightEnd + cornerSize * cosf(a);
				float cz  = -straightEnd + cornerSize * sinf(a);
				float cx2 = -straightEnd + cornerSize * cosf(a + eps);
				float cz2 = -straightEnd + cornerSize * sinf(a + eps);
				float yaw = glm::degrees(atan2f(cx2 - cx, cz2 - cz)) + 90.0f;
				drawTile(cx, cz, yaw);
			}
			// Esquina SE: centro (straightEnd, -straightEnd), arco 270→360°
			for (int s = 0; s < CURVE_STEPS; s++) {
				float a   = glm::radians(270.0f + (s + 0.5f) / CURVE_STEPS * 90.0f);
				float cx  =  straightEnd + cornerSize * cosf(a);
				float cz  = -straightEnd + cornerSize * sinf(a);
				float cx2 =  straightEnd + cornerSize * cosf(a + eps);
				float cz2 = -straightEnd + cornerSize * sinf(a + eps);
				float yaw = glm::degrees(atan2f(cx2 - cx, cz2 - cz)) + 90.0f;
				drawTile(cx, cz, yaw);
			}
			glDepthMask(GL_TRUE);
			glDisable(GL_BLEND);
			glUniform1i(uniformNoLighting, 0);
		}

		// --- Desactivar shader y presentar frame ---
		glUseProgram(0);          // Desenlazar el shader
		mainWindow.swapBuffers(); // Intercambiar buffers (doble buffer)
		// El doble buffer evita parpadeo: mientras uno se muestra
		// en pantalla, el otro se dibuja en memoria.
	}

	return 0;
}