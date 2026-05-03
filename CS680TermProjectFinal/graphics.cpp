#include "graphics.h"

Graphics::Graphics()
{

}

void Graphics::setKeyState(int key, bool pressed) {
	switch(key) {
        case Graphics::FWD: m_keyFwd = pressed; break;
		case Graphics::BACK: m_keyBack = pressed; break;
		case Graphics::LEFT: m_keyLeft = pressed; break;
		case Graphics::RIGHT: m_keyRight = pressed; break;
		case Graphics::ROLL_L: m_keyRollL = pressed; break;
		case Graphics::ROLL_R: m_keyRollR = pressed; break;
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

	////// The Earth
	m_sphere2 = new Sphere(48, "assets\\2k_earth_daymap.jpg");
	////
	////// The moon
	m_sphere3 = new Sphere(48, "assets\\2k_moon.jpg");

	// Planets
	m_mercury = new Sphere(48, "assets\\Mercury.jpg");
	m_venus   = new Sphere(48, "assets\\Venus.jpg");
	m_mars    = new Sphere(48, "assets\\Mars.jpg");
	m_jupiter = new Sphere(48, "assets\\Jupiter.jpg");
	m_saturn  = new Sphere(48, "assets\\Saturn.jpg");
	m_uranus  = new Sphere(48, "assets\\Uranus.jpg");
	m_neptune = new Sphere(48, "assets\\Neptune.jpg");

	// Sky sphere
	m_skySphere = new Sphere(64, "assets\\Galaxy.jpg");

	// Saturn ring - simple quad mesh could be reused from Mesh loader with texture, here create thin disk via Mesh using existing ship as placeholder if needed
	//m_saturnRing = new Mesh();
	//m_saturnRing = new Mesh(glm::vec3(0.f, 0.f, 0.f),
	//	"assets\\Saturn_ring.png");  // we'll use a disk geometry

	createRingMesh(1.4f, 2.4f, 64, "assets\\Saturn_ring.png");




	//enable depth testing
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	return true;
}

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

	// Planets: Mercury
	ComputeTransforms(at, {4.7f,0.f,4.7f}, {4.f,0.f,4.f}, {0.5f}, glm::vec3(0,1,0), {0.2f,0.2f,0.2f}, tmat, rmat, smat);
	glm::mat4 mercuryModel = glm::rotate(glm::mat4(1.f), glm::radians(0.03f), glm::vec3(0,0,1)) * tmat * rmat * smat;
	m_mercury->Update(mercuryModel);

	// Venus
	ComputeTransforms(at, {3.5f,0.f,3.5f}, {6.f,0.f,6.f}, {0.3f}, glm::vec3(0,1,0), {0.5f,0.5f,0.5f}, tmat, rmat, smat);
	glm::mat4 venusModel = glm::rotate(glm::mat4(1.f), glm::radians(177.f), glm::vec3(0,0,1)) * tmat * rmat * smat;
	m_venus->Update(venusModel);

	// Earth (existing)
	ComputeTransforms(at, {3.0f,0.f,3.0f}, {9.f,0.f,9.f}, {1.5f}, glm::vec3(0,1,0), {0.5f,0.5f,0.5f}, tmat, rmat, smat);
	glm::mat4 earthModel = glm::rotate(glm::mat4(1.f), glm::radians(23.4f), glm::vec3(0,0,1)) * tmat * rmat * smat;
	m_sphere2->Update(earthModel);

	// Moon around Earth (reuse previous approach)
	glm::mat4 moonTilt = glm::rotate(glm::mat4(1.f), glm::radians(30.f), glm::vec3(0,0,1));
	glm::mat4 moonOrbit = glm::translate(glm::mat4(1.f), glm::vec3(cos(1.5f * at) * 2.0f, sin(1.5f * at) * 2.0f * 0.5f, sin(1.5f * at) * 2.0f));
	glm::mat4 moonSpin = glm::rotate(glm::mat4(1.f), (float)(2.f * at), glm::vec3(0,1,0));
	glm::mat4 moonScale = glm::scale(glm::vec3(0.27f));
	glm::mat4 earthTransOnly = glm::translate(glm::mat4(1.f), glm::vec3(cos(3.0f * at) * 9.f, 0.f, sin(3.0f * at) * 9.f));
	m_sphere3->Update(earthTransOnly * moonTilt * moonOrbit * moonSpin * moonScale);

	// Mars
	ComputeTransforms(at, {2.4f,0.f,2.4f}, {12.f,0.f,12.f}, {1.2f}, glm::vec3(0,1,0), {0.3f,0.3f,0.3f}, tmat, rmat, smat);
	glm::mat4 marsModel = glm::rotate(glm::mat4(1.f), glm::radians(25.f), glm::vec3(0,0,1)) * tmat * rmat * smat;
	m_mars->Update(marsModel);

	// Jupiter
	ComputeTransforms(at, {1.3f,0.f,1.3f}, {18.f,0.f,18.f}, {0.8f}, glm::vec3(0,1,0), {1.4f,1.4f,1.4f}, tmat, rmat, smat);
	glm::mat4 jupModel = glm::rotate(glm::mat4(1.f), glm::radians(3.f), glm::vec3(0,0,1)) * tmat * rmat * smat;
	m_jupiter->Update(jupModel);

	// Saturn and ring
	//ComputeTransforms(at, {0.9f,0.f,0.9f}, {24.f,0.f,24.f}, {0.6f}, glm::vec3(0,1,0), {1.2f,1.2f,1.2f}, tmat, rmat, smat);
	//glm::mat4 satModel = glm::rotate(glm::mat4(1.f), glm::radians(27.f), glm::vec3(0,0,1)) * tmat * rmat * smat;
	//m_saturn->Update(satModel);
	//// Saturn ring - place slightly larger, flat around Y
	//glm::mat4 ringModel = tmat * glm::rotate(glm::mat4(1.f), glm::radians(90.f), glm::vec3(1,0,0)) * glm::scale(glm::vec3(1.8f,1.8f,1.8f));
	////m_saturnRing->Update(ringModel);
	//m_ringModel = tmat * glm::rotate(glm::mat4(1.f), glm::radians(27.f),
	//	glm::vec3(0.f, 0.f, 1.f))
	//	* glm::scale(glm::vec3(1.2f, 1.2f, 1.2f));

	// Saturn
	ComputeTransforms(at, { 0.9f,0.f,0.9f }, { 24.f,0.f,24.f }, { 0.6f },
		glm::vec3(0, 1, 0), { 1.2f,1.2f,1.2f }, tmat, rmat, smat);

	// Build Saturn's base: translate to orbit position first, then tilt
	glm::mat4 saturnBase = tmat * glm::rotate(glm::mat4(1.f), glm::radians(27.f), glm::vec3(0, 0, 1));

	// Planet: base * spin * scale
	m_saturn->Update(saturnBase * rmat * smat);

	// Ring: same base position and tilt, no spin, scale matches sphere radius
	// Saturn sphere scale is 1.2f, ring inner=1.4 outer=2.4 in local space
	// So ring needs to be scaled by 1.2f to wrap around the 1.2-unit sphere
	m_ringModel = saturnBase * glm::scale(glm::mat4(1.f), glm::vec3(1.2f, 1.2f, 1.2f));


	// Uranus
	ComputeTransforms(at, {0.7f,0.f,0.7f}, {30.f,0.f,30.f}, {0.4f}, glm::vec3(0,1,0), {0.8f,0.8f,0.8f}, tmat, rmat, smat);
	glm::mat4 urModel = glm::rotate(glm::mat4(1.f), glm::radians(98.f), glm::vec3(0,0,1)) * tmat * rmat * smat;
	m_uranus->Update(urModel);

	// Neptune
	ComputeTransforms(at, {0.5f,0.f,0.5f}, {36.f,0.f,36.f}, {0.3f}, glm::vec3(0,1,0), {0.8f,0.8f,0.8f}, tmat, rmat, smat);
	glm::mat4 nepModel = glm::rotate(glm::mat4(1.f), glm::radians(28.f), glm::vec3(0,0,1)) * tmat * rmat * smat;
	m_neptune->Update(nepModel);

	// Sky sphere: large scale around origin with slow rotation
	glm::mat4 skyModel = glm::scale(glm::mat4(1.f), glm::vec3(150.f));
	skyModel = glm::rotate(skyModel, (float)(0.002 * dt), glm::vec3(0,1,0));
	m_skySphere->Update(skyModel);

	// Update ship from input state
	UpdateShip((float)dt, m_keyFwd, m_keyBack, m_keyLeft, m_keyRight, m_keyRollL, m_keyRollR, m_mouseDX, m_mouseDY);
	// reset mouse delta after applied
	m_mouseDX = 0.f; m_mouseDY = 0.f;
}


