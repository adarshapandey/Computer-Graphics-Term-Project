# Solar System Exploration Game
### CS480/680 — Computer Graphics Final Project

A real-time interactive solar system simulation built with OpenGL 4.6. Fly a starship
through the solar system in Exploration Mode, or switch to Planetary Observation Mode
to orbit and inspect any celestial body up close.

---

## Controls

### Exploration Mode (default)
| Key / Input | Action |
|---|---|
| `W` | Accelerate forward |
| `S` | Brake |
| `A` / `D` | Yaw left / right |
| `↑` / `↓` Arrow | Pitch up / down |
| `Q` / `E` | Roll left / right |
| Mouse move | Steer ship (yaw + pitch) |
| Scroll wheel | Zoom (FoV) |
| `TAB` | Switch to Planetary Observation Mode |
| `V` | Switch to Cockpit (first-person) view |
| `ESC` | Quit |

### Planetary Observation Mode (`TAB`)
| Key / Input | Action |
|---|---|
| Mouse move | Rotate orbit camera around planet |
| Scroll wheel | Zoom in / out |
| `N` | Cycle to next celestial body |
| `B` | Cycle to previous celestial body |
| `TAB` | Return to Exploration Mode |

### Cockpit Mode (`V`)
| Key / Input | Action |
|---|---|
| Mouse move | Free-look from ship nose |
| `V` | Return to Exploration Mode |

### Selectable Bodies in Planetary Mode
Sun, Mercury, Venus, Earth, Moon, Mars, Jupiter, Saturn, Uranus, Neptune,
Halley's Comet, Inner Asteroid Belt, Outer Asteroid Belt, Pluto, Haumea, Eris, Ceres

---

## Dependencies

These libraries match the course project template and require no additional setup
beyond what was already configured for the programming assignments.

#### OpenGL
- **Purpose:** Core graphics API for all rendering — shaders, buffers, textures, draw calls.
- **Version:** OpenGL 4.6 (GLSL `#version 460`)
- **Included via:** System OpenGL drivers + GLEW

#### GLEW — OpenGL Extension Wrangler Library
- **Purpose:** Loads OpenGL extensions at runtime. Required on Windows/Linux to access
  any OpenGL function beyond 1.1.
- **Header:** `#include <GL/glew.h>`
- **Used in:** `graphics_headers.h`, `graphics.cpp`
- **Notes:** Must be initialized before any OpenGL call — `glewInit()` is called in
  `Graphics::Initialize()` immediately after the window is created.

#### GLFW — Graphics Library Framework
- **Purpose:** Window creation, OpenGL context management, keyboard/mouse input callbacks.
- **Header:** `#include <GLFW/glfw3.h>`
- **Used in:** `window.h`, `engine.cpp`
- **Key usage:** `glfwSetCursorPosCallback`, `glfwSetScrollCallback`,
  `glfwSetInputMode(GLFW_CURSOR_DISABLED)` to capture the mouse.

#### GLM — OpenGL Mathematics
- **Purpose:** Vector and matrix math (vec2, vec3, vec4, mat4). Used for all transforms,
  orbital mechanics, camera calculations, and shader data.
- **Header:** `#include <glm/glm.hpp>` and extensions
- **Used everywhere:** `graphics.cpp`, `camera.cpp`, `shader.cpp`, `sphere.cpp`, `mesh.cpp`
- **Extensions used:**
  - `glm/gtc/matrix_transform.hpp` — `glm::translate`, `glm::rotate`, `glm::scale`, `glm::lookAt`, `glm::perspective`
  - `glm/gtc/type_ptr.hpp` — `glm::value_ptr` for passing matrices to OpenGL uniforms
  - `glm/gtx/rotate_vector.hpp` — vector rotation utilities

#### Assimp — Open Asset Import Library
- **Purpose:** Loads 3D model files. Used specifically to load the starship OBJ file
  (`SpaceShip-1.obj`) in `Mesh::loadModelFromFile()`.
- **Header:** `#include <assimp/Importer.hpp>`, `<assimp/scene.h>`, `<assimp/postprocess.h>`
- **Used in:** `mesh.cpp`
- **Key flag:** `aiProcess_Triangulate` — ensures all faces are triangles regardless
  of the source polygon count.

