#include "renderer.h"
#include "layout.h"
#include "game/game.h"
#include "log.h"
#include "ppm.h"

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
    "in vec2 in_uv;\n"
    "in vec4 in_color;\n"
    "out vec4 pixel_color;\n"
    "out vec2 uv_coords;\n"
    "void main() {\n"
    "  pixel_color = in_color;\n"
    "  uv_coords = in_uv;\n"
    "  gl_Position = projection * vec4(in_pos.xy, 0, 1);\n"
    "}\n";

  // A program that runs on GPU for each pixel
  // It determines the resulting color of the pixel on screen
  const char* pixel_shader =
    "#version 300 es\n"
    "precision mediump float;\n"
    "uniform sampler2D texture_id;\n"
    "in vec4 pixel_color;\n"
    "in vec2 uv_coords;\n"
    "out vec4 out_color;\n"
    "void main() {\n"
    "  out_color = pixel_color * texture(texture_id, uv_coords.st);\n"
    "}\n";

  if (shader_compile(&renderer->shader, vertex_shader, pixel_shader) < 0) {
    // The error is logged in shader_compile()
    return false;
  }

  renderer->attributes.pos   = var_id(renderer, "in_pos", false);
  renderer->attributes.uv    = var_id(renderer, "in_uv", false);
  renderer->attributes.color = var_id(renderer, "in_color", false);

  if (renderer->attributes.pos == -1 ||
      renderer->attributes.uv == -1 ||
      renderer->attributes.color == -1) {
    return false;
  }

  renderer->attributes.projection = var_id(renderer, "projection", true);
  renderer->attributes.texture = var_id(renderer, "texture_id", true);
  if (renderer->attributes.projection == -1 ||
      renderer->attributes.texture == -1) {
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

  VertexLayout layout;
  layout_init(&layout);
  layout_float(&layout, renderer->attributes.pos, 2);
  layout_float(&layout, renderer->attributes.uv, 2);
  layout_bytes(&layout, renderer->attributes.color, 4);

  ObjectID vertex_array;
  vgl.glGenVertexArrays(1, &vertex_array);
  layout_set(&layout, vertex_array);
  renderer->vertex_array = vertex_array;

  return true;
}

static bool load_texture(Texture* texture, const char* path) {
  PPM image;
  if (ppm_open(&image, path) < 0) {
    return false;
  }

  texture_init(texture);
  texture_bind(texture);
  texture_set(texture, image.data, image.width, image.height);
  ppm_close(&image);
  return true;
}

static bool gen_uniform_texture(Texture* texture, char r, char g, char b) {
  unsigned char color[3] = { r, g, b };
  texture_init(texture);
  texture_set(texture, color, 1, 1);
  return true;
}

static bool gen_texture(Texture* texture, TextureID id) {
  switch (id) {
    case TEXTURE_WHITE:
      return gen_uniform_texture(texture, 0xff, 0xff, 0xff);
    case TEXTURE_BLACK:
      return gen_uniform_texture(texture, 0x00, 0x00, 0x00);
    case TEXTURE_RED:
      return gen_uniform_texture(texture, 0xff, 0x00, 0x00);
    case TEXTURE_GREEN:
      return gen_uniform_texture(texture, 0x00, 0xff, 0x00);
    case TEXTURE_BLUE:
      return gen_uniform_texture(texture, 0x00, 0x00, 0xff);
    default:
      PANIC("Unable to generate texture with id = %d", id);
  }

  return false;
}

bool renderer_load_textures(Renderer* renderer) {
  const char* paths[] = {
    [TEXTURE_WHITE] = NULL,
    [TEXTURE_BLACK] = NULL,
    [TEXTURE_RED]   = NULL,
    [TEXTURE_GREEN] = NULL,
    [TEXTURE_BLUE]  = NULL,
    [TEXTURE_LOST]  = "assets/lost.ppm",
  };

  static_assert(sizeof(paths) / sizeof(*paths) == TEXTURE_MAX, "Not all texture paths are set");
  for (int i = 0; i < TEXTURE_MAX; ++i) {
    if (!paths[i]) {
      gen_texture(&renderer->textures[i], i);
      continue;
    }

    if (!load_texture(&renderer->textures[i], paths[i])) {
      return false;
    }
  }

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

  if (!renderer_load_textures(renderer)) {
    LOG_ERROR("Failed to load textures");
    return -1;
  }

  return 0;
}

static void renderer_release_buffers(Renderer* renderer) {
  index_buffer_release(&renderer->indices);
  vertex_buffer_release(&renderer->vertices);
  for (int i = 0; i < TEXTURE_MAX; ++i) {
    texture_release(&renderer->textures[i]);
  }
  vgl.glDeleteVertexArrays(1, &renderer->vertex_array);
}

void renderer_close(Renderer* renderer) {
  renderer_release_buffers(renderer);
  shader_release(&renderer->shader);
  SDL_GL_DeleteContext(renderer->context);
}

static void render_object(Renderer* renderer, GameObject* object) {
  float x = object->bbox.position.x;
  float y = object->bbox.position.y;
  float w = object->bbox.size.x;
  float h = object->bbox.size.y;

  Vertex object_vertices[] = {
    { .position = {x,     y    }, .uv = { 0.0, 0.0 }, .color = {0xff, 0xff, 0xff, 0xff} },
    { .position = {x + w, y    }, .uv = { 1.0, 0.0 }, .color = {0xff, 0xff, 0xff, 0xff} },
    { .position = {x,     y + h}, .uv = { 0.0, 1.0 }, .color = {0xff, 0xff, 0xff, 0xff} },
    { .position = {x + w, y + h}, .uv = { 1.0, 1.0 }, .color = {0xff, 0xff, 0xff, 0xff} }
  };

  vertex_buffer_bind(&renderer->vertices);
  Vertex* vertices = vertex_buffer_map(&renderer->vertices);
  memcpy(vertices, object_vertices, sizeof(object_vertices));
  vertex_buffer_unmap(&renderer->vertices);

  unsigned int rect_indices[] = {0, 1, 2, 3, 2, 1};

  index_buffer_bind(&renderer->indices);
  unsigned int* indices = index_buffer_map(&renderer->indices);
  memcpy(indices, rect_indices, sizeof(rect_indices));
  index_buffer_unmap(&renderer->indices);

  // setup shader variables
  shader_bind(&renderer->shader);

  // TODO: avoid transferring this matrix to GPU every draw call.
  //       It is constant, so it should be enough to send it a single time.
  float ortho[4][4] = {
    { 1.0f,  0.0f,  0.0f, 0.0f},
    { 0.0f,  1.0f,  0.0f, 0.0f},
    { 0.0f,  0.0f,  1.0f, 0.0f},
    { 0.0f,  0.0f,  0.0f, 1.0f}
  };
  vgl.glUniformMatrix4fv(renderer->attributes.projection, 1, GL_FALSE, &ortho[0][0]);
  vgl.glUniform1i(renderer->attributes.texture, 0);

  texture_bind(&renderer->textures[object->texture]);
  // actually draw
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
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

void renderer_render(Renderer* renderer, GameObject** objects, int n) {
  renderer_clear(renderer);

  for (int i = 0; i < n; ++i) {
    render_object(renderer, objects[i]);
  }

  renderer_present(renderer);
}