void Graphics::UpdateShip(float dt, bool fwd, bool back, bool left, bool right, bool rollLeft, bool rollRight, float mouseDX, float mouseDY) {
	// 1-2: apply yaw/pitch from mouse
	m_shipYaw -= mouseDX * 0.1f;
	m_shipPitch -= mouseDY * 0.1f;

	if (left)  m_shipYaw -= 60.f * dt;
	if (right) m_shipYaw += 60.f * dt;

	// 3 roll
	if (rollLeft) m_shipRoll -= 60.f * dt;
	if (rollRight) m_shipRoll += 60.f * dt;

	// 4 clamp pitch
	if (m_shipPitch > 89.f) m_shipPitch = 89.f;
	if (m_shipPitch < -89.f) m_shipPitch = -89.f;

	// 5 rebuild forward
	glm::vec3 forward;
	forward.x = cos(glm::radians(m_shipYaw)) * cos(glm::radians(m_shipPitch));
	forward.y = sin(glm::radians(m_shipPitch));
	forward.z = sin(glm::radians(m_shipYaw)) * cos(glm::radians(m_shipPitch));
	m_shipForward = glm::normalize(forward);

	// 6 right
	glm::vec3 worldUp = glm::vec3(0,1,0);
	//m_shipRight = glm::normalize(glm::cross(m_shipForward, worldUp));

	m_shipRight = glm::normalize(glm::cross(worldUp, m_shipForward));
	m_shipUp = glm::normalize(glm::cross(m_shipForward, m_shipRight));

	// 7 up and apply roll
	m_shipUp = glm::cross(m_shipRight, m_shipForward);
	// rotate up/right around forward by roll angle
	if (m_shipRoll != 0.f) {
		glm::mat4 rm = glm::rotate(glm::mat4(1.f), glm::radians(m_shipRoll), m_shipForward);
		//m_shipRight = glm::vec3(rm * glm::vec4(m_shipRight, 0.f));
		//m_shipUp = glm::vec3(rm * glm::vec4(m_shipUp, 0.f));
		m_shipRight = glm::normalize(glm::vec3(rm * glm::vec4(m_shipRight, 0.f)));
		m_shipUp = glm::normalize(glm::vec3(rm * glm::vec4(m_shipUp, 0.f)));
	}

	// 8 speed control
	if (fwd) m_shipSpeed = std::min(m_shipSpeed + m_shipAccel * dt, m_shipMaxSpeed);
	if (back) m_shipSpeed = std::max(m_shipSpeed - m_shipAccel * dt * 3.f, 0.f);
	if (!fwd && !back) m_shipSpeed = std::max(m_shipSpeed - m_shipAccel * 0.5f * dt, 0.f);

	// 9 move
	m_shipPosition += m_shipForward * m_shipSpeed * dt;

	// 10 build model matrix
	glm::mat4 rotMat = glm::mat4(
		glm::vec4(m_shipRight, 0),
		glm::vec4(m_shipUp, 0),
		glm::vec4(m_shipForward, 0),
		glm::vec4(0,0,0,1)
	);
	glm::mat4 shipModel = glm::translate(glm::mat4(1.f), m_shipPosition) * rotMat * glm::scale(glm::vec3(0.05f));
	m_mesh->Update(shipModel);

	}


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

