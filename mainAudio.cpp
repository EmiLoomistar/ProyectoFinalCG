/*
 * ================================================================
 * Proyecto de Computación Gráfica e Interacción Humano Computadora
 * ================================================================
 */

 // Macro necesaria para que stb_image.h genere la implementación
 // de sus funciones (solo debe definirse en UN archivo .cpp
#define STB_IMAGE_IMPLEMENTATION

// Implementación única de miniaudio (header-only)
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include <stdio.h>
#include <string.h>
#include <cmath>
#include <vector>

// --- Bibliotecas de OpenGL ---
#include <glew.h>   // Extensiones de OpenGL (funciones modernas).
#include <glfw3.h>  // Creación de ventana y manejo de input

// --- Biblioteca de matemáticas GLM ---
#include <glm.hpp>
#include <gtc\matrix_transform.hpp>
#include <gtc\type_ptr.hpp>

// --- Clases propias del proyecto ---
#include "Window.h"
#include "Mesh.h"
#include "Shader_light.h"
#include "Camera.h"
#include "Texture.h"
#include "Model.h"
#include "Skybox.h"
#include "AnimatedModel.h"

// --- Clases de iluminación ---
#include "CommonValues.h"
#include "DirectionalLight.h"
#include "PointLight.h"
#include "SpotLight.h"
#include "Material.h"

// ============================================================
// Variables globales
// ============================================================

Window mainWindow;
std::vector<Mesh*> meshList;
std::vector<Shader> shaderList;
Camera camera;       // modo 1 — 3ra persona (sigue al avatar)
Camera aerialCamera; // modo 2 — aérea cenital
Camera freeCamera;   // modo 3 — libre (primera persona)
Camera poi1Camera;   // modo 4 — galería de bustos: frente
Camera poi2Camera;   // modo 5 — galería de bustos: lateral
Camera poi3Camera;   // modo 6 — galería de bustos: elevada
int cameraMode = 0;

// Avatar (Joker)
glm::vec3 avatarPos(0.0f, 6.0f, 0.0f);
float     avatarYaw = 0.0f;
static const float AVATAR_SPEED      = 30.0f;
static const float AVATAR_TURN_SPEED = 0.3f;

Texture pisoTexture;
Model lamp_model;

// Ladrones Fantasma
Model Joker_M;
Model Ladrones_M;

// Joker bailarín (partes separadas para animación procedural)
Model JokerDance_Cabeza_M;
Model JokerDance_Cuerpo_M;
Model JokerDance_BrazoDer_M;
Model JokerDance_BrazoIzq_M;
Model JokerDance_PiernaDer_M;
Model JokerDance_PiernaIzq_M;

// Joker avatar (mismas partes, animación de caminar)
Model JokerAvatar_Cabeza_M;
Model JokerAvatar_Cuerpo_M;
Model JokerAvatar_BrazoDer_M;
Model JokerAvatar_BrazoIzq_M;
Model JokerAvatar_PiernaDer_M;
Model JokerAvatar_PiernaIzq_M;
float avatarWalkCycle = 0.0f; // acumula tiempo para la animación
bool  avatarMoving    = false;

// Escenario Steampunk
Model CentralBuilding_M;
Model SteampunkHouse_M;
Model SteampunkHouse2_M;
Model SteampunkPostOffice_M;
Model SteampunkProp_M;
Model BazaarSteampunk_M;
Model TimePortal_M;
Model TimePortalBase_M;
Model TimePortalDisk_M;
Model Subway_M;
Model Shop_M;
Model PotionShop_M;
Model SteampunkLamp_M;

// Proyector Steampunk
Model ProyectorSteampunk_M;
Model Engranaje1_M, Engranaje2_M, Engranaje3_M;
Model EngranajeAdicional1_M, EngranajeAdicional2_M;
Model EngranajeAtras1_M, EngranajeAtras2_M;
Model EngranajeProyector1_M, EngranajeProyector2_M;
float engranajeAngle = 0.0f;

// Galería de bustos
Model Pilar_M;
Model Cervantes_M;
Model Poe_M;
Model Shakespeare_M;

// Batman universe
Model Batman_M;
Model Catwoman_M;
Model Robin_M;
Model Batwing_M;
Model StreetLamp_M;
Model Robot_M;
Model BatmanRigged_M;
AnimatedModel BatmanAnim;

// Eva
Model Rei_M;
Model Misato_M;
Model Asuka_M;
Model Unit01_M;

// Globe (elemento decorativo animado)
Model Globe_M;
Model Globe_Ball_M;

Skybox skybox;
Material Material_opaco;

// ---- Tren ----
Model Train_M;
Model TrainWheels_M;
Model TrainBars_M;
Texture trackTexture;
Texture humoTexture;

static const float TRACK_R    = 280.0f;
static const float TRAIN_SPEED = 25.0f;
static const float PERIMETER  = 8.0f * TRACK_R;

float trainT     = 0.0f;
float wheelAngle = 0.0f;
float barsAngle  = 0.0f;

// Partículas de humo
struct SmokeParticle {
    glm::vec3 pos;
    float life;
    float size;
    float alpha;
};
static const int MAX_SMOKE = 20;
SmokeParticle smoke[MAX_SMOKE];
float smokeTimer = 0.0f;

// Tiempo
GLfloat deltaTime = 0.0f;
GLfloat lastTime  = 0.0f;
static double limitFPS = 1.0 / 60.0;

// Ciclo día/noche
static const float CYCLE_DURATION = 60.0f;
float cycleElapsed = 0.0f;
bool  cycleRunning = false;

// Luces
DirectionalLight mainLight;
PointLight pointLights[MAX_POINT_LIGHTS];
SpotLight  spotLights[MAX_SPOT_LIGHTS];

static const char* vShader = "shaders/shader_light.vert";
static const char* fShader = "shaders/shader_light.frag";

// ============================================================
// Audio (miniaudio)
// ============================================================
ma_engine audioEngine;
ma_sound  soundSoundtrack;
ma_sound  soundAmbient;
ma_sound  soundFootstep;
bool      footstepPlaying = false;

// ============================================================
// CreateObjects
// ============================================================
void CreateObjects()
{
	unsigned int floorIndices[] = { 0, 2, 1, 1, 2, 3 };

	GLfloat floorVertices[] = {
		-10.0f, 0.0f, -10.0f,   0.0f,  0.0f,  0.0f, -1.0f, 0.0f,
		 10.0f, 0.0f, -10.0f,  20.0f,  0.0f,  0.0f, -1.0f, 0.0f,
		-10.0f, 0.0f,  10.0f,   0.0f, 20.0f,  0.0f, -1.0f, 0.0f,
		 10.0f, 0.0f,  10.0f,  20.0f, 20.0f,  0.0f, -1.0f, 0.0f
	};
	Mesh* piso = new Mesh();
	piso->CreateMesh(floorVertices, floorIndices, 32, 6);
	meshList.push_back(piso); // índice 0

	// Quad tile para vías del tren
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
// CreateShaders
// ============================================================
void CreateShaders()
{
	shaderList.reserve(2);

	Shader* shader1 = new Shader();
	shader1->CreateFromFiles(vShader, fShader);
	shaderList.push_back(*shader1);

	// Shader para modelos animados (skinning)
	Shader* shaderAnim = new Shader();
	shaderAnim->CreateFromFiles("shaders/shader_anim.vert", fShader);
	shaderList.push_back(*shaderAnim);
}

// ============================================================
// dibuja_modelo: helper rápido
// ============================================================
void dibuja_modelo(Model model, float x, float y, float z, float escala)
{
	glm::mat4 world(1.0);
	glm::vec3 color(1.0f, 1.0f, 1.0f);
	GLuint uniformModel = shaderList[0].GetModelLocation();
	GLuint uniformColor = shaderList[0].getColorLocation();
	world = glm::translate(glm::mat4(1.0), glm::vec3(x, y, z));
	world = glm::rotate(world, 0.0f, glm::vec3(1.0f, 0.0f, 0.0f));
	world = glm::scale(world, glm::vec3(escala, escala, escala));
	glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(world));
	glUniform3fv(uniformColor, 1, glm::value_ptr(color));
	model.RenderModel();
}

