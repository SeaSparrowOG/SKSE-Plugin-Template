cmake_minimum_required(VERSION 3.24)

Option(GITHUB "Turned on when building with Github Actions. Turns off post-build operations." OFF)

# -------- Project ----------
project(
	TemplateProject
	VERSION 1.0.0
	LANGUAGES CXX
)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

list(APPEND CMAKE_MODULE_PATH
	"${PROJECT_SOURCE_DIR}/tools/SKSE-CMakeModules/Modules"
)
list(APPEND CMAKE_MODULE_PATH
	"${PROJECT_SOURCE_DIR}/tools/Bethesda-CMakeModules/Modules"
)

include(SKSEPlugin)
set(SKSE_USE_XBYAK ON)
add_compile_definitions(SKSE_SUPPORT_XBYAK)
add_compile_definitions(SKYRIM_AE)
add_compile_definitions(SKYRIM_SUPPORT_AE)

SKSEPlugin_Add(
	${PROJECT_NAME}
	SOURCE_DIR src
	SOURCES
		README.md
		vcpkg.json
	PRECOMPILE_HEADERS
		src/common/PCH.h
)

find_path(CLIB_UTIL_INCLUDE_DIRS "ClibUtil/utils.hpp")
find_package(jsoncpp CONFIG REQUIRED)

target_link_libraries(
		"${PROJECT_NAME}"
		PRIVATE
			JsonCpp::JsonCpp
)

target_include_directories(
	${PROJECT_NAME}
	PRIVATE
		${CLIB_UTIL_INCLUDE_DIRS}
)

set_target_properties(CommonLibSSE PROPERTIES
	FOLDER External
)

set(DATA_DIR "${PROJECT_SOURCE_DIR}/Data")
set(SKSE_DIR "${DATA_DIR}/SKSE")
set(SKSE_PLUGINS_DIR "${SKSE_DIR}/Plugins")

set(INI_PATH "${SKSE_PLUGINS_DIR}/${PROJECT_NAME}.ini")
set(PAPYRUS_SOURCE_DIR "${DATA_DIR}/Source/Scripts")
set(CUSTOM_CONSOLE_DIR "${SKSE_DIR}/CustomConsole")

if (IS_DIRECTORY PAPYRUS_SOURCE_DIR)
	install(
		DIRECTORY "${DATA_DIR}/Source"
		DESTINATION "."
		COMPONENT components
	)
endif()

if (IS_DIRECTORY CUSTOM_CONSOLE_DIR)
	install(
		DIRECTORY "${SKSE_DIR}/CustomConsole"
		DESTINATION "."
		COMPONENT components
	)
endif()

include(components.cmake)