//void Graphics::Render()
//{
//	//clear the screen
//	//glClearColor(0.5, 0.2, 0.2, 1.0);
//	glClearColor(0.0f, 0.0f, 0.05f, 1.0f); // dark space color
//
//	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//
//	// Start the correct program
//	m_shader->Enable();
//
//	// Send in the projection and view to the shader (stay the same while camera intrinsic(perspective) and extrinsic (view) parameters are the same
//	glUniformMatrix4fv(m_projectionMatrix, 1, GL_FALSE, glm::value_ptr(m_camera->GetProjection()));
//	glUniformMatrix4fv(m_viewMatrix, 1, GL_FALSE, glm::value_ptr(m_camera->GetView()));
//
//    // Set lighting uniforms (sun at origin)
//	glUniform3f(m_lightPos, 0.f, 0.f, 0.f);
//	// viewPos from camera
//	glm::vec3 camPos = glm::vec3(0.f);
//	// try to get camera internals via GetView matrix inverse
//	glm::mat4 view = m_camera->GetView();
//	glm::mat4 invView = glm::inverse(view);
//	camPos = glm::vec3(invView[3]);
//	glUniform3f(m_viewPos, camPos.x, camPos.y, camPos.z);
//	glUniform3f(m_lightColor, 1.0f, 0.95f, 0.8f);
//	glUniform1f(m_ambientStr, 0.15f);
//	glUniform1f(m_specStr, 0.5f);
//
//	// Render sky sphere first with depth writes disabled
//	if (m_skySphere != NULL) {
//		glDepthMask(GL_FALSE);
//		glUniformMatrix4fv(m_modelMatrix, 1, GL_FALSE, glm::value_ptr(m_skySphere->GetModel()));
//		if (m_skySphere->hasTex) {
//			glActiveTexture(GL_TEXTURE0);
//			glBindTexture(GL_TEXTURE_2D, m_skySphere->getTextureID());
//			GLuint sampler = m_shader->GetUniformLocation("sp");
//			glUniform1i(sampler, 0);
//			m_skySphere->Render(m_positionAttrib, m_colorAttrib, m_tcAttrib, m_hasTexture);
//		}
//		glDepthMask(GL_TRUE);
//	}
//
//	// Render the objects
//	/*if (m_cube != NULL){
//		glUniformMatrix4fv(m_modelMatrix, 1, GL_FALSE, glm::value_ptr(m_cube->GetModel()));
//		m_cube->Render(m_positionAttrib,m_colorAttrib);
//	}*/
//
//	if (m_mesh != NULL) {
//		glUniform1i(m_hasTexture, false);
//		glUniformMatrix4fv(m_modelMatrix, 1, GL_FALSE, glm::value_ptr(m_mesh->GetModel()));
//		if (m_mesh->hasTex) {
//			glActiveTexture(GL_TEXTURE0);
//			glBindTexture(GL_TEXTURE_2D, m_mesh->getTextureID()); // corrected; previously it was m_sphere->getTextureID()
//			GLuint sampler = m_shader->GetUniformLocation("sp");
//			if (sampler == INVALID_UNIFORM_LOCATION)
//			{
//				printf("Sampler Not found not found\n");
//			}
//			glUniform1i(sampler, 0);
//			m_mesh->Render(m_positionAttrib, m_colorAttrib, m_tcAttrib, m_hasTexture);
//		}
//	}
//
//	/*if (m_pyramid != NULL) {
//		glUniformMatrix4fv(m_modelMatrix, 1, GL_FALSE, glm::value_ptr(m_pyramid->GetModel()));
//		m_pyramid->Render(m_positionAttrib, m_colorAttrib);
//	}*/
//
//	if (m_sphere != NULL) {
//		glUniformMatrix4fv(m_modelMatrix, 1, GL_FALSE, glm::value_ptr(m_sphere->GetModel()));
//		if (m_sphere->hasTex) {
//			glActiveTexture(GL_TEXTURE0);
//			glBindTexture(GL_TEXTURE_2D, m_sphere->getTextureID());
//			GLuint sampler = m_shader->GetUniformLocation("sp");
//			if (sampler == INVALID_UNIFORM_LOCATION)
//			{
//				printf("Sampler Not found not found\n");
//			}
//			glUniform1i(sampler, 0);
//			m_sphere->Render(m_positionAttrib, m_colorAttrib, m_tcAttrib, m_hasTexture);
//		}
//	}
//
//	if (m_sphere2 != NULL) {
//		glUniformMatrix4fv(m_modelMatrix, 1, GL_FALSE, glm::value_ptr(m_sphere2->GetModel()));
//		if (m_sphere2->hasTex) {
//			glActiveTexture(GL_TEXTURE0);
//			glBindTexture(GL_TEXTURE_2D, m_sphere2->getTextureID());
//			GLuint sampler = m_shader->GetUniformLocation("sp");
//			if (sampler == INVALID_UNIFORM_LOCATION)
//			{
//				printf("Sampler Not found not found\n");
//			}
//			glUniform1i(sampler, 0);
//			m_sphere2->Render(m_positionAttrib, m_colorAttrib, m_tcAttrib, m_hasTexture);
//		}
//	}
//
//
//	// Render Moon
//	if (m_sphere3 != NULL) {
//		glUniformMatrix4fv(m_modelMatrix, 1, GL_FALSE, glm::value_ptr(m_sphere3->GetModel()));
//		if (m_sphere3->hasTex) {
//			glActiveTexture(GL_TEXTURE0);
//			glBindTexture(GL_TEXTURE_2D, m_sphere3->getTextureID());
//			GLuint sampler = m_shader->GetUniformLocation("sp");
//			if (sampler == INVALID_UNIFORM_LOCATION)
//			{
//				printf("Sampler Not found not found\n");
//			}
//			glUniform1i(sampler, 0);
//			m_sphere3->Render(m_positionAttrib, m_colorAttrib, m_tcAttrib, m_hasTexture);
//		}
//	}
//
//	// Get any errors from OpenGL
//	auto error = glGetError();
//	if (error != GL_NO_ERROR)
//	{
//		string val = ErrorString(error);
//		std::cout << "Error initializing OpenGL! " << error << ", " << val << std::endl;
//	}
//}

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

	GLuint sampler = m_shader->GetUniformLocation("sp");
	glUniform1i(sampler, 0);

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
	}

	// ── HELPER LAMBDA to render any sphere ────────────────────────────
	auto renderSphere = [&](Sphere* s) {
		if (s == NULL) return;
		glUniformMatrix4fv(m_modelMatrix, 1, GL_FALSE,
			glm::value_ptr(s->GetModel()));
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, s->getTextureID());
		s->Render(m_positionAttrib, m_colorAttrib, m_tcAttrib, m_hasTexture);
		};

	// ── PLANETS ───────────────────────────────────────────────────────
	renderSphere(m_mercury);
	renderSphere(m_venus);
	renderSphere(m_sphere2);   // Earth
	renderSphere(m_sphere3);   // Moon
	renderSphere(m_mars);
	renderSphere(m_jupiter);
	renderSphere(m_saturn);
	renderSphere(m_uranus);
	renderSphere(m_neptune);

	// ── SATURN RING ───────────────────────────────────────────────────
	glUniformMatrix4fv(m_modelMatrix, 1, GL_FALSE,
		glm::value_ptr(m_ringModel));
	RenderRing();

	// ── STARSHIP ──────────────────────────────────────────────────────
	if (m_mesh != NULL && m_mesh->hasTex) {
		glUniform1f(m_ambientStr, 0.4f); // ship slightly brighter
		glUniformMatrix4fv(m_modelMatrix, 1, GL_FALSE,
			glm::value_ptr(m_mesh->GetModel()));
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_mesh->getTextureID());
		glUniform1i(sampler, 0);
		m_mesh->Render(m_positionAttrib, m_colorAttrib,
			m_tcAttrib, m_hasTexture);
		glUniform1f(m_ambientStr, 0.15f);
	}

	auto error = glGetError();
	if (error != GL_NO_ERROR)
		std::cout << "OpenGL Error: " << ErrorString(error) << std::endl;
}

