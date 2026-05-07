// graphics.cpp
// Manages the OpenGL scene: shader setup, solar system transforms, ship physics,
// and the full render pipeline including lighting and special effects.

#include "graphics.h"

Graphics::Graphics()
{

}

void Graphics::setKeyState(int key, bool pressed) {
	switch (key) {
	case Graphics::FWD:        m_keyFwd = pressed; break;
	case Graphics::BACK:       m_keyBack = pressed; break;
	case Graphics::LEFT:       m_keyLeft = pressed; break;
	case Graphics::RIGHT:      m_keyRight = pressed; break;
	case Graphics::ROLL_L:     m_keyRollL = pressed; break;
	case Graphics::ROLL_R:     m_keyRollR = pressed; break;
	case Graphics::PITCH_UP:   m_keyPitchUp = pressed; break;
	case Graphics::PITCH_DOWN: m_keyPitchDown = pressed; break;
	}
}
void createRingMesh(float innerR, float outerR, int segments, const char* texFile);

void Graphics::createRingMesh(float innerR, float outerR,
	int segments, const char* texFile) {
	// Build ring geometry manually and inject into a mesh
	// We'll use a raw OpenGL approach via a dedicated helper struct
	// Store in a local Vertices/Indices then upload

	std::vector<Vertex> verts;
	std::vector<unsigned int> inds;

	for (int i = 0; i <= segments; i++) {
		float angle = glm::two_pi<float>() * i / segments;
		float c = cos(angle), s = sin(angle);
		float u = (float)i / segments;

		// Outer vertex
		verts.push_back(Vertex(
			glm::vec3(outerR * c, 0.f, outerR * s),
			glm::vec3(0.f, 1.f, 0.f),
			glm::vec2(u, 1.f)
		));
		// Inner vertex
		verts.push_back(Vertex(
			glm::vec3(innerR * c, 0.f, innerR * s),
			glm::vec3(0.f, 1.f, 0.f),
			glm::vec2(u, 0.f)
		));
	}

	for (int i = 0; i < segments; i++) {
		int base = i * 2;
		// Triangle 1
		inds.push_back(base + 0);
		inds.push_back(base + 1);
		inds.push_back(base + 2);
		// Triangle 2
		inds.push_back(base + 1);
		inds.push_back(base + 3);
		inds.push_back(base + 2);
	}

	// Inject into mesh using its public InitBuffers indirectly
	// Since Mesh doesn't expose this, we render ring separately with raw GL
	// Store in m_ringVerts etc — see graphics.h additions below
	m_ringVertices = verts;
	m_ringIndices = inds;
	m_ringTexture = new Texture(texFile);

	glGenVertexArrays(1, &m_ringVAO);
	glBindVertexArray(m_ringVAO);

	glGenBuffers(1, &m_ringVBO);
	glBindBuffer(GL_ARRAY_BUFFER, m_ringVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * verts.size(), verts.data(), GL_STATIC_DRAW);

	glGenBuffers(1, &m_ringIBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ringIBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * inds.size(), inds.data(), GL_STATIC_DRAW);

	glBindVertexArray(0);
}


void Graphics::setMouseDelta(float dx, float dy) {
	m_mouseDX += dx;
	m_mouseDY += dy;
}

Graphics::~Graphics()
{

}

