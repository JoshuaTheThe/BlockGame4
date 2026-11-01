#pragma once

#include "renderer.hpp"

namespace BlockGame4 {

// Owns a GL_SHADER_STORAGE_BUFFER. Does not own shader binding state —
// you bind it to a binding point each dispatch (or once, if it's stable).
class GpuBuffer {
private:
  GLuint    buf_{0};
  GLsizeiptr  size_{0};
  GLbitfield  flags_{GL_DYNAMIC_DRAW};

public:
  GpuBuffer(GpuBuffer&) = delete;
  GpuBuffer(GpuBuffer&&) = delete;
  GpuBuffer(const GpuBuffer&) = delete;
  GpuBuffer(const GpuBuffer&&) = delete;

  // size in bytes; data may be nullptr for uninitialized storage
  explicit GpuBuffer(GLsizeiptr size,
             const void* data = nullptr,
             GLbitfield flags = GL_DYNAMIC_DRAW)
    : size_(size), flags_(flags) {
    glGenBuffers(1, &buf_);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buf_);
    glBufferData(GL_SHADER_STORAGE_BUFFER, size_, data, flags_);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
  }

  ~GpuBuffer() {
    if (buf_) glDeleteBuffers(1, &buf_);
  }

  // Upload the whole buffer. Reallocates if size changed, else sub-updates.
  void upload(const void* data, GLsizeiptr size) {
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buf_);
    if (size != size_) {
      glBufferData(GL_SHADER_STORAGE_BUFFER, size, data, flags_);
      size_ = size;
    } else {
      glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, size, data);
    }
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
  }

  // Bind to a shader binding point (matches `layout(binding = N)`)
  void bind(GLuint bindingPoint) const {
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindingPoint, buf_);
  }

  // Blocking readback. Call only after a fence has signaled AND a
  // glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT) has been issued.
  // size == 0 reads the whole buffer.
  void readback(void* dst, GLsizeiptr size = 0) const {
    GLsizeiptr n = (size == 0) ? size_ : size;
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, buf_);
    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, n, dst);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
  }

  GLuint    handle(void) const { return buf_; }
  GLsizeiptr  size(void)   const { return size_; }
  GLbitfield  flags(void)  const { return flags_; }
};

class ComputeShader {
private:
  int prog_{-1};

  int compile(const char* src) {
    GLuint sh = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(sh, 1, &src, NULL);
    glCompileShader(sh);
    GLint ok = 0;
    glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok) {
      GLint len = 0;
      glGetShaderiv(sh, GL_INFO_LOG_LENGTH, &len);
      char buf[4096] = {0};
      GLsizei written = 0;
      glGetShaderInfoLog(sh, sizeof(buf) - 1, &written, buf);
      glDeleteShader(sh);
      throw InternalError(InternalError::Class::CompileError,
                std::string(buf, written > 0 ? written : 0));
    }
    return sh;
  }

public:
  ComputeShader(ComputeShader&) = delete;
  ComputeShader(ComputeShader&&) = delete;
  ComputeShader(const ComputeShader&) = delete;
  ComputeShader(const ComputeShader&&) = delete;

  explicit ComputeShader(const char* src) {
    int cs = compile(src);
    prog_ = glCreateProgram();
    glAttachShader(prog_, cs);
    glLinkProgram(prog_);
    glDeleteShader(cs);

    GLint ok = 0;
    glGetProgramiv(prog_, GL_LINK_STATUS, &ok);
    if (!ok) {
      char buf[4096] = {0};
      GLsizei written = 0;
      glGetProgramInfoLog(prog_, sizeof(buf) - 1, &written, buf);
      glDeleteProgram(prog_);
      prog_ = -1;
      throw InternalError(InternalError::Class::LinkError,
                std::string(buf, written > 0 ? written : 0));
    }
  }

  ~ComputeShader() {
    if (prog_ != -1) glDeleteProgram(prog_);
  }

  inline void apply(void) {
    if (prog_ == -1) {
      throw InternalError(InternalError::Class::NoProgram,
                "No compute program");
    }
    glUseProgram(prog_);
  }

  // Dispatch. gx/gy/gz are WORKGROUP counts, not invocation counts.
  // Total invocations = gx*gy*gz * local_size (declared in the shader).
  inline void dispatch(GLuint gx, GLuint gy = 1, GLuint gz = 1) {
    apply();
    glDispatchCompute(gx, gy, gz);
  }

  // Must be called after dispatch and before any consumer (readback,
  // texture sampling, draw) sees the results. Bits depend on what
  // the shader wrote: SSBO -> GL_SHADER_STORAGE_BARRIER_BIT,
  // image -> GL_SHADER_IMAGE_ACCESS_BARRIER_BIT, VBO -> GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT,
  // indirect args -> GL_COMMAND_BARRIER_BIT.
  inline void barrier(GLbitfield bits) {
    glMemoryBarrier(bits);
  }

  // Uniform helpers. Each calls glUseProgram first; fine for setup
  // and per-dispatch, cheap but not free.
  inline void setInt(const char* name, int v) {
    glUseProgram(prog_);
    glUniform1i(gLocation(name), v);
  }
  inline void setUint(const char* name, GLuint v) {
    glUseProgram(prog_);
    glUniform1ui(gLocation(name), v);
  }
  inline void setFloat(const char* name, float v) {
    glUseProgram(prog_);
    glUniform1f(gLocation(name), v);
  }
  inline void setVec3(const char* name, float x, float y, float z) {
    glUseProgram(prog_);
    glUniform3f(gLocation(name), x, y, z);
  }
  inline void setIVec3(const char* name, int x, int y, int z) {
    glUseProgram(prog_);
    glUniform3i(gLocation(name), x, y, z);
  }

  // Bind an SSBO to a shader binding point (layout(binding = N)).
  // Accepts the raw handle so this header doesn't depend on GpuBuffer.
  inline void bindBuffer(GLuint bindingPoint, GLuint bufHandle) {
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, bindingPoint, bufHandle);
  }

  // Bind an image texture to an image unit (layout(binding = N) uniform image2D).
  inline void bindImage(GLuint unit, GLuint tex,
              GLenum access, GLenum format,
              GLboolean layered = GL_FALSE,
              GLint layer = 0) {
    glBindImageTexture(unit, tex, 0, layered, layer, access, format);
  }

  inline int gLocation(const char* sym) {
    if (prog_ == -1) {
      throw InternalError(InternalError::Class::NoProgram,
                "No compute program");
    }
    int off = glGetUniformLocation(prog_, sym);
    if (off == -1) {
      throw InternalError(InternalError::Class::NoSuchSymbol, sym);
    }
    return off;
  }

  inline int gProgram(void) const { return prog_; }
};

} // namespace BlockGame4
