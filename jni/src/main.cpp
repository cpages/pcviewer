#include <cstdlib>
#include <string>
#include <stdexcept>
#include <sstream>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
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
    struct GLState
    {
        GLState():
            window(NULL), context(NULL)
        {}

        SDL_Window *window;
        SDL_GLContext context;
        GLuint shaderProgram;
        GLuint vertexBuffer;
    };

    struct ModelPos
    {
        ModelPos():
            tx(0.f), ty(0.f), tz(0.f), rx(0.f), ry(0.f), rz(0.f)
        {}

        float tx;
        float ty;
        float tz;
        float rx;
        float ry;
        float rz;
    };

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
            gl_Position = MVP * v; \
            gl_PointSize = 15.0; \
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

    glm::mat4
    mModelFromPos(ModelPos &mp)
    {
        glm::mat4 trans = glm::translate(glm::mat4(1.f),
                glm::vec3(mp.tx, mp.ty, mp.tz));
        glm::quat rotQuat(glm::vec3(mp.rx, mp.ry, mp.rz));
        return trans * glm::toMat4(rotQuat);
    }

    void
    render(GLState &glState, const glm::mat4 &mModel)
    {
        glClearColor(0.0f, 1.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 mProj(glm::perspective(45.0f, 4.0f / 3.0f, 0.1f, 100.0f));
        glm::mat4 mView= glm::lookAt(
                glm::vec3(0,3,10),
                glm::vec3(0,0,0),
                glm::vec3(0,1,0)
                );
        glm::mat4 mvp = mProj * mView * mModel;
        GLuint mvpID = glGetUniformLocation(glState.shaderProgram, "MVP");
        glUniformMatrix4fv(mvpID, 1, GL_FALSE, &mvp[0][0]);

        GLuint vertexPosID = glGetAttribLocation(glState.shaderProgram, "vertexPos");
        glEnableVertexAttribArray(vertexPosID);
        glBindBuffer(GL_ARRAY_BUFFER, glState.vertexBuffer);
        glVertexAttribPointer(vertexPosID, 3, GL_FLOAT, GL_FALSE, 0, 0);

        glDrawArrays(GL_POINTS, 0, 9);

        glDisableVertexAttribArray(vertexPosID);

        SDL_GL_SwapWindow(glState.window);
    }
}

int main(int argc, char *argv[])
{
    GLState glState;
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

        //TODO: learn about _DESKTOP
        glState.window = SDL_CreateWindow("PCViewer", SDL_WINDOWPOS_UNDEFINED,
                SDL_WINDOWPOS_UNDEFINED, WIDTH, HEIGHT,
                SDL_WINDOW_OPENGL | SDL_WINDOW_FULLSCREEN);
        if (glState.window == NULL)
            throw std::runtime_error(fillSDLError("Failed to create window"));

        glState.context = SDL_GL_CreateContext(glState.window);
        if (glState.context == NULL)
            throw std::runtime_error(fillSDLError("Failed to create context"));

        //SDL_GL_SetSwapInterval(1);

        int status = SDL_GL_MakeCurrent(glState.window, glState.context);
        if (status) {
            fprintf(stderr, "SDL_GL_MakeCurrent(): %s\n", SDL_GetError());
        }

        GLuint vertexShader = load_shader(vshaderSrc, GL_VERTEX_SHADER);
        GLuint fragmentShader = load_shader(fshaderSrc, GL_FRAGMENT_SHADER);

        glState.shaderProgram = glCreateProgram();
        glAttachShader(glState.shaderProgram, vertexShader);
        glAttachShader(glState.shaderProgram, fragmentShader);

        glLinkProgram(glState.shaderProgram);
        glUseProgram(glState.shaderProgram);

        glGenBuffers(1, &glState.vertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, glState.vertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(cubePoints), cubePoints, GL_STATIC_DRAW);

        ModelPos pcPos;
        SDL_Event event;
        bool run = true;
        while (run)
        {
            while (SDL_PollEvent(&event))
            {
                switch (event.type)
                {
                    case SDL_QUIT:
                        run = false;
                        break;
                }
            }

            render(glState, mModelFromPos(pcPos));
            pcPos.ry += .01;
            SDL_Delay(10);
        }
    }
    catch (const std::exception &e)
    {
#ifndef __ANDROID__
        std::cerr << "Error: " << e.what() << std::endl;
#endif
    }

    if (glState.context)
        SDL_GL_DeleteContext(glState.context);

    if (glState.window)
        SDL_DestroyWindow(glState.window);

    SDL_Quit();

    return 0;
}