// Initialize: sets up GLEW, camera, shader program, all scene objects, and GL state.
bool Graphics::Initialize(int width, int height)
{
	// Used for the linux OS
#if !defined(__APPLE__) && !defined(MACOSX)
  // cout << glewGetString(GLEW_VERSION) << endl;
	glewExperimental = GL_TRUE;

	auto status = glewInit();

	// This is here to grab the error that comes from glew init.
	// This error is an GL_INVALID_ENUM that has no effects on the performance
	glGetError();

	//Check for error
	if (status != GLEW_OK)
	{
		std::cerr << "GLEW Error: " << glewGetErrorString(status) << "\n";
		return false;
	}
#endif

	// Init Camera
	m_camera = new Camera();
	if (!m_camera->Initialize(width, height))
	{
		printf("Camera Failed to Initialize\n");
		return false;
	}

	// Set up the shaders
	m_shader = new Shader();
	if (!m_shader->Initialize())
	{
		printf("Shader Failed to Initialize\n");
		return false;
	}

	// Add the vertex shader
	if (!m_shader->AddShader(GL_VERTEX_SHADER))
	{
		printf("Vertex Shader failed to Initialize\n");
		return false;
	}

	// Add the fragment shader
	if (!m_shader->AddShader(GL_FRAGMENT_SHADER))
	{
		printf("Fragment Shader failed to Initialize\n");
		return false;
	}

	// Connect the program
	if (!m_shader->Finalize())
	{
		printf("Program to Finalize\n");
		return false;
	}

	// Populate location bindings of the shader uniform/attribs
	if (!collectShPrLocs()) {
		printf("Some shader attribs not located!\n");
	}

	// Starship
	m_mesh = new Mesh(glm::vec3(2.0f, 3.0f, -5.0f), "assets\\SpaceShip-1.obj", "assets\\SpaceShip-1.png");

	// The Sun
	m_sphere = new Sphere(64, "assets\\2k_sun.jpg");

	// The Earth
	m_sphere2 = new Sphere(48, "assets\\2k_earth_daymap.jpg");

	// The Moon
	m_sphere3 = new Sphere(48, "assets\\2k_moon.jpg");

	// Planets
	m_mercury = new Sphere(48, "assets\\Mercury.jpg");
	m_venus = new Sphere(48, "assets\\Venus.jpg");
	m_mars = new Sphere(48, "assets\\Mars.jpg");
	m_jupiter = new Sphere(48, "assets\\Jupiter.jpg");
	m_saturn = new Sphere(48, "assets\\Saturn.jpg");
	m_uranus = new Sphere(48, "assets\\Uranus.jpg");
	m_neptune = new Sphere(48, "assets\\Neptune.jpg");

	//Dwarf planets
	m_pluto = new Sphere(32, "assets\\Pluto.png");
	m_haumea = new Sphere(32, "assets\\Haumea.jpg");
	m_eris = new Sphere(32, "assets\\Eris.jpg");
	m_ceres = new Sphere(32, "assets\\Ceres.jpg");

	// Halley's Comet — low precision sphere for lumpy look
	m_comet = new Sphere(16, "assets\\HalleysComet.jpg");
	m_cometTail = new Sphere(16, "assets\\2k_sun.jpg");

	// Asteroid belt — single mesh for instancing, low precision for variety
	m_asteroid = new Sphere(8, "assets\\asteroid.jpg");
	m_asteroidNormal = new Texture("assets\\asteroid-n.png");

	// Seed random for reproducible belt layout
	srand(42);
	auto randF = [](float lo, float hi) {
		return lo + (hi - lo) * (rand() / (float)RAND_MAX);
		};

	// Inner belt: between Mars (15) and Jupiter (21)
	for (int i = 0; i < 300; i++) {
		AsteroidInstance inst;
		inst.orbitRadius = randF(16.f, 18.f);
		inst.orbitSpeed = randF(0.3f, 0.7f);
		inst.orbitAngle = randF(0.f, glm::two_pi<float>());
		inst.orbitHeight = randF(-0.5f, 0.5f);
		inst.scale = randF(0.04f, 0.12f);
		inst.model = glm::mat4(1.f);
		m_innerBelt.push_back(inst);
	}

	// Outer belt: beyond Neptune (36)
	for (int i = 0; i < 500; i++) {
		AsteroidInstance inst;
		inst.orbitRadius = randF(38.f, 50.f);
		inst.orbitSpeed = randF(0.05f, 0.15f);
		inst.orbitAngle = randF(0.f, glm::two_pi<float>());
		inst.orbitHeight = randF(-1.5f, 1.5f);
		inst.scale = randF(0.05f, 0.15f);
		inst.model = glm::mat4(1.f);
		m_outerBelt.push_back(inst);
	}

	// Sky sphere
	m_skySphere = new Sphere(64, "assets\\Galaxy.jpg");

	// Saturn ring (procedural mesh + texture)
	createRingMesh(1.4f, 2.4f, 64, "assets\\Saturn_ring.png");

	// Suggested orbit camera distances for each body (matched to their visual scale)
	m_bodyOrbitRadius[0] = 8.0f;  // Sun     (scale 3.0)
	m_bodyOrbitRadius[1] = 0.8f;  // Mercury (scale 0.2)
	m_bodyOrbitRadius[2] = 1.5f;  // Venus   (scale 0.5)
	m_bodyOrbitRadius[3] = 1.5f;  // Earth   (scale 0.5)
	m_bodyOrbitRadius[4] = 0.5f;  // Moon    (scale 0.13)
	m_bodyOrbitRadius[5] = 1.0f;  // Mars    (scale 0.3)
	m_bodyOrbitRadius[6] = 4.0f;  // Jupiter (scale 1.4)
	m_bodyOrbitRadius[7] = 4.0f;  // Saturn  (scale 1.2)
	m_bodyOrbitRadius[8] = 2.5f;  // Uranus  (scale 0.8)
	m_bodyOrbitRadius[9] = 2.5f;  // Neptune (scale 0.8)

	m_bodyOrbitRadius[13] = 1.2f; // Pluto
	m_bodyOrbitRadius[14] = 1.2f; // Haumea
	m_bodyOrbitRadius[15] = 1.2f; // Eris
	m_bodyOrbitRadius[16] = 0.8f; // Ceres (small, closer)

	m_bodyOrbitRadius[10] = 3.0f;  // comet (small object)
	m_bodyOrbitRadius[11] = 6.0f;  // inner belt
	m_bodyOrbitRadius[12] = 10.0f; // outer belt

	// Normal maps (only for planets that have them)
	m_mercuryNormal = new Texture("assets\\Mercury-n.jpg");
	m_venusNormal = new Texture("assets\\Venus-n.jpg");
	m_earthNormal = new Texture("assets\\2k_earth_daymap-n.jpg");
	m_moonNormal = new Texture("assets\\2k_moon-n.jpg");
	m_marsNormal = new Texture("assets\\Mars-n.jpg");
	m_jupiterNormal = new Texture("assets\\Jupiter-n.jpg");
	m_saturnNormal = new Texture("assets\\Saturn-n.jpg");
	m_uranusNormal = new Texture("assets\\Uranus-n.jpg");
	m_neptuneNormal = new Texture("assets\\Neptune-n.jpg");
	// Normal maps for dwarf planets
	m_plutoNormal = new Texture("assets\\pluto-n.jpg");
	m_haumeaNormal = new Texture("assets\\Haumea-n.jpg");
	m_erisNormal = new Texture("assets\\Eris-n.jpg");
	m_ceresNormal = new Texture("assets\\Ceres-n.jpg");

	// Enable depth testing
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	return true;
}

