#include "Helpers.h"

#include <iostream>
#include <fstream>

void VertexArrayObject::init()
{
  glGenVertexArrays(1, &id);
  std::cout << id << std::endl;
  check_gl_error();
}

void VertexArrayObject::bind()
{
  glBindVertexArray(id);
  check_gl_error();
}

void VertexArrayObject::free()
{
  glDeleteVertexArrays(1, &id);
  check_gl_error();
}

void VertexBufferObject::init()
{
  glGenBuffers(1,&id);
  std::cout << id << std::endl;
  check_gl_error();
}

void VertexBufferObject::bind()
{
  glBindBuffer(GL_ARRAY_BUFFER,id);
  check_gl_error();
}

void VertexBufferObject::free()
{
  glDeleteBuffers(1,&id);
  check_gl_error();
}

void VertexBufferObject::updateV(std::vector<float>& vertices)
{
  assert(id != 0);
  glBindBuffer(GL_ARRAY_BUFFER, id);
  glBufferData(GL_ARRAY_BUFFER, sizeof(float)*vertices.size(), vertices.data(), GL_STATIC_DRAW);
  check_gl_error();
}

void VertexBufferObject::updateSubV(std::vector<float>& vertex, int i)
{
    assert(id != 0);
    glBindBuffer(GL_ARRAY_BUFFER, id);
    glBufferSubData(GL_ARRAY_BUFFER, i*sizeof(float)*6,sizeof(float)*vertex.size(), vertex.data());
    check_gl_error();
}

void VertexBufferObject::updateE(std::vector<int>& indices)
{
  assert(id != 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(float)*indices.size(), indices.data(), GL_STATIC_DRAW);
  check_gl_error();
}

void VertexBufferObject::updateN(std::vector<glm::vec3>& normals)
{
  assert(id != 0);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(float)*normals.size(), normals.data(), GL_DYNAMIC_DRAW);
  check_gl_error();
}

bool Program::init(
  const std::string &vertex_shader_string,
  const std::string &fragment_shader_string,
  const std::string &fragment_data_name)
{
  using namespace std;
  vertex_shader = create_shader_helper(GL_VERTEX_SHADER, vertex_shader_string);
  fragment_shader = create_shader_helper(GL_FRAGMENT_SHADER, fragment_shader_string);

  if (!vertex_shader || !fragment_shader)
    return false;

  program_shader = glCreateProgram();
  glEnable(GL_PROGRAM_POINT_SIZE);

  glAttachShader(program_shader, vertex_shader);
  glAttachShader(program_shader, fragment_shader);

  glBindFragDataLocation(program_shader, 0, fragment_data_name.c_str());
  glLinkProgram(program_shader);

  GLint status;
  glGetProgramiv(program_shader, GL_LINK_STATUS, &status);

  if (status != GL_TRUE)
  {
    char buffer[512];
    glGetProgramInfoLog(program_shader, 512, NULL, buffer);
    cerr << "Linker error: " << endl << buffer << endl;
    program_shader = 0;
    return false;
  }

  check_gl_error();
  return true;
}

void Program::bind()
{
  glUseProgram(program_shader);
  //std::cout << "Debug! program bind()" << std::endl;
  check_gl_error();
}

GLint Program::attrib(const std::string &name) const
{
  return glGetAttribLocation(program_shader, name.c_str());
}

GLint Program::uniform(const std::string &name) const
{
  return glGetUniformLocation(program_shader, name.c_str());
}

GLint Program::bindVertexAttribArray(
        const std::string &name, VertexBufferObject& VBO) const
{
  GLint id = attrib(name);
  if (id < 0)
    return id;
  if (VBO.id == 0)
  {
     std::cout << "Inside!" << std::endl;
    glDisableVertexAttribArray(id);
    return id;
  }
  std::cout << "ID:  " << id << std::endl;
  std::cout << "OUTSide! " << VBO.id << std::endl;
  VBO.bind();
  glEnableVertexAttribArray(id);
  glVertexAttribPointer(id, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
  check_gl_error();

  return id;
}

void Program::free()
{
  if (program_shader)
  {
    glDeleteProgram(program_shader);
    program_shader = 0;
  }
  if (vertex_shader)
  {
    glDeleteShader(vertex_shader);
    vertex_shader = 0;
  }
  if (fragment_shader)
  {
    glDeleteShader(fragment_shader);
    fragment_shader = 0;
  }
  check_gl_error();
}

GLuint Program::create_shader_helper(GLint type, const std::string &shader_string)
{
  using namespace std;
  if (shader_string.empty())
    return (GLuint) 0;

  GLuint id = glCreateShader(type);
  const char *shader_string_const = shader_string.c_str();
  glShaderSource(id, 1, &shader_string_const, NULL);
  glCompileShader(id);

  GLint status;
  glGetShaderiv(id, GL_COMPILE_STATUS, &status);

  if (status != GL_TRUE)
  {
    char buffer[512];
    if (type == GL_VERTEX_SHADER)
      cerr << "Vertex shader:" << endl;
    else if (type == GL_FRAGMENT_SHADER)
      cerr << "Fragment shader:" << endl;
    else if (type == GL_GEOMETRY_SHADER)
      cerr << "Geometry shader:" << endl;
    cerr << shader_string << endl << endl;
    glGetShaderInfoLog(id, 512, NULL, buffer);
    cerr << "Error: " << endl << buffer << endl;
    return (GLuint) 0;
  }
  check_gl_error();

  return id;
}

void _check_gl_error(const char *file, int line)
{
  GLenum err (glGetError());

  while(err!=GL_NO_ERROR)
  {
    std::string error;

    switch(err)
    {
      case GL_INVALID_OPERATION:      error="INVALID_OPERATION";      break;
      case GL_INVALID_ENUM:           error="INVALID_ENUM";           break;
      case GL_INVALID_VALUE:          error="INVALID_VALUE";          break;
      case GL_OUT_OF_MEMORY:          error="OUT_OF_MEMORY";          break;
      case GL_INVALID_FRAMEBUFFER_OPERATION:  error="INVALID_FRAMEBUFFER_OPERATION";  break;
    }

    std::cerr << "GL_" << error.c_str() << " - " << file << ":" << line << std::endl;
    err = glGetError();
  }
}

const char* getShader(const char *path){
        std::string code;
        std::ifstream file;
        // ensure ifstream objects can throw exceptions:
        file.exceptions (std::ifstream::failbit | std::ifstream::badbit);
        try
        {
            // open files
            file.open(path);
            std::stringstream shaderStream;
            std::cout << "Inside!" << std:: endl;
            // read file's buffer contents into streams
            shaderStream << file.rdbuf();
            std:: cout << file.rdbuf() << std::endl;
            // close file handlers
            file.close();
            // convert stream into string
            code = shaderStream.str();
        }
        catch (std::ifstream::failure e)
        {
            std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
        }
        std::cout << "size: " << code.size() << std::endl;
        const char* shaderCode = code.c_str();
        return shaderCode;
}