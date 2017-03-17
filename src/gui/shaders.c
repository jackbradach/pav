#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <GL/glut.h>

#include "file_utils.h"


void _gl_perror(void)
{
    GLenum err = glGetError();

    while(GL_NO_ERROR != err) {
        const char *err_str;
        switch(err) {
        case GL_INVALID_OPERATION:
            err_str = "INVALID_OPERATION";
            break;
        case GL_INVALID_ENUM:
            err_str = "INVALID_ENUM";
            break;
        case GL_INVALID_VALUE:
            err_str = "INVALID_VALUE";
            break;
        case GL_OUT_OF_MEMORY:
            err_str = "OUT_OF_MEMORY";
            break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            err_str = "INVALID_FRAMEBUFFER_OPERATION";
            break;
        }
        perror(err_str);
        err = glGetError();
    }
}

int _glReportShaderCompileStatus(GLint shader)
{
    GLint compiled;

    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

    if (!compiled) {
        int len;
        GLchar *info;

        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);

        info = calloc(len + 1, sizeof(GLchar));
        glGetShaderInfoLog(shader, len, &len, info);
        fprintf(stderr, "ShaderInfoLog: %s\n", info);
        free(info);
    }
    return compiled;
}

int _glReportProgramLinkStatus(GLint program)
{
    GLint linked;

    glGetProgramiv(program, GL_LINK_STATUS, &linked);

    if (!linked) {
        int log_len;
        GLchar *info;

        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &log_len);

        info = calloc(log_len + 1, sizeof(GLchar));
        glGetProgramInfoLog(program, log_len, &log_len, info);
        fprintf(stderr, "ProgramInfoLog: %s\n", info);
        free(info);
    } else {
        fprintf(stderr, "Shader link success!\n");
    }
    return linked;
}

GLint shader_compile(const GLchar *src, GLint src_len, GLenum type);

//TODO: rename to 'compile_shader_from_path'
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
    GLint shader = glCreateShader(type);

    glShaderSource(shader, 1, shader_srcs, &src_len);
    glCompileShader(shader);

    if (_glReportShaderCompileStatus(shader)) {
        return shader;
    }

    return 0;
}

// TODO: write a compile checkstatus.

GLint shader_compile_program(const char *vertex_src, const char *fragment_src)
{
    GLuint vertex, fragment;
    GLuint program;

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

    _glReportProgramLinkStatus(program);

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