// HierarchicalUpdate2: advances every solar-system body, the ship, and engine glow orbs.
// absoluteTime drives orbital positions; dt is the per-frame step for ship physics.
void Graphics::HierarchicalUpdate2(double absoluteTime, float dt) {
	glm::mat4 tmat, rmat, smat;

	// absolute time
	double at = absoluteTime;

	// SUN (center, slow self-rotation)
	ComputeTransforms(dt,
		{ 0.f, 0.f, 0.f },          // no orbit
		{ 0.f, 0.f, 0.f },
		{ 0.3f }, glm::vec3(0, 1, 0), // slow Y spin
		{ 3.0f, 3.0f, 3.0f },
		tmat, rmat, smat);
	glm::mat4 sunModel = rmat * smat;    // no translation = stays at origin
	m_sphere->Update(sunModel);
	m_bodyPos[0] = glm::vec3(0.f);      // Sun always at world origin

	// Mercury
	ComputeTransforms(at, { 4.7f,0.f,4.7f }, { 5.f,0.f,5.f }, { 2.0f }, glm::vec3(0, 1, 0), { 0.2f,0.2f,0.2f }, tmat, rmat, smat);
	glm::mat4 mercuryModel = tmat * glm::rotate(glm::mat4(1.f), glm::radians(0.03f), glm::vec3(0, 0, 1)) * rmat * smat;
	m_mercury->Update(mercuryModel);
	m_bodyPos[1] = glm::vec3(mercuryModel[3]);

	// Venus
	ComputeTransforms(at, { 3.5f,0.f,3.5f }, { 8.f,0.f,8.f }, { 1.5f }, glm::vec3(0, 1, 0), { 0.5f,0.5f,0.5f }, tmat, rmat, smat);
	glm::mat4 venusModel = tmat * glm::rotate(glm::mat4(1.f), glm::radians(177.f), glm::vec3(0, 0, 1)) * rmat * smat;
	m_venus->Update(venusModel);
	m_bodyPos[2] = glm::vec3(venusModel[3]);

	// Earth
	ComputeTransforms(at, { 3.0f,0.f,3.0f }, { 11.f,0.f,11.f }, { 1.2f }, glm::vec3(0, 1, 0), { 0.5f,0.5f,0.5f }, tmat, rmat, smat);
	glm::mat4 earthModel = tmat * glm::rotate(glm::mat4(1.f), glm::radians(23.4f), glm::vec3(0, 0, 1)) * rmat * smat;
	m_sphere2->Update(earthModel);
	m_bodyPos[3] = glm::vec3(earthModel[3]);

	// Moon around Earth
	glm::vec3 earthPos = glm::vec3(
		cos(3.0f * at) * 11.f,
		0.f,
		sin(3.0f * at) * 11.f
	);
	glm::mat4 earthTransOnly = glm::translate(glm::mat4(1.f), earthPos);

	float moonOrbitRadius = 1.5f;
	float moonOrbitSpeed = 6.0f;
	glm::vec3 moonLocalPos = glm::vec3(
		cos(moonOrbitSpeed * at) * moonOrbitRadius,
		0.f,
		sin(moonOrbitSpeed * at) * moonOrbitRadius
	);
	glm::mat4 moonOrbitMat = glm::translate(glm::mat4(1.f), moonLocalPos);
	glm::mat4 moonSpin = glm::rotate(glm::mat4(1.f), (float)(2.f * at), glm::vec3(0, 1, 0));
	glm::mat4 moonScale = glm::scale(glm::vec3(0.13f));

	glm::mat4 moonModel = earthTransOnly * moonOrbitMat * moonSpin * moonScale;
	m_sphere3->Update(moonModel);
	m_bodyPos[4] = glm::vec3(moonModel[3]);

	// Mars
	ComputeTransforms(at, { 2.4f,0.f,2.4f }, { 15.f,0.f,15.f }, { 1.0f }, glm::vec3(0, 1, 0), { 0.3f,0.3f,0.3f }, tmat, rmat, smat);
	glm::mat4 marsModel = tmat * glm::rotate(glm::mat4(1.f), glm::radians(25.f), glm::vec3(0, 0, 1)) * rmat * smat;
	m_mars->Update(marsModel);
	m_bodyPos[5] = glm::vec3(marsModel[3]);

	// Jupiter
	ComputeTransforms(at, { 1.3f,0.f,1.3f }, { 21.f,0.f,21.f }, { 0.5f }, glm::vec3(0, 1, 0), { 1.4f,1.4f,1.4f }, tmat, rmat, smat);
	glm::mat4 jupModel = tmat * glm::rotate(glm::mat4(1.f), glm::radians(3.f), glm::vec3(0, 0, 1)) * rmat * smat;
	m_jupiter->Update(jupModel);
	m_bodyPos[6] = glm::vec3(jupModel[3]);

	// Saturn
	ComputeTransforms(at, { 0.9f,0.f,0.9f }, { 24.f,0.f,24.f }, { 0.6f },
		glm::vec3(0, 1, 0), { 1.2f,1.2f,1.2f }, tmat, rmat, smat);

	// Build Saturn's base: translate to orbit position first, then tilt
	glm::mat4 saturnBase = tmat * glm::rotate(glm::mat4(1.f), glm::radians(27.f), glm::vec3(0, 0, 1));

	// Planet: base * spin * scale
	glm::mat4 satModel = saturnBase * rmat * smat;
	m_saturn->Update(satModel);
	m_bodyPos[7] = glm::vec3(satModel[3]);

	// Ring: same base position and tilt, no spin, scale matches sphere radius
	// Saturn sphere scale is 1.2f, ring inner=1.4 outer=2.4 in local space
	// So ring needs to be scaled by 1.2f to wrap around the 1.2-unit sphere
	m_ringModel = saturnBase * glm::scale(glm::mat4(1.f), glm::vec3(1.2f, 1.2f, 1.2f));

	// Uranus
	ComputeTransforms(at, { 0.7f,0.f,0.7f }, { 30.f,0.f,30.f }, { 0.4f }, glm::vec3(0, 1, 0), { 0.8f,0.8f,0.8f }, tmat, rmat, smat);
	glm::mat4 urModel = tmat * glm::rotate(glm::mat4(1.f), glm::radians(98.f), glm::vec3(0, 0, 1)) * rmat * smat;
	m_uranus->Update(urModel);
	m_bodyPos[8] = glm::vec3(urModel[3]);

	// Neptune
	ComputeTransforms(at, { 0.5f,0.f,0.5f }, { 36.f,0.f,36.f }, { 0.3f }, glm::vec3(0, 1, 0), { 0.8f,0.8f,0.8f }, tmat, rmat, smat);
	glm::mat4 nepModel = tmat * glm::rotate(glm::mat4(1.f), glm::radians(28.f), glm::vec3(0, 0, 1)) * rmat * smat;
	m_neptune->Update(nepModel);
	m_bodyPos[9] = glm::vec3(nepModel[3]);

	// Pluto
	ComputeTransforms(at,
		{ 0.3f, 0.f, 0.3f },   // slow orbit
		{ 45.f, 0.f, 45.f },   // far distance
		{ 0.5f }, glm::vec3(0, 1, 0),
		{ 0.2f,0.2f,0.2f },
		tmat, rmat, smat);

	glm::mat4 plutoModel = tmat * rmat * smat;
	m_pluto->Update(plutoModel);
	m_bodyPos[13] = glm::vec3(plutoModel[3]);

	// Haumea
	ComputeTransforms(at,
		{ 0.35f, 0.f, 0.35f },
		{ 47.f, 0.f, 47.f },
		{ 2.0f }, glm::vec3(1, 0, 0),
		{ 0.25f,0.15f,0.15f },
		tmat, rmat, smat);

	glm::mat4 haumeaModel = tmat * rmat * smat;
	m_haumea->Update(haumeaModel);
	m_bodyPos[14] = glm::vec3(haumeaModel[3]);

	// Eris
	ComputeTransforms(at,
		{ 0.2f, 0.f, 0.2f },
		{ 55.f, 0.f, 55.f },
		{ 0.4f }, glm::vec3(0, 1, 0),
		{ 0.2f,0.2f,0.2f },
		tmat, rmat, smat);

	glm::mat4 erisModel = tmat * rmat * smat;
	m_eris->Update(erisModel);
	m_bodyPos[15] = glm::vec3(erisModel[3]);

	// Ceres
	ComputeTransforms(at,
		{ 0.8f, 0.f, 0.8f },
		{ 17.f, 0.f, 17.f },
		{ 1.0f }, glm::vec3(0, 1, 0),
		{ 0.15f,0.15f,0.15f },
		tmat, rmat, smat);

	glm::mat4 ceresModel = tmat * rmat * smat;
	m_ceres->Update(ceresModel);
	m_bodyPos[16] = glm::vec3(ceresModel[3]);


	// Sky sphere: large scale around origin with slow rotation
	glm::mat4 skyModel = glm::scale(glm::mat4(1.f), glm::vec3(150.f));
	skyModel = glm::rotate(skyModel, (float)(0.002 * dt), glm::vec3(0, 1, 0));
	m_skySphere->Update(skyModel);

	// ── HALLEY'S COMET ────────────────────────────────────────────────────────
	// Real Halley's: perihelion ~0.6 AU, aphelion ~35 AU
	// We simulate with a stretched ellipse: close pass near sun, long tail out past Neptune
	float cometSpeed = 0.08f;
	float cometA = 40.f;   // semi-major axis (long, like real Halley's)
	float cometB = 8.f;    // semi-minor axis
	float cometAngle = (float)(cometSpeed * at);

	// Elliptical orbit: x along major axis, z along minor, slight y wobble
	glm::vec3 cometPos = glm::vec3(
		cos(cometAngle) * cometA,
		sin(cometAngle) * 3.f,
		sin(cometAngle) * cometB
	);
	m_bodyPos[10] = cometPos;

	// ── NUCLEUS ───────────────────────────────────────────────────────────────
	glm::mat4 nucleusModel =
		glm::translate(glm::mat4(1.f), cometPos)
		* glm::rotate(glm::mat4(1.f), (float)(2.f * at), glm::vec3(0.3f, 1.f, 0.1f))
		* glm::scale(glm::mat4(1.f), glm::vec3(0.25f));
	m_comet->Update(nucleusModel);

	// ── TAIL ──────────────────────────────────────────────────────────────────
	// Tail always points AWAY from sun (away from origin)
	glm::vec3 awayFromSun = glm::normalize(cometPos); // direction tail streams

	// Build an orthonormal basis with awayFromSun as the Z axis
	// so the sphere gets stretched along that axis
	glm::vec3 tempUp = glm::vec3(0.f, 1.f, 0.f);
	if (glm::abs(glm::dot(awayFromSun, tempUp)) > 0.99f)
		tempUp = glm::vec3(1.f, 0.f, 0.f);  // avoid parallel degenerate case

	glm::vec3 tailRight = glm::normalize(glm::cross(tempUp, awayFromSun));
	glm::vec3 tailUp = glm::normalize(glm::cross(awayFromSun, tailRight));

	// Rotation matrix: columns are right, up, forward(=awayFromSun)
	glm::mat4 tailRot = glm::mat4(
		glm::vec4(tailRight, 0.f),   // col 0 = local X
		glm::vec4(tailUp, 0.f),   // col 1 = local Y
		glm::vec4(awayFromSun, 0.f),   // col 2 = local Z (stretch axis)
		glm::vec4(0.f, 0.f, 0.f, 1.f)
	);

	// Tail length grows when near sun (perihelion), shrinks at aphelion
	float distFromSun = glm::length(cometPos);
	float tailLength = glm::clamp(distFromSun * 0.15f + 1.5f, 1.5f, 1.f);
	float tailWidth = 0.3f;

	// Place tail center BEHIND the nucleus along awayFromSun
	// Nucleus is at cometPos; tail root starts there and extends outward
	glm::vec3 tailCenter = cometPos + awayFromSun * tailLength;

	m_cometTailModel =
		glm::translate(glm::mat4(1.f), tailCenter)
		* tailRot
		* glm::scale(glm::mat4(1.f), glm::vec3(tailWidth, tailWidth, tailLength));

	m_cometTail->Update(m_cometTailModel);

	// ── ASTEROID BELTS ────────────────────────────────────────────────
	for (auto& inst : m_innerBelt) {
		float angle = inst.orbitAngle + inst.orbitSpeed * (float)at;
		glm::vec3 pos = glm::vec3(
			cos(angle) * inst.orbitRadius,
			inst.orbitHeight,
			sin(angle) * inst.orbitRadius
		);
		// Random tumble using orbitAngle as a seed for unique rotation axis
		glm::vec3 tumbleAxis = glm::normalize(glm::vec3(
			sin(inst.orbitAngle),
			cos(inst.orbitAngle * 1.3f),
			sin(inst.orbitAngle * 0.7f)
		));
		glm::mat4 spin = glm::rotate(glm::mat4(1.f),
			inst.orbitSpeed * 3.f * (float)at, tumbleAxis);
		inst.model = glm::translate(glm::mat4(1.f), pos)
			* spin
			* glm::scale(glm::vec3(inst.scale));
	}
	m_bodyPos[11] = glm::vec3(17.f, 0.f, 0.f);

	for (auto& inst : m_outerBelt) {
		float angle = inst.orbitAngle + inst.orbitSpeed * (float)at;
		glm::vec3 pos = glm::vec3(
			cos(angle) * inst.orbitRadius,
			inst.orbitHeight,
			sin(angle) * inst.orbitRadius
		);
		glm::vec3 tumbleAxis = glm::normalize(glm::vec3(
			sin(inst.orbitAngle),
			cos(inst.orbitAngle * 1.3f),
			sin(inst.orbitAngle * 0.7f)
		));
		glm::mat4 spin = glm::rotate(glm::mat4(1.f),
			inst.orbitSpeed * 3.f * (float)at, tumbleAxis);
		inst.model = glm::translate(glm::mat4(1.f), pos)
			* spin
			* glm::scale(glm::vec3(inst.scale));
	}
	m_bodyPos[12] = glm::vec3(44.f, 0.f, 0.f);

	// Update ship from input state
	UpdateShip((float)dt, m_keyFwd, m_keyBack, m_keyLeft, m_keyRight, m_keyRollL, m_keyRollR, m_keyPitchUp, m_keyPitchDown, m_mouseDX, m_mouseDY);
	// Reset mouse delta after applied
	m_mouseDX = 0.f; m_mouseDY = 0.f;
}


