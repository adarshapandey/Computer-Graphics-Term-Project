// shader.cpp
// Compiles and links the GLSL vertex and fragment shaders.
// The fragment shader implements Phong lighting with a sun point-light,
// optional fill light, emissive engine glow, and optional normal mapping.

#include "shader.h"

Shader::Shader()
{
    m_shaderProg = 0;
}

Shader::~Shader()
{
    for (std::vector<GLuint>::iterator it = m_shaderObjList.begin(); it != m_shaderObjList.end(); it++)
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
            "layout (location = 1) in vec3 v_color;\n"      // carries vertex normals
            "layout (location = 2) in vec2 v_tc;\n"
            "out vec3 v_normal;\n"
            "out vec3 v_fragPos;\n"
            "out vec3 v_localPos;\n"                         // for emissive masking
            "out vec3 color;\n"
            "out vec2 tc;\n"
            "out mat3 TBN;\n"                                // tangent-space matrix
            "uniform mat4 projectionMatrix;\n"
            "uniform mat4 viewMatrix;\n"
            "uniform mat4 modelMatrix;\n"
            "void main(void)\n"
            "{\n"
            "  vec4 v = vec4(v_position, 1.0);\n"
            "  gl_Position = (projectionMatrix * viewMatrix * modelMatrix) * v;\n"
            "  mat3 normalMat = mat3(transpose(inverse(modelMatrix)));\n"
            "  v_normal  = normalize(normalMat * v_color);\n"
            "  v_fragPos = vec3(modelMatrix * v);\n"
            "  v_localPos = v_position;\n"
            "  color = v_color;\n"
            "  tc = v_tc;\n"

            "  // Procedural tangent for sphere geometry\n"
            "  // Sphere normal == position (unit sphere), so tangent is dP/du direction\n"
            "  vec3 N = normalize(v_color);\n"
            "  vec3 up = abs(N.y) < 0.999 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);\n"
            "  vec3 T = normalize(cross(up, N));\n"          // tangent along U direction
            "  vec3 B = cross(N, T);\n"                      // bitangent along V direction
            "  TBN = mat3(normalMat * T, normalMat * B, normalMat * N);\n"
            "}\n";
    }
    else if (ShaderType == GL_FRAGMENT_SHADER)
    {
        s = "#version 460\n"
            "uniform sampler2D sp;\n"                        // diffuse — TEXTURE0
            "uniform sampler2D normalMap;\n"                 // normal map — TEXTURE1
            "uniform bool hasTexture;\n"
            "uniform bool hasNormalMap;\n"

            "uniform vec3 lightPos;\n"
            "uniform vec3 viewPos;\n"
            "uniform vec3 lightColor;\n"
            "uniform float ambientStrength;\n"
            "uniform float specularStrength;\n"
            "uniform float shininess;\n"

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
            "in mat3 TBN;\n"

            "out vec4 frag_color;\n"

            "void main(void)\n"
            "{\n"
            "  vec3 texColor = hasTexture ? vec3(texture(sp, tc)) : color;\n"

            "  // Normal selection: normal map or vertex normal\n"
            "  vec3 norm;\n"
            "  if (hasNormalMap) {\n"
            "    vec3 nmSample = texture(normalMap, tc).rgb;\n"
            "    nmSample = nmSample * 2.0 - 1.0;\n"
            "    norm = normalize(TBN * nmSample);\n"
            "  } else {\n"
            "    norm = normalize(v_normal);\n"
            "  }\n"

            "  // Ambient\n"
            "  vec3 ambient = ambientStrength * lightColor * texColor;\n"

            "  // Main light (sun)\n"
            "  vec3 lightDir = normalize(lightPos - v_fragPos);\n"
            "  float diff    = max(dot(norm, lightDir), 0.0);\n"
            "  vec3 diffuse  = diff * lightColor * texColor;\n"

            "  // Specular\n"
            "  vec3 viewDir    = normalize(viewPos - v_fragPos);\n"
            "  vec3 reflectDir = reflect(-lightDir, norm);\n"
            "  float spec      = pow(max(dot(viewDir, reflectDir), 0.0), shininess);\n"
            "  vec3 specular   = specularStrength * spec * lightColor;\n"

            "  // Fill light (planetary observation mode)\n"
            "  vec3 fillDir  = normalize(fillLightPos - v_fragPos);\n"
            "  float fillDif = max(dot(norm, fillDir), 0.0);\n"
            "  vec3 fillContr = fillStrength * fillDif * fillLightColor * texColor;\n"

            "  // Emissive mask: 1.0 only inside model-space box [min, max]\n"
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