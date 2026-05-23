/*
 * ================================================================
 * Proyecto de Computación Gráfica e Interacción Humano Computadora
 * ================================================================
 */

 // Macro necesaria para que stb_image.h genere la implementación
 // de sus funciones (solo debe definirse en UN archivo .cpp
#define STB_IMAGE_IMPLEMENTATION

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

// Avatar (Batman animado)
glm::vec3 avatarPos(0.0f, -1.0f, 0.0f);
float     avatarYaw = -90.0f;
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
float avatarWalkCycle = 0.0f;
bool  avatarMoving    = false;

// NPC autónomo — mismo modelo que el avatar, camina de lado a lado
float npcOffset    =  0.0f;
float npcWalkCycle =  0.0f;
bool  npcGoingPos  =  true;

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
Model TitanJoker_M;
Model Batimoto_M;
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
bool  cycleRunning = true;

// Luces
DirectionalLight mainLight;
PointLight pointLights[MAX_POINT_LIGHTS];
SpotLight  spotLights[MAX_SPOT_LIGHTS];

static const char* vShader = "shaders/shader_light.vert";
static const char* fShader = "shaders/shader_light.frag";

// ============================================================
// CreateObjects
// ============================================================
void CreateObjects()
{
	unsigned int floorIndices[] = { 0, 2, 1, 1, 2, 3 };

	GLfloat floorVertices[] = {
		-10.0f, 0.0f, -10.0f,   0.0f,  0.0f,  0.0f, 1.0f, 0.0f,
		 10.0f, 0.0f, -10.0f,  20.0f,  0.0f,  0.0f, 1.0f, 0.0f,
		-10.0f, 0.0f,  10.0f,   0.0f, 20.0f,  0.0f, 1.0f, 0.0f,
		 10.0f, 0.0f,  10.0f,  20.0f, 20.0f,  0.0f, 1.0f, 0.0f
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
	BatmanAnim.LoadModel("Models/batmanRigged.glb");
	Catwoman_M = Model(); Catwoman_M.LoadModel("Models/catwoman.glb");
	Robin_M = Model(); Robin_M.LoadModel("Models/robin.glb");
	Batwing_M = Model(); Batwing_M.LoadModel("Models/batwing.glb");
	StreetLamp_M = Model(); StreetLamp_M.LoadModel("Models/street_lamp.glb");
	Robot_M = Model(); Robot_M.LoadModel("Models/robot.obj");
	TitanJoker_M = Model(); TitanJoker_M.LoadModel("Models/titanJoker.glb");
	Batimoto_M = Model(); Batimoto_M.LoadModel("Models/batimoto.glb");

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
	Asuka_M = Model(); Asuka_M.LoadModel("Models/Evangelion/asuka.glb");
	Unit01_M = Model(); Unit01_M.LoadModel("Models/Evangelion/unit01.glb");


	for (int i = 0; i < MAX_SMOKE; i++) smoke[i].life = 1.0f;

	// --- 6. SKYBOX ---
	std::vector<std::string> skyboxFaces;
	skyboxFaces.push_back("Textures/new_Skybox/miramar_rt.tga");
	skyboxFaces.push_back("Textures/new_Skybox/miramar_lf.tga");
	skyboxFaces.push_back("Textures/new_Skybox/miramar_dn.tga");
	skyboxFaces.push_back("Textures/new_Skybox/miramar_up_fixed.tga");
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
	// StreetLamp_M — 10 instancias, cabeza local y≈39 x≈-4 → offset world (-4, +39, 0)
	pointLights[ 4] = PointLight(1.0f, 0.85f, 0.5f, 1.0f, 10.0f,   -4.0f, 38.0f,   50.0f, 1.0f, 0.035f, 0.003f);
	pointLights[ 5] = PointLight(1.0f, 0.85f, 0.5f, 1.0f, 10.0f,  116.0f, 38.0f,   90.0f, 1.0f, 0.035f, 0.003f);
	pointLights[ 6] = PointLight(1.0f, 0.85f, 0.5f, 1.0f, 10.0f,  236.0f, 38.0f,  180.0f, 1.0f, 0.035f, 0.003f);
	pointLights[ 7] = PointLight(1.0f, 0.85f, 0.5f, 1.0f, 10.0f, -114.0f, 38.0f,   85.0f, 1.0f, 0.035f, 0.003f);
	pointLights[ 8] = PointLight(1.0f, 0.85f, 0.5f, 1.0f, 10.0f, -204.0f, 38.0f,  200.0f, 1.0f, 0.035f, 0.003f);
	pointLights[ 9] = PointLight(1.0f, 0.85f, 0.5f, 1.0f, 10.0f, -154.0f, 38.0f, -100.0f, 1.0f, 0.035f, 0.003f);
	pointLights[10] = PointLight(1.0f, 0.85f, 0.5f, 1.0f, 10.0f, -234.0f, 38.0f, -220.0f, 1.0f, 0.035f, 0.003f);
	pointLights[11] = PointLight(1.0f, 0.85f, 0.5f, 1.0f, 10.0f,  146.0f, 38.0f, -100.0f, 1.0f, 0.035f, 0.003f);
	pointLights[12] = PointLight(1.0f, 0.85f, 0.5f, 1.0f, 10.0f,  226.0f, 38.0f, -200.0f, 1.0f, 0.035f, 0.003f);
	pointLights[13] = PointLight(1.0f, 0.85f, 0.5f, 1.0f, 10.0f,   36.0f, 38.0f, -215.0f, 1.0f, 0.035f, 0.003f);
	pointLightCount = 14;

	unsigned int spotLightCount = 0;
	spotLights[0] = SpotLight(
		1.0f, 1.0f, 1.0f,
		0.0f, 3.0f,
		0.0f, 0.0f, 0.0f,
		0.0f, -1.0f, 0.0f,
		1.0f, 0.005f, 0.0002f,
		20.0f
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

	// Faro delantero Batwing — amarillo, se activa cuando avanza (+Z, nave_j < 4)
	spotLights[2] = SpotLight(
		1.0f, 0.9f, 0.1f,           // color: amarillo
		0.0f, 6.0f,
		-50.0f, 23.0f, 166.0f,      // posición inicial (actualizada cada frame)
		0.0f, 1.0f, 0.0f,           // dirección inicial: arriba (inactivo)
		1.0f, 0.014f, 0.0007f,
		25.0f
	);

	// Faro trasero Batwing — amarillo, se activa cuando retrocede (-Z, nave_j >= 4)
	spotLights[3] = SpotLight(
		1.0f, 0.9f, 0.1f,           // color: amarillo
		0.0f, 6.0f,
		-50.0f, 23.0f, 154.0f,      // posición inicial (actualizada cada frame)
		0.0f, 1.0f, 0.0f,           // dirección inicial: arriba (inactivo)
		1.0f, 0.014f, 0.0007f,
		25.0f
	);
	spotLightCount += 2;  // total: 4

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
			if (avatarMoving) avatarWalkCycle += deltaTime * 4.0f;
			glm::vec3 camPos = avatarPos - avFwd * 40.0f + glm::vec3(0.0f, 20.0f, 0.0f);
			camera.setPositionAndLookAt(camPos, avatarPos + glm::vec3(0.0f, 4.0f, 0.0f));
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

		// Spotlight toggle (tecla G)
		static bool gKeyPrev = false;
		static bool spotlightOn = true;
		if (keys[GLFW_KEY_G] && !gKeyPrev) spotlightOn = !spotlightOn;
		gKeyPrev = keys[GLFW_KEY_G];

		// Ciclo día/noche — corre por default
		// static bool key7Prev = false;
		// if (keys[GLFW_KEY_7] && !key7Prev) { cycleElapsed = 0.0f; cycleRunning = true; }
		// key7Prev = keys[GLFW_KEY_7];
		if (cycleRunning) {
			cycleElapsed += deltaTime;
			if (cycleElapsed >= CYCLE_DURATION) cycleElapsed -= CYCLE_DURATION; // loop continuo
		}

		float cycleTime = cycleElapsed / CYCLE_DURATION;
		float sunAngle  = cycleTime * 2.0f * glm::pi<float>();
		float dayFactor = glm::max(0.0f, cosf(sunAngle));
		// Arco solar en plano X-Y: sale por +X (este), culmina en -Y (cénit), se pone en -X (oeste).
		// El vector (sin, -cos, 0) es siempre unitario — no necesita normalize.
		float sunDirX = sinf(sunAngle);
		float sunDirY = -cosf(sunAngle);
		float sunDirZ = 0.0f;

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
		// Luces encendidas solo de noche (dayFactor == 0 → sol bajo el horizonte)
		pointLightCount = (dayFactor <= 0.0f) ? 14 : 0;
		glm::vec3 skyTintColor = glm::max(sunColor, glm::vec3(0.04f, 0.04f, 0.12f));
		skybox.SetTint(skyTintColor.r, skyTintColor.g, skyTintColor.b);

		// NPC: update siempre, independiente de cámara
		{
			const float NPC_SPEED = 1.0f;
			const float NPC_RANGE = 2.0f;
			if (npcGoingPos) { npcOffset += NPC_SPEED * deltaTime; if (npcOffset >=  NPC_RANGE) npcGoingPos = false; }
			else             { npcOffset -= NPC_SPEED * deltaTime; if (npcOffset <= -NPC_RANGE) npcGoingPos = true;  }
			npcWalkCycle += deltaTime * 4.0f;
		}

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
		GLuint uniformUseBones = shaderList[0].GetUniformLocation("useBones");
		GLuint uniformGBones   = shaderList[0].GetUniformLocation("gBones");

		glUniform1f(uniformAlpha, 1.0f);
		glUniform1i(uniformNoLighting, 0);
		glUniform1i(uniformUseTexture, 1);
		glUniform1i(uniformUseBones, 0);

		glUniformMatrix4fv(uniformProjection, 1, GL_FALSE, glm::value_ptr(projection));
		glUniformMatrix4fv(uniformView, 1, GL_FALSE, glm::value_ptr(activeCamera->calculateViewMatrix()));
		glUniform3f(uniformEyePosition,
			activeCamera->getCameraPosition().x,
			activeCamera->getCameraPosition().y,
			activeCamera->getCameraPosition().z);

		// Flashlight: solo activa en cámara libre (modo 2) con G encendido
		if (cameraMode == 2 && spotlightOn) {
			glm::vec3 lowerLight = freeCamera.getCameraPosition();
			lowerLight.y -= 0.3f;
			spotLights[0].SetFlash(lowerLight, freeCamera.getCameraDirection());
		} else {
			spotLights[0].SetFlash(glm::vec3(0.0f, -1000.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
		}

		// Faros Batwing — posición calculada de nave_pos/nave_j del frame anterior (lag 1 frame, imperceptible)
		{
			glm::vec3 batwingBase(-50.0f, 25.0f, 160.0f + nave_pos);
			glm::vec3 inactiveDir(0.0f, 1.0f, 0.0f); // apunta al cielo → no ilumina nada

			if (nave_j < 4.0f) {
				// Avance (+Z): faro delantero azul activo
				spotLights[2].SetFlash(batwingBase + glm::vec3(0.0f, -2.0f, 6.0f),
				                       glm::normalize(glm::vec3(0.0f, -1.0f, 0.8f)));
				spotLights[3].SetFlash(batwingBase, inactiveDir);
				spotLightCount = 3;
			} else if (nave_j < 8.0f) {
				// Retroceso (-Z): faro trasero ámbar activo
				spotLights[2].SetFlash(batwingBase, inactiveDir);
				spotLights[3].SetFlash(batwingBase + glm::vec3(0.0f, -2.0f, -6.0f),
				                       glm::normalize(glm::vec3(0.0f, -1.0f, -0.8f)));
				spotLightCount = 4;
			} else {
				// Transición idle: ambos apagados
				spotLights[2].SetFlash(batwingBase, inactiveDir);
				spotLights[3].SetFlash(batwingBase, inactiveDir);
				spotLightCount = 2;
			}
		}

		shaderList[0].SetDirectionalLight(&mainLight);
		shaderList[0].SetPointLights(pointLights, pointLightCount);
		shaderList[0].SetSpotLights(spotLights, spotLightCount);

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

		// --- BATMAN ANIMADO (avatar jugable — cámara 1 tercera persona) ---
		{
			static float batmanAnimTime = 0.0f;
			if (avatarMoving) batmanAnimTime += deltaTime;

			glUniform1i(uniformUseBones, 1);
			model = glm::mat4(1.0f);
			model = glm::translate(model, avatarPos);
			// 90 - avatarYaw: alinea el frente del modelo Mixamo (+Z local) con avFwd
			model = glm::rotate(model, glm::radians(90.0f - avatarYaw), glm::vec3(0.0f, 1.0f, 0.0f));
			model = glm::scale(model, glm::vec3(5.0f, 5.0f, 5.0f));
			glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
			glUniform3fv(uniformColor, 1, glm::value_ptr(color));
			Material_opaco.UseMaterial(uniformSpecularIntensity, uniformShininess);
			BatmanAnim.Update(batmanAnimTime, "walk", true);
			glDisable(GL_CULL_FACE);
			BatmanAnim.Render(uniformGBones);
			glEnable(GL_CULL_FACE);
			glUniform1i(uniformUseBones, 0);
		}

		// --- NPC JOKER CAMINANTE (mismo modelo que avatar, autónomo) ---
		{
			float wc        = npcWalkCycle;
			float legSwing  = sinf(wc) * 25.0f;
			float armSwing  = -sinf(wc) * 20.0f;
			float bodyBob   = fabsf(sinf(wc * 2.0f)) * 0.3f;
			const float npcScale = 0.06f;
			glm::vec3 npcBase(0.0f + npcOffset, 6.0f + bodyBob, 50.0f);
			float npcFaceYaw = npcGoingPos ? 90.0f : 270.0f;

			auto npcPart = [&](glm::vec3 offset, float rotX, Model& mdl) {
				model = glm::mat4(1.0f);
				model = glm::translate(model, npcBase);
				model = glm::rotate(model, glm::radians(npcFaceYaw), glm::vec3(0,1,0));
				model = glm::translate(model, offset);
				if (rotX != 0) model = glm::rotate(model, glm::radians(rotX), glm::vec3(1,0,0));
				model = glm::scale(model, glm::vec3(npcScale));
				glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
				glUniform3fv(uniformColor, 1, glm::value_ptr(color));
				Material_opaco.UseMaterial(uniformSpecularIntensity, uniformShininess);
				glDisable(GL_CULL_FACE); mdl.RenderModel(); glEnable(GL_CULL_FACE);
			};

			npcPart(glm::vec3( 0.0f, -4.2f, 0),  0,           JokerAvatar_Cuerpo_M);
			npcPart(glm::vec3( 0.0f,  4.8f, 0),  0,           JokerAvatar_Cabeza_M);
			npcPart(glm::vec3(-0.6f,  4.3f, 0),  armSwing,    JokerAvatar_BrazoDer_M);
			npcPart(glm::vec3( 0.6f,  4.3f, 0), -armSwing,    JokerAvatar_BrazoIzq_M);
			npcPart(glm::vec3( 0.5f,  1.2f, 0),  legSwing,    JokerAvatar_PiernaDer_M);
			npcPart(glm::vec3(-0.5f,  1.2f, 0), -legSwing,    JokerAvatar_PiernaIzq_M);
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

		// --- BATMAN ---
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(0.0f, -1.0f, -25.0f));
		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(5.0f, 5.0f, 5.0f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glUniform3fv(uniformColor, 1, glm::value_ptr(color));
		Material_opaco.UseMaterial(uniformSpecularIntensity, uniformShininess);
		glDisable(GL_CULL_FACE);
		Batman_M.RenderModel();
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

		// --- TITAN JOKER ---
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(50.0f, -1.0f, -25.0f));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.05f, 0.05f, 0.05f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glUniform3fv(uniformColor, 1, glm::value_ptr(color));
		Material_opaco.UseMaterial(uniformSpecularIntensity, uniformShininess);
		glDisable(GL_CULL_FACE);
		TitanJoker_M.RenderModel();
		glEnable(GL_CULL_FACE);

		// --- BATIMOTO ---
		model = glm::mat4(1.0);
		model = glm::translate(model, glm::vec3(30.0f, 2.6f, 10.0f));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.8f, 0.8f, 0.8f));
		glUniformMatrix4fv(uniformModel, 1, GL_FALSE, glm::value_ptr(model));
		glUniform3fv(uniformColor, 1, glm::value_ptr(color));
		Material_opaco.UseMaterial(uniformSpecularIntensity, uniformShininess);
		glDisable(GL_CULL_FACE);
		Batimoto_M.RenderModel();
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
		glDisable(GL_CULL_FACE); TimePortal_M.RenderModel(); glEnable(GL_CULL_FACE);

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

	return 0;
}