// UpdateShip: integrates ship rotation (yaw/pitch/roll) and speed from keyboard/mouse,
// then rebuilds the orientation basis vectors and model matrix each frame.
void Graphics::UpdateShip(float dt, bool fwd, bool back, bool left, bool right, bool rollLeft, bool rollRight, bool pitchUp, bool pitchDown, float mouseDX, float mouseDY) {
	// 1-2: apply yaw/pitch from mouse
	m_shipYaw -= mouseDX * 0.1f;
	m_shipPitch -= mouseDY * 0.1f;

	if (left)  m_shipYaw -= 60.f * dt;
	if (right) m_shipYaw += 60.f * dt;

	if (pitchUp)   m_shipPitch += 60.f * dt;
	if (pitchDown) m_shipPitch -= 60.f * dt;

	// 3: roll
	if (rollLeft)  m_shipRoll -= 60.f * dt;
	if (rollRight) m_shipRoll += 60.f * dt;

	// 4: clamp pitch
	if (m_shipPitch > 89.f) m_shipPitch = 89.f;
	if (m_shipPitch < -89.f) m_shipPitch = -89.f;

	// 5: rebuild forward
	glm::vec3 forward;
	forward.x = cos(glm::radians(m_shipYaw)) * cos(glm::radians(m_shipPitch));
	forward.y = sin(glm::radians(m_shipPitch));
	forward.z = sin(glm::radians(m_shipYaw)) * cos(glm::radians(m_shipPitch));
	m_shipForward = glm::normalize(forward);

	// 6: derive right from world-up cross forward
	glm::vec3 worldUp = glm::vec3(0, 1, 0);
	m_shipRight = glm::normalize(glm::cross(worldUp, m_shipForward));
	m_shipUp = glm::normalize(glm::cross(m_shipForward, m_shipRight));

	// 7: recompute up, then apply roll by rotating up/right around forward
	m_shipUp = glm::cross(m_shipRight, m_shipForward);
	if (m_shipRoll != 0.f) {
		glm::mat4 rm = glm::rotate(glm::mat4(1.f), glm::radians(m_shipRoll), m_shipForward);
		m_shipRight = glm::normalize(glm::vec3(rm * glm::vec4(m_shipRight, 0.f)));
		m_shipUp = glm::normalize(glm::vec3(rm * glm::vec4(m_shipUp, 0.f)));
	}

	// 8: speed control
	if (fwd)  m_shipSpeed = std::min(m_shipSpeed + m_shipAccel * dt, m_shipMaxSpeed);
	if (back) m_shipSpeed = std::max(m_shipSpeed - m_shipAccel * dt * 3.f, 0.f);
	if (!fwd && !back) m_shipSpeed = std::max(m_shipSpeed - m_shipAccel * 0.5f * dt, 0.f);

	// 9: move
	m_shipPosition += m_shipForward * m_shipSpeed * dt;

	// 10: build model matrix
	glm::mat4 rotMat = glm::mat4(
		glm::vec4(m_shipRight, 0),
		glm::vec4(m_shipUp, 0),
		glm::vec4(m_shipForward, 0),
		glm::vec4(0, 0, 0, 1)
	);
	glm::mat4 shipModel = glm::translate(glm::mat4(1.f), m_shipPosition) * rotMat * glm::scale(glm::vec3(0.05f));
	m_mesh->Update(shipModel);
}


