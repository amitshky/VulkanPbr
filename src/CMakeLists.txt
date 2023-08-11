file(GLOB_RECURSE SHADERS_BASICS_SRC_FILES "${PROJECT_SOURCE_DIR}/src/*.cpp")

add_executable(
	${PROJECT_NAME}
	"${SHADERS_BASICS_SRC_FILES}"
	../lib/imgui/backends/imgui_impl_glfw.cpp
	../lib/imgui/backends/imgui_impl_vulkan.cpp
)

if(NOT ${SHADERS_BASICS_USE_PRE_BUILT_LIB})
	# copy binaries from output directory to binaries
	# so that we can reuse them during build
	if(${CMAKE_BUILD_TYPE} MATCHES "Debug" AND (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU"))
		file(MAKE_DIRECTORY "../binaries")
		add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
			COMMAND "${CMAKE_COMMAND}" -E copy
				"${CMAKE_SOURCE_DIR}/build/lib/glfw/src/glfw3.lib"
				"${CMAKE_SOURCE_DIR}/binaries/")
		add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
			COMMAND "${CMAKE_COMMAND}" -E copy
				"${CMAKE_SOURCE_DIR}/build/lib/spdlog/spdlogd.lib"
				"${CMAKE_SOURCE_DIR}/binaries/")
		add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
			COMMAND "${CMAKE_COMMAND}" -E copy
				"${CMAKE_SOURCE_DIR}/build/lib/imgui.lib"
				"${CMAKE_SOURCE_DIR}/binaries/")

	elseif(${CMAKE_BUILD_TYPE} MATCHES "Release" AND (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU"))
		file(MAKE_DIRECTORY "../binaries")
		add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
			COMMAND "${CMAKE_COMMAND}" -E copy
				"${CMAKE_SOURCE_DIR}/build/lib/glfw/src/glfw3.lib"
				"${CMAKE_SOURCE_DIR}/binaries/")
		add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
			COMMAND "${CMAKE_COMMAND}" -E copy
				"${CMAKE_SOURCE_DIR}/build/lib/spdlog/spdlog.lib"
				"${CMAKE_SOURCE_DIR}/binaries/")
		add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
			COMMAND "${CMAKE_COMMAND}" -E copy
				"${CMAKE_SOURCE_DIR}/build/lib/imgui.lib"
				"${CMAKE_SOURCE_DIR}/binaries/")

	elseif(${CMAKE_BUILD_TYPE} MATCHES "Debug" AND CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
		file(MAKE_DIRECTORY "../binaries")
		add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
			COMMAND "${CMAKE_COMMAND}" -E copy
				"${CMAKE_SOURCE_DIR}/build/lib/glfw/src/Debug/glfw3.lib"
				"${CMAKE_SOURCE_DIR}/binaries/")
		add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
			COMMAND "${CMAKE_COMMAND}" -E copy
				"${CMAKE_SOURCE_DIR}/build/lib/spdlog/Debug/spdlogd.lib"
				"${CMAKE_SOURCE_DIR}/binaries/")
		add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
			COMMAND "${CMAKE_COMMAND}" -E copy
				"${CMAKE_SOURCE_DIR}/build/lib/Debug/imgui.lib"
				"${CMAKE_SOURCE_DIR}/binaries/")

	elseif(${CMAKE_BUILD_TYPE} MATCHES "Release" AND CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
		file(MAKE_DIRECTORY "../binaries")
		add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
			COMMAND "${CMAKE_COMMAND}" -E copy
				"${CMAKE_SOURCE_DIR}/build/lib/glfw/src/Release/glfw3.lib"
				"${CMAKE_SOURCE_DIR}/binaries/")
		add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
			COMMAND "${CMAKE_COMMAND}" -E copy
				"${CMAKE_SOURCE_DIR}/build/lib/spdlog/Release/spdlog.lib"
				"${CMAKE_SOURCE_DIR}/binaries/")
		add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
			COMMAND "${CMAKE_COMMAND}" -E copy
				"${CMAKE_SOURCE_DIR}/build/lib/Release/imgui.lib"
				"${CMAKE_SOURCE_DIR}/binaries/")
	endif()
endif()