#### SOIL2 — Simple OpenGL Image Library 2
- **Purpose:** Loads image files (JPG, PNG) and uploads them directly to OpenGL textures.
  Used for all diffuse textures and normal maps.
- **Header:** `#include <SOIL2/SOIL2.h>`
- **Used in:** `texture.cpp`
- **Key call:** `SOIL_load_OGL_texture(fileName, SOIL_LOAD_AUTO, SOIL_CREATE_NEW_ID, SOIL_FLAG_INVERT_Y)`
- **Note:** `SOIL_FLAG_INVERT_Y` flips the image vertically on load to match OpenGL's
  bottom-left texture coordinate origin.

---

### Not Used in Programming Assignments

No additional third-party libraries beyond the ones listed above are used in this project.
All features — procedural sphere generation, asteroid belt instancing, Saturn ring mesh,
comet tail, normal mapping, per-material lighting — are implemented directly using
the standard course libraries.

---

## Project Structure

```
project/
├── assets/                  # All textures and models
│   ├── 2k_sun.jpg
│   ├── 2k_earth_daymap.jpg
│   ├── 2k_earth_daymap-n.jpg
│   ├── 2k_moon.jpg
│   ├── 2k_moon-n.jpg
│   ├── Mercury.jpg / Mercury-n.jpg
│   ├── Venus.jpg / Venus-n.jpg
│   ├── Mars.jpg / Mars-n.jpg
│   ├── Jupiter.jpg / Jupiter-n.jpg
│   ├── Saturn.jpg / Saturn-n.jpg / Saturn_ring.png
│   ├── Uranus.jpg / Uranus-n.jpg
│   ├── Neptune.jpg / Neptune-n.jpg
│   ├── Pluto.png / pluto-n.jpg
│   ├── Haumea.jpg / Haumea-n.jpg
│   ├── Eris.jpg / Eris-n.jpg
│   ├── Ceres.jpg / Ceres-n.jpg
│   ├── HalleysComet.jpg
│   ├── asteroid.jpg / asteroid-n.png
│   ├── Galaxy.jpg
│   ├── SpaceShip-1.obj
│   └── SpaceShip-1.png
│
├── camera.h / camera.cpp    # All three camera modes (third-person, orbit, cockpit)
├── engine.h / engine.cpp    # Game loop, input processing, mode switching
├── graphics.h / graphics.cpp# Scene setup, orbital mechanics, full render pipeline
├── shader.h / shader.cpp    # GLSL vertex + fragment shader (normal mapping, fill light, emissive)
├── sphere.h / sphere.cpp    # Procedural UV sphere generation
├── mesh.h / mesh.cpp        # OBJ model loader (Assimp)
├── object.h / object.cpp    # Base renderable object
├── texture.h / texture.cpp  # Texture loading (SOIL2)
├── window.h / window.cpp    # GLFW window wrapper
├── graphics_headers.h       # Common includes and Vertex struct
└── main.cpp                 # Entry point
```

---

## Building the Project

The project uses the same build configuration as the course programming assignments
(CMake or Visual Studio project). No additional CMake entries are needed beyond what
was already set up for PA1–PA5 since all dependencies are the same.

If building fresh, ensure the following are linked:
```
opengl32 (or libGL on Linux)
glew32 (or libGLEW)
glfw3
assimp
SOIL2
```

All header search paths should point to the same include directories used in the
programming assignments.

---

## Features Implemented

- Full solar system: Sun, 8 planets, Moon, 4 dwarf planets (Pluto, Haumea, Eris, Ceres)
- Correct axial tilts and orbital mechanics for all bodies
- Halley's Comet with elliptical orbit and solar-wind-driven tail direction
- Inner asteroid belt (300 instances) and outer belt (500 instances) — pseudo-instancing
- Normal map texturing on all planets that have normal maps
- Per-material specular color and shininess
- Saturn procedural ring mesh with alpha transparency
- Blinn-Phong lighting with sun as point light source
- Fill light in Planetary Observation Mode (cool blue-white for night-side visibility)
- Emissive thruster glow on starship that scales with speed (red → orange → white)
- Three camera modes: third-person smooth follow, orbit (planetary), cockpit first-person
- Skysphere background (Galaxy.jpg)