// ComputeTransforms: builds T/R/S matrices for a planet.
// speed/dist drive the orbit translation; rotSpeed/rotVector drive self-spin; scale sets size.
void Graphics::ComputeTransforms(double dt, std::vector<float> speed, std::vector<float> dist,
	std::vector<float> rotSpeed, glm::vec3 rotVector, std::vector<float> scale, glm::mat4& tmat, glm::mat4& rmat, glm::mat4& smat) {
	tmat = glm::translate(glm::mat4(1.f),
		glm::vec3(cos(speed[0] * dt) * dist[0], sin(speed[1] * dt) * dist[1], sin(speed[2] * dt) * dist[2])
	);
	rmat = glm::rotate(glm::mat4(1.f), rotSpeed[0] * (float)dt, rotVector);
	smat = glm::scale(glm::vec3(scale[0], scale[1], scale[2]));
}


void Graphics::RenderRing()
{
	if (m_ringVAO == 0 || m_ringTexture == nullptr) return;

	glBindVertexArray(m_ringVAO);
	glBindBuffer(GL_ARRAY_BUFFER, m_ringVBO);

	glEnableVertexAttribArray(m_positionAttrib);
	glEnableVertexAttribArray(m_colorAttrib);
	glEnableVertexAttribArray(m_tcAttrib);

	glVertexAttribPointer(m_positionAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
		(void*)offsetof(Vertex, vertex));
	glVertexAttribPointer(m_colorAttrib, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
		(void*)offsetof(Vertex, normal));
	glVertexAttribPointer(m_tcAttrib, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
		(void*)offsetof(Vertex, texcoord));

	glUniform1i(m_hasTexture, true);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_ringTexture->getTextureID());
	glUniform1i(m_shader->GetUniformLocation("sp"), 0);

	// Enable blending for ring transparency
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ringIBO);
	glDrawElements(GL_TRIANGLES, (GLsizei)m_ringIndices.size(), GL_UNSIGNED_INT, 0);

	glDisable(GL_BLEND);

	glDisableVertexAttribArray(m_positionAttrib);
	glDisableVertexAttribArray(m_colorAttrib);
	glDisableVertexAttribArray(m_tcAttrib);
	glBindVertexArray(0);
}