bool Graphics::collectShPrLocs() {
	bool anyProblem = true;
	// Locate the projection matrix in the shader
	m_projectionMatrix = m_shader->GetUniformLocation("projectionMatrix");
	if (m_projectionMatrix == INVALID_UNIFORM_LOCATION)
	{
		printf("m_projectionMatrix not found\n");
		anyProblem = false;
	}

	// Locate the view matrix in the shader
	m_viewMatrix = m_shader->GetUniformLocation("viewMatrix");
	if (m_viewMatrix == INVALID_UNIFORM_LOCATION)
	{
		printf("m_viewMatrix not found\n");
		anyProblem = false;
	}

	// Locate the model matrix in the shader
	m_modelMatrix = m_shader->GetUniformLocation("modelMatrix");
	if (m_modelMatrix == INVALID_UNIFORM_LOCATION)
	{
		printf("m_modelMatrix not found\n");
		anyProblem = false;
	}

	// Locate the position vertex attribute
	m_positionAttrib = m_shader->GetAttribLocation("v_position");
	if (m_positionAttrib == -1)
	{
		printf("v_position attribute not found\n");
		anyProblem = false;
	}

	// Locate the color vertex attribute
	m_colorAttrib = m_shader->GetAttribLocation("v_color");
	if (m_colorAttrib == -1)
	{
		printf("v_color attribute not found\n");
		anyProblem = false;
	}

	// Locate the color vertex attribute
	m_tcAttrib = m_shader->GetAttribLocation("v_tc");
	if (m_tcAttrib == -1)
	{
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

	return anyProblem;
}

std::string Graphics::ErrorString(GLenum error)
{
	if (error == GL_INVALID_ENUM)
	{
		return "GL_INVALID_ENUM: An unacceptable value is specified for an enumerated argument.";
	}

	else if (error == GL_INVALID_VALUE)
	{
		return "GL_INVALID_VALUE: A numeric argument is out of range.";
	}

	else if (error == GL_INVALID_OPERATION)
	{
		return "GL_INVALID_OPERATION: The specified operation is not allowed in the current state.";
	}

	else if (error == GL_INVALID_FRAMEBUFFER_OPERATION)
	{
		return "GL_INVALID_FRAMEBUFFER_OPERATION: The framebuffer object is not complete.";
	}

	else if (error == GL_OUT_OF_MEMORY)
	{
		return "GL_OUT_OF_MEMORY: There is not enough memory left to execute the command.";
	}
	else
	{
		return "None";
	}
}

