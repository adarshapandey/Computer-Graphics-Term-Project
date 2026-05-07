// shader.cpp
// Compiles and links the GLSL vertex and fragment shaders.
// The fragment shader implements Phong lighting with a sun point-light, an optional
// fill light for Planetary Observation mode, and an emissive term for engine glow.

#include "shader.h"

Shader::Shader()
{
  m_shaderProg = 0;
}

Shader::~Shader()
{
  for (std::vector<GLuint>::iterator it = m_shaderObjList.begin() ; it != m_shaderObjList.end() ; it++)
  {
    glDeleteShader(*it);
  }

  if (m_shaderProg != 0)
  {
    glDeleteProgram(m_shaderProg);
    m_shaderProg = 0;
  }
}

bool Shader::Initialize()
{
  m_shaderProg = glCreateProgram();

  if (m_shaderProg == 0) 
  {
    std::cerr << "Error creating shader program\n";
    return false;
  }

  return true;
}

// Use this method to add shaders to the program. When finished - call finalize()
bool Shader::AddShader(GLenum ShaderType)
{
  std::string s;

  if (ShaderType == GL_VERTEX_SHADER)
  {
      s = "#version 460\n"
          "layout (location = 0) in vec3 v_position;\n"
          "layout (location = 1) in vec3 v_color;\n"
          "layout (location = 2) in vec2 v_tc;\n"
          "out vec3 v_normal;\n"
          "out vec3 v_fragPos;\n"
          "out vec3 v_localPos;\n"
          "out vec3 color;\n"
          "out vec2 tc;\n"
          "uniform mat4 projectionMatrix;\n"
          "uniform mat4 viewMatrix;\n"
          "uniform mat4 modelMatrix;\n"
          "void main(void)\n"
          "{\n"
          "  vec4 v = vec4(v_position, 1.0);\n"
          "  gl_Position = (projectionMatrix * viewMatrix * modelMatrix) * v;\n"
          "  v_normal  = mat3(transpose(inverse(modelMatrix))) * v_color;\n"
          "  v_fragPos = vec3(modelMatrix * v);\n"
          "  v_localPos = v_position;\n"
          "  color = v_color;\n"
          "  tc = v_tc;\n"
          "}\n";
  }
  else if (ShaderType == GL_FRAGMENT_SHADER)
  {
      s = "#version 460\n"
          "uniform sampler2D sp;\n"
          "uniform bool hasTexture;\n"
          "uniform vec3 lightPos;\n"
          "uniform vec3 viewPos;\n"
          "uniform vec3 lightColor;\n"
          "uniform float ambientStrength;\n"
          "uniform float specularStrength;\n"
          "uniform vec3 fillLightPos;\n"
          "uniform vec3 fillLightColor;\n"
          "uniform float fillStrength;\n"
          "uniform vec3  emissiveColor;\n"
          "uniform float emissiveStrength;\n"
          "uniform vec3  emissiveMaskMin;\n"  // model-space lower corner of mask
          "uniform vec3  emissiveMaskMax;\n"  // model-space upper corner of mask
          "in vec3 v_normal;\n"
          "in vec3 v_fragPos;\n"
          "in vec3 v_localPos;\n"
          "in vec3 color;\n"
          "in vec2 tc;\n"
          "out vec4 frag_color;\n"
          "void main(void)\n"
          "{\n"
          "  vec3 texColor = hasTexture ? vec3(texture(sp, tc)) : color;\n"
          "  vec3 norm     = normalize(v_normal);\n"
          "  vec3 ambient  = ambientStrength * lightColor * texColor;\n"
          "  vec3 lightDir = normalize(lightPos - v_fragPos);\n"
          "  float diff    = max(dot(norm, lightDir), 0.0);\n"
          "  vec3 diffuse  = diff * lightColor * texColor;\n"
          "  vec3 viewDir    = normalize(viewPos - v_fragPos);\n"
          "  vec3 reflectDir = reflect(-lightDir, norm);\n"
          "  float spec      = pow(max(dot(viewDir, reflectDir), 0.0), 32.0);\n"
          "  vec3 specular   = specularStrength * spec * lightColor;\n"
          "  vec3 fillDir  = normalize(fillLightPos - v_fragPos);\n"
          "  float fillDif = max(dot(norm, fillDir), 0.0);\n"
          "  vec3 fillContr = fillStrength * fillDif * fillLightColor * texColor;\n"
          // Emissive mask: 1.0 only inside the model-space box [min, max].
          // Smooth falloff over a small inset so the glow blends naturally.
          // Set min == max to disable emissive entirely (default for non-ship draws).
          "  vec3 boxSize = max(emissiveMaskMax - emissiveMaskMin, vec3(0.0001));\n"
          "  vec3 inset   = boxSize * 0.15;\n"
          "  vec3 lo = smoothstep(emissiveMaskMin, emissiveMaskMin + inset, v_localPos);\n"
          "  vec3 hi = smoothstep(emissiveMaskMax, emissiveMaskMax - inset, v_localPos);\n"
          "  float mask = lo.x*lo.y*lo.z * hi.x*hi.y*hi.z;\n"
          "  vec3 emissive = emissiveStrength * emissiveColor * mask;\n"
          "  frag_color = vec4(ambient + diffuse + specular + fillContr + emissive, 1.0);\n"
          "}\n";
  }


  GLuint ShaderObj = glCreateShader(ShaderType);

  if (ShaderObj == 0) 
  {
    std::cerr << "Error creating shader type " << ShaderType << std::endl;
    return false;
  }

  // Save the shader object - will be deleted in the destructor
  m_shaderObjList.push_back(ShaderObj);

  const GLchar* p[1];
  p[0] = s.c_str();
  GLint Lengths[1] = { (GLint)s.size() };

  glShaderSource(ShaderObj, 1, p, Lengths);

  glCompileShader(ShaderObj);

  GLint success;
  glGetShaderiv(ShaderObj, GL_COMPILE_STATUS, &success);

  if (!success) 
  {
    GLchar InfoLog[1024];
    glGetShaderInfoLog(ShaderObj, 1024, NULL, InfoLog);
    std::cerr << "Error compiling: " << InfoLog << std::endl;
    return false;
  }

  glAttachShader(m_shaderProg, ShaderObj);

  return true;
}