// Render: clears the frame, uploads per-frame uniforms, and draws every scene object
// in order: sky sphere -> sun -> planets -> Saturn ring -> starship -> comet -> asteroid belts.
void Graphics::Render()
{
	glClearColor(0.0f, 0.0f, 0.05f, 1.0f); // dark space color
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	m_shader->Enable();

	glUniformMatrix4fv(m_projectionMatrix, 1, GL_FALSE,
		glm::value_ptr(m_camera->GetProjection()));
	glUniformMatrix4fv(m_viewMatrix, 1, GL_FALSE,
		glm::value_ptr(m_camera->GetView()));

	// Extract camera position from inverse view matrix
	glm::mat4 invView = glm::inverse(m_camera->GetView());
	glm::vec3 camPos = glm::vec3(invView[3]);

	// Set sun lighting uniforms (used for all objects)
	glUniform3f(m_lightPos, 0.f, 0.f, 0.f);
	glUniform3f(m_viewPos, camPos.x, camPos.y, camPos.z);
	glUniform3f(m_lightColor, 1.0f, 0.95f, 0.8f);
	glUniform1f(m_ambientStr, 0.15f);
	glUniform1f(m_specStr, 0.4f);

	// Fill light: active only in Planetary Observation mode.
	// Positioned at the camera so the observer can illuminate any side.
	// Warm day light comes from the sun; this cool fill lets you see the night side.
	if (m_planetaryMode) {
		glUniform3f(m_uFillLightPos, camPos.x, camPos.y, camPos.z);
		glUniform3f(m_uFillLightColor, 0.4f, 0.55f, 0.9f); // cool blue-white
		glUniform1f(m_uFillStrength, 0.35f);
	}
	else {
		glUniform3f(m_uFillLightColor, 0.f, 0.f, 0.f);
		glUniform1f(m_uFillStrength, 0.f);
		glUniform3f(m_uFillLightPos, 0.f, 0.f, 0.f);
	}

	GLuint sampler = m_shader->GetUniformLocation("sp");
	glUniform1i(sampler, 0);

	// Default emissive off — only the ship thruster overrides this.
	// Mask collapsed to a zero-volume box -> mask=0 everywhere for non-ship draws.
	glUniform1f(m_uEmissiveStrength, 0.f);
	glUniform3f(m_uEmissiveColor, 0.f, 0.f, 0.f);
	glUniform3f(m_uEmissiveMaskMin, 0.f, 0.f, 0.f);
	glUniform3f(m_uEmissiveMaskMax, 0.f, 0.f, 0.f);

	// ── SKY SPHERE (render first, no depth write) ─────────────────────
	if (m_skySphere != NULL) {
		glDepthMask(GL_FALSE);
		glUniform1f(m_ambientStr, 1.0f); // sky fully bright
		glUniformMatrix4fv(m_modelMatrix, 1, GL_FALSE,
			glm::value_ptr(m_skySphere->GetModel()));
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_skySphere->getTextureID());
		m_skySphere->Render(m_positionAttrib, m_colorAttrib,
			m_tcAttrib, m_hasTexture);
		glDepthMask(GL_TRUE);
		glUniform1f(m_ambientStr, 0.15f); // restore
		glUniform1i(m_hasNormalMap, false);
	}

	// ── SUN (fully emissive, no shading) ──────────────────────────────
	if (m_sphere != NULL) {
		glUniform1f(m_ambientStr, 1.0f); // sun glows fully
		glUniform1f(m_specStr, 0.0f);
		glUniformMatrix4fv(m_modelMatrix, 1, GL_FALSE,
			glm::value_ptr(m_sphere->GetModel()));
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_sphere->getTextureID());
		m_sphere->Render(m_positionAttrib, m_colorAttrib,
			m_tcAttrib, m_hasTexture);
		// Restore normal lighting for planets
		glUniform1f(m_ambientStr, 0.15f);
		glUniform1f(m_specStr, 0.4f);
		glUniform1i(m_hasNormalMap, false);
	}

	// ── HELPER LAMBDA to render any sphere with optional normal map ────
	auto renderSphere = [&](Sphere* s, Texture* normalTex) {
		if (s == NULL) return;
		glUniformMatrix4fv(m_modelMatrix, 1, GL_FALSE,
			glm::value_ptr(s->GetModel()));

		// Diffuse on TEXTURE0 (already bound by sampler "sp" = 0)
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, s->getTextureID());

		// Normal map on TEXTURE1
		if (normalTex != nullptr && normalTex->isLoaded()) {
			glUniform1i(m_hasNormalMap, true);
			glUniform1i(m_normalMapSampler, 1);     // sampler reads unit 1
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, normalTex->getTextureID());
		}
		else {
			glUniform1i(m_hasNormalMap, false);
		}

		s->Render(m_positionAttrib, m_colorAttrib, m_tcAttrib, m_hasTexture);

		// Clean up TEXTURE1 so other draw calls aren't affected
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, 0);
		glActiveTexture(GL_TEXTURE0);
		};

	// ── PLANETS ───────────────────────────────────────────────────────

	// Rocky/low-shine planets
	glUniform1f(m_shininess, 8.f);
	glUniform1f(m_specStr, 0.2f);
	renderSphere(m_mercury, m_mercuryNormal);
	renderSphere(m_mars, m_marsNormal);

	// Gas giants — moderate shine
	glUniform1f(m_shininess, 16.f);
	glUniform1f(m_specStr, 0.3f);
	renderSphere(m_jupiter, m_jupiterNormal);
	renderSphere(m_saturn, m_saturnNormal);
	renderSphere(m_uranus, m_uranusNormal);
	renderSphere(m_neptune, m_neptuneNormal);

	// Earth/Venus — higher shine (water/clouds)
	glUniform1f(m_shininess, 32.f);
	glUniform1f(m_specStr, 0.5f);
	renderSphere(m_sphere2, m_earthNormal);   // Earth
	renderSphere(m_venus, m_venusNormal);

	// Moon — very low shine
	glUniform1f(m_shininess, 4.f);
	glUniform1f(m_specStr, 0.1f);
	renderSphere(m_sphere3, m_moonNormal);

	// Dwarf planets
	glUniform1f(m_shininess, 8.f);
	glUniform1f(m_specStr, 0.2f);

	renderSphere(m_pluto, m_plutoNormal);
	renderSphere(m_haumea, m_haumeaNormal);
	renderSphere(m_eris, m_erisNormal);
	renderSphere(m_ceres, m_ceresNormal);

	// ── HALLEY'S COMET NUCLEUS ────────────────────────────────────────
	glUniform1f(m_ambientStr, 0.4f);
	glUniform1f(m_shininess, 8.f);
	glUniform1f(m_specStr, 0.15f);
	glUniform1i(m_hasNormalMap, false);
	renderSphere(m_comet, nullptr);

	// ── HALLEY'S COMET TAIL (additive blend = glow) ───────────────────
	if (m_cometTail != NULL) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE);  // additive glow
		glDepthMask(GL_FALSE);              // don't write depth — tail is transparent

		glUniform1i(m_hasNormalMap, false);
		glUniform1f(m_ambientStr, 0.8f);
		glUniform1f(m_specStr, 0.0f);
		glUniform3f(m_lightColor, 0.6f, 0.8f, 1.0f);  // blue-white glow

		glUniformMatrix4fv(m_modelMatrix, 1, GL_FALSE,
			glm::value_ptr(m_cometTail->GetModel()));
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_cometTail->getTextureID());
		m_cometTail->Render(m_positionAttrib, m_colorAttrib, m_tcAttrib, m_hasTexture);

		glDepthMask(GL_TRUE);
		glDisable(GL_BLEND);

		// Restore lighting
		glUniform3f(m_lightColor, 1.0f, 0.95f, 0.8f);
		glUniform1f(m_ambientStr, 0.15f);
		glUniform1f(m_specStr, 0.4f);
	}

	// ── ASTEROID BELTS ────────────────────────────────────────────────
	glUniform1f(m_shininess, 6.f);
	glUniform1f(m_specStr, 0.1f);
	glUniform1f(m_ambientStr, 0.2f);
	glUniform1i(m_hasNormalMap, true);
	glUniform1i(m_normalMapSampler, 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, m_asteroidNormal->getTextureID());

	for (auto& inst : m_innerBelt) {
		glUniformMatrix4fv(m_modelMatrix, 1, GL_FALSE,
			glm::value_ptr(inst.model));
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_asteroid->getTextureID());
		m_asteroid->Render(m_positionAttrib, m_colorAttrib,
			m_tcAttrib, m_hasTexture);
	}

	for (auto& inst : m_outerBelt) {
		glUniformMatrix4fv(m_modelMatrix, 1, GL_FALSE,
			glm::value_ptr(inst.model));
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_asteroid->getTextureID());
		m_asteroid->Render(m_positionAttrib, m_colorAttrib,
			m_tcAttrib, m_hasTexture);
	}

	// Clean up normal map state
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE0);
	glUniform1i(m_hasNormalMap, false);

	// ── SATURN RING ───────────────────────────────────────────────────
	glUniformMatrix4fv(m_modelMatrix, 1, GL_FALSE,
		glm::value_ptr(m_ringModel));
	RenderRing();

	// ── STARSHIP ──────────────────────────────────────────────────────
	// Extra-credit emissive: only the 6 back-burner cylinders glow, and
	// the brightness scales with ship speed. The fragment shader masks the
	// emissive term to a model-space box at the bottom-rear of the mesh.
	// SpaceShip-1.obj bounds: x=±25.2, y=-4.4..8.3, z=-29.6..36.2.
	// Cylinders sit roughly at y < -2 and z < -15.
	if (m_mesh != NULL && m_mesh->hasTex) {
		glUniform1f(m_ambientStr, 0.4f); // ship slightly brighter

		float speedRatio = (m_shipMaxSpeed > 0.f)
			? (m_shipSpeed / m_shipMaxSpeed) : 0.f;

		// Colour ramps deep red -> orange -> yellow-white as speed builds
		float r = 1.0f;
		float g = 0.10f + speedRatio * 0.75f;
		float b = speedRatio * speedRatio * 0.40f;
		// Strength: 0 at idle (cylinders look dark) -> 2.0 at max speed
		float strength = speedRatio * 2.0f;

		glUniform3f(m_uEmissiveColor, r, g, b);
		glUniform1f(m_uEmissiveStrength, strength);

		// Mask the emissive to the back-burner region (model-space box)
		glUniform3f(m_uEmissiveMaskMin, -25.0f, -5.0f, -30.0f);
		glUniform3f(m_uEmissiveMaskMax, 25.0f, -1.5f, -15.0f);

		glUniformMatrix4fv(m_modelMatrix, 1, GL_FALSE,
			glm::value_ptr(m_mesh->GetModel()));
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_mesh->getTextureID());
		glUniform1i(sampler, 0);
		m_mesh->Render(m_positionAttrib, m_colorAttrib,
			m_tcAttrib, m_hasTexture);

		// Restore: clear emissive + collapse mask box for any later draws
		glUniform1f(m_ambientStr, 0.15f);
		glUniform1f(m_uEmissiveStrength, 0.f);
		glUniform3f(m_uEmissiveColor, 0.f, 0.f, 0.f);
		glUniform3f(m_uEmissiveMaskMin, 0.f, 0.f, 0.f);
		glUniform3f(m_uEmissiveMaskMax, 0.f, 0.f, 0.f);
		glUniform1i(m_hasNormalMap, false);
	}

	auto error = glGetError();
	if (error != GL_NO_ERROR)
		std::cout << "OpenGL Error: " << ErrorString(error) << std::endl;
}

