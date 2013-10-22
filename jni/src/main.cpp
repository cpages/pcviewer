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
#define DIR_SEP	"/"
#define DIR_CUR "/sdcard/"
#define WIDTH 320
#define HEIGHT 480
#else
#include  <iostream>
#define DIR_SEP	"/"
#define DIR_CUR	""
#define WIDTH 1280
#define HEIGHT 800
#endif
#define DATAFILE(X)	DIR_CUR "data" DIR_SEP X

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
}

const char vertex_src [] =
"                                        \
   attribute vec4        position;       \
   varying mediump vec2  pos;            \
   uniform vec4          offset;         \
                                         \
   void main()                           \
   {                                     \
      gl_Position = position + offset;   \
      pos = position.xy;                 \
   }                                     \
";
 
 
const char fragment_src [] =
"                                                      \
   varying mediump vec2    pos;                        \
   uniform mediump float   phase;                      \
                                                       \
   void  main()                                        \
   {                                                   \
      gl_FragColor  =  vec4( 1., 0.9, 0.7, 1.0 ) *     \
        cos( 30.*sqrt(pos.x*pos.x + 1.5*pos.y*pos.y)   \
             + atan(pos.y,pos.x) - phase );            \
   }                                                   \
";
//  some more formulas to play with...
//      cos( 20.*(pos.x*pos.x + pos.y*pos.y) - phase );
//      cos( 20.*sqrt(pos.x*pos.x + pos.y*pos.y) + atan(pos.y,pos.x) - phase );
//      cos( 30.*sqrt(pos.x*pos.x + 1.5*pos.y*pos.y - 1.8*pos.x*pos.y*pos.y)
//            + atan(pos.y,pos.x) - phase );
 
 
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
load_shader (
   const char  *shader_source,
   GLenum       type
)
{
   GLuint  shader = glCreateShader( type );
 
   glShaderSource  ( shader , 1 , &shader_source , NULL );
   glCompileShader ( shader );
 
   //print_shader_info_log ( shader );
 
   return shader;
}
 
 
 
GLfloat
   norm_x    =  0.0,
   norm_y    =  0.0,
   offset_x  =  0.0,
   offset_y  =  0.0,
   p1_pos_x  =  0.0,
   p1_pos_y  =  0.0;
 
GLint
   phase_loc,
   offset_loc,
   position_loc;
 
 
bool        update_pos = false;
 
const float vertexArray[] = {
   0.0,  0.5,  0.0,
  -0.5,  0.0,  0.0,
   0.0, -0.5,  0.0,
   0.5,  0.0,  0.0,
   0.0,  0.5,  0.0 
};

