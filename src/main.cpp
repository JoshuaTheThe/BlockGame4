
#include "glad.h"
#include "renderer.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace BlockGame4;

std::string readFile(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) {
    throw std::runtime_error("readFile: cannot open " + path);
  }
  std::ostringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

int main(void) {
  Renderer r{};
  MeshObject m{};
  m.loadFromObjFile("monkey.obj");
  m.synchronize();
  ShaderProgram sh(readFile("vertex.glsl").c_str(), readFile("fragment.glsl").c_str());
  glm::mat4 view = glm::lookAt(
      glm::vec3(0.0f, 0.0f, 0.0f),   // camera pos
      glm::vec3(0.0f, 0.0f, -10.0f),
      glm::vec3(0.0f, 1.0f, 1.0f));  // up
  glm::mat4 proj = glm::perspective(
      glm::radians(90.0f),           // fov
      800.0f / 600.0f,               // aspect (hardcode since no resize)
      0.1f, 100.0f);                 // near, far
  const int uModel = sh.gLocation("uModel");
  const int uView  = sh.gLocation("uView");
  const int uProj  = sh.gLocation("uProjection");

  int frames = 0;
  sh.apply();
  glUniformMatrix4fv(uView, 1, GL_FALSE, glm::value_ptr(view));
  glUniformMatrix4fv(uProj, 1, GL_FALSE, glm::value_ptr(proj));
  SDL_GL_SetSwapInterval(1);
  float x = SDL_GetTicks();
  while (!r.windowShouldClose()) {
    float t = SDL_GetTicks() / 100.0f;   // seconds
    glm::mat4 model = glm::rotate(
        glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -3.0f)),
        t,
        glm::vec3(0.0f, 1.0f, 0.0f));
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    sh.apply();
    glUniformMatrix4fv(uModel, 1, GL_FALSE, glm::value_ptr(model));
    r.drawMesh(m);
    r.flip();
    ++frames;
  }

  printf("avg %f frames/sec\n", (float)frames/((SDL_GetTicks()-x) / 1000.0));
  return 0;
}
