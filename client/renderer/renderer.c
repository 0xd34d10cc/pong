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
  unsigned char color[4];
} Vertex;

static_assert(12 == sizeof(Vertex), "Unexpected padding in Vertex");

static AttributeID var_id(Renderer* renderer, const char* name, bool is_uniform) {
  AttributeID id = is_uniform ? shader_uniform(&renderer->shader, name)
                              : shader_var(&renderer->shader, name);
  if (id == -1) {
    const char* uniform = is_uniform ? " uniform" : "";
    LOG_ERROR("No%s variable named \"%s\" in shader", uniform, name);
    return -1;
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
    "in vec4 in_color;\n"
    "out vec4 pixel_color;\n"
    "void main() {\n"
    "  pixel_color = in_color;\n"
    "  gl_Position = projection * vec4(in_pos.xy, 0, 1);\n"
    "}\n";

  // A program that runs on GPU for each pixel
  // It determines the resulting color of the pixel on screen
  const char* pixel_shader =
    "#version 300 es\n"
    "precision mediump float;\n" // TODO: do we need this?
    "in vec4 pixel_color;\n"
    "out vec4 out_color;\n"
    "void main() {\n"
    "  out_color = pixel_color;\n"
    "}\n";

  if (shader_compile(&renderer->shader, vertex_shader, pixel_shader) < 0) {
    // The error is logged in shader_compile()
    return false;
  }

  renderer->attributes.pos   = var_id(renderer, "in_pos", false);
  renderer->attributes.color = var_id(renderer, "in_color", false);

  if (renderer->attributes.pos == -1 || renderer->attributes.color == -1) {
    return false;
  }

  renderer->attributes.projection = var_id(renderer, "projection", true);
  if (renderer->attributes.projection == -1) {
    return false;
  }

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
    LOG_WARN("Failed to initialize OpenGL debug logging");
  }
#endif
  return true;
}

static int MAX_VERTICES = 4096;
static int MAX_INDICES = 4096;

static bool renderer_init_buffers(Renderer* renderer) {
  vertex_buffer_init(&renderer->vertices, sizeof(Vertex) * MAX_VERTICES);
  vertex_buffer_bind(&renderer->vertices);

  index_buffer_init(&renderer->indices, sizeof(unsigned int) * MAX_INDICES);
  index_buffer_bind(&renderer->indices);

  // TODO: abstract
  ObjectID vertex_array;
  vgl.glGenVertexArrays(1, &vertex_array);
  vgl.glBindVertexArray(vertex_array);

  vgl.glEnableVertexAttribArray(renderer->attributes.pos);
  vgl.glVertexAttribPointer(renderer->attributes.pos, 2, GL_FLOAT, GL_FALSE,
                           sizeof(Vertex), (const void*)offsetof(Vertex, position));

  vgl.glEnableVertexAttribArray(renderer->attributes.color);
  vgl.glVertexAttribPointer(renderer->attributes.color, 4, GL_UNSIGNED_BYTE, GL_TRUE,
                            sizeof(Vertex), (const void*)offsetof(Vertex, color));

  renderer->vertex_array = vertex_array;

  return true;
}

int renderer_init(Renderer* renderer, SDL_Window* window) {
  if (!renderer_init_context(renderer, window)) {
    return -1;
  }

  if (vgl_load() < 0) {
    LOG_ERROR("Failed to load all required OpenGL functions");
    return -1;
  }

  if (!renderer_init_shader(renderer)) {
    LOG_ERROR("Failed to initialize shader");
    return -1;
  }

  if (!renderer_init_buffers(renderer)) {
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
  index_buffer_release(&renderer->indices);
  vertex_buffer_release(&renderer->vertices);
  vgl.glDeleteVertexArrays(1, &renderer->vertex_array);
}

void renderer_close(Renderer* renderer) {
  renderer_release_buffers(renderer);
  shader_release(&renderer->shader);
  SDL_GL_DeleteContext(renderer->context);
}

static void render_rectangle(Renderer* renderer, Rectangle rect) {
  float x = rect.position.x;
  float y = rect.position.y;
  float w = rect.size.x;
  float h = rect.size.y;

  Vertex rect_vertices[] = {
    { .position = {x,     y    }, .color = {0xff, 0xff, 0xff} },
    { .position = {x + w, y    }, .color = {0xff, 0xff, 0xff} },
    { .position = {x,     y + h}, .color = {0xff, 0xff, 0xff} },
    { .position = {x + w, y + h}, .color = {0xff, 0xff, 0xff} }
  };

  vertex_buffer_bind(&renderer->vertices);
  Vertex* vertices = vertex_buffer_map(&renderer->vertices);
  memcpy(vertices, rect_vertices, sizeof(rect_vertices));
  vertex_buffer_unmap(&renderer->vertices);

  unsigned int rect_indices[] = {0, 1, 2, 1, 2, 3};

  index_buffer_bind(&renderer->indices);
  unsigned int* indices = index_buffer_map(&renderer->indices);
  memcpy(indices, rect_indices, sizeof(rect_indices));
  index_buffer_unmap(&renderer->indices);

  // setup shader variables
  shader_bind(&renderer->shader);

  float ortho[4][4] = {
    { 1.0f,  0.0f,  0.0f, 0.0f},
    { 0.0f,  1.0f,  0.0f, 0.0f},
    { 0.0f,  0.0f,  1.0f, 0.0f},
    { 0.0f,  0.0f,  0.0f, 1.0f}
  };
  vgl.glUniformMatrix4fv(renderer->attributes.projection, 1, GL_FALSE, &ortho[0][0]);

  // actually draw
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

static void render_lost(Renderer* renderer) {
  (void)renderer;
  // draw lose screen
}

static void render_running(Renderer* renderer, Game* game) {
  render_rectangle(renderer, game->player);
  render_rectangle(renderer, game->opponent);
  render_rectangle(renderer, game->ball);
}

static void renderer_clear(Renderer* renderer) {
  (void)renderer;
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
  glClear(GL_COLOR_BUFFER_BIT);
}

static void renderer_present(Renderer* renderer) {
  // for (RenderCommand* cmd = render_queue_begin(renderer->queue), ...)
  //    renderer_execute(cmd)

  SDL_GL_SwapWindow(renderer->window);
}

void renderer_render(Renderer* renderer, Game* game) {
  renderer_clear(renderer);

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

  renderer_present(renderer);
}
