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
Camera aerialCamera;
int cameraMode = 0; // 0 = 3ra persona (sigue al avatar), 1 = aérea

// Avatar (Joker como personaje principal)
glm::vec3 avatarPos(0.0f, -1.0f, 0.0f); // posición en el mundo (Y=-1 = nivel del suelo)
float     avatarYaw = -90.0f;            // dirección que mira (°), -90 = hacia -Z
static const float AVATAR_SPEED      = 30.0f; // unidades/segundo
static const float AVATAR_TURN_SPEED = 0.3f;  // grados por unidad de mouse

Texture pisoTexture;     // Textura que se aplica al plano del piso
Model lamp_model;        // Modelo 3D de una lámpara

// Modelo Joker (Ladrones Fantasma).
Model Joker_M;

// Escenario
Model CentralBuilding_M;
Model SteampunkHouse_M;
Model SteampunkHouse2_M;
Model SteampunkPostOffice_M;
Model SteampunkProp_M;
Model BazaarSteampunk_M;
Model TimePortal_M;
Model Pilar_M;
Model Cervantes_M;
Model Poe_M;
Model Shakespeare_M;

Skybox skybox;           // Skybox (fondo envolvente)
Material Material_opaco; // Material con bajo brillo especular

// Variables de control de tiempo para movimiento uniforme
GLfloat deltaTime = 0.0f;  // Tiempo entre frames
GLfloat lastTime = 0.0f;   // Tiempo del frame anterior
static double limitFPS = 1.0 / 60.0;  // Límite de 60 FPS

// Ciclo día/noche: duración total en segundos (máximo 120 según lineamientos)
static const float CYCLE_DURATION = 60.0f;

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

	// Cámara aérea: posición elevada, mirando hacia abajo (pitch=-70°)
	// Tecla 2 activa este modo, tecla 1 regresa al modo normal
	aerialCamera = Camera(
		glm::vec3(0.0f, 700.0f, 0.0f), // posición elevada sobre el centro
		glm::vec3(0.0f, 1.0f, 0.0f),   // vector up
		-90.0f, -89.0f,                 // yaw, pitch (mirando recto hacia abajo)
		1.0f, 0.3f                      // velocidad, sensibilidad
	);

	// --- 4. CARGA DE TEXTURAS ---
	// LoadTextureA() carga la textura incluyendo canal alfa (transparencia)
	pisoTexture = Texture("Textures/Suelocyberpunk.jpg");
	pisoTexture.LoadTexture();

	// --- 5. CARGA DE MODELOS 3D ---
	// Assimp lee el archivo .obj/.fbx y genera los meshes con sus texturas
	lamp_model = Model();
	lamp_model.LoadModel("Models/redstone_lamp.obj");

	// Joker (Ladrones Fantasma)
	Joker_M = Model();
	Joker_M.LoadModel("Models/LadronesFantasma/Joker.glb");

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
		glfwPollEvents();
		bool* keys = mainWindow.getsKeys();

		// Cambio de cámara: tecla 1 = normal (3a persona) | tecla 2 = aérea
		static bool key1Prev = false, key2Prev = false;
		if (keys[GLFW_KEY_1] && !key1Prev) cameraMode = 0;
		if (keys[GLFW_KEY_2] && !key2Prev) cameraMode = 1;
		key1Prev = keys[GLFW_KEY_1];
		key2Prev = keys[GLFW_KEY_2];

		Camera* activeCamera = (cameraMode == 0) ? &camera : &aerialCamera;
		if (cameraMode == 0) {
			// --- 3ra PERSONA: mover avatar con WASD, girar con mouse horizontal ---
			avatarYaw += mainWindow.getXChange() * AVATAR_TURN_SPEED;
			mainWindow.getYChange(); // consumir vertical sin usarla

			float yawRad = glm::radians(avatarYaw);
			glm::vec3 avFwd(  cosf(yawRad), 0.0f,  sinf(yawRad));
			glm::vec3 avRight(-sinf(yawRad), 0.0f,  cosf(yawRad));
			float spd = AVATAR_SPEED * deltaTime;
			if (keys[GLFW_KEY_W]) avatarPos += avFwd   * spd;
			if (keys[GLFW_KEY_S]) avatarPos -= avFwd   * spd;
			if (keys[GLFW_KEY_A]) avatarPos -= avRight * spd;
			if (keys[GLFW_KEY_D]) avatarPos += avRight * spd;

			// Cámara detrás y encima del avatar, mirando a su torso
			glm::vec3 camPos   = avatarPos - avFwd * 8.0f + glm::vec3(0.0f, 4.0f, 0.0f);
			glm::vec3 lookAt   = avatarPos + glm::vec3(0.0f, 2.0f, 0.0f);
			camera.setPositionAndLookAt(camPos, lookAt);
		} else {
			aerialCamera.keyControlAerial(keys, 100 * deltaTime);
			mainWindow.getXChange();
			mainWindow.getYChange();
		}

		// Toggle del spotlight con tecla F
		static bool fKeyPrev = false;
		static bool spotlightOn = true;
		if (keys[GLFW_KEY_F] && !fKeyPrev) spotlightOn = !spotlightOn;
		fKeyPrev = keys[GLFW_KEY_F];

		// ============================================================
		// CICLO DÍA / NOCHE
		// cycleTime: 0.0 = mediodía, 0.5 = medianoche
		// ============================================================
		float cycleTime = fmod((float)glfwGetTime(), CYCLE_DURATION) / CYCLE_DURATION;
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

		// --- Desactivar shader y presentar frame ---
		glUseProgram(0);          // Desenlazar el shader
		mainWindow.swapBuffers(); // Intercambiar buffers (doble buffer)
		// El doble buffer evita parpadeo: mientras uno se muestra
		// en pantalla, el otro se dibuja en memoria.
	}

	return 0;
}