# ---- Post build ----
if (!GITHUB)
	get_filename_component(
		Skyrim64Path
		"[HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\Bethesda Softworks\\Skyrim Special Edition;installed path]"
		ABSOLUTE
	)

	set(Skyrim64Path "${Skyrim64Path}" CACHE PATH "Skyrim SE install directory")

	if(NOT IS_DIRECTORY "${Skyrim64Path}")
		message(WARNING "Skyrim SE install path not found; Papyrus scripts will not be compiled or archived.")
	endif()

	if(DEFINED ENV{SKYRIM_MODS_FOLDER} AND IS_DIRECTORY "$ENV{SKYRIM_MODS_FOLDER}")
		set(OUTPUT_FOLDER_BASE "$ENV{SKYRIM_MODS_FOLDER}/${PROJECT_NAME}")
		set(DLL_FOLDER "${OUTPUT_FOLDER_BASE} - $<CONFIG>/SKSE/Plugins")
	endif()

	# ---- Compile Papyrus Scripts ----
	include(Papyrus)
	set(COPY_SCRIPTS  OFF)

	if(EXISTS "${PAPYRUS_SOURCE_DIR}" AND IS_DIRECTORY "${Skyrim64Path}")
		file(GLOB ${PROJECT_NAME}_SOURCES
			LIST_DIRECTORIES false
			CONFIGURE_DEPENDS
			"${PAPYRUS_SOURCE_DIR}/*.psc"
		)

		Papyrus_Add(
			"Papyrus"
			GAME ${Skyrim64Path}
			IMPORTS ${PAPYRUS_SOURCE_DIR}
			SOURCES ${${PROJECT_NAME}_SOURCES}
			OPTIMIZE ANONYMIZE
		)		
		set(COPY_SCRIPTS ON)
		
	elseif(EXISTS "${PAPYRUS_SOURCE_DIR}" AND NOT IS_DIRECTORY "${Skyrim64Path}")
		message(STATUS "Scripts exist in the Data directory, but Skyrim's path was not found, compilation will be skipped.")
	else()
		message(STATUS "No scripts found to compile.")
	endif()

	# ---- Automatically copy artifacts to MO2 ----
	if(DEFINED OUTPUT_FOLDER)
		set(DLL_FOLDER "$"${OUTPUT_FOLDER_BASE} - $<CONFIG>"/SKSE/Plugins")

		set(POST_BUILD_COMMANDS
			COMMAND "${CMAKE_COMMAND}" -E copy_if_different
				"$<TARGET_FILE:${PROJECT_NAME}>"
				"${DLL_FOLDER}/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
			COMMAND "${CMAKE_COMMAND}" -E copy_if_different
				"$<$<BOOL:$<TARGET_PDB_FILE:${PROJECT_NAME}>>:$<TARGET_PDB_FILE:${PROJECT_NAME}>>"
				"${DLL_FOLDER}/$<TARGET_PDB_FILE_NAME:${PROJECT_NAME}>"
			COMMAND "${CMAKE_COMMAND}"
				-DSRC_DIR=${DATA_DIR}
				-DDST_DIR=$"${OUTPUT_FOLDER_BASE} - $<CONFIG>"
				-DPROJECT_NAME=${PROJECT_NAME}
				-P "${PROJECT_SOURCE_DIR}/CMake/Scripts/CMake-Copy-Artifacts.cmake"
		)

		if(COPY_SCRIPTS)
			list(APPEND POST_BUILD_COMMANDS
				COMMAND "${CMAKE_COMMAND}"
					-DSOURCE_SCRIPTS=${PAPYRUS_SOURCE_DIR}
					-DOUTPUT_FOLDER=$"${OUTPUT_FOLDER_BASE} - $<CONFIG>"
					-P "${PROJECT_SOURCE_DIR}/CMake/Scripts/CMake-Copy-Sources.cmake"
			)
			list(APPEND POST_BUILD_COMMANDS
				COMMAND "${CMAKE_COMMAND}"
					-DCOMPILED_SCRIPTS="${CMAKE_BINARY_DIR}/Scripts"
					-DOUTPUT_FOLDER=$"${OUTPUT_FOLDER_BASE} - $<CONFIG>"
					-P "${PROJECT_SOURCE_DIR}/CMake/Scripts/CMake-Copy-Scripts.cmake"
			)
		endif()
		
		
		message(STATUS "==== Build Configuration ====")
		message(STATUS "FULL_RELEASE      = ${FULL_RELEASE}")
		message(STATUS "Skyrim64Path      = ${Skyrim64Path}")
		message(STATUS "OUTPUT_FOLDER     = $"${OUTPUT_FOLDER_BASE} - $<CONFIG>"")
		message(STATUS "COPY_SCRIPTS      = ${COPY_SCRIPTS}")
		message(STATUS "=============================")

		add_custom_command(
			TARGET "${PROJECT_NAME}"
			POST_BUILD
			${POST_BUILD_COMMANDS}
			VERBATIM
		)
	else()
		message(STATUS "Output folder environment variable not set, build files will not be automatically deployed to MO2.")
	endif()
endif()