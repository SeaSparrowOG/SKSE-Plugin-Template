cmake_minimum_required(VERSION 3.24)

option(FULL_RELEASE "Finalizes the build, compressing everything into a BSA." OFF)

# SKSE Plugin Addd
function(SKSEPlugin_Add TARGET)
	set(options "")
	set(oneValueArgs SOURCE_DIR INCLUDE_DIR)
	set(multiValueArgs SOURCES PRECOMPILE_HEADERS)
	cmake_parse_arguments(SKSE "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

	include(CMakeDependentOption)

	cmake_host_system_information(
		RESULT SkyrimSE_PATH
		QUERY WINDOWS_REGISTRY "HKLM/SOFTWARE/Bethesda Softworks/Skyrim Special Edition"
		VALUE "installed path"
		VIEW 32
	)

	set(SkyrimSE_PATH ${SkyrimSE_PATH} CACHE PATH "Installed path of Skyrim Special Edition.")

	cmake_host_system_information(
		RESULT SkyrimVR_PATH
		QUERY WINDOWS_REGISTRY "HKLM/SOFTWARE/Bethesda Softworks/Skyrim VR"
		VALUE "installed path"
		VIEW 32
	)

	set(GAME_DIR ${SkyrimSE_PATH})

	add_library("${TARGET}" SHARED)

	if(SKSE_INCLUDE_DIR)
		cmake_path(IS_RELATIVE SKSE_INCLUDE_DIR SKSE_INCLUDE_DIR_IS_RELATIVE)
		if(SKSE_INCLUDE_DIR_IS_RELATIVE)
			cmake_path(ABSOLUTE_PATH SKSE_INCLUDE_DIR BASE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
		endif()

		file(GLOB_RECURSE INCLUDE_FILES
			LIST_DIRECTORIES false
			CONFIGURE_DEPENDS
			"${SKSE_INCLUDE_DIR}/*.h"
			"${SKSE_INCLUDE_DIR}/*.hpp"
			"${SKSE_INCLUDE_DIR}/*.hxx"
			"${SKSE_INCLUDE_DIR}/*.inl"
		)

		source_group(
			TREE ${SKSE_INCLUDE_DIR}
			PREFIX "Header Files"
			FILES ${INCLUDE_FILES}
		)

		target_sources("${TARGET}" PUBLIC ${INCLUDE_FILES})
	endif()

	configure_file(
		CMake/Plugin.h.in
		${CMAKE_CURRENT_BINARY_DIR}/src/Plugin.h
		@ONLY
	)

	configure_file(
		CMake/version.rc.in
		${CMAKE_CURRENT_BINARY_DIR}/version.rc
		@ONLY
	)

	target_sources(
		"${TARGET}"
		PRIVATE
			${CMAKE_CURRENT_BINARY_DIR}/version.rc
	)

	target_include_directories(
		"${TARGET}"
		PRIVATE
			${CMAKE_CURRENT_BINARY_DIR}/src
			${SKSE_INCLUDE_DIR}
			${SKSE_SOURCE_DIR}
	)

	if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
		target_compile_options(
			"${TARGET}"
			PRIVATE
				"/sdl"             # Enable Additional Security Checks
				"/utf-8"           # Set Source and Executable character sets to UTF-8
				"/Zi"              # Debug Information Format

				"/permissive-"     # Standards conformance
				"/Zc:preprocessor" # Enable preprocessor conformance mode

				"/wd4200"          # nonstandard extension used : zero-sized array in struct/union

				"$<$<CONFIG:DEBUG>:>"
				"$<$<CONFIG:RELEASE>:/Zc:inline;/JMC-;/Ob3>"
		)

		target_link_options(
			"${TARGET}"
			PRIVATE
				"$<$<CONFIG:DEBUG>:/INCREMENTAL;/OPT:NOREF;/OPT:NOICF>"
				"$<$<CONFIG:RELEASE>:/INCREMENTAL:NO;/OPT:REF;/OPT:ICF;/DEBUG:FULL>"
		)
	endif()

	find_package(xbyak CONFIG REQUIRED)
	target_link_libraries("${TARGET}" PRIVATE xbyak::xbyak)
	set(SKSE_SUPPORT_XBYAK ON CACHE INTERNAL "Enables trampoline support for Xbyak." FORCE)
	set(SKSE_COMMONLIBSSE_PATH "external/CommonLibSSE")

	add_subdirectory(${SKSE_COMMONLIBSSE_PATH} CommonLibSSE EXCLUDE_FROM_ALL)

	find_package(spdlog CONFIG REQUIRED)

	target_link_libraries(
		"${TARGET}"
		PRIVATE
			CommonLibSSE::CommonLibSSE
			spdlog::spdlog
	)
endfunction()

# -------- Project ----------
project(
	TemplateProject
	VERSION 1.0.0
	LANGUAGES CXX
)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

get_filename_component(
	Skyrim64Path
	"[HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\Bethesda Softworks\\Skyrim Special Edition;installed path]"
	ABSOLUTE CACHE
)

list(APPEND CMAKE_IGNORE_PATH "${PROJECT_BINARY_DIR}")
list(APPEND CMAKE_MODULE_PATH
	"${PROJECT_SOURCE_DIR}/tools/Bethesda-CMakeModules/Modules"
)
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

find_package(jsoncpp CONFIG REQUIRED)

target_link_libraries(
		"${PROJECT_NAME}"
		PRIVATE
			JsonCpp::JsonCpp
)

set_target_properties(CommonLibSSE PROPERTIES
	FOLDER External
)

set(DATA_DIR "${PROJECT_SOURCE_DIR}/data")

include(Papyrus)

file(GLOB ${PROJECT_NAME}_SOURCES
	LIST_DIRECTORIES false
	CONFIGURE_DEPENDS
	"Data/Source/Scripts/*.psc"
)

Papyrus_Add(
	"Papyrus"
	GAME ${Skyrim64Path}
	IMPORTS ${CMAKE_CURRENT_SOURCE_DIR}/Data/Source/Scripts
	SOURCES ${${PROJECT_NAME}_SOURCES}
	OPTIMIZE ANONYMIZE
)

# ---- Automatically copy artifacts to MO2 ----
if(DEFINED OUTPUT_FOLDER)
    set(DLL_FOLDER "${OUTPUT_FOLDER}/SKSE/Plugins")

    add_custom_command(
        TARGET "${PROJECT_NAME}"
        POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_FILE:${PROJECT_NAME}>" "${DLL_FOLDER}/$<TARGET_FILE_NAME:${PROJECT_NAME}>"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different "$<TARGET_PDB_FILE:${PROJECT_NAME}>" "${DLL_FOLDER}/$<TARGET_PDB_FILE_NAME:${PROJECT_NAME}>"

		# Clean Config Directory
		COMMAND "${CMAKE_COMMAND}" -E remove_directory "${OUTPUT_FOLDER}/SKSE/Plugins/${PROJECT_NAME}"
		COMMAND "${CMAKE_COMMAND}" -E make_directory "${OUTPUT_FOLDER}/SKSE/Plugins/${PROJECT_NAME}"
        COMMAND "${CMAKE_COMMAND}" -E copy_directory 
			"${PROJECT_SOURCE_DIR}/Data/SKSE/Plugins/${PROJECT_NAME}"
			"${OUTPUT_FOLDER}/SKSE/Plugins/${PROJECT_NAME}"

		# Update Settings INI
		COMMAND "${CMAKE_COMMAND}" -E copy_if_different
			"${PROJECT_SOURCE_DIR}/Data/SKSE/Plugins/${PROJECT_NAME}.ini"
			"${OUTPUT_FOLDER}/SKSE/Plugins/${PROJECT_NAME}.ini"

		# Update ESP plugin and the corresponding archive
		COMMAND "${CMAKE_COMMAND}" -E copy_if_different
			"${PROJECT_SOURCE_DIR}/Data/${PROJECT_NAME}.esp"
			"${OUTPUT_FOLDER}/${PROJECT_NAME}.esp"
		COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${CMAKE_BINARY_DIR}/${PROJECT_NAME}.bsa" "${OUTPUT_FOLDER}/${PROJECT_NAME}.bsa"
        VERBATIM
    )

	# Loose scripts for non-release builds (why? Because I said so smile)
    if(NOT FULL_RELEASE)
        add_custom_command(
            TARGET "${PROJECT_NAME}"
            POST_BUILD
			COMMAND "${CMAKE_COMMAND}" -E remove_directory "${OUTPUT_FOLDER}/Source"
			COMMAND "${CMAKE_COMMAND}" -E copy_directory "${PROJECT_SOURCE_DIR}/Data/Source/Scripts" "${OUTPUT_FOLDER}/Source/Scripts"

            COMMAND "${CMAKE_COMMAND}" -E remove_directory "${OUTPUT_FOLDER}/Scripts"
            COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_BINARY_DIR}/Scripts" "${OUTPUT_FOLDER}/Scripts"
            VERBATIM
        )
    endif()
endif()