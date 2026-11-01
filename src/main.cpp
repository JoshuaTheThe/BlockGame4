
#include "glad.h"
#include "renderer.hpp"
#include "compute.hpp"

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

  constexpr GLuint N = 256;
  // 1. Create the output SSBO (uninitialized storage).
  GpuBuffer outBuf(N * sizeof(uint32_t), nullptr);

  // 2. Compile the compute shader.
  ComputeShader cs(readFile("compute.glsl").c_str());
  // 3. Bind the SSBO to binding point 0 (matches `layout(binding = 0)`).
  cs.bindBuffer(0, outBuf.handle());
  
  // 4. Dispatch. 256 invocations / 64 per group = 4 workgroups.
  cs.dispatch(N / 64);
  // 5. Make the writes visible to the CPU, then force completion so
  //    we can read back immediately. (In a real pipeline, use a fence
  //    and read back next frame instead of glFinish.)
  cs.barrier(GL_SHADER_STORAGE_BARRIER_BIT);
  glFinish();
  // 6. Read back and verify.
  std::vector<uint32_t> got(N);
  outBuf.readback(got.data());
  bool ok = true;
  for (GLuint i = 0; i < N; ++i) {
      if (got[i] != i) { ok = false; break; }
  }
  printf("compute %s: got[0]=%u got[%u]=%u\n",
         ok ? "OK" : "FAILED", got[0], N - 1, got[N - 1]);

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
