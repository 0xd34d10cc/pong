#include "renderer.h"
#include "game/game.h"
#include "log.h"

#include <assert.h>

#include <SDL2/SDL_video.h>

#ifdef PONG_DEBUG

static const char* gl_type_str(GLenum type) {
  switch (type) {
    case GL_DEBUG_TYPE_ERROR:
      return "error";
    case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
      return "deprecated";
    case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
      return "UB";
    case GL_DEBUG_TYPE_PORTABILITY:
      return "unportable";
    case GL_DEBUG_TYPE_PERFORMANCE:
      return "perf";
    case GL_DEBUG_TYPE_MARKER:
      return "cmdstream";
    case GL_DEBUG_TYPE_PUSH_GROUP:
      return "begin";
    case GL_DEBUG_TYPE_POP_GROUP:
      return "end";
    case GL_DEBUG_TYPE_OTHER:
      return "other";
    default:
      return "unknown";
  }
}

static void GLAPIENTRY opengl_debug_log(
    GLenum source,
    GLenum type,
    GLuint id,
    GLenum severity,
    GLsizei len,
    const GLchar* message,
    const void* user_param
) {
  (void)source;
  (void)id;
  (void)len;
  (void)user_param;

  const char* t = gl_type_str(type);
  switch (severity) {
    case GL_DEBUG_SEVERITY_LOW:
      LOG_INFO("[GL] [%s] %s", t, message);
      break;
    case GL_DEBUG_SEVERITY_MEDIUM:
      LOG_WARN("[GL] [%s] %s", t, message);
      break;
    case GL_DEBUG_SEVERITY_HIGH:
      LOG_ERROR("[GL] [%s] %s", t, message);
      break;
    case GL_DEBUG_SEVERITY_NOTIFICATION:
      // ignore, these are purerly informational
      break;
    default:
      LOG_ERROR("[GL] [???] %s", message);
      break;
  }
}

#endif

typedef struct {
  float position[2];
  float uv[2];
  unsigned char color[4];
} Vertex;

static_assert(20 == sizeof(Vertex), "Unexpected padding in Vertex");

static bool renderer_init_shader(Renderer* renderer) {
  // A program that runs on GPU for each vertex
  // It determines the color of vertices and their mappings to texture (if any)
  const char* vertex_shader =
    "#version 300 es\n"
    "uniform mat4 projection;\n"
    "in vec2 pos;\n"
    "in vec2 uv;\n"
    "in vec4 color;\n"
    "out vec2 pixel_uv;\n"
    "out vec4 pixel_color;\n"
    "void main() {\n"
    "  pixel_uv = uv;\n"
    "  pixel_color = projection * vec4(pos.xy, 0, 1);\n"
    "}\n";

  // A program that runs on GPU for each pixel
  // It determines the resulting color of the pixel on screen
  const char* pixel_shader =
    "#version 300 es\n"
    "precision mediump float;\n" // TODO: do we need this?
    "uniform sampler2D tex;\n"
    "in vec2 pixel_uv;\n"
    "in vec4 pixel_color;\n"
    "out vec4 out_color;\n"
    "void main() {\n"
    "  out_color = pixel_color * texture(tex, pixel_uv.st);\n"
    "}\n";

  if (shader_compile(&renderer->shader, &renderer->vgl, vertex_shader, pixel_shader) < 0) {
    // The error is logged in shader_compile()
    return false;
  }

  renderer->attributes.pos   = shader_var(&renderer->shader, &renderer->vgl, "pos");
  renderer->attributes.uv    = shader_var(&renderer->shader, &renderer->vgl, "uv");
  renderer->attributes.color = shader_var(&renderer->shader, &renderer->vgl, "color");

  renderer->attributes.projection = shader_uniform(&renderer->shader, &renderer->vgl, "projection");
  renderer->attributes.texture    = shader_uniform(&renderer->shader, &renderer->vgl, "tex");
  return true;
}

static bool renderer_init_context(Renderer* renderer, SDL_Window* window) {
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

#ifdef PONG_DEBUG
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
#endif

  // Enable VSync, not sure if it actually should be enabled by default
  SDL_GL_SetSwapInterval(1);

  renderer->context = SDL_GL_CreateContext(window);
  if (!renderer->context) {
    LOG_ERROR("Failed to initialie OpenGL context: %s", SDL_GetError());
    return false;
  }

#ifdef PONG_DEBUG
  PFNGLDEBUGMESSAGECALLBACKPROC set_debug_callback = SDL_GL_GetProcAddress("glDebugMessageCallback");
  if (set_debug_callback) {
    glEnable(GL_DEBUG_OUTPUT);
    set_debug_callback(opengl_debug_log, NULL);
  }
  else {
    LOG_WARN("[GL] Failed to initialize debug logging");
  }
#endif
  return true;
}

int renderer_init(Renderer* renderer, SDL_Window* window) {
  if (!renderer_init_context(renderer, window)) {
    return -1;
  }

  if (vgl_load(&renderer->vgl) < 0) {
    LOG_ERROR("Failed to load all required OpenGL functions");
    return -1;
  }

  if (!renderer_init_shader(renderer)) {
    return -1;
  }

  // TODO: setup vertex/index buffers
  return 0;
}

void renderer_close(Renderer* renderer) {
  SDL_GL_DeleteContext(renderer->context);
}

static void render_lost(Renderer* renderer) {
  (void)renderer;
  // draw lose screen
}

static void render_running(Renderer* renderer, Game* game) {
  (void)renderer;
  (void)game;
  // draw player, ball and opponent
}

void renderer_render(Renderer* renderer, Game* game) {
  // clear screen

  switch (game_state(game)) {
    case STATE_LOST:
      render_lost(renderer);
      break;
    case STATE_RUNNING:
      render_running(renderer, game);
      break;
    case STATE_WON:
      // TODO: render
      break;
  }

  // swap
}