// After all the shaders have been added to the program call this function
// to link and validate the program.
bool Shader::Finalize()
{
  GLint Success = 0;
  GLchar ErrorLog[1024] = { 0 };

  glLinkProgram(m_shaderProg);

  glGetProgramiv(m_shaderProg, GL_LINK_STATUS, &Success);
  if (Success == 0)
  {
    glGetProgramInfoLog(m_shaderProg, sizeof(ErrorLog), NULL, ErrorLog);
    std::cerr << "Error linking shader program: " << ErrorLog << std::endl;
    return false;
  }

  glValidateProgram(m_shaderProg);
  glGetProgramiv(m_shaderProg, GL_VALIDATE_STATUS, &Success);
  if (!Success)
  {
    glGetProgramInfoLog(m_shaderProg, sizeof(ErrorLog), NULL, ErrorLog);
    std::cerr << "Invalid shader program: " << ErrorLog << std::endl;
    return false;
  }

  // Delete the intermediate shader objects that have been added to the program
  for (std::vector<GLuint>::iterator it = m_shaderObjList.begin(); it != m_shaderObjList.end(); it++)
  {
    glDeleteShader(*it);
  }

  m_shaderObjList.clear();

  return true;
}


void Shader::Enable()
{
    glUseProgram(m_shaderProg);
}


GLint Shader::GetUniformLocation(const char* pUniformName)
{
    GLuint Location = glGetUniformLocation(m_shaderProg, pUniformName);

    if (Location == INVALID_UNIFORM_LOCATION) {
        fprintf(stderr, "Warning! Unable to get the location of uniform '%s'\n", pUniformName);
    }

    return Location;
}

GLint Shader::GetAttribLocation(const char* pAttribName)
{
    GLuint Location = glGetAttribLocation(m_shaderProg, pAttribName);

    if (Location == -1) {
        fprintf(stderr, "Warning! Unable to get the location of attribute '%s'\n", pAttribName);
    }

    return Location;
}
