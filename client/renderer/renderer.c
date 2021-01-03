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

static AttributeID attribute_id(Renderer* renderer, const char* name) {
  AttributeID id = shader_var(&renderer->shader, &renderer->vgl, name);
  if (id == -1) {
    LOG_ERROR("No such attribute in shader: %s", name);
  }
  return id;
}

static bool renderer_init_shader(Renderer* renderer) {
  // A program that runs on GPU for each vertex
  // It determines the color of vertices and their mappings to texture (if any)
  const char* vertex_shader =
    "#version 300 es\n"
    "uniform mat4 projection;\n"
    "in vec2 in_pos;\n"
    "in vec2 in_uv;\n"
    "in vec4 in_color;\n"
    "out vec2 pixel_uv;\n"
    "out vec4 pixel_color;\n"
    "void main() {\n"
    "  pixel_uv = in_uv;\n"
    "  pixel_color = in_color;\n"
    "  gl_Position = projection * vec4(in_pos.xy, 0, 1);\n"
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

  renderer->attributes.pos   = attribute_id(renderer, "in_pos");
  renderer->attributes.uv    = attribute_id(renderer, "in_uv");
  renderer->attributes.color = attribute_id(renderer, "in_color");

  if (renderer->attributes.pos == -1 || renderer->attributes.uv == -1 || renderer->attributes.color == -1) {
    return false;
  }

  renderer->attributes.projection = shader_uniform(&renderer->shader, &renderer->vgl, "projection");
  renderer->attributes.texture    = shader_uniform(&renderer->shader, &renderer->vgl, "tex");

  // TODO: check that uniform variables were found

  return true;
}

static bool renderer_init_context(Renderer* renderer, SDL_Window* window) {
  renderer->window = window;

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

static bool renderer_setup_buffers(Renderer* renderer) {
  // TODO: check errors?
  ObjectID vertex_array;
  ObjectID vertices;
  ObjectID indices;

  renderer->vgl.glGenVertexArrays(1, &vertex_array);
  renderer->vgl.glGenBuffers(1, &vertices);
  renderer->vgl.glGenBuffers(1, &indices);

  renderer->vgl.glBindVertexArray(vertex_array);
  renderer->vgl.glBindBuffer(GL_ARRAY_BUFFER, vertices);
  renderer->vgl.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices);

  renderer->vgl.glEnableVertexAttribArray(renderer->attributes.pos);
  renderer->vgl.glVertexAttribPointer(renderer->attributes.pos, 2, GL_FLOAT, GL_FALSE,
                                      sizeof(Vertex), (const void*)offsetof(Vertex, position));

  renderer->vgl.glEnableVertexAttribArray(renderer->attributes.uv);
  renderer->vgl.glVertexAttribPointer(renderer->attributes.uv, 2, GL_FLOAT, GL_FALSE,
                                      sizeof(Vertex), (const void*)offsetof(Vertex, uv));

  renderer->vgl.glEnableVertexAttribArray(renderer->attributes.color);
  renderer->vgl.glVertexAttribPointer(renderer->attributes.color, 4, GL_UNSIGNED_BYTE, GL_TRUE,
                                      sizeof(Vertex), (const void*)offsetof(Vertex, color));

  renderer->vertex_array = vertex_array;
  renderer->vertices = vertices;
  renderer->indices = indices;

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
    LOG_ERROR("Failed to initialize shader");
    return -1;
  }

  if (!renderer_setup_buffers(renderer)) {
    LOG_ERROR("Failed to initialize vertex buffers");
    return -1;
  }

  // TODO
  // if (!renderer_load_textures(renderer)) {
  //    ...
  // }

  return 0;
}

static void renderer_release_buffers(Renderer* renderer) {
  renderer->vgl.glDeleteBuffers(1, &renderer->indices);
  renderer->vgl.glDeleteBuffers(1, &renderer->vertices);
  renderer->vgl.glDeleteVertexArrays(1, &renderer->vertex_array);
}

void renderer_close(Renderer* renderer) {
  renderer_release_buffers(renderer);
  shader_release(&renderer->shader, &renderer->vgl);
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
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  // render everything
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

  // present the new frame
  SDL_GL_SwapWindow(renderer->window);
}
