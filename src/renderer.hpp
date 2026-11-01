
#pragma once

extern "C" {

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_surface.h>
#include <SDL3_image/SDL_image.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_video.h>

#include "glad.h"

};

#include <fstream>
#include <sstream>
#include <unordered_map>

#include <functional>
#include <glm/glm.hpp>
#include <vector>
#include <string>

namespace BlockGame4 {  
  template<typename T>
  using Vec=std::vector<T>;
  
  class Vertex {
  private:
    glm::vec3 pos;
    glm::vec2 uv;
    glm::vec3 normal;
  public:
    Vertex(glm::vec3 p, glm::vec2 uv, glm::vec3 normal) : pos(p),uv(uv),normal(normal) {}
    const auto &gPos(void) const {
      return pos;
    }

    const auto &gUv(void) const {
      return uv;
    }

    const auto &gNormal(void) const {
      return normal;
    }

    static constexpr auto gPosOffset(void) {
      return offsetof(Vertex, pos);
    }

    static constexpr auto gUvOffset(void) {
      return offsetof(Vertex, uv);
    }

    static constexpr auto gNormalOffset(void) {
      return offsetof(Vertex, normal);
    }
  };

  class InternalError {
  public:
    enum class Class {
      None=0,
      FileNotFound,
      CompileError,
      LinkError,
      NoProgram,
      NoSuchSymbol,
      InvalidImage,
      AllocationFailiure,
      EmptyImage,
      InitFail,
    };
    
    InternalError(InternalError&) = default;
    InternalError(InternalError&&) = default;
    InternalError(const InternalError&) = default;
    InternalError(Class c, std::string p) : Class_(c), Payload_(p) {}
    ~InternalError() {}
    auto gClass(void) const { return Class_; }
    auto gPayload(void) const {return Payload_; }
  private:
    Class Class_;
    std::string Payload_;
  };

  class ShaderProgram {
  private:
    int prog_{-1};
    int compile(int type, const char *src) {
      GLuint sh = glCreateShader(type);
      glShaderSource(sh, 1, &src, NULL);
      glCompileShader(sh);
      GLint ok = 0;
      glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
      if (!ok) {
        GLint len = 0;
        glGetShaderiv(sh, GL_INFO_LOG_LENGTH, &len);
        std::string log(len+1, '\0');
        glGetShaderInfoLog(sh, len, NULL, &log[0]);
        glDeleteShader(sh);
        throw InternalError(
          InternalError::Class::CompileError, std::move(log));
      }

      return sh;
    }
    
    int link(int vs, int fs) {
      GLuint prog = glCreateProgram();
      glAttachShader(prog, vs);
      glAttachShader(prog, fs);
      glLinkProgram(prog);
      GLint ok = 0;
      glGetProgramiv(prog, GL_LINK_STATUS, &ok);
      if (!ok) {
        GLint len = 0;
        glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);
        std::string log(len+1, '\0');
        glGetProgramInfoLog(prog, len, NULL, &log[0]);
        glDeleteProgram(prog);
        throw InternalError(
          InternalError::Class::LinkError, std::move(log));
      }
      return prog;
    }
    
  public:
    ShaderProgram(ShaderProgram&) = delete;
    ShaderProgram(ShaderProgram&&) = delete;
    ShaderProgram(const ShaderProgram&) = delete;
    ShaderProgram(const ShaderProgram&&) = delete;
   
    ShaderProgram(const char *v_src, const char *f_src) {
      int vs = compile(GL_VERTEX_SHADER, v_src);
      int fs = compile(GL_FRAGMENT_SHADER, f_src);
      prog_ = link(vs, fs);
      glDetachShader(prog_, vs);
      glDetachShader(prog_, fs);
      glDeleteShader(fs);
      glDeleteShader(vs);
    }
    
    ~ShaderProgram() {
      if (prog_ != -1)
        glDeleteProgram(prog_);
    }

    inline void apply(void) {
      if (prog_ == -1) {
        throw InternalError(
          InternalError::Class::NoProgram,
          "No such GL program is bound to ShaderProgram");
      }
      glUseProgram(prog_);
    }
    
