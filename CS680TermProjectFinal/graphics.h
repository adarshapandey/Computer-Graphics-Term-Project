#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <iostream>
#include <stack>
using namespace std;

#include "graphics_headers.h"
#include "camera.h"
#include "shader.h"
#include "object.h"
#include "sphere.h"
#include "mesh.h"

#define numVBOs 2;
#define numIBs 2;


class Graphics
{
  public:
    Graphics();
    ~Graphics();
    bool Initialize(int width, int height);
    void HierarchicalUpdate2(double absoluteTime, float dt);

    void Render();

    Camera* getCamera() { return m_camera; }
    // Ship getters for third-person camera
    glm::vec3 getShipPosition() { return m_shipPosition; }
    glm::vec3 getShipForward()  { return m_shipForward; }
    glm::vec3 getShipUp()       { return m_shipUp; }
    glm::vec3 getShipRight()    { return m_shipRight; }

    // Input
    enum ShipKey { FWD, BACK, LEFT, RIGHT, ROLL_L, ROLL_R, PITCH_UP, PITCH_DOWN };
    void setKeyState(int key, bool pressed);
    void setMouseDelta(float dx, float dy);

    // Planetary Observation mode
    // Indices: 0=Sun 1=Mercury 2=Venus 3=Earth 4=Moon 5=Mars 6=Jupiter 7=Saturn 8=Uranus 9=Neptune
    static constexpr int NUM_BODIES = 10;
    glm::vec3 GetSelectedBodyPos()         { return m_bodyPos[m_selectedBody]; }
    float     GetSelectedBodyOrbitRadius() { return m_bodyOrbitRadius[m_selectedBody]; }
    void      CycleBody(int dir)           { m_selectedBody = (m_selectedBody + dir + NUM_BODIES) % NUM_BODIES; }
    void      SetPlanetaryMode(bool pm)    { m_planetaryMode = pm; }

  private:
    std::string ErrorString(GLenum error);
    // Saturn ring raw GL buffers (since Mesh needs .obj file)
    std::vector<Vertex>       m_ringVertices;
    std::vector<unsigned int> m_ringIndices;
    Texture* m_ringTexture = nullptr;
    glm::mat4                 m_ringModel = glm::mat4(1.f);
    GLuint m_ringVAO = 0, m_ringVBO = 0, m_ringIBO = 0;
    void createRingMesh(float innerR, float outerR,
        int segments, const char* texFile);
    void RenderRing();

    bool collectShPrLocs();
    void ComputeTransforms (double dt, std::vector<float> speed, std::vector<float> dist,
        std::vector<float> rotSpeed, glm::vec3 rotVector, std::vector<float> scale, 
        glm::mat4& tmat, glm::mat4& rmat, glm::mat4& smat);

    stack<glm::mat4> modelStack;

    Camera *m_camera;
    Shader *m_shader;

    GLint m_projectionMatrix;
    GLint m_viewMatrix;
    GLint m_modelMatrix;
    GLint m_positionAttrib;
    GLint m_colorAttrib;
    GLint m_tcAttrib;
    GLint m_hasTexture;

    // Lighting uniform locations
    GLint m_lightPos;
    GLint m_viewPos;
    GLint m_lightColor;
    GLint m_ambientStr;
    GLint m_specStr;

    // Fill light (night-side / planetary observation)
    GLint m_uFillLightPos;
    GLint m_uFillLightColor;
    GLint m_uFillStrength;

    // Emissive thruster glow (speed-driven)
    GLint m_uEmissiveColor;
    GLint m_uEmissiveStrength;
    GLint m_uEmissiveMaskMin;   // model-space box lower corner
    GLint m_uEmissiveMaskMax;   // model-space box upper corner

    // Celestial body world positions and suggested orbit radii
    glm::vec3 m_bodyPos[NUM_BODIES]         = {};
    float     m_bodyOrbitRadius[NUM_BODIES] = {};
    int       m_selectedBody                = 3; // default: Earth
    bool      m_planetaryMode               = false;


    Sphere* m_sphere;
    Sphere* m_sphere2;
    Sphere* m_sphere3;

    // Solar system bodies
    Sphere* m_mercury;
    Sphere* m_venus;
    Sphere* m_mars;
    Sphere* m_jupiter;
    Sphere* m_saturn;
    Sphere* m_uranus;
    Sphere* m_neptune;

    // Sky sphere
    Sphere* m_skySphere;

    // Saturn ring mesh
    Mesh* m_saturnRing;

    Mesh* m_mesh;



    // Starship state
    glm::vec3 m_shipPosition  = glm::vec3(0.f, 0.f, -15.f);
    glm::vec3 m_shipForward   = glm::vec3(0.f, 0.f,  1.f);
    glm::vec3 m_shipUp        = glm::vec3(0.f, 1.f,  0.f);
    glm::vec3 m_shipRight     = glm::vec3(1.f, 0.f,  0.f);
    float m_shipYaw           = 0.f;
    float m_shipPitch         = 0.f;
    float m_shipRoll          = 0.f;
    float m_shipSpeed         = 0.f;
    float m_shipMaxSpeed      = 20.f;
    float m_shipAccel         = 5.f;

    // Input state
    bool m_keyFwd = false, m_keyBack = false, m_keyLeft = false, m_keyRight = false,
        m_keyRollL = false, m_keyRollR = false,
        m_keyPitchUp = false, m_keyPitchDown = false;
    float m_mouseDX = 0.f, m_mouseDY = 0.f;

    void UpdateShip(float dt, bool fwd, bool back, bool left, bool right, bool rollLeft, bool rollRight, bool pitchUp, bool pitchDown, float mouseDX, float mouseDY);



};

#endif /* GRAPHICS_H */