// ============================================================
// main
// ============================================================
int main()
{
	// --- 1. VENTANA ---
	mainWindow = Window(1366, 768);
	mainWindow.Initialise();

	// --- AUDIO ---
	ma_engine_init(NULL, &audioEngine);

	ma_sound_init_from_file(&audioEngine, "Sounds/soundtrack.mp3",
		MA_SOUND_FLAG_STREAM | MA_SOUND_FLAG_ASYNC, NULL, NULL, &soundSoundtrack);
	ma_sound_set_looping(&soundSoundtrack, MA_TRUE);
	ma_sound_set_volume(&soundSoundtrack, 0.6f);
	ma_sound_start(&soundSoundtrack);

	ma_sound_init_from_file(&audioEngine, "Sounds/ambient.wav",
		MA_SOUND_FLAG_STREAM | MA_SOUND_FLAG_ASYNC, NULL, NULL, &soundAmbient);
	ma_sound_set_looping(&soundAmbient, MA_TRUE);
	ma_sound_set_volume(&soundAmbient, 0.35f);
	ma_sound_start(&soundAmbient);

	ma_sound_init_from_file(&audioEngine, "Sounds/effect.wav",
		0, NULL, NULL, &soundFootstep);
	ma_sound_set_looping(&soundFootstep, MA_TRUE);
	ma_sound_set_volume(&soundFootstep, 0.8f);

	// --- 2. GEOMETRÍA Y SHADERS ---
	CreateObjects();
	CreateShaders();

	// --- 3. CÁMARAS ---
	camera = Camera(
		glm::vec3(0.0f, 2.5f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f),
		-60.0f, 0.0f,
		0.3f, 0.5f
	);
	aerialCamera = Camera(
		glm::vec3(0.0f, 700.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f),
		-90.0f, -89.0f,
		1.0f, 0.3f
	);
	freeCamera = Camera(
		glm::vec3(0.0f, 2.5f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f),
		-60.0f, 0.0f,
		1.0f, 0.5f
	);
	poi1Camera = Camera(glm::vec3(0, 0, 0), glm::vec3(0, 1, 0), 0, 0, 0, 0);
	poi1Camera.setPositionAndLookAt(glm::vec3(38.0f, 8.0f, -155.0f), glm::vec3(38.0f, 8.0f, -200.0f));
	poi2Camera = Camera(glm::vec3(0, 0, 0), glm::vec3(0, 1, 0), 0, 0, 0, 0);
	poi2Camera.setPositionAndLookAt(glm::vec3(5.0f, 8.0f, -197.0f), glm::vec3(46.0f, 8.0f, -200.0f));
	poi3Camera = Camera(glm::vec3(0, 0, 0), glm::vec3(0, 1, 0), 0, 0, 0, 0);
	poi3Camera.setPositionAndLookAt(glm::vec3(20.0f, 30.0f, -155.0f), glm::vec3(40.0f, 5.0f, -200.0f));

	// --- 4. TEXTURAS ---
	pisoTexture = Texture("Textures/suelo2.png");
	pisoTexture.LoadTextureA();

	// --- 5. MODELOS ---
	lamp_model = Model();
	lamp_model.LoadModel("Models/redstone_lamp.obj");

	// Ladrones Fantasma
	Joker_M = Model(); Joker_M.LoadModel("Models/LadronesFantasma/Joker.glb");
	Ladrones_M = Model(); Ladrones_M.LoadModel("Models/LadronesFantasma/Ladrones.glb");

	// Joker bailarín — partes separadas
	JokerDance_Cabeza_M = Model(); JokerDance_Cabeza_M.LoadModel("Models/LadronesFantasma/Joker/joker_cabeza.glb");
	JokerDance_Cuerpo_M = Model(); JokerDance_Cuerpo_M.LoadModel("Models/LadronesFantasma/Joker/joker_cuerpo.glb");
	JokerDance_BrazoDer_M = Model(); JokerDance_BrazoDer_M.LoadModel("Models/LadronesFantasma/Joker/joker_brazo_der.glb");
	JokerDance_BrazoIzq_M = Model(); JokerDance_BrazoIzq_M.LoadModel("Models/LadronesFantasma/Joker/joker_brazo_izq.glb");
	JokerDance_PiernaDer_M = Model(); JokerDance_PiernaDer_M.LoadModel("Models/LadronesFantasma/Joker/joker_pierna_der.glb");
	JokerDance_PiernaIzq_M = Model(); JokerDance_PiernaIzq_M.LoadModel("Models/LadronesFantasma/Joker/joker_pierna_izq.glb");

	// Joker avatar — mismas partes, instancias separadas
	JokerAvatar_Cabeza_M    = Model(); JokerAvatar_Cabeza_M.LoadModel("Models/LadronesFantasma/Joker/joker_cabeza.glb");
	JokerAvatar_Cuerpo_M    = Model(); JokerAvatar_Cuerpo_M.LoadModel("Models/LadronesFantasma/Joker/joker_cuerpo.glb");
	JokerAvatar_BrazoDer_M  = Model(); JokerAvatar_BrazoDer_M.LoadModel("Models/LadronesFantasma/Joker/joker_brazo_der.glb");
	JokerAvatar_BrazoIzq_M  = Model(); JokerAvatar_BrazoIzq_M.LoadModel("Models/LadronesFantasma/Joker/joker_brazo_izq.glb");
	JokerAvatar_PiernaDer_M = Model(); JokerAvatar_PiernaDer_M.LoadModel("Models/LadronesFantasma/Joker/joker_pierna_der.glb");
	JokerAvatar_PiernaIzq_M = Model(); JokerAvatar_PiernaIzq_M.LoadModel("Models/LadronesFantasma/Joker/joker_pierna_izq.glb");

	// Escenario Steampunk
	CentralBuilding_M = Model(); CentralBuilding_M.LoadModel("Models/escenario/centralBuilding.glb");
	SteampunkHouse_M = Model(); SteampunkHouse_M.LoadModel("Models/escenario/steampunk_house.glb");
	SteampunkHouse2_M = Model(); SteampunkHouse2_M.LoadModel("Models/escenario/steampunk_house2.glb");
	SteampunkPostOffice_M = Model(); SteampunkPostOffice_M.LoadModel("Models/escenario/steampunk_post_office.glb");
	SteampunkProp_M = Model(); SteampunkProp_M.LoadModel("Models/escenario/steampunk_prop.glb");
	BazaarSteampunk_M = Model(); BazaarSteampunk_M.LoadModel("Models/escenario/bazaar_steampunk.glb");
	TimePortal_M = Model(); TimePortal_M.LoadModel("Models/escenario/time_portal_steampunk.glb");
	Subway_M = Model(); Subway_M.LoadModel("Models/LadronesFantasma/subway.glb");
	Shop_M = Model(); Shop_M.LoadModel("Models/Escenario/shop.glb");
	PotionShop_M = Model(); PotionShop_M.LoadModel("Models/Escenario/potion_shop.glb");
	SteampunkLamp_M = Model(); SteampunkLamp_M.LoadModel("Models/Escenario/steampunk_lamp.glb");

	// Proyector Steampunk
	ProyectorSteampunk_M = Model(); ProyectorSteampunk_M.LoadModel("Models/Escenario/ProyectorSteampunk/ProyectorSteampunk.obj");
	Engranaje1_M = Model(); Engranaje1_M.LoadModel("Models/Escenario/ProyectorSteampunk/Engranaje1.obj");
	Engranaje2_M = Model(); Engranaje2_M.LoadModel("Models/Escenario/ProyectorSteampunk/Engranaje2.obj");
	Engranaje3_M = Model(); Engranaje3_M.LoadModel("Models/Escenario/ProyectorSteampunk/Engranaje3.obj");
	EngranajeAdicional1_M = Model(); EngranajeAdicional1_M.LoadModel("Models/Escenario/ProyectorSteampunk/Engranajeadicional1.obj");
	EngranajeAdicional2_M = Model(); EngranajeAdicional2_M.LoadModel("Models/Escenario/ProyectorSteampunk/Engranajeadicional2.obj");
	EngranajeAtras1_M = Model(); EngranajeAtras1_M.LoadModel("Models/Escenario/ProyectorSteampunk/Engranajeatras1.obj");
	EngranajeAtras2_M = Model(); EngranajeAtras2_M.LoadModel("Models/Escenario/ProyectorSteampunk/Engranajeatras2.obj");
	EngranajeProyector1_M = Model(); EngranajeProyector1_M.LoadModel("Models/Escenario/ProyectorSteampunk/Engranajeproyector1.obj");
	EngranajeProyector2_M = Model(); EngranajeProyector2_M.LoadModel("Models/Escenario/ProyectorSteampunk/Engranajeproyector2.obj");

	// Galería de bustos
	Pilar_M = Model(); Pilar_M.LoadModel("Models/bustos/pilar.glb");
	Cervantes_M = Model(); Cervantes_M.LoadModel("Models/bustos/cervantes_statue.glb");
	Poe_M = Model(); Poe_M.LoadModel("Models/bustos/poe_statue.glb");
	Shakespeare_M = Model(); Shakespeare_M.LoadModel("Models/bustos/william_shakespeare_statue.glb");

	// Batman universe
	Batman_M = Model(); Batman_M.LoadModel("Models/arkham_city_batman.glb");
	BatmanRigged_M = Model(); BatmanRigged_M.LoadModel("Models/batmanRigged.glb");
	BatmanAnim.LoadModel("Models/batmanRigged.glb");
	Catwoman_M = Model(); Catwoman_M.LoadModel("Models/catwoman.glb");
	Robin_M = Model(); Robin_M.LoadModel("Models/robin.glb");
	Batwing_M = Model(); Batwing_M.LoadModel("Models/batwing.glb");
	StreetLamp_M = Model(); StreetLamp_M.LoadModel("Models/street_lamp.glb");
	Robot_M = Model(); Robot_M.LoadModel("Models/robot.obj");

	// Globe decorativo
	Globe_M = Model(); Globe_M.LoadModel("Models/Escenario/globe.glb");
	Globe_Ball_M = Model(); Globe_Ball_M.LoadModel("Models/Escenario/globe_ball.glb");

	// Tren
	Train_M = Model(); Train_M.LoadModel("Models/Escenario/Train/train.glb");
	TrainWheels_M = Model(); TrainWheels_M.LoadModel("Models/Escenario/Train/wheels.glb");
	TrainBars_M = Model(); TrainBars_M.LoadModel("Models/Escenario/Train/bars.glb");
	trackTexture = Texture("Textures/train_tracks.png"); trackTexture.LoadTextureA();
	humoTexture = Texture("Textures/Humo.png");         humoTexture.LoadTextureA();

	// Evangelion
	Rei_M = Model(); Rei_M.LoadModel("Models/Evangelion/rei.glb");
	Misato_M = Model(); Misato_M.LoadModel("Models/Evangelion/misato2.glb");
	Asuka_M = Model(); Asuka_M.LoadModel("Models/Evangelion/asuka.glb");
	Unit01_M = Model(); Unit01_M.LoadModel("Models/Evangelion/unit01.glb");


	for (int i = 0; i < MAX_SMOKE; i++) smoke[i].life = 1.0f;

	// --- 6. SKYBOX ---
	std::vector<std::string> skyboxFaces;
	skyboxFaces.push_back("Textures/new_Skybox/miramar_rt.tga");
	skyboxFaces.push_back("Textures/new_Skybox/miramar_lf.tga");
	skyboxFaces.push_back("Textures/new_Skybox/miramar_dn.tga");
	skyboxFaces.push_back("Textures/new_Skybox/miramar_up.tga");
	skyboxFaces.push_back("Textures/new_Skybox/miramar_bk.tga");
	skyboxFaces.push_back("Textures/new_Skybox/miramar_ft.tga");
	skybox = Skybox(skyboxFaces);

	// --- 7. MATERIALES ---
	Material_opaco = Material(0.3f, 4);
	Material Material_metalico = Material(2.0f, 64);

	// --- 8. LUCES ---
	mainLight = DirectionalLight(
		1.0f, 1.0f, 1.0f,
		0.3f, 0.8f,
		0.0f, -1.0f, 0.0f
	);

	unsigned int pointLightCount = 0;

	// 4 luces — una por cada steampunk lamp
	// Posiciones: top de cada lámpara (Y=8)
	// (60,-1,80), (20,-1,-80), (150,-1,-50), (-150,-1,120)
	pointLights[0] = PointLight(1.0f, 0.85f, 0.5f, 0.0f, 2.0f,   60.0f, 8.0f,   80.0f, 1.0f, 0.007f, 0.0002f);
	pointLights[1] = PointLight(1.0f, 0.85f, 0.5f, 0.0f, 2.0f,   20.0f, 8.0f,  -80.0f, 1.0f, 0.007f, 0.0002f);
	pointLights[2] = PointLight(1.0f, 0.85f, 0.5f, 0.0f, 2.0f,  150.0f, 8.0f,  -50.0f, 1.0f, 0.007f, 0.0002f);
	pointLights[3] = PointLight(1.0f, 0.85f, 0.5f, 0.0f, 2.0f, -150.0f, 8.0f,  120.0f, 1.0f, 0.007f, 0.0002f);
	pointLightCount = 4;

	unsigned int spotLightCount = 0;
	spotLights[0] = SpotLight(
		1.0f, 1.0f, 1.0f,
		0.0f, 2.0f,
		0.0f, 0.0f, 0.0f,
		0.0f, -1.0f, 0.0f,
		1.0f, 0.0f, 0.0f,
		5.0f
	);
	spotLightCount++;

	// Haz del proyector steampunk — apunta hacia adelante-abajo, siempre encendido
	spotLights[1] = SpotLight(
		0.85f, 0.9f, 1.0f,
		0.1f, 3.0f,
		-50.0f, 4.0f, -80.0f,   // posición del proyector
		1.0f, -0.15f, 0.0f,     // dirección: +X (frente nativo del modelo)
		1.0f, 0.02f, 0.001f,
		35.0f
	);
	spotLightCount++;

	// --- 9. UNIFORMS ---
	GLuint uniformProjection = 0, uniformModel = 0, uniformView = 0,
		uniformEyePosition = 0, uniformSpecularIntensity = 0,
		uniformShininess = 0, uniformColor = 0,
		uniformAlpha = 0, uniformNoLighting = 0,
		uniformUseTexture = 0;

	// --- 10. PROYECCIÓN ---
	glm::mat4 projection = glm::perspective(
		glm::radians(45.0f),
		(GLfloat)mainWindow.getBufferWidth() / mainWindow.getBufferHeight(),
		0.1f, 1000.0f
	);

	// Variables de animación persistentes
	static float nave_pos = 0.0f;
	static float nave_j = 0.0f;
	static float globe_i = 0.0f;

	// ============================================================
	// 11. RENDER LOOP
	// ============================================================
	while (!mainWindow.getShouldClose())
	{
		GLfloat now = glfwGetTime();
		deltaTime = now - lastTime;
		lastTime = now;

		glfwPollEvents();
		bool* keys = mainWindow.getsKeys();

		// Cambio de cámara (flanco)
		static bool keyPrev[6] = {};
		int camKeys[6] = { GLFW_KEY_1, GLFW_KEY_2, GLFW_KEY_3,
						   GLFW_KEY_4, GLFW_KEY_5, GLFW_KEY_6 };
		for (int i = 0; i < 6; i++) {
			if (keys[camKeys[i]] && !keyPrev[i]) cameraMode = i;
			keyPrev[i] = keys[camKeys[i]];
		}

		Camera* camPtrs[6] = { &camera, &aerialCamera, &freeCamera,
							   &poi1Camera, &poi2Camera, &poi3Camera };
		Camera* activeCamera = camPtrs[cameraMode];

		if (cameraMode == 0) {
			avatarYaw += mainWindow.getXChange() * AVATAR_TURN_SPEED;
			mainWindow.getYChange();
			float yawRad = glm::radians(avatarYaw);
			glm::vec3 avFwd(cosf(yawRad), 0.0f, sinf(yawRad));
			glm::vec3 avRight(-sinf(yawRad), 0.0f, cosf(yawRad));
			float spd = AVATAR_SPEED * deltaTime;
			avatarMoving = false;
			if (keys[GLFW_KEY_W]) { avatarPos += avFwd   * spd; avatarMoving = true; }
			if (keys[GLFW_KEY_S]) { avatarPos -= avFwd   * spd; avatarMoving = true; }
			if (keys[GLFW_KEY_A]) { avatarPos -= avRight * spd; avatarMoving = true; }
			if (keys[GLFW_KEY_D]) { avatarPos += avRight * spd; avatarMoving = true; }
			if (avatarMoving) avatarWalkCycle += deltaTime * 4.0f; // velocidad del ciclo
			// Cámara más atrás para ver el modelo más grande
			glm::vec3 camPos = avatarPos - avFwd * 30.0f + glm::vec3(0.0f, 14.0f, 0.0f);
			camera.setPositionAndLookAt(camPos, avatarPos + glm::vec3(0.0f, 6.0f, 0.0f));
		}
		else if (cameraMode == 1) {
			aerialCamera.keyControlAerial(keys, 100 * deltaTime);
			mainWindow.getXChange(); mainWindow.getYChange();
		}
		else if (cameraMode == 2) {
			freeCamera.keyControl(keys, 100 * deltaTime);
			freeCamera.mouseControl(mainWindow.getXChange(), mainWindow.getYChange());
		}
		else {
			mainWindow.getXChange(); mainWindow.getYChange();
		}

		// Efecto de pasos: inicia/detiene según movimiento del avatar en modo 3ra persona
		if (cameraMode == 0) {
			if (avatarMoving && !footstepPlaying) {
				ma_sound_start(&soundFootstep);
				footstepPlaying = true;
			} else if (!avatarMoving && footstepPlaying) {
				ma_sound_stop(&soundFootstep);
				footstepPlaying = false;
			}
		} else if (footstepPlaying) {
			ma_sound_stop(&soundFootstep);
			footstepPlaying = false;
		}

		// Spotlight toggle
		static bool fKeyPrev = false;
		static bool spotlightOn = true;
		if (keys[GLFW_KEY_F] && !fKeyPrev) spotlightOn = !spotlightOn;
		fKeyPrev = keys[GLFW_KEY_F];

		// Ciclo día/noche — tecla 7 lo activa una vez
		static bool key7Prev = false;
		if (keys[GLFW_KEY_7] && !key7Prev) { cycleElapsed = 0.0f; cycleRunning = true; }
		key7Prev = keys[GLFW_KEY_7];
		if (cycleRunning) {
			cycleElapsed += deltaTime;
			if (cycleElapsed >= CYCLE_DURATION) { cycleElapsed = CYCLE_DURATION; cycleRunning = false; }
		}

		float cycleTime = cycleElapsed / CYCLE_DURATION;
		float sunAngle = cycleTime * 2.0f * 3.14159265f;
		float dayFactor = glm::max(0.0f, cosf(sunAngle));
		float sunDirX = 0.3f;
		float sunDirY = -cosf(sunAngle);
		float sunDirZ = sinf(sunAngle);

		glm::vec3 dayColor(1.00f, 0.95f, 0.80f);
		glm::vec3 dawnColor(1.00f, 0.45f, 0.10f);
		glm::vec3 nightColor(0.02f, 0.02f, 0.10f);
		glm::vec3 sunColor;
		if (dayFactor > 0.3f) sunColor = glm::mix(dawnColor, dayColor, (dayFactor - 0.3f) / 0.7f);
		else if (dayFactor > 0.0f) sunColor = glm::mix(nightColor, dawnColor, dayFactor / 0.3f);
		else                       sunColor = nightColor;

		mainLight = DirectionalLight(
			sunColor.r, sunColor.g, sunColor.b,
			0.05f + dayFactor * 0.35f,
			dayFactor * 0.8f,
			sunDirX, sunDirY, sunDirZ
		);
		// Las 4 lámparas siempre encendidas; de día aportan menos pero siguen activas
		pointLightCount = 4;
		glm::vec3 skyTintColor = glm::max(sunColor, glm::vec3(0.04f, 0.04f, 0.12f));
		skybox.SetTint(skyTintColor.r, skyTintColor.g, skyTintColor.b);

		// Limpiar buffers
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Skybox
		skybox.DrawSkybox(activeCamera->calculateViewMatrix(), projection);

		// Activar shader principal
		shaderList[0].UseShader();

		uniformModel = shaderList[0].GetModelLocation();
		uniformProjection = shaderList[0].GetProjectionLocation();
		uniformView = shaderList[0].GetViewLocation();
		uniformEyePosition = shaderList[0].GetEyePositionLocation();
		uniformColor = shaderList[0].getColorLocation();
		uniformAlpha = shaderList[0].getAlphaLocation();
		uniformNoLighting = shaderList[0].getNoLightingLocation();
		uniformSpecularIntensity = shaderList[0].GetSpecularIntensityLocation();
		uniformShininess = shaderList[0].GetShininessLocation();
		uniformUseTexture = shaderList[0].GetUniformLocation("useTexture");

		glUniform1f(uniformAlpha, 1.0f);
		glUniform1i(uniformNoLighting, 0);
		glUniform1i(uniformUseTexture, 1);

		glUniformMatrix4fv(uniformProjection, 1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix4fv(uniformView, 1, GL_FALSE, glm::value_ptr(activeCamera->calculateViewMatrix()));
		glUniform3f(uniformEyePosition,
			activeCamera->getCameraPosition().x,
			activeCamera->getCameraPosition().y,
			activeCamera->getCameraPosition().z);

		glm::vec3 lowerLight = activeCamera->getCameraPosition();
		lowerLight.y -= 0.3f;
		spotLights[0].SetFlash(lowerLight, activeCamera->getCameraDirection());

		shaderList[0].SetDirectionalLight(&mainLight);
		shaderList[0].SetPointLights(pointLights, pointLightCount);
		shaderList[0].SetSpotLights(spotLights, spotlightOn ? spotLightCount : 0);

		// ========================================================
		// DIBUJADO DE OBJETOS
		// ========================================================
		glm::mat4 model(1.0);
		glm::vec3 color(1.0f, 1.0f, 1.0f);

		// --- PISO ---
		model = glm::translate(glm::mat4(1.0), glm::vec3(0.0f, -1.0f, 0.0f));
		model = glm::scale(model, glm::vec3(30.0f, 1.0f, 30.0f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glUniform3fv(uniformColor, 1, glm::value_ptr(color));
		pisoTexture.UseTexture();
		Material_opaco.UseMaterial(uniformSpecularIntensity, uniformShininess);
		meshList[0]->RenderMesh();

		// ------------------------------------------------------------------ AQUI DEFINIMOS EL MUNDO ------------------------------------------

		// --- LÁMPARAS DE CALLE (10 instancias) ---
		glm::vec3 lampPositions[] = {
			glm::vec3(0.0f, -1.0f,  50.0f),
			glm::vec3(120.0f, -1.0f,  90.0f),
			glm::vec3(240.0f, -1.0f, 180.0f),
			glm::vec3(-110.0f, -1.0f,  85.0f),
			glm::vec3(-200.0f, -1.0f, 200.0f),
			glm::vec3(-150.0f, -1.0f,-100.0f),
			glm::vec3(-230.0f, -1.0f,-220.0f),
			glm::vec3(150.0f, -1.0f,-100.0f),
			glm::vec3(230.0f, -1.0f,-200.0f),
			glm::vec3(40.0f, -1.0f,-215.0f),
		};
		for (auto& lpos : lampPositions) {
			model = glm::mat4(1.0);
			model = glm::translate(model, lpos);
			model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f));
			glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
			glUniform3fv(uniformColor, 1, glm::value_ptr(color));
			Material_opaco.UseMaterial(uniformSpecularIntensity, uniformShininess);
			glDisable(GL_CULL_FACE);
			StreetLamp_M.RenderModel();
			glEnable(GL_CULL_FACE);
		}

		// --- LÁMPARA redstone ---
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(-20.0f, -1.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.05f, 0.05f, 0.05f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glUniform3fv(uniformColor, 1, glm::value_ptr(color));
		Material_opaco.UseMaterial(uniformSpecularIntensity, uniformShininess);
		lamp_model.RenderModel();

		// --- JOKER ESTÁTICO — centro de la escena, visible para todos ---
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(0.0f, 9.0f, 20.0f));
		model = glm::rotate(model, glm::radians(90.0f),  glm::vec3(1.0f, 0.0f, 0.0f));  // pararlo
		model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));  // cara al frente
		model = glm::scale(model, glm::vec3(0.01f, 0.01f, 0.01f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glUniform3fv(uniformColor, 1, glm::value_ptr(color));
		Material_opaco.UseMaterial(uniformSpecularIntensity, uniformShininess);
		glDisable(GL_CULL_FACE);
		Joker_M.RenderModel();
		glEnable(GL_CULL_FACE);

		// --- JOKER AVATAR (partes con animación de caminar) ---
		{
			float wc = avatarWalkCycle;
			// Animación de caminar: brazos y piernas oscilan al ritmo de los pasos
			// Si no se mueve, todo en 0 (pose neutral)
			float walkAmt   = avatarMoving ? 1.0f : 0.0f;
			float legSwing  = sinf(wc) * 25.0f * walkAmt;
			float armSwing  = -sinf(wc) * 20.0f * walkAmt; // contrario a piernas
			float bodyBob   = fabsf(sinf(wc * 2.0f)) * 0.3f * walkAmt;

			// Base del avatar: misma escala que el bailarín (0.1f)
			const float avScale = 0.06f;
			glm::vec3 avBase = avatarPos + glm::vec3(0.0f, bodyBob, 0.0f);
			float faceYawAv  = -avatarYaw + 90.0f;
			glm::vec3 noPivotAv(0,0,0);

			auto avatarPart = [&](glm::vec3 offset, float rotX, float rotY, float rotZ, Model& mdl) {
				model = glm::mat4(1.0f);
				model = glm::translate(model, avBase);
				model = glm::rotate(model, glm::radians(faceYawAv), glm::vec3(0,1,0));
				model = glm::translate(model, offset);
				if (rotX != 0) model = glm::rotate(model, glm::radians(rotX), glm::vec3(1,0,0));
				if (rotY != 0) model = glm::rotate(model, glm::radians(rotY), glm::vec3(0,1,0));
				if (rotZ != 0) model = glm::rotate(model, glm::radians(rotZ), glm::vec3(0,0,1));
				model = glm::scale(model, glm::vec3(avScale));
				glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
				glUniform3fv(uniformColor, 1, glm::value_ptr(color));
				Material_opaco.UseMaterial(uniformSpecularIntensity, uniformShininess);
				glDisable(GL_CULL_FACE); mdl.RenderModel(); glEnable(GL_CULL_FACE);
			};

			avatarPart(glm::vec3(0,    -4.2f, 0),  0,           0, 0, JokerAvatar_Cuerpo_M);
			avatarPart(glm::vec3(0,     4.8f, 0),  0,           0, 0, JokerAvatar_Cabeza_M);
			avatarPart(glm::vec3(-0.6f,  4.3f, 0),  armSwing,    0, 0, JokerAvatar_BrazoDer_M);
			avatarPart(glm::vec3( 0.6f,  4.3f, 0), -armSwing,    0, 0, JokerAvatar_BrazoIzq_M);
			avatarPart(glm::vec3( 0.5f,  1.2f, 0),  legSwing,    0, 0, JokerAvatar_PiernaDer_M);
			avatarPart(glm::vec3(-0.5f,  1.2f, 0), -legSwing,    0, 0, JokerAvatar_PiernaIzq_M);
		}

		// --- LADRONES FANTASMA ---
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

		// ================================================================
		// JOKER BAILARÍN — animación procedural por partes cerca del subway
		// Subway está en (210, -3.5, 90); Joker baila en (195, -1, 60)
		// ================================================================
		{
			float t = (float)glfwGetTime();

			// Ritmo base: 1.8 Hz (baile rápido tipo shuffle)
			float beat = t * 1.8f;
			// Bounce del cuerpo entero: sube/baja ligeramente
			float bounce = fabsf(sinf(beat * 2.0f)) * 1.2f;
			// Balanceo lateral del cuerpo
			float sway = sinf(beat) * 4.0f;  // grados de inclinación lateral

			// Posición base del Joker bailarín
			// Y elevada para que los pies queden al nivel del suelo (ajustar según modelo)
			glm::vec3 dancePos(195.0f, 8.0f + bounce, 60.0f);

			// Escala x10 respecto a versión anterior (0.01 → 0.1)
			float baseScale = 0.1f;
			// Mirando hacia el centro de la escena (~180°)
			float faceYaw = 180.0f;

			// Helper: construye la matrix base del Joker (traslación + orientación)
			// pivotOffset: punto de giro LOCAL (en unidades de modelo, antes de escalar)
			//   → trasladamos al pivot, rotamos, volvemos. Útil para hombros/caderas.
			auto danceModel = [&](glm::vec3 localOffset, float rotX, float rotY, float rotZ,
				glm::vec3 pivotOffset, Model& mdl) {
					model = glm::mat4(1.0f);
					// 1. Mover al punto de baile
					model = glm::translate(model, dancePos);
					// 2. Rotación global (facing + sway lateral)
					model = glm::rotate(model, glm::radians(faceYaw), glm::vec3(0, 1, 0));
					model = glm::rotate(model, glm::radians(sway), glm::vec3(0, 0, 1));
					// 3. Offset local de la parte en espacio mundo-local
					model = glm::translate(model, localOffset);
					// 4. Pivot: moverse al punto de giro, rotar, volver
					model = glm::translate(model, pivotOffset);
					if (rotX != 0) model = glm::rotate(model, glm::radians(rotX), glm::vec3(1, 0, 0));
					if (rotY != 0) model = glm::rotate(model, glm::radians(rotY), glm::vec3(0, 1, 0));
					if (rotZ != 0) model = glm::rotate(model, glm::radians(rotZ), glm::vec3(0, 0, 1));
					model = glm::translate(model, -pivotOffset);
					// 5. Escala
					model = glm::scale(model, glm::vec3(baseScale, baseScale, baseScale));
					glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
					glUniform3fv(uniformColor, 1, glm::value_ptr(color));
					Material_opaco.UseMaterial(uniformSpecularIntensity, uniformShininess);
					glDisable(GL_CULL_FACE);
					mdl.RenderModel();
					glEnable(GL_CULL_FACE);
				};

			// Animaciones por parte:
			// Brazos: reducidos a ±20° para evitar desganche del hombro
			float armSwingDer = sinf(beat) * 20.0f;
			float armSwingIzq = -sinf(beat) * 20.0f;
			float armRaiseDer = cosf(beat * 0.5f) * 10.0f;
			float armRaiseIzq = -cosf(beat * 0.5f) * 10.0f;

			// Piernas: alternadas, más suaves
			float legDer = sinf(beat) * 25.0f;
			float legIzq = -sinf(beat) * 25.0f;

			// Cabeza: movimiento más sutil
			float headNod = sinf(beat * 2.0f) * 3.0f;
			float headTurn = sinf(beat * 0.7f) * 5.0f;

			glm::vec3 noPivot(0, 0, 0);

			// CUERPO — más abajo
			danceModel(glm::vec3(0, -7.0f, 0), 0, 0, 0, noPivot, JokerDance_Cuerpo_M);

			// CABEZA
			danceModel(glm::vec3(0, 8.0f, 0), headNod, headTurn, 0, noPivot, JokerDance_Cabeza_M);

			// BRAZO DERECHO
			danceModel(glm::vec3(-1.0f, 7.2f, 0), armSwingDer, 0, armRaiseDer, noPivot, JokerDance_BrazoDer_M);

			// BRAZO IZQUIERDO
			danceModel(glm::vec3(1.0f, 7.2f, 0), armSwingIzq, 0, armRaiseIzq, noPivot, JokerDance_BrazoIzq_M);

			// PIERNA DERECHA — separada a la derecha
			danceModel(glm::vec3(0.8f, 2.0f, 0), legDer, 0, 0, noPivot, JokerDance_PiernaDer_M);

			// PIERNA IZQUIERDA — separada a la izquierda
			danceModel(glm::vec3(-0.8f, 2.0f, 0), legIzq, 0, 0, noPivot, JokerDance_PiernaIzq_M);
		}

		// --- BATMAN RIGGED ---
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(0.0f, -1.0f, -25.0f));
		model = glm::scale(model, glm::vec3(5.0f, 5.0f, 5.0f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glUniform3fv(uniformColor, 1, glm::value_ptr(color));
		Material_opaco.UseMaterial(uniformSpecularIntensity, uniformShininess);
		glDisable(GL_CULL_FACE);
		BatmanRigged_M.RenderModel();
		glEnable(GL_CULL_FACE);

		// --- CATWOMAN ---
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(115.0f, -1.0f, -25.0f));
		model = glm::scale(model, glm::vec3(0.05625f, 0.05625f, 0.05625f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glUniform3fv(uniformColor, 1, glm::value_ptr(color));
		Material_opaco.UseMaterial(uniformSpecularIntensity, uniformShininess);
		glDisable(GL_CULL_FACE);
		Catwoman_M.RenderModel();
		glEnable(GL_CULL_FACE);

		// --- ROBIN ---
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(5.0f, -1.0f, -15.0f));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(5.0f, 5.0f, 5.0f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glUniform3fv(uniformColor, 1, glm::value_ptr(color));
		Material_opaco.UseMaterial(uniformSpecularIntensity, uniformShininess);
		glDisable(GL_CULL_FACE);
		Robin_M.RenderModel();
		glEnable(GL_CULL_FACE);

		// --- BATWING (animado: va y viene) ---
		engranajeAngle += 0.8f * deltaTime;  // engranajes giran siempre
		nave_j += deltaTime;
		if (nave_j < 4.0f)      nave_pos += 30.0f * deltaTime;
		else if (nave_j < 8.0f) nave_pos -= 30.0f * deltaTime;
		else { nave_j = 0.0f; }
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(-50.0f, 25.0f, 160.0f + nave_pos));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		if (nave_j >= 4.0f) model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(0.25f, 0.25f, 0.25f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glUniform3fv(uniformColor, 1, glm::value_ptr(color));
		Material_opaco.UseMaterial(uniformSpecularIntensity, uniformShininess);
		glDisable(GL_CULL_FACE);
		Batwing_M.RenderModel();
		glEnable(GL_CULL_FACE);

		// --- GLOBE (base + bola giratoria) ---
		globe_i += deltaTime;
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(20.0f, -1.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.3f, 0.3f, 0.3f));
		model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glDisable(GL_CULL_FACE);
		Globe_M.RenderModel();
		glEnable(GL_CULL_FACE);
		model = glm::translate(model, glm::vec3(0.0f, -16.5f, 0.0f));
		model = glm::rotate(model, glm::radians(360.0f * sinf(globe_i)), glm::vec3(0.0f, 1.0f, 0.0f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glDisable(GL_CULL_FACE);
		Globe_Ball_M.RenderModel();
		glEnable(GL_CULL_FACE);

		// --- ROBOT (sin textura, material metálico) ---
		glUniform1i(uniformUseTexture, 0);
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(-101.0f, 1.0f, -45.0f));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(98.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glm::vec3 colorMetalico(0.55f, 0.60f, 0.65f);
		glUniform3fv(uniformColor, 1, glm::value_ptr(colorMetalico));
		Material_metalico.UseMaterial(uniformSpecularIntensity, uniformShininess);
		glDisable(GL_CULL_FACE);
		Robot_M.RenderModel();
		glEnable(GL_CULL_FACE);
		glUniform1i(uniformUseTexture, 1);
		glUniform3fv(uniformColor, 1, glm::value_ptr(color));

		// --- CENTRAL BUILDING ---
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(0.0f, -1.0f, -30.0f));
		model = glm::scale(model, glm::vec3(7.5f, 7.5f, 7.5f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glUniform3fv(uniformColor, 1, glm::value_ptr(color));
		Material_opaco.UseMaterial(uniformSpecularIntensity, uniformShininess);
		glDisable(GL_CULL_FACE);
		CentralBuilding_M.RenderModel();
		glEnable(GL_CULL_FACE);

		// --- STEAMPUNK HOUSES ---
		// periferia derecha delantera
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(220.0f, -1.0f, 70.0f));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(25.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(19.5f, 19.5f, 19.5f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glDisable(GL_CULL_FACE); SteampunkHouse_M.RenderModel(); glEnable(GL_CULL_FACE);

		// periferia derecha trasera
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(250.0f, -1.0f, -170.0f));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(-35.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(19.5f, 19.5f, 19.5f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glDisable(GL_CULL_FACE); SteampunkHouse_M.RenderModel(); glEnable(GL_CULL_FACE);

		// periferia izquierda delantera (House2)
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(-230.0f, 7.0f, 50.0f));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(-20.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(15.0f, 15.0f, 15.0f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glDisable(GL_CULL_FACE); SteampunkHouse2_M.RenderModel(); glEnable(GL_CULL_FACE);

		// periferia izquierda trasera (House2)
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(-260.0f, 7.0f, -160.0f));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(40.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(15.0f, 15.0f, 15.0f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glDisable(GL_CULL_FACE); SteampunkHouse2_M.RenderModel(); glEnable(GL_CULL_FACE);

		// frente derecha
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(130.0f, -1.0f, 230.0f));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(55.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(19.5f, 19.5f, 19.5f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glDisable(GL_CULL_FACE); SteampunkHouse_M.RenderModel(); glEnable(GL_CULL_FACE);

		// atrás centro
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(50.0f, -1.0f, -245.0f));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(-15.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(19.5f, 19.5f, 19.5f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glDisable(GL_CULL_FACE); SteampunkHouse_M.RenderModel(); glEnable(GL_CULL_FACE);

		// frente izquierda (House2)
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(-130.0f, 7.0f, 225.0f));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(-50.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(15.0f, 15.0f, 15.0f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glDisable(GL_CULL_FACE); SteampunkHouse2_M.RenderModel(); glEnable(GL_CULL_FACE);

		// atrás izquierda (House2)
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(-240.0f, 7.0f, -230.0f));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(20.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(15.0f, 15.0f, 15.0f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glDisable(GL_CULL_FACE); SteampunkHouse2_M.RenderModel(); glEnable(GL_CULL_FACE);

		// extra 1
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(-100.0f, -1.0f, -200.0f));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(10.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(19.5f, 19.5f, 19.5f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glDisable(GL_CULL_FACE); SteampunkHouse_M.RenderModel(); glEnable(GL_CULL_FACE);

		// extra 2
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(200.0f, -1.0f, -80.0f));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(-20.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(19.5f, 19.5f, 19.5f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glDisable(GL_CULL_FACE); SteampunkHouse_M.RenderModel(); glEnable(GL_CULL_FACE);

		// extra 3
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(-180.0f, -1.0f, 100.0f));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(30.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(19.5f, 19.5f, 19.5f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glDisable(GL_CULL_FACE); SteampunkHouse_M.RenderModel(); glEnable(GL_CULL_FACE);

		// House2 extra 1
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(150.0f, 7.0f, -210.0f));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(15.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(15.0f, 15.0f, 15.0f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glDisable(GL_CULL_FACE); SteampunkHouse2_M.RenderModel(); glEnable(GL_CULL_FACE);

		// House2 extra 2
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(-160.0f, 7.0f, 170.0f));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(-35.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(15.0f, 15.0f, 15.0f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glDisable(GL_CULL_FACE); SteampunkHouse2_M.RenderModel(); glEnable(GL_CULL_FACE);

		// House2 extra 3
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(60.0f, 7.0f, 260.0f));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(-45.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(15.0f, 15.0f, 15.0f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glDisable(GL_CULL_FACE); SteampunkHouse2_M.RenderModel(); glEnable(GL_CULL_FACE);

		// --- BAZAAR ---
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(80.0f, -1.0f, 40.0f));
		model = glm::rotate(model, glm::radians(15.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.095f, 0.095f, 0.095f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glDisable(GL_CULL_FACE); BazaarSteampunk_M.RenderModel(); glEnable(GL_CULL_FACE);

		// --- TIME PORTALS ---
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(-80.0f, -1.0f, 60.0f));
		model = glm::rotate(model, glm::radians(-30.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::scale(model, glm::vec3(4.5f, -4.5f, 4.5f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glDisable(GL_CULL_FACE); TimePortalBase_M.RenderModel(); glEnable(GL_CULL_FACE);
		model = glm::translate(model, glm::vec3(0.0f, 2.0f, 0.0f));
		//model = glm::rotate(model, glm::radians(-30.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glDisable(GL_CULL_FACE); TimePortalDisk_M.RenderModel(); glEnable(GL_CULL_FACE);

		// --- TIME PORTALS ---
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(-200.0f, -1.0f, -160.0f));
		model = glm::rotate(model, glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::scale(model, glm::vec3(4.5f, -4.5f, 4.5f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glDisable(GL_CULL_FACE); TimePortal_M.RenderModel(); glEnable(GL_CULL_FACE);

		// --- STEAMPUNK PROP ---
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(0.0f, -1.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.015f, 0.015f, 0.015f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glDisable(GL_CULL_FACE); SteampunkProp_M.RenderModel(); glEnable(GL_CULL_FACE);

		// --- POST OFFICE ---
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(100.0f, -1.0f, -60.0f));
		model = glm::scale(model, glm::vec3(7.5f, 7.5f, 7.5f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glDisable(GL_CULL_FACE); SteampunkPostOffice_M.RenderModel(); glEnable(GL_CULL_FACE);

		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(100.0f, -1.0f, -60.0f));
		model = glm::scale(model, glm::vec3(7.5f, 7.5f, -7.5f)); // espejo Z
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glDisable(GL_CULL_FACE); SteampunkPostOffice_M.RenderModel(); glEnable(GL_CULL_FACE);

		// --- SUBWAY ---
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(210.0f, -3.5f, 90.0f));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(30.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(5.0f, 5.0f, 5.0f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glDisable(GL_CULL_FACE); Subway_M.RenderModel(); glEnable(GL_CULL_FACE);

		// --- SHOP ---
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(80.0f, -1.0f, -60.0f));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.45f, 0.45f, 0.45f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glDisable(GL_CULL_FACE); Shop_M.RenderModel(); glEnable(GL_CULL_FACE);

		// --- POTION SHOP ---
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(-140.0f, -1.0f, 30.0f));
		model = glm::scale(model, glm::vec3(0.17f, 0.17f, 0.17f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glDisable(GL_CULL_FACE); PotionShop_M.RenderModel(); glEnable(GL_CULL_FACE);
	
		// --- Rei ---
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(20.0f, 0.0f, 100.0f));
		model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.2f, 0.2f, 0.2f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glDisable(GL_CULL_FACE); Rei_M.RenderModel(); glEnable(GL_CULL_FACE);

		// --- Asuka ---
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(-20.0f, 0.0f, 100.0f));
		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.04f, 0.04f, 0.04f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glDisable(GL_CULL_FACE); Asuka_M.RenderModel(); glEnable(GL_CULL_FACE);

		// --- Unit01 ---
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(0.0f, 0.0f, 100.0f));
		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.1f, 0.1f, 0.1f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glDisable(GL_CULL_FACE); Unit01_M.RenderModel(); glEnable(GL_CULL_FACE);

		// --- STEAMPUNK LAMPS (4 instancias distribuidas por el mapa) ---
		{
			glm::vec3 lampSteamPos[] = {
				glm::vec3(  60.0f, -1.0f,  80.0f),
				glm::vec3(  20.0f, -1.0f, -80.0f),  // alejada del proyector
				glm::vec3( 150.0f, -1.0f, -50.0f),
				glm::vec3(-150.0f, -1.0f, 120.0f),
			};
			for (auto& lp : lampSteamPos) {
				model = glm::mat4(1.0);
				model = glm::translate(model, lp);
				model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
				model = glm::scale(model, glm::vec3(12.0f, 12.0f, 12.0f));
				glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
				glUniform3fv(uniformColor, 1, glm::value_ptr(color));
				Material_opaco.UseMaterial(uniformSpecularIntensity, uniformShininess);
				glDisable(GL_CULL_FACE); SteampunkLamp_M.RenderModel(); glEnable(GL_CULL_FACE);
			}
		}

		// --- GALERÍA DE BUSTOS ---
		// Pilar 1 + Cervantes (X=30)
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(30.0f, -1.0f, -200.0f));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(1.5f, 1.5f, 1.5f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glDisable(GL_CULL_FACE); Pilar_M.RenderModel(); glEnable(GL_CULL_FACE);

		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(30.0f, 10.0f, -193.0f));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(1.2f, 1.2f, 1.2f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glDisable(GL_CULL_FACE); Cervantes_M.RenderModel(); glEnable(GL_CULL_FACE);

		// Pilar 2 + Poe (X=38)
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(38.0f, -1.0f, -200.0f));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(1.5f, 1.5f, 1.5f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glDisable(GL_CULL_FACE); Pilar_M.RenderModel(); glEnable(GL_CULL_FACE);

		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(38.0f, 10.0f, -200.0f));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::scale(model, glm::vec3(4.0f, 4.0f, 4.0f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glDisable(GL_CULL_FACE); Poe_M.RenderModel(); glEnable(GL_CULL_FACE);

		// Pilar 3 + Shakespeare (X=46)
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(46.0f, -1.0f, -200.0f));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(1.5f, 1.5f, 1.5f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glDisable(GL_CULL_FACE); Pilar_M.RenderModel(); glEnable(GL_CULL_FACE);

		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(46.0f, 7.0f, -200.0f));
		model = glm::scale(model, glm::vec3(12.0f, 12.0f, 12.0f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glDisable(GL_CULL_FACE); Shakespeare_M.RenderModel(); glEnable(GL_CULL_FACE);

		// Pilares vacíos 4, 5, 6
		for (int px = 54; px <= 70; px += 8) {
			model = glm::mat4(1.0);
			model = glm::translate(model, glm::vec3((float)px, -1.0f, -200.0f));
			model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
			model = glm::scale(model, glm::vec3(1.5f, 1.5f, 1.5f));
			glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
			glDisable(GL_CULL_FACE); Pilar_M.RenderModel(); glEnable(GL_CULL_FACE);
		}

		// ================================================================
		// PROYECTOR STEAMPUNK — proyector fijo + engranajes girando
		// ================================================================
		{
			const glm::vec3 proyPos(-50.0f, 4.0f, -80.0f);
			const float     proyScale = 7.0f;
			const glm::vec3 gAxis(0.0f, 0.0f, 1.0f);

			// Sin .mtl: desactivar textura y usar color cobre oscuro directamente
			glUniform1i(uniformUseTexture, 0);

			// Cuerpo del proyector (estático) — color bronce oscuro
			color = glm::vec3(0.55f, 0.35f, 0.15f);
			glUniform3fv(uniformColor, 1, glm::value_ptr(color));
			model = glm::mat4(1.0f);
			model = glm::translate(model, proyPos);
			model = glm::scale(model, glm::vec3(proyScale));
			glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
			Material_opaco.UseMaterial(uniformSpecularIntensity, uniformShininess);
			glDisable(GL_CULL_FACE); ProyectorSteampunk_M.RenderModel(); glEnable(GL_CULL_FACE);

			// Engranajes — color cobre brillante
			color = glm::vec3(0.8f, 0.55f, 0.2f);
			glUniform3fv(uniformColor, 1, glm::value_ptr(color));

			struct GearEntry { Model* m; glm::vec3 c; float dir; };
			GearEntry gears[] = {
				{ &Engranaje1_M,          glm::vec3(-0.2056f,  0.1182f,  0.0577f),  1.0f },
				{ &Engranaje2_M,          glm::vec3(-0.1093f,  0.1942f,  0.0576f), -1.0f },
				{ &Engranaje3_M,          glm::vec3(-0.2404f,  0.2781f,  0.0600f),  1.0f },
				{ &EngranajeAdicional1_M, glm::vec3(-0.3042f,  0.1453f,  0.0546f), -1.0f },
				{ &EngranajeAdicional2_M, glm::vec3(-0.1319f,  0.1865f, -0.1731f),  1.0f },
				{ &EngranajeAtras1_M,     glm::vec3(-0.2475f,  0.2781f, -0.0312f), -1.0f },
				{ &EngranajeAtras2_M,     glm::vec3(-0.3565f,  0.2222f, -0.0363f),  1.0f },
				{ &EngranajeProyector1_M, glm::vec3(-0.6199f,  0.5783f,  0.0615f), -1.0f },
				{ &EngranajeProyector2_M, glm::vec3(-0.5595f,  0.0187f,  0.0530f),  1.0f },
			};
			for (auto& g : gears) {
				glm::vec3 cS = g.c * proyScale;
				model = glm::mat4(1.0f);
				model = glm::translate(model, proyPos);
				model = glm::translate(model,  cS);
				model = glm::rotate(model, engranajeAngle * g.dir, gAxis);
				model = glm::translate(model, -cS);
				model = glm::scale(model, glm::vec3(proyScale));
				glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
				glDisable(GL_CULL_FACE); g.m->RenderModel(); glEnable(GL_CULL_FACE);
			}
			// Restaurar textura para el resto de objetos
			glUniform1i(uniformUseTexture, 1);
			color = glm::vec3(1.0f, 1.0f, 1.0f);
			glUniform3fv(uniformColor, 1, glm::value_ptr(color));
		}

		// ================================================================
		// TREN — animación y render
		// ================================================================
		{
			const float cornerSize  = 80.0f;
			const float straightEnd = TRACK_R - cornerSize;

			trainT += TRAIN_SPEED * deltaTime;
			if (trainT >= PERIMETER) trainT -= PERIMETER;
			wheelAngle += TRAIN_SPEED * deltaTime * 5.0f;  // ruedas giran proporcional a velocidad
			barsAngle += TRAIN_SPEED * deltaTime * 3.0f;

			auto getCircuitPos = [&](float dist) -> glm::vec3 {
				float d = fmodf(dist, PERIMETER);
				if (d < 0) d += PERIMETER;
				float straightLen = 2.0f * straightEnd;
				float curveLen    = (3.14159265f * 0.5f) * cornerSize;

				// Seg 0 recto (+Z)
				if (d < straightLen) {
					float frac = d / straightLen;
					return glm::vec3(TRACK_R, -1.0f, -straightEnd + frac * straightLen);
				}
				d -= straightLen;
				// Curva NE
				if (d < curveLen) {
					float a = glm::radians(d / curveLen * 90.0f);
					return glm::vec3(straightEnd + cornerSize * cosf(a), -1.0f, straightEnd + cornerSize * sinf(a));
				}
				d -= curveLen;
				// Seg 1 recto (-X)
				if (d < straightLen) {
					float frac = d / straightLen;
					return glm::vec3(straightEnd - frac * straightLen, -1.0f, TRACK_R);
				}
				d -= straightLen;
				// Curva NW
				if (d < curveLen) {
					float a = glm::radians(90.0f + d / curveLen * 90.0f);
					return glm::vec3(-straightEnd + cornerSize * cosf(a), -1.0f, straightEnd + cornerSize * sinf(a));
				}
				d -= curveLen;
				// Seg 2 recto (-Z)
				if (d < straightLen) {
					float frac = d / straightLen;
					return glm::vec3(-TRACK_R, -1.0f, straightEnd - frac * straightLen);
				}
				d -= straightLen;
				// Curva SW
				if (d < curveLen) {
					float a = glm::radians(180.0f + d / curveLen * 90.0f);
					return glm::vec3(-straightEnd + cornerSize * cosf(a), -1.0f, -straightEnd + cornerSize * sinf(a));
				}
				d -= curveLen;
				// Seg 3 recto (+X)
				if (d < straightLen) {
					float frac = d / straightLen;
					return glm::vec3(-straightEnd + frac * straightLen, -1.0f, -TRACK_R);
				}
				d -= straightLen;
				// Curva SE
				if (d < curveLen) {
					float a = glm::radians(270.0f + d / curveLen * 90.0f);
					return glm::vec3(straightEnd + cornerSize * cosf(a), -1.0f, -straightEnd + cornerSize * sinf(a));
				}
				return glm::vec3(TRACK_R, -1.0f, -straightEnd);
			};

			glm::vec3 trainPos  = getCircuitPos(trainT);
			glm::vec3 trainPos2 = getCircuitPos(trainT + 0.5f);
			glm::vec3 trainDir  = glm::normalize(trainPos2 - trainPos);
			float modelYaw      = glm::degrees(atan2f(trainDir.x, trainDir.z)) + 270.0f;
			glm::vec3 trainFront = trainPos + trainDir * 15.0f;

			// Cuerpo
			model = glm::mat4(1.0f);
			model = glm::translate(model, trainPos);
			model = glm::rotate(model, glm::radians(modelYaw), glm::vec3(0.0f, 1.0f, 0.0f));
			model = glm::scale(model, glm::vec3(20.0f, 20.0f, 20.0f));
			glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
			glDisable(GL_CULL_FACE); Train_M.RenderModel(); glEnable(GL_CULL_FACE);

			// Ruedas (estáticas — sin animación de giro)
			glm::vec3 wheelsPos = trainPos + trainDir * 18.0f + glm::vec3(0.0f, 3.0f, 0.0f);
			model = glm::mat4(1.0f);
			model = glm::translate(model, wheelsPos);
			model = glm::rotate(model, glm::radians(modelYaw), glm::vec3(0.0f, 1.0f, 0.0f));
			model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
			model = glm::rotate(model, glm::radians(-10.0f), glm::vec3(0.0f, 0.0f, 1.0f));
			model = glm::scale(model, glm::vec3(20.0f, 20.0f, 20.0f));
			glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
			glDisable(GL_CULL_FACE); TrainWheels_M.RenderModel(); glEnable(GL_CULL_FACE);

			// Barras (óvalo)
			float barsRad = glm::radians(barsAngle);
			glm::vec3 localRight    = glm::normalize(glm::cross(trainDir, glm::vec3(0,1,0)));
			float barsOffsetSide    = cosf(barsRad) * 1.2f;
			float barsOffsetUp      = sinf(barsRad) * 0.3f;
			glm::vec3 barsPos = trainPos + localRight * barsOffsetSide + glm::vec3(0.0f, 8.0f + barsOffsetUp, 0.0f);
			model = glm::mat4(1.0f);
			model = glm::translate(model, barsPos);
			model = glm::rotate(model, glm::radians(modelYaw), glm::vec3(0.0f, 1.0f, 0.0f));
			model = glm::scale(model, glm::vec3(20.0f, 20.0f, 20.0f));
			glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
			glDisable(GL_CULL_FACE); TrainBars_M.RenderModel(); glEnable(GL_CULL_FACE);

			// Humo billboard
			smokeTimer += deltaTime;
			if (smokeTimer >= 0.6f) {
				smokeTimer = 0.0f;
				for (int i = 0; i < MAX_SMOKE; i++) {
					if (smoke[i].life >= 1.0f) {
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
			glUniform1i(uniformNoLighting, 1);
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
				GLfloat sv[] = {
					v0.x,v0.y,v0.z,0,0, 0,1,0,
					v1.x,v1.y,v1.z,1,0, 0,1,0,
					v2.x,v2.y,v2.z,1,1, 0,1,0,
					v3.x,v3.y,v3.z,0,1, 0,1,0
				};
				unsigned int si[] = {0,1,2, 0,2,3};
				Mesh* sq = new Mesh();
				sq->CreateMesh(sv, si, 32, 6);
				glUniform1f(uniformAlpha, smoke[i].alpha);
				glUniform3fv(uniformColor, 1, glm::value_ptr(color));
				model = glm::mat4(1.0f);
				glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
				sq->RenderMesh();
				delete sq;
			}
			glUniform1f(uniformAlpha, 1.0f);
			glUniform1i(uniformNoLighting, 0);
			glDepthMask(GL_TRUE);
			glDisable(GL_BLEND);
		}

		// ================================================================
		// TRAIN TRACKS
		// ================================================================
		{
			glUniform1i(uniformNoLighting, 1);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glDepthMask(GL_FALSE);
			trackTexture.UseTexture();
			glUniform3fv(uniformColor, 1, glm::value_ptr(color));
			const float tileSize    = 30.0f;
			const float r           = TRACK_R;
			const float cornerSize  = 90.0f;
			const float straightEnd = r - cornerSize;

			auto drawTile = [&](float tx, float tz, float yawDeg) {
				model = glm::mat4(1.0f);
				model = glm::translate(model, glm::vec3(tx, -0.99f, tz));
				model = glm::rotate(model, glm::radians(yawDeg), glm::vec3(0.0f,1.0f,0.0f));
				model = glm::scale(model, glm::vec3(tileSize, 1.0f, tileSize));
				glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
				meshList[1]->RenderMesh();
			};

			float halfTile = tileSize * 0.5f;
			for (float z = -straightEnd; z < straightEnd; z += tileSize)
				drawTile( r, z + halfTile, 90.0f);
			for (float z = -straightEnd; z < straightEnd; z += tileSize)
				drawTile(-r, z + halfTile, 90.0f);
			for (float x = -straightEnd; x < straightEnd; x += tileSize)
				drawTile(x + halfTile,  r, 0.0f);
			for (float x = -straightEnd; x < straightEnd; x += tileSize)
				drawTile(x + halfTile, -r, 0.0f);

			const int   CURVE_STEPS = 12;
			const float eps         = 0.01f;
			// NE
			for (int s = 0; s < CURVE_STEPS; s++) {
				float a   = glm::radians((s + 0.5f) / CURVE_STEPS * 90.0f);
				float cx  =  straightEnd + cornerSize * cosf(a);
				float cz  =  straightEnd + cornerSize * sinf(a);
				float cx2 =  straightEnd + cornerSize * cosf(a + eps);
				float cz2 =  straightEnd + cornerSize * sinf(a + eps);
				drawTile(cx, cz, glm::degrees(atan2f(cx2-cx, cz2-cz)) + 90.0f);
			}
			// NW
			for (int s = 0; s < CURVE_STEPS; s++) {
				float a   = glm::radians(90.0f + (s + 0.5f) / CURVE_STEPS * 90.0f);
				float cx  = -straightEnd + cornerSize * cosf(a);
				float cz  =  straightEnd + cornerSize * sinf(a);
				float cx2 = -straightEnd + cornerSize * cosf(a + eps);
				float cz2 =  straightEnd + cornerSize * sinf(a + eps);
				drawTile(cx, cz, glm::degrees(atan2f(cx2-cx, cz2-cz)) + 90.0f);
			}
			// SW
			for (int s = 0; s < CURVE_STEPS; s++) {
				float a   = glm::radians(180.0f + (s + 0.5f) / CURVE_STEPS * 90.0f);
				float cx  = -straightEnd + cornerSize * cosf(a);
				float cz  = -straightEnd + cornerSize * sinf(a);
				float cx2 = -straightEnd + cornerSize * cosf(a + eps);
				float cz2 = -straightEnd + cornerSize * sinf(a + eps);
				drawTile(cx, cz, glm::degrees(atan2f(cx2-cx, cz2-cz)) + 90.0f);
			}
			// SE
			for (int s = 0; s < CURVE_STEPS; s++) {
				float a   = glm::radians(270.0f + (s + 0.5f) / CURVE_STEPS * 90.0f);
				float cx  =  straightEnd + cornerSize * cosf(a);
				float cz  = -straightEnd + cornerSize * sinf(a);
				float cx2 =  straightEnd + cornerSize * cosf(a + eps);
				float cz2 = -straightEnd + cornerSize * sinf(a + eps);
				drawTile(cx, cz, glm::degrees(atan2f(cx2-cx, cz2-cz)) + 90.0f);
			}
			glDepthMask(GL_TRUE);
			glDisable(GL_BLEND);
			glUniform1i(uniformNoLighting, 0);
		}

		glUseProgram(0);
		mainWindow.swapBuffers();
	}

	ma_sound_uninit(&soundFootstep);
	ma_sound_uninit(&soundAmbient);
	ma_sound_uninit(&soundSoundtrack);
	ma_engine_uninit(&audioEngine);

	return 0;
}