bool Graphics::collectShPrLocs() {
	bool anyProblem = true;

	// Locate the projection matrix in the shader
	m_projectionMatrix = m_shader->GetUniformLocation("projectionMatrix");
	if (m_projectionMatrix == INVALID_UNIFORM_LOCATION) {
		printf("m_projectionMatrix not found\n");
		anyProblem = false;
	}

	// Locate the view matrix in the shader
	m_viewMatrix = m_shader->GetUniformLocation("viewMatrix");
	if (m_viewMatrix == INVALID_UNIFORM_LOCATION) {
		printf("m_viewMatrix not found\n");
		anyProblem = false;
	}

	// Locate the model matrix in the shader
	m_modelMatrix = m_shader->GetUniformLocation("modelMatrix");
	if (m_modelMatrix == INVALID_UNIFORM_LOCATION) {
		printf("m_modelMatrix not found\n");
		anyProblem = false;
	}

	// Locate the position vertex attribute
	m_positionAttrib = m_shader->GetAttribLocation("v_position");
	if (m_positionAttrib == -1) {
		printf("v_position attribute not found\n");
		anyProblem = false;
	}

	// Locate the color vertex attribute
	m_colorAttrib = m_shader->GetAttribLocation("v_color");
	if (m_colorAttrib == -1) {
		printf("v_color attribute not found\n");
		anyProblem = false;
	}

	// Locate the texcoord vertex attribute
	m_tcAttrib = m_shader->GetAttribLocation("v_tc");
	if (m_tcAttrib == -1) {
		printf("v_texcoord attribute not found\n");
		anyProblem = false;
	}

	m_hasTexture = m_shader->GetUniformLocation("hasTexture");
	if (m_hasTexture == INVALID_UNIFORM_LOCATION) {
		printf("hasTexture uniform not found\n");
		anyProblem = false;
	}

	// Lighting uniforms
	m_lightPos = m_shader->GetUniformLocation("lightPos");
	m_viewPos = m_shader->GetUniformLocation("viewPos");
	m_lightColor = m_shader->GetUniformLocation("lightColor");
	m_ambientStr = m_shader->GetUniformLocation("ambientStrength");
	m_specStr = m_shader->GetUniformLocation("specularStrength");
	m_shininess = m_shader->GetUniformLocation("shininess");

	// Normal map uniforms
	m_hasNormalMap = m_shader->GetUniformLocation("hasNormalMap");
	m_normalMapSampler = m_shader->GetUniformLocation("normalMap");

	// Fill light uniforms (planetary observation night-side)
	m_uFillLightPos = m_shader->GetUniformLocation("fillLightPos");
	m_uFillLightColor = m_shader->GetUniformLocation("fillLightColor");
	m_uFillStrength = m_shader->GetUniformLocation("fillStrength");

	// Emissive thruster uniforms
	m_uEmissiveColor = m_shader->GetUniformLocation("emissiveColor");
	m_uEmissiveStrength = m_shader->GetUniformLocation("emissiveStrength");
	m_uEmissiveMaskMin = m_shader->GetUniformLocation("emissiveMaskMin");
	m_uEmissiveMaskMax = m_shader->GetUniformLocation("emissiveMaskMax");

	return anyProblem;
}

std::string Graphics::ErrorString(GLenum error)
{
	if (error == GL_INVALID_ENUM)
		return "GL_INVALID_ENUM: An unacceptable value is specified for an enumerated argument.";
	else if (error == GL_INVALID_VALUE)
		return "GL_INVALID_VALUE: A numeric argument is out of range.";
	else if (error == GL_INVALID_OPERATION)
		return "GL_INVALID_OPERATION: The specified operation is not allowed in the current state.";
	else if (error == GL_INVALID_FRAMEBUFFER_OPERATION)
		return "GL_INVALID_FRAMEBUFFER_OPERATION: The framebuffer object is not complete.";
	else if (error == GL_OUT_OF_MEMORY)
		return "GL_OUT_OF_MEMORY: There is not enough memory left to execute the command.";
	else
		return "None";
}