static GLuint make_shader(GLenum type, char *code)
{
    GLint length = strlen(code);
    GLchar *source = code;
    GLuint shader;
    GLint shader_ok;

    if (!source)
        return 0;

    shader = glCreateShader(type);
    glShaderSource(shader, 1, (const GLchar**)&source, &length);
    //free(source);
    glCompileShader(shader);

    glGetShaderiv(shader, GL_COMPILE_STATUS, &shader_ok);
    if (!shader_ok) {
        fprintf(stderr, "Failed to compile %d:\n", type);
#ifdef __ANDROID__
		__android_log_print(ANDROID_LOG_VERBOSE, APPNAME, "Failed to compile %d\n", type);
#endif
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

static GLuint make_program(GLuint vertex_shader, GLuint fragment_shader)
{
    GLint program_ok;

    GLuint program = glCreateProgram();

    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);

    glGetProgramiv(program, GL_LINK_STATUS, &program_ok);
    if (!program_ok) {
        fprintf(stderr, "Failed to link shader program:\n");
#ifdef __ANDROID__
		__android_log_print(ANDROID_LOG_VERBOSE, APPNAME, "Failed to link shader prog\n");
#endif
        glDeleteProgram(program);
        return 0;
    }
    return program;
}

static const GLfloat g_vertex_buffer_data[] = { 
	-1.0f, -1.0f, 0.0f,
	 1.0f, -1.0f, 0.0f,
	 0.0f,  1.0f, 0.0f,
};
static GLuint program;

void  render()
{
   static float  phase = 0;
   static int    donesetup = 0;
 
 
   //// draw
   glClear ( GL_COLOR_BUFFER_BIT );
 
   glUniform1f ( phase_loc , phase );  // write the value of phase to the shaders phase
   phase  =  fmodf ( phase + 0.5f , 2.f * 3.141f );    // and update the local variable
 
   if ( update_pos ) {  // if the position of the texture has changed due to user action
      GLfloat old_offset_x  =  offset_x;
      GLfloat old_offset_y  =  offset_y;
 
      offset_x  =  norm_x - p1_pos_x;
      offset_y  =  norm_y - p1_pos_y;
 
      p1_pos_x  =  norm_x;
      p1_pos_y  =  norm_y;
 
      offset_x  +=  old_offset_x;
      offset_y  +=  old_offset_y;
 
      update_pos = false;
   }
 
   glUniform4f ( offset_loc  ,  offset_x , offset_y , 0.0 , 0.0 );
 
   glVertexAttribPointer ( position_loc, 3, GL_FLOAT, false, 0, vertexArray );
   glEnableVertexAttribArray ( position_loc );
   glDrawArrays ( GL_TRIANGLE_STRIP, 0, 5 );
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

	   GLuint vertexShader   = load_shader ( vertex_src , GL_VERTEX_SHADER  );     // load vertex shader
	   GLuint fragmentShader = load_shader ( fragment_src , GL_FRAGMENT_SHADER );  // load fragment shader
	 
	   GLuint shaderProgram  = glCreateProgram ();                 // create program object
	   glAttachShader ( shaderProgram, vertexShader );             // and attach both...
	   glAttachShader ( shaderProgram, fragmentShader );           // ... shaders to it
	 
	   glLinkProgram ( shaderProgram );    // link the program
	   glUseProgram  ( shaderProgram );    // and select it for usage
	 
	   //// now get the locations (kind of handle) of the shaders variables
	   position_loc  = glGetAttribLocation  ( shaderProgram , "position" );
	   phase_loc     = glGetUniformLocation ( shaderProgram , "phase"    );
	   offset_loc    = glGetUniformLocation ( shaderProgram , "offset"   );
	   if ( position_loc < 0  ||  phase_loc < 0  ||  offset_loc < 0 ) {
#ifndef __ANDROID__
		   std::cerr << "Unable to get uniform location" << std::endl;
#endif
		  return 1;
	   }

#if 0
		GLuint vshader = make_shader(GL_VERTEX_SHADER, g_vshader);
		GLuint fshader = make_shader(GL_FRAGMENT_SHADER, g_fshader);
		program = make_program(vshader, fshader); 
		if (program == 0) {
			fprintf(stderr, "Error making program from shaders");
		}

		GLuint vertexPosition_modelspaceID = glGetAttribLocation(program, "vertexPosition_modelspace");

		GLuint vertexbuffer;
		glGenBuffers(1, &vertexbuffer);
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

		glClearColor(0.0f, 1.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(program);

		// 1rst attribute buffer : vertices
		glEnableVertexAttribArray(vertexPosition_modelspaceID);
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
		glVertexAttribPointer(
			vertexPosition_modelspaceID, // The attribute we want to configure
			3,                  // size
			GL_FLOAT,           // type
			GL_FALSE,           // normalized?
			0,                  // stride
			(void*)0            // array buffer offset
		);

		// Draw the triangle !
		glDrawArrays(GL_TRIANGLES, 0, 3); // 3 indices starting at 0 -> 1 triangle

		glDisableVertexAttribArray(vertexPosition_modelspaceID);
#endif
		render();

		SDL_GL_SwapWindow(window);

		SDL_Delay(1000);

		return 0;
	}
	catch (const std::exception &e)
	{
#ifndef __ANDROID__
		std::cerr << "Error: " << e.what() << std::endl;
#endif

		if (context)
			SDL_GL_DeleteContext(context);

		if (window)
			SDL_DestroyWindow(window);

		SDL_Quit();
	}
	return -1;
}
