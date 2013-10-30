#include <cstdlib>
#include <string>
#include <stdexcept>
#include <sstream>
#include <cmath>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include "SDL.h"
#include "SDL_opengles2.h"
#include "shared.hpp"
#include "plyReader.hpp"

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
    std::vector<Vertex> vertices;

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
            gl_PointSize = 2.0; \
        }";

    const char *fshaderSrc =
        " \
        void main() { \
            gl_FragColor = vec4(1,0,0,1); \
        }";

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
        glm::mat4 trans = glm::translate(mp.tx, mp.ty, mp.tz);
        glm::quat rotQuat(glm::vec3(mp.rx, mp.ry, mp.rz));
        return trans * glm::toMat4(rotQuat);
    }

    glm::mat4
    computeCenterAndScale(std::vector<Vertex> &vertices)
    {
        float cx, cy, cz;
        cx = cy = cz = 0.f;
        for (unsigned int i = 0; i < vertices.size(); ++i)
        {
            cx += vertices[i].pos[0];
            cy += vertices[i].pos[1];
            cz += vertices[i].pos[2];
        }
        cx /= vertices.size();
        cy /= vertices.size();
        cz /= vertices.size();
        glm::mat4 mTrans = glm::translate(-cx, -cy, -cz);

        float maxDist = 0.f;
        for (unsigned int i = 0; i < vertices.size(); ++i)
        {
            const Vertex &v(vertices[i]);
            float dist = (v.pos[0]-cx) * (v.pos[0]-cx) +
                (v.pos[1]-cy) * (v.pos[1]-cy) +
                (v.pos[2]-cz) * (v.pos[2]-cz);
            maxDist = std::max(dist, maxDist);
        }

        const float scale = 1 / std::sqrt(maxDist);
        glm::mat4 mScale = glm::scale(scale, scale, scale);

        return mScale * mTrans;
    }

    void
    render(GLState &glState, const glm::mat4 &mvp)
    {
        glClearColor(0.0f, 1.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        GLuint mvpID = glGetUniformLocation(glState.shaderProgram, "MVP");
        glUniformMatrix4fv(mvpID, 1, GL_FALSE, &mvp[0][0]);

        GLuint vertexPosID = glGetAttribLocation(glState.shaderProgram, "vertexPos");
        glEnableVertexAttribArray(vertexPosID);
        glBindBuffer(GL_ARRAY_BUFFER, glState.vertexBuffer);
        glVertexAttribPointer(vertexPosID, 3, GL_FLOAT, GL_FALSE,
                sizeof(Vertex), 0);

        glDrawArrays(GL_POINTS, 0, vertices.size());

        glDisableVertexAttribArray(vertexPosID);

        SDL_GL_SwapWindow(glState.window);
    }
}

int main(int argc, char *argv[])
{
    GLState glState;
    float fov = 45.f;
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

        readPLY("bun_zipper.ply", vertices);
        glm::mat4 mCenterAndScale(computeCenterAndScale(vertices));

        glGenBuffers(1, &glState.vertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, glState.vertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex),
                &vertices[0], GL_STATIC_DRAW);

        //prepare fix view matrices
        const glm::mat4 mView = glm::lookAt(
                glm::vec3(0,0,5),
                glm::vec3(0,0,0),
                glm::vec3(0,1,0)
                );

        ModelPos pcPos;
        SDL_Event event;
        bool translating = false;
        bool rotating = false;
        SDL_FingerID activeFinger;
        int fingerCount = 0;
        bool run = true;
        while (run)
        {
            while (SDL_PollEvent(&event))
            {
                switch (event.type)
                {
#ifdef __ANDROID__
                    //TODO: add translation (2 fingers?)
                    case SDL_FINGERDOWN:
                        ++fingerCount;
                        if (!rotating)
                        {
                            rotating = true;
                            activeFinger = event.tfinger.fingerId;
                        }
                        break;
                    case SDL_FINGERUP:
                        --fingerCount;
                        if (rotating && event.tfinger.fingerId == activeFinger)
                            rotating = false;
                        break;
                    case SDL_FINGERMOTION:
                        if (rotating && fingerCount == 1 && event.tfinger.fingerId == activeFinger)
                        {
                            pcPos.rx += event.tfinger.dy * 5;
                            pcPos.ry += event.tfinger.dx * 5;
                        }
                        break;
                    case SDL_MULTIGESTURE:
                        fov -= event.mgesture.dDist * 100;
                        break;
#else
                    case SDL_MOUSEBUTTONDOWN:
                        if (event.button.button == SDL_BUTTON_LEFT)
                            translating = true;
                        else if (event.button.button == SDL_BUTTON_RIGHT)
                            rotating = true;
                        break;
                    case SDL_MOUSEBUTTONUP:
                        if (event.button.button == SDL_BUTTON_LEFT)
                            translating = false;
                        else if (event.button.button == SDL_BUTTON_RIGHT)
                            rotating = false;
                        break;
                    case SDL_MOUSEWHEEL:
                        if (event.wheel.y > 0)
                            fov += 1;
                        else if (event.wheel.y < 0)
                            fov -= 1;
                        break;
                    case SDL_MOUSEMOTION:
                        if (translating)
                        {
                            pcPos.tx += event.motion.xrel * .01;
                            pcPos.ty -= event.motion.yrel * .01;
                        }
                        else if (rotating)
                        {
                            pcPos.rx += event.motion.yrel * .01;
                            pcPos.ry += event.motion.xrel * .01;
                        }
                        break;
#endif
                    case SDL_QUIT:
                        run = false;
                        break;
                }
            }

            const glm::mat4 mProj =
                glm::perspective(fov, float(WIDTH) / HEIGHT, 0.1f, 100.0f);
            const glm::mat4 mvp = mProj * mView * mModelFromPos(pcPos) * mCenterAndScale;
            render(glState, mvp);
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
