#include <cstdlib>
#include <string>
#include <stdexcept>
#include <sstream>
#include <cmath>
#include "SDL.h"
#include "SDL_opengles2.h"

#ifdef __ANDROID__
#include <android/log.h>
#define APPNAME "SDLApp"
#define DIR_SEP "/"
#define DIR_CUR "/sdcard/"
#define WIDTH 320
#define HEIGHT 480
#else
#include  <iostream>
#define DIR_SEP "/"
#define DIR_CUR ""
#define WIDTH 1280
#define HEIGHT 800
#endif
#define DATAFILE(X) DIR_CUR "data" DIR_SEP X

namespace
{
    SDL_Window *window = NULL;
    SDL_GLContext context = NULL;

    std::string
    fillSDLError(const std::string &base)
    {
        std::ostringstream oss;
        oss << base << ": " << SDL_GetError() << std::endl;
        return oss.str();
    }

    const char *vshaderSrc =
        "attribute vec3 vertexPos; \
        uniform mat4 MVP; \
        \
        void main() { \
            vec4 v = vec4(vertexPos, 1); \
            gl_Position = v; \
            gl_PointSize = 16.0; \
        }";

    const char *fshaderSrc =
        " \
        void main() { \
            gl_FragColor = vec4(1,0,0,1); \
        }";

    const GLfloat cubePoints[] =
    {
        -1.0f, -1.0f, 1.0f,
        1.0f, -1.0f, 1.0f,
        -1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        -1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        -1.0f, 1.0f, -1.0f,
        1.0f, 1.0f, -1.0f,
        0.0f, 0.0f, 0.0f
    };

#if 0
    void
    print_shader_info_log (
       GLuint  shader      // handle to the shader
    )
    {
       GLint  length;

       glGetShaderiv ( shader , GL_INFO_LOG_LENGTH , &length );

       if ( length ) {
          char* buffer  =  new char [ length ];
          glGetShaderInfoLog ( shader , length , NULL , buffer );
          std::cout << "shader info: " <<  buffer << std::flush;
          delete [] buffer;

          GLint success;
          glGetShaderiv( shader, GL_COMPILE_STATUS, &success );
          if ( success != GL_TRUE )   exit ( 1 );
       }
    }
#endif

    GLuint
    load_shader(const char *shader_source, GLenum type)
    {
       GLuint  shader = glCreateShader( type );

       glShaderSource  ( shader , 1 , &shader_source , NULL );
       glCompileShader ( shader );

       //print_shader_info_log ( shader );

       return shader;
    }
}

int main(int argc, char *argv[])
{
    try
    {
        srand(time(NULL));

        if (SDL_Init(SDL_INIT_VIDEO) != 0)
        {
#ifndef __ANDROID__
            std::cout << "Couldn't initialize SDL: " << SDL_GetError()
                << std::endl;
#endif
            return -1;
        }

        //ask for gles2
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_EGL, 1);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
        //SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);

        window = SDL_CreateWindow("PCViewer", SDL_WINDOWPOS_UNDEFINED,
                SDL_WINDOWPOS_UNDEFINED, WIDTH, HEIGHT,
                SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN);
        if (window == NULL)
            throw std::runtime_error(fillSDLError("Failed to create window"));

        context = SDL_GL_CreateContext(window);
        if (context == NULL)
            throw std::runtime_error(fillSDLError("Failed to create context"));

        //SDL_GL_SetSwapInterval(1);

        int status = SDL_GL_MakeCurrent(window, context);
        if (status) {
            fprintf(stderr, "SDL_GL_MakeCurrent(): %s\n", SDL_GetError());
        }

        GLuint vertexShader = load_shader(vshaderSrc, GL_VERTEX_SHADER);
        GLuint fragmentShader = load_shader(fshaderSrc, GL_FRAGMENT_SHADER);

        GLuint shaderProgram = glCreateProgram();
        glAttachShader(shaderProgram, vertexShader);
        glAttachShader(shaderProgram, fragmentShader);

        glLinkProgram(shaderProgram);
        glUseProgram(shaderProgram);

        GLuint vertexBuffer;
        glGenBuffers(1, &vertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(cubePoints), cubePoints, GL_STATIC_DRAW);

        glClearColor(0.0f, 1.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        const GLfloat mvp[] =
        {
            1.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        };
        GLuint mvpID = glGetAttribLocation(shaderProgram, "MVP");
        glUniformMatrix4fv(mvpID, 1, GL_FALSE, mvp);

        GLuint vertexPosID = glGetAttribLocation(shaderProgram, "vertexPos");
        glEnableVertexAttribArray(vertexPosID);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer);
        glVertexAttribPointer(vertexPosID, 3, GL_FLOAT, GL_FALSE, 0, 0);

        glDrawArrays(GL_POINTS, 0, 9);

        glDisableVertexAttribArray(vertexPosID);

        SDL_GL_SwapWindow(window);

        SDL_Delay(1000);
    }
    catch (const std::exception &e)
    {
#ifndef __ANDROID__
        std::cerr << "Error: " << e.what() << std::endl;
#endif
    }

    if (context)
        SDL_GL_DeleteContext(context);

    if (window)
        SDL_DestroyWindow(window);

    SDL_Quit();

    return 0;
}
