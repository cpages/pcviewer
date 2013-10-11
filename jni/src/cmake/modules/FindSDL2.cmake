# clean result
set (SDL2_FOUND false)
set (SDL2_INCLUDE_DIRS "")
set (SDL2_LIBRARIES "")

# use pkg-config to get hints
include (LibFindMacros)
libfind_pkg_check_modules (SDL2_PKGCONF sdl2)

set (COMMON_SEARCH_PATHS
  / # cross-compile with CMAKE_FIND_ROOT_PATH set
  /sw # Fink
  /opt/local # DarwinPorts
  /opt/csw # Blastwave
  /opt)

set (INC_SEARCH_PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /usr/local/include/SDL2
  /usr/include/SDL2)

find_path (SDL2_INCLUDE_DIR SDL.h
  HINTS $ENV{SDL2DIR}
  PATH_SUFFIXES include/SDL2 include
  PATHS ${SDL2_PKGCONF_INCLUDE_DIRS}
  ${INC_SEARCH_PATHS}
  ${COMMON_SEARCH_PATHS})
mark_as_advanced (SDL2_INCLUDE_DIR)
#MESSAGE ("SDL2_INCLUDE_DIR is ${SDL2_INCLUDE_DIR}")

find_library (SDL2_LIB 
  NAMES SDL2
  HINTS $ENV{SDL2DIR}
  PATH_SUFFIXES lib64 lib
    lib/wii # Wii
  PATHS ${SDL2_PKGCONF_LIBRARY_DIRS}
  ${COMMON_SEARCH_PATHS})
mark_as_advanced (SDL2_LIB)
#MESSAGE ("SDL2_LIB is ${SDL2_LIB}")

if (NOT SDL2_BUILDING_LIBRARY)
  if (NOT ${SDL2_INCLUDE_DIR} MATCHES ".framework")
    # Non-OS X framework versions expect you to also dynamically link to 
    # SDLmain. This is mainly for Windows and OS X. Other (Unix) platforms 
    # seem to provide SDLmain for compatibility even though they don't
    # necessarily need it.
    find_library (SDL2MAIN_LIBRARY 
      NAMES SDL2main
      HINTS $ENV{SDL2DIR}
      PATH_SUFFIXES lib64 lib
      PATHS ${SDL2_PKGCONF_LIBRARY_DIRS}
      ${COMMON_SEARCH_PATHS})
    mark_as_advanced (SDL2MAIN_LIBRARY)
  endif ()
endif ()

# SDL may require threads on your system.
# The Apple build may not need an explicit flag because one of the 
# frameworks may already provide it. 
# But for non-OSX systems, I will use the CMake Threads package.
if (NOT APPLE)
  find_package (Threads)
endif ()

# MinGW needs an additional library, mwindows
# It's total link flags should look like -lmingw32 -lSDLmain -lSDL -lmwindows
# (Actually on second look, I think it only needs one of the m* libraries.)
if (MINGW)
  set (MINGW32_LIBRARY mingw32 CACHE STRING "mwindows for MinGW")
endif ()

macro (FIND_LIBS _LIBS)
  foreach (LIB ${${_LIBS}})
    #MESSAGE ("Searching for SDL2_${LIB}")
    libfind_pkg_check_modules (SDL2_${LIB}_PKGCONF SDL2_${LIB})
    find_library (SDL2_${LIB}
      NAMES SDL2_${LIB}
      HINTS $ENV{SDL2DIR}
      PATH_SUFFIXES lib64 lib
        lib/wii # Wii
      PATHS ${SDL2_${LIB}_PKGCONF_LIBRARY_DIRS}
      ${COMMON_SEARCH_PATHS})
    #MESSAGE ("SDL2_LIBRARY is ${SDL2_${LIB}}")
    mark_as_advanced (SDL2_${LIB})
  endforeach ()
endmacro ()

FIND_LIBS (SDL2_FIND_COMPONENTS)

set (SDL2_PROCESS_INCLUDES SDL2_INCLUDE_DIR)
set (SDL2_PROCESS_LIBS "")
foreach (LIB ${SDL2_FIND_COMPONENTS})
  list (APPEND SDL2_PROCESS_LIBS SDL2_${LIB})
endforeach ()
list (APPEND SDL2_PROCESS_LIBS SDL2_LIB)

libfind_process(SDL2)

if (SDL2_FOUND)
  # For SDLmain
  if (NOT SDL2_BUILDING_LIBRARY)
    if (SDL2MAIN_LIBRARY)
      set (SDL2_LIBRARIES ${SDL2MAIN_LIBRARY} ${SDL2_LIBRARIES})
    endif ()
  endif ()

  # For OS X, SDL uses Cocoa as a backend so it must link to Cocoa.
  # CMake doesn't display the -framework Cocoa string in the UI even 
  # though it actually is there if I modify a pre-used variable.
  # I think it has something to do with the CACHE STRING.
  # So I use a temporary variable until the end so I can set the 
  # "real" variable in one-shot.
  if (APPLE)
    set (SDL2_LIBRARIES ${SDL2_LIBRARIES} "-framework Cocoa")
  endif ()
    
  # For threads, as mentioned Apple doesn't need this.
  # In fact, there seems to be a problem if I used the Threads package
  # and try using this line, so I'm just skipping it entirely for OS X.
  if (NOT APPLE)
    set (SDL2_LIBRARIES ${SDL2_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
  endif ()

  # For MinGW library
  if (MINGW)
    set (SDL2_LIBRARIES ${MINGW32_LIBRARY} ${SDL2_LIBRARIES})
  endif ()
endif ()
