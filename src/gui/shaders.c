#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <GL/glut.h>

#include "file_utils.h"

GLint shader_compile(const GLchar *src, GLint src_len, GLenum type);

GLint shader_load_path(const char *path, GLenum type)
{
    GLint shader;
    void *buf;
    size_t len;

    if (file_load_path(path, &buf, &len) < 0) {
        fprintf(stderr, "Failed to open shader source < %s >\n", path);
        return 0;
    }

    shader = shader_compile((const GLchar *)buf, len, type);
    if (!shader) {
        fprintf(stderr, "Failed to compile shader < %s >\n", path);
        return 0;
    }

    return shader;
}

GLint shader_compile(const GLchar *src, GLint src_len, GLenum type)
{
    const GLchar *shader_srcs[] = {src};
    GLint shader;
    GLint compiled;
    shader = glCreateShader(type);

    glShaderSource(shader, 1, shader_srcs, &src_len);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

    if (!compiled) {
        return 0;
    }

    return shader;
}

GLint shader_compile_program(const char *vertex_src, const char *fragment_src)
{
    GLuint vertex, fragment;
    GLuint program = 0;
    GLint linked;

    program = glCreateProgram();
    if (!program) {
        fprintf(stderr, "Failed creating a new shader program\n");
        return -1;
    }

    vertex = shader_load_path(vertex_src, GL_VERTEX_SHADER);
    if (!vertex) {
        fprintf(stderr, "Failed to compile vertex shader < %s >\n", vertex_src);
        return 0;
    }

    fragment = shader_load_path(fragment_src, GL_FRAGMENT_SHADER);
    if (!fragment) {
        fprintf(stderr, "Failed to compile fragment shader < %s >\n", fragment_src);
        return 0;
    }

    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);
    glGetShaderiv(program, GL_LINK_STATUS, &linked);
    if (!linked) {
        int loglen;
        fprintf(stderr, "Failed to link shader program:\n");
        glGetShaderiv(program, GL_INFO_LOG_LENGTH, &loglen);
        if (loglen > 0) {
            char *info = (char *) malloc(loglen);
            glGetShaderInfoLog(program, loglen, NULL, info);
            free(info);
        }
        return 0;
    }

    glDetachShader(program, vertex);
    glDetachShader(program, fragment);

    return program;
}

#if 0
GLint shader_compile_resources()
{
    GLuint vertex, fragment;
    GLuint program = 0;
    GLint linked;

    program = glCreateProgram();
    if (!program) {
        fprintf(stderr, "Failed creating a new shader program\n");
        return -1;
    }

    shader = shader_compile((const GLchar *) vertex_buf, len, type);
    if (!shader) {
        fprintf(stderr, "Failed to compile shader < %s >\n", path);
        return 0;
    }

    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);
    glGetShaderiv(program, GL_LINK_STATUS, &linked);
    if (!linked) {
        int loglen;
        fprintf(stderr, "Failed to link shader program:\n");
        glGetShaderiv(program, GL_INFO_LOG_LENGTH, &loglen);
        if (loglen > 0) {
            char *info = (char *) malloc(loglen);
            glGetShaderInfoLog(program, loglen, NULL, info);
            free(info);
        }
        return 0;
    }

    glDetachShader(program, vertex);
    glDetachShader(program, fragment);

    return program;
}
#endif
