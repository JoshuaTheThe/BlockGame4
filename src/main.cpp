
#include "glad.h"
#include "renderer.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace BlockGame4;

int main(void) {
  Renderer r{};
  MeshObject m{};
  m.loadFromObjFile("monkey.obj");
  m.synchronize();
  ShaderProgram sh(
    "#version 330 core\n"
    "layout(location = 0) in vec3 aPos;\n"
    "layout(location = 1) in vec2 aUV;\n"
    "layout(location = 2) in vec3 aNormal;\n"
    "uniform mat4 uModel;\n"
    "uniform mat4 uView;\n"
    "uniform mat4 uProjection;\n"
    "out vec2 vUV;\n"
    "out vec3 vNormal;\n"
    "void main() {\n"
    "  vUV = aUV;\n"
    "  vNormal = aNormal;\n"
    "  gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);\n"
    "}\n",
    
    "#version 330 core\n"
    "in vec2 vUV;\n"
    "in vec3 vNormal;\n"
    "out vec4 fragColor;\n"
    "void main() {\n"
    "  float linearDepth = (2.0 * 0.1 * 100.0) / (100.0 + 0.1 - gl_FragCoord.z * (100.0 - 0.1));\n"
    "  vec3 n = normalize(vNormal);\n"
    "  vec3 color = n * 0.5 + 0.5;\n"      // map [-1,1] -> [0,1]
    "  float depthFade = 1.0 - smoothstep(1.0, 100.0, linearDepth);\n"
    "  fragColor = vec4(color * depthFade, 1.0);\n"
    "}\n");
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
