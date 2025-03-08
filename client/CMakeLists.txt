cmake_minimum_required(VERSION 2.8.12)

project(client)
include(CheckIncludeFile)
include(CompilerRuntime)
check_include_file("tgmath.h" HAVE_TGMATH_H)

# client source files
list(APPEND CLDLL_SOURCES
	"ammo.cpp"
	"ammo_secondary.cpp"
	"ammohistory.cpp"
	"battery.cpp"
	"cdll_int.cpp"
	"death.cpp"
	"flashlight.cpp"
	"geiger.cpp"
	"health.cpp"
	"entity.cpp"
	"hud.cpp"
	"hud_msg.cpp"
	"hud_redraw.cpp"
	"hud_update.cpp"
	"hud_flashlight.cpp"
	"input.cpp"
	"input_xash3d.cpp"
	"input_mouse.cpp"
	"menu.cpp"
	"message.cpp"
	"motd.cpp"
	"parsemsg.cpp"
	"saytext.cpp"
	"scoreboard.cpp"
	"status_icons.cpp"
	"statusbar.cpp"
	"text_message.cpp"
	"train.cpp"
	"utils.cpp"
	"r_view.cpp"
)

# shared source files
list(APPEND CLDLL_SOURCES
	"${CMAKE_SOURCE_DIR}/game_shared/bone_setup.cpp"
	"${CMAKE_SOURCE_DIR}/game_shared/common.cpp"
	"${CMAKE_SOURCE_DIR}/game_shared/ikcontext.cpp"
	"${CMAKE_SOURCE_DIR}/game_shared/jigglebones.cpp"
	"${CMAKE_SOURCE_DIR}/game_shared/material.cpp"
	"${CMAKE_SOURCE_DIR}/game_shared/mathlib.cpp"
	"${CMAKE_SOURCE_DIR}/game_shared/matrix.cpp"
	"${CMAKE_SOURCE_DIR}/game_shared/meshdesc.cpp"
	"${CMAKE_SOURCE_DIR}/game_shared/playermove.cpp"
	"${CMAKE_SOURCE_DIR}/game_shared/procbones.cpp"
	"${CMAKE_SOURCE_DIR}/game_shared/stringlib.cpp"
	"${CMAKE_SOURCE_DIR}/game_shared/virtualfs.cpp"
	"${CMAKE_SOURCE_DIR}/game_shared/trace.cpp"
)

# render source files
file(GLOB RENDER_SOURCES "render/*.cpp" "render/features/*.cpp")

# ImGui source files
if(NOT ENABLE_VGUI_COMPATIBILITY)
	list(APPEND IMGUI_SOURCES
		"ui/vgui_support_int.cpp"
		"ui/imgui_manager.cpp"
		"ui/imgui_window_system.cpp"
		"ui/ui_main.cpp"
		"ui/ui_demo_window.cpp"
		"ui/ui_material_editor.cpp"
		"${CMAKE_SOURCE_DIR}/external/imgui/imgui.cpp"
		"${CMAKE_SOURCE_DIR}/external/imgui/imgui_demo.cpp"
		"${CMAKE_SOURCE_DIR}/external/imgui/imgui_draw.cpp"
		"${CMAKE_SOURCE_DIR}/external/imgui/imgui_tables.cpp"
		"${CMAKE_SOURCE_DIR}/external/imgui/imgui_widgets.cpp"
	)
	list(APPEND CLDLL_SOURCES ${IMGUI_SOURCES})
endif()

list(APPEND CLDLL_SOURCES ${RENDER_SOURCES})
add_library(${PROJECT_NAME} SHARED ${CLDLL_SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE
	"."
	"render/"
	"ui/"
	"../common"
	"../engine"
	"../game_shared"
	"../public"
	"${CMAKE_SOURCE_DIR}/external/imgui"
)

if(ENABLE_VGUI_COMPATIBILITY)
	target_include_directories(${PROJECT_NAME} PRIVATE
		"${CMAKE_SOURCE_DIR}/external/vgui/include"
	)
endif()

if(HAVE_TGMATH_H)
	target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_TGMATH_H=1)
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE 
	CLIENT_WEAPONS=1 
	CLIENT_DLL=1
)

if(NOT MSVC)
	target_compile_options(${PROJECT_NAME} PRIVATE -fno-exceptions) # GCC/Clang flag
	target_compile_options(${PROJECT_NAME} PRIVATE -Wno-write-strings) # GCC/Clang flag
	target_compile_options(${PROJECT_NAME} PRIVATE -fvisibility=hidden) # GCC/Clang flag
	target_compile_definitions(${PROJECT_NAME} PRIVATE _LINUX=1 LINUX=1) # It seems enough for all non-Win32 systems
	target_compile_definitions(${PROJECT_NAME} PRIVATE stricmp=strcasecmp strnicmp=strncasecmp)
	if(NOT MINGW)
		target_compile_definitions(${PROJECT_NAME} PRIVATE _snprintf=snprintf _vsnprintf=vsnprintf)
	endif()
else()
	# disable annoying CRT warnings
	target_compile_definitions(${PROJECT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS=1 _CRT_NONSTDC_NO_DEPRECATE=1)
endif()

if(MSVC)
	# debug mode compiler flags
	target_compile_options(${PROJECT_NAME} PRIVATE $<$<CONFIG:Debug>:/Od>) # disable optimizing at all
	target_compile_options(${PROJECT_NAME} PRIVATE $<$<CONFIG:Debug>:/ZI>) # enable "Edit and Continue" MSVC feature
	target_link_options(${PROJECT_NAME} PRIVATE $<$<CONFIG:Debug>:
		/INCREMENTAL 
		/SAFESEH:NO
	>)

	# release mode compiler flags
	target_compile_options(${PROJECT_NAME} PRIVATE $<$<CONFIG:Release>:/Ob1>) # disable force function inlining, to prevent segfault in release mode
	target_compile_options(${PROJECT_NAME} PRIVATE $<$<CONFIG:Release>:/O1>) # enable optimizing to minimize size
endif()

# set static compiler runtime
if(ENABLE_STATIC_LINKING)
	set_compiler_runtime(${PROJECT_NAME} STATIC)
endif()

# linking vgui library if needed
if(ENABLE_VGUI_COMPATIBILITY)
	if(MSVC)
		find_library(LIBRARY_VGUI
			NAMES vgui
			HINTS "${CMAKE_SOURCE_DIR}/external/vgui/lib"
			PATH_SUFFIXES win32_vc6
		)
		target_link_libraries(${PROJECT_NAME} PRIVATE "${LIBRARY_VGUI}")
	else()
		target_link_directories(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/external/vgui/lib")
		target_link_libraries(${PROJECT_NAME} PRIVATE :vgui.so)
	endif()
endif()

if(WIN32)
	target_link_libraries(${PROJECT_NAME} PRIVATE user32.lib)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
	POSITION_INDEPENDENT_CODE 1)

# disable library prefix for Windows/iOS/Linux
if(APPLE OR WIN32 OR ${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
	set_target_properties(${PROJECT_NAME} PROPERTIES
		OUTPUT_NAME "client"
		PREFIX "")
endif()

install( TARGETS ${PROJECT_NAME}
	DESTINATION "${GAMEDIR}/${CLIENT_INSTALL_DIR}/"
	PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
	    GROUP_READ GROUP_EXECUTE
		WORLD_READ WORLD_EXECUTE 
)
