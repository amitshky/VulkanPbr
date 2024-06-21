cmake_minimum_required(VERSION 3.24)

set(CMAKE_CXX_STANDARD 17)
project(vulkanPbr LANGUAGES CXX)

# set Visual Studio startup project
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})


# project options
option(VKPBR_USE_PRE_BUILT_LIB "Use pre-built libraries" OFF)
option(VKPBR_ENABLE_CLANG_TIDY_CHECK "Enables clang-tidy check during build" OFF) # .clang-tidy required

# GLFW options
option(GLFW_BUILD_EXAMPLES "Build the GLFW example programs" OFF)
option(GLFW_BUILD_TESTS "Build the GLFW test programs" OFF)
option(GLFW_BUILD_DOCS "Build the GLFW documentation" OFF)
option(GLFW_INSTALL "Generate installation target" OFF)
option(GLFW_DOCUMENT_INTERNALS "Include internals in documentation" OFF)

# Assimp options
option(BUILD_SHARED_LIBS "Build package with shared libraries." OFF)
option(ASSIMP_BUILD_FRAMEWORK "Build package as Mac OS X Framework bundle." OFF)
option(ASSIMP_DOUBLE_PRECISION "Set to ON to enable double precision processing" OFF)
option(ASSIMP_OPT_BUILD_PACKAGES "Set to ON to generate CPack configuration files and packaging targets" OFF)
option(ASSIMP_ANDROID_JNIIOSYSTEM "Android JNI IOSystem support is active" OFF)
option(ASSIMP_NO_EXPORT "Disable Assimp's export functionality." OFF)
option(ASSIMP_BUILD_SAMPLES "If the official samples are built as well (need Glu)." OFF)
option(ASSIMP_BUILD_TESTS "If the test suite for Assimp is built in addition to the library." OFF)
option(ASSIMP_COVERALLS "Enable this to measure test coverage." OFF)
option(ASSIMP_INSTALL "Disable this if you want to use assimp as a submodule." OFF)
option(ASSIMP_WARNINGS_AS_ERRORS "Treat all warnings as errors." OFF)
option(ASSIMP_ASAN "Enable AddressSanitizer." OFF)
option(ASSIMP_UBSAN "Enable Undefined Behavior sanitizer." OFF)
option(ASSIMP_BUILD_DOCS "Build documentation using Doxygen." OFF)
option(ASSIMP_INJECT_DEBUG_POSTFIX "Inject debug postfix in .a/.so/.dll lib names" ON)
option(ASSIMP_IGNORE_GIT_HASH "Don't call git to get the hash." OFF)


# Update submodules as needed
find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
	option(GIT_SUBMODULE "Check submodules during build" ON)
	if(GIT_SUBMODULE)
		message(STATUS "Submodule update")
		execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
						WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
						RESULT_VARIABLE GIT_SUBMOD_RESULT)
		if(NOT GIT_SUBMOD_RESULT EQUAL "0")
			message(FATAL_ERROR "git submodule update --init --recursive failed with ${GIT_SUBMOD_RESULT}, please checkout submodules")
		endif()
	endif()
endif()


set(LINUX FALSE)
if(UNIX AND NOT APPLE)
	set(LINUX TRUE)
endif()


add_subdirectory(src)
if (NOT ${VKPBR_USE_PRE_BUILT_LIB})
	add_subdirectory(lib)
endif()


if(MSVC)
	target_compile_options(${PROJECT_NAME} PUBLIC "/W4;/analyze;/MP;")
elseif(GNU OR Clang)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS};-Wall;-Wextra;-Wpedantic;-Wconversion;-Wshadow;")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG};-O0;-g;")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE};-O3;")

	if(LINUX)
		target_link_libraries(${PROJECT_NAME} pthreads)
	endif()
endif()


find_package(Vulkan REQUIRED)


# setup clang-tidy
# .clang-tidy required
if (${VKPBR_ENABLE_CLANG_TIDY_CHECK})
	find_program(CLANG_TIDY_EXE NAMES "clang-tidy")
	set(CLANG_TIDY_COMMAND "${CLANG_TIDY_EXE}")
	set_target_properties(${PROJECT_NAME} PROPERTIES CXX_CLANG_TIDY "${CLANG_TIDY_COMMAND}")
endif()


target_include_directories(
	${PROJECT_NAME}
	PUBLIC
	"src/"
	"lib/glm/"
	"lib/glfw/include/"
	"lib/spdlog/include/"
	"lib/assimp/include/"
	"lib/imgui/"
	"lib/stb_image/"
	"${Vulkan_INCLUDE_DIR}"
)

if(${VKPBR_USE_PRE_BUILT_LIB})
	file(GLOB_RECURSE BUILD_LIB "${CMAKE_SOURCE_DIR}/binaries/${CMAKE_BUILD_TYPE}/*.lib")
	target_include_directories(
		${PROJECT_NAME}
		PUBLIC
		# assimp config headers
		"binaries/${CMAKE_BUILD_TYPE}/include/"
		"binaries/${CMAKE_BUILD_TYPE}/contrib/zlib/"
	)
else()
	set(
		BUILD_LIB

		glfw
		spdlog::spdlog
		assimp
		imgui
	)
endif()

target_link_libraries(
	${PROJECT_NAME}
	"${BUILD_LIB}"
	"${Vulkan_LIBRARY}"
)


# compile shaders
set(VKPBR_SHADER_SRC "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders")
set(VKPBR_SHADER_BIN "${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/out")

file(
	GLOB SHADERS
	"${VKPBR_SHADER_SRC}/*.vert"
	"${VKPBR_SHADER_SRC}/*.frag"
)

add_custom_command(
	COMMAND
	"${CMAKE_COMMAND}" -E make_directory "${VKPBR_SHADER_BIN}"
	OUTPUT "${VKPBR_SHADER_BIN}"
	COMMENT "Creating ${VKPBR_SHADER_BIN}"
)

foreach(source IN LISTS SHADERS)
	get_filename_component(FILENAME "${source}" NAME)
	add_custom_command(
		COMMAND
		"${Vulkan_GLSLC_EXECUTABLE}" "${source}" -o "${VKPBR_SHADER_BIN}/${FILENAME}.spv"
		OUTPUT "${VKPBR_SHADER_BIN}/${FILENAME}.spv"
		DEPENDS "${source}" "${VKPBR_SHADER_BIN}"
		COMMENT "Compiling ${FILENAME}")
	list(APPEND VKPBR_SPV_SHADERS "${VKPBR_SHADER_BIN}/${FILENAME}.spv")
endforeach()

add_custom_target(shaders ALL DEPENDS ${VKPBR_SPV_SHADERS})
