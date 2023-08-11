cmake_minimum_required(VERSION 3.24)

set(CMAKE_CXX_STANDARD 17)
project(vulkanPbr LANGUAGES CXX)

# set Visual Studio startup project
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})


# project options
option(VKPBR_USE_PRE_BUILT_LIB "Use pre-built libraries" ON)
option(VKPBR_ENABLE_CLANG_TIDY_CHECK "Enables clang-tidy check during build" OFF)

# GLFW options
option(GLFW_BUILD_EXAMPLES "Build the GLFW example programs" OFF)
option(GLFW_BUILD_TESTS "Build the GLFW test programs" OFF)
option(GLFW_BUILD_DOCS "Build the GLFW documentation" OFF)
option(GLFW_INSTALL "Generate installation target" OFF)
option(GLFW_DOCUMENT_INTERNALS "Include internals in documentation" OFF)


add_subdirectory(src)
if (NOT ${VKPBR_USE_PRE_BUILT_LIB})
	add_subdirectory(lib)
endif()


if(UNIX AND NOT APPLE)
	set(LINUX TRUE)
endif()

if(MSVC)
	target_compile_options(${PROJECT_NAME} PUBLIC "/W4;/analyze;/MP;")
elseif(GNU OR Clang)
	set(GCC_CLANG_COMPILE_OPTIONS "-Wall;-Wextra;-Wpedantic;-Wconversion;-Wshadow;")
	set(GCC_CLANG_COMPILE_OPTIONS_DEBUG "-O0;-g;")
	set(GCC_CLANG_COMPILE_OPTIONS_RELEASE "-O3;")

	target_compile_options(${PROJECT_NAME} PUBLIC ${GCC_CLANG_COMPILE_OPTIONS})
	target_compile_options(${PROJECT_NAME} PUBLIC $<$<CONFIG:Debug>:${GCC_CLANG_COMPILE_OPTIONS_DEBUG}>)
	target_compile_options(${PROJECT_NAME} PUBLIC $<$<CONFIG:Release>:${GCC_CLANG_COMPILE_OPTIONS_RELEASE}>)

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
	"lib/"
	"lib/glm/"
	"lib/glfw/include/"
	"lib/spdlog/include/"
	"lib/imgui/"
	"${Vulkan_INCLUDE_DIR}"
)


set(
	DEBUG_BUILD_LIB
	"${CMAKE_SOURCE_DIR}/binaries/glfw3.lib"
	"${CMAKE_SOURCE_DIR}/binaries/imgui.lib"
	"${CMAKE_SOURCE_DIR}/binaries/spdlogd.lib"
)
set(
	RELEASE_BUILD_LIB
	"${CMAKE_SOURCE_DIR}/binaries/glfw3.lib"
	"${CMAKE_SOURCE_DIR}/binaries/imgui.lib"
	"${CMAKE_SOURCE_DIR}/binaries/spdlog.lib"
)


if(CMAKE_BUILD_TYPE MATCHES Debug AND ${VKPBR_USE_PRE_BUILT_LIB})
	set(BUILD_LIB ${DEBUG_BUILD_LIB})
elseif(CMAKE_BUILD_TYPE MATCHES Release AND ${VKPBR_USE_PRE_BUILT_LIB})
	set(BUILD_LIB ${RELEASE_BUILD_LIB})
else()
	set(
		BUILD_LIB

		glfw
		spdlog::spdlog
		imgui
	)
endif()

target_link_libraries(
	${PROJECT_NAME}
	"${BUILD_LIB}"
	"${Vulkan_LIBRARY}"
)