    inline int gLocation(const char* sym) {
      if (prog_ == -1) {
        throw InternalError(
          InternalError::Class::NoProgram,
          "No such GL program is bound to ShaderProgram");
      }
      
      int off = glGetUniformLocation(prog_, sym);
      if (off == -1) {
        throw InternalError(
          InternalError::Class::NoSuchSymbol,
          sym);
      }
      return off;
    }

    inline int gProgram(void) const {
      if (prog_ == -1) {
        throw InternalError(
          InternalError::Class::NoProgram,
          "No such GL program is bound to ShaderProgram");
      }
      return prog_;
    }
  };

  class TextureObject {
  private:
    int handle_{-1}; // OpenGL handle
  public:
    auto gHandle(void) {
      if (handle_ == -1) {
        throw InternalError(
          InternalError::Class::EmptyImage,
          "No Image present in TextureObject");
      }
      return handle_;
    }

    inline void apply(void) {
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, handle_);
    }

    ~TextureObject() {
      if (handle_ != -1) {
        GLuint h=handle_;
        glDeleteTextures(1, &h);
        handle_ = -1;
      }
    }
    
    TextureObject(const char *texturePath) {
      SDL_Surface *raw = IMG_Load(texturePath);
      if (!raw) {
        throw InternalError(
          InternalError::Class::EmptyImage,
          SDL_GetError());
      }
  
      SDL_Surface *surf = SDL_ConvertSurface(raw, SDL_PIXELFORMAT_RGBA32);
      SDL_DestroySurface(raw);
      if (!surf) {
        throw InternalError(
          InternalError::Class::EmptyImage,
          SDL_GetError());
      }
  
      GLuint tex = 0;
      glGenTextures(1, &tex);
      glBindTexture(GL_TEXTURE_2D, tex);
      glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
      glTexImage2D(
        GL_TEXTURE_2D,
        0,                  // mip level
        GL_RGBA8,           // internal format
        surf->w, surf->h,
        0,                  // border
        GL_RGBA,            // source format
        GL_UNSIGNED_BYTE,   // source type
        surf->pixels);
  
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      glBindTexture(GL_TEXTURE_2D, 0);
      SDL_DestroySurface(surf);
      handle_ = static_cast<int>(tex);
    }

    TextureObject(TextureObject&) = delete;
    TextureObject(TextureObject&&) = delete;
    TextureObject(const TextureObject&) = delete;
    TextureObject(const TextureObject&&) = delete;
  };
  
  class MeshObject {
  private:
    std::vector<Vertex> v_;
    std::vector<uint32_t> i_;
    GLuint vao{-1u},vbo{-1u},ebo{-1u};
    struct ObjKey {
      int p, t, n;
      bool operator==(const ObjKey& o) const {
        return p == o.p && t == o.t && n == o.n;
      }
    };
  
    struct ObjKeyHash {
      std::size_t operator()(const ObjKey& k) const noexcept {
      std::size_t h = std::hash<int>()(k.p);
        h = h * 31u + std::hash<int>()(k.t);
        h = h * 31u + std::hash<int>()(k.n);
        return h;
      }
    };
  public:
    void loadFromObjFile(const std::string& path) {
      std::ifstream in(path);
      if (!in) {
          throw InternalError(
              InternalError::Class::FileNotFound,
              path);
      }
  
      std::vector<glm::vec3> positions;
      std::vector<glm::vec2> uvs;
      std::vector<glm::vec3> normals;
  
      v_.clear();
      i_.clear();
      std::unordered_map<ObjKey, uint32_t, ObjKeyHash> dedup;
  
      auto resolve = [](int i, std::size_t count) -> int {
          if (i > 0) return i - 1;
          if (i < 0) return static_cast<int>(count) + i;
          return -1;
      };
  
      auto getOrAdd = [&](const ObjKey& k) -> uint32_t {
          auto it = dedup.find(k);
          if (it != dedup.end()) return it->second;
  
          glm::vec3 p(0.0f);
          glm::vec2 t(0.0f);
          glm::vec3 n(0.0f, 0.0f, 1.0f);
          if (k.p >= 0 && k.p < (int)positions.size()) p = positions[k.p];
          if (k.t >= 0 && k.t < (int)uvs.size())       t = uvs[k.t];
          if (k.n >= 0 && k.n < (int)normals.size())   n = normals[k.n];
  
          uint32_t idx = static_cast<uint32_t>(v_.size());
          v_.emplace_back(p, t, n);
          dedup.emplace(k, idx);
          return idx;
      };
  
      std::string rawLine, line;
      while (std::getline(in, rawLine)) {
          if (!rawLine.empty() && rawLine.back() == '\r') rawLine.pop_back();
  
          line += rawLine;
          if (!rawLine.empty() && rawLine.back() == '\\') {
              line.pop_back();
              continue;
          }
  
          std::size_t first = line.find_first_not_of(" \t");
          if (first == std::string::npos || line[first] == '#') {
              line.clear();
              continue;
          }
  
          std::istringstream ss(line);
          std::string tag;
          ss >> tag;
  
          if (tag == "v") {
              float x = 0, y = 0, z = 0;
              ss >> x >> y >> z;
              positions.emplace_back(x, y, z);
          } else if (tag == "vt") {
              float u = 0, v = 0;
              ss >> u >> v;
              uvs.emplace_back(u, v);
          } else if (tag == "vn") {
              float x = 0, y = 0, z = 0;
              ss >> x >> y >> z;
              normals.emplace_back(x, y, z);
          } else if (tag == "f") {
              std::vector<ObjKey> face;
              std::string tok;
              while (ss >> tok) {
                  int p = 0, t = 0, n = 0;
                  std::size_t s1 = tok.find('/');
                  if (s1 == std::string::npos) {
                      p = std::stoi(tok);
                  } else {
                      if (s1 > 0) p = std::stoi(tok.substr(0, s1));
                      std::size_t s2 = tok.find('/', s1 + 1);
                      if (s2 == std::string::npos) {
                          if (s1 + 1 < tok.size())
                              t = std::stoi(tok.substr(s1 + 1));
                      } else {
                          if (s2 > s1 + 1)
                              t = std::stoi(tok.substr(s1 + 1, s2 - s1 - 1));
                          if (s2 + 1 < tok.size())
                              n = std::stoi(tok.substr(s2 + 1));
                      }
                  }
                  ObjKey k;
                  k.p = resolve(p, positions.size());
                  k.t = resolve(t, uvs.size());
                  k.n = resolve(n, normals.size());
                  face.push_back(k);
              }
  
              if (face.size() < 3) {
                  line.clear();
                  continue;
              }
  
              uint32_t i0 = getOrAdd(face[0]);
              for (std::size_t j = 1; j + 1 < face.size(); ++j) {
                  uint32_t i1 = getOrAdd(face[j]);
                  uint32_t i2 = getOrAdd(face[j + 1]);
                  i_.push_back(i0);
                  i_.push_back(i1);
                  i_.push_back(i2);
              }
          }
  
          line.clear();
      }
    }
    
    MeshObject(MeshObject&) = delete;
    MeshObject(MeshObject&&) = delete;
    MeshObject(const MeshObject&) = delete;
    MeshObject(const MeshObject&&) = delete;
    MeshObject() : v_({}),i_({}) {
      glGenVertexArrays(1, &vao);
      glGenBuffers(1, &vbo);
      glGenBuffers(1, &ebo);
      glBindVertexArray(vao);
    }
    
    ~MeshObject() {
      if (vao != 0) glDeleteVertexArrays(1, &vao);
      if (vbo != 0) glDeleteBuffers(1, &vbo);
      if (ebo != 0) glDeleteBuffers(1, &ebo);
    }

    void bindVertexData(Vec<Vertex> v, Vec<uint32_t> i) {
      v_ = v;
      i_ = i;
    }
    
    void synchronize(void) {
      // upload vertex
      glBindVertexArray(this->vao);
      glBindBuffer(GL_ARRAY_BUFFER, this->vbo);
      glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex)*v_.size(),&v_[0], GL_STATIC_DRAW);
  
      // upload indicies
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->ebo);
      glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t)*i_.size(),&i_[0], GL_STATIC_DRAW);
  
      // describe vertex format to shader
      glEnableVertexAttribArray(0);
      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)Vertex::gPosOffset());
      glEnableVertexAttribArray(1);
      glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)Vertex::gUvOffset());
      glEnableVertexAttribArray(2);
      glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)Vertex::gNormalOffset());
      glBindVertexArray(0);
    }

    inline const Vertex* gVertStart(void) {
      return &v_[0];
    }

    inline const Vertex* gVertEnd(void) {
      return &v_[v_.size()-1];
    }

    inline const uint32_t* gIndStart(void) {
      return &i_[0];
    }

    inline const uint32_t* gIndEnd(void) {
      return &i_[i_.size()-1];
    }

    inline const uint32_t gVao(void) const {
      return vao;
    }

    inline const uint32_t gVbo(void) const {
      return vbo;
    }
    
    inline const uint32_t gEbo(void) const {
      return ebo;
    }

    inline const size_t gIndCardinality(void) const {
      return i_.size();
    }

    inline const size_t gVertCardinality(void) const {
      return v_.size();
    }
  };

  class Renderer {
  private:
    SDL_Window* Win_{nullptr};
    SDL_Renderer* Ren_{nullptr};
    SDL_GLContext GL_{0};
    int W_{0}, H_{0};
    const char *T_{nullptr};
  public:
    Renderer(Renderer&) = delete;
    Renderer(Renderer&&) = delete;
    Renderer(const Renderer&) = delete;
    Renderer(const Renderer&&) = delete;

    ~Renderer() {
      SDL_GL_DestroyContext(GL_);
      if(Ren_)
        SDL_DestroyRenderer(Ren_);
      if(Win_)
        SDL_DestroyWindow(Win_);
      SDL_Quit();
    }

    bool windowShouldClose(void) {
      SDL_Event e;
      while (SDL_PollEvent(&e)) {
        if (e.type == SDL_EVENT_QUIT) {
          return true;
        }
      }

      return false;
    }
    
    Renderer(const char *title="Bg4",
             const uint32_t w=800,
             const uint32_t h=600) {
      W_=w;
      H_=h;
      T_=title;

      SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
      SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
      SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  
      SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
      
      if (!SDL_Init(0)) {
        throw InternalError(
          InternalError::Class::InitFail,
          SDL_GetError());
      }

      if(!(Win_=SDL_CreateWindow(T_, W_, H_, SDL_WINDOW_OPENGL))) {
        throw InternalError(
          InternalError::Class::InitFail,
          SDL_GetError());
      }

      if (!(Ren_=SDL_CreateRenderer(Win_, NULL))) {
        throw InternalError(
          InternalError::Class::InitFail,
          SDL_GetError());
      }

      if(!(GL_=SDL_GL_CreateContext(Win_))){
        throw InternalError(
          InternalError::Class::InitFail,
          SDL_GetError());
      }

      if (!SDL_GL_MakeCurrent(Win_, GL_)) {
        throw InternalError(
          InternalError::Class::InitFail,
          SDL_GetError());
      }

      if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        throw InternalError(
          InternalError::Class::InitFail,
          "Failed to load GL/4.6 procedures"
        );
      }

      const char* ver = (const char*)glGetString(GL_VERSION);
      if (!ver) {
        printf("GL_VERSION is NULL, context is dead\n");
      } else {
        printf("GL_VERSION = %s\n", ver);
      }

      glEnable(GL_DEPTH_TEST);
      glDepthFunc(GL_LEQUAL);
    }

    inline void flip(void) const {
      SDL_GL_SwapWindow(Win_);
    }

    void drawMesh(const MeshObject& m) {
      glBindVertexArray(m.gVao());
      glDrawElements(GL_TRIANGLES,
        m.gIndCardinality(), GL_UNSIGNED_INT, NULL);
    }
  };
};
