file(GLOB_RECURSE VKPBR_SRC_FILES "${PROJECT_SOURCE_DIR}/src/*.cpp")

add_executable(
	${PROJECT_NAME}
	"${VKPBR_SRC_FILES}"
	../lib/imgui/backends/imgui_impl_glfw.cpp
	../lib/imgui/backends/imgui_impl_vulkan.cpp
)

if(NOT ${VKPBR_USE_PRE_BUILT_LIB} AND NOT ${LINUX})
	file(MAKE_DIRECTORY "${CMAKE_SOURCE_DIR}/binaries")
	file(MAKE_DIRECTORY "${CMAKE_SOURCE_DIR}/binaries/${CMAKE_BUILD_TYPE}")
	# copy binaries from output directory to binaries
	# so that we can reuse them during build
	if(${CMAKE_BUILD_TYPE} MATCHES "Debug" AND (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU"))
		add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
			COMMAND "${CMAKE_COMMAND}" -E copy
				"${CMAKE_SOURCE_DIR}/build/lib/glfw/src/glfw3.lib"
				"${CMAKE_SOURCE_DIR}/binaries/${CMAKE_BUILD_TYPE}/")
		add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
			COMMAND "${CMAKE_COMMAND}" -E copy
				"${CMAKE_SOURCE_DIR}/build/lib/spdlog/spdlogd.lib"
				"${CMAKE_SOURCE_DIR}/binaries/${CMAKE_BUILD_TYPE}/")
		add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
			COMMAND "${CMAKE_COMMAND}" -E copy
				"${CMAKE_SOURCE_DIR}/build/lib/imguid.lib"
				"${CMAKE_SOURCE_DIR}/binaries/${CMAKE_BUILD_TYPE}/")
		# assimp
		add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
			COMMAND "${CMAKE_COMMAND}" -E copy
				"${CMAKE_SOURCE_DIR}/build/lib/assimp/lib/assimpd.lib"
				"${CMAKE_SOURCE_DIR}/binaries/${CMAKE_BUILD_TYPE}/")
		add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
			COMMAND "${CMAKE_COMMAND}" -E copy
				"${CMAKE_SOURCE_DIR}/build/lib/assimp/contrib/zlib/zlibstaticd.lib"
				"${CMAKE_SOURCE_DIR}/binaries/${CMAKE_BUILD_TYPE}/")
		add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
			COMMAND "${CMAKE_COMMAND}" -E copy_directory
				"${CMAKE_SOURCE_DIR}/build/lib/assimp/include/"
				"${CMAKE_SOURCE_DIR}/binaries/${CMAKE_BUILD_TYPE}/include/")

	elseif(${CMAKE_BUILD_TYPE} MATCHES "Release" AND (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU"))
		add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
			COMMAND "${CMAKE_COMMAND}" -E copy
				"${CMAKE_SOURCE_DIR}/build/lib/glfw/src/glfw3.lib"
				"${CMAKE_SOURCE_DIR}/binaries/${CMAKE_BUILD_TYPE}/")
		add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
			COMMAND "${CMAKE_COMMAND}" -E copy
				"${CMAKE_SOURCE_DIR}/build/lib/spdlog/spdlog.lib"
				"${CMAKE_SOURCE_DIR}/binaries/${CMAKE_BUILD_TYPE}/")
		add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
			COMMAND "${CMAKE_COMMAND}" -E copy
				"${CMAKE_SOURCE_DIR}/build/lib/imgui.lib"
				"${CMAKE_SOURCE_DIR}/binaries/${CMAKE_BUILD_TYPE}/")
		# assimp
		add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
			COMMAND "${CMAKE_COMMAND}" -E copy
				"${CMAKE_SOURCE_DIR}/build/lib/assimp/lib/assimp.lib"
				"${CMAKE_SOURCE_DIR}/binaries/${CMAKE_BUILD_TYPE}/")
		add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
			COMMAND "${CMAKE_COMMAND}" -E copy
				"${CMAKE_SOURCE_DIR}/build/lib/assimp/contrib/zlib/zlibstatic.lib"
				"${CMAKE_SOURCE_DIR}/binaries/${CMAKE_BUILD_TYPE}/")
		add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
			COMMAND "${CMAKE_COMMAND}" -E copy_directory
				"${CMAKE_SOURCE_DIR}/build/lib/assimp/include/"
				"${CMAKE_SOURCE_DIR}/binaries/${CMAKE_BUILD_TYPE}/include/")

	elseif(${CMAKE_BUILD_TYPE} MATCHES "Debug" AND CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
		add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
			COMMAND "${CMAKE_COMMAND}" -E copy
				"${CMAKE_SOURCE_DIR}/build/lib/glfw/src/Debug/glfw3.lib"
				"${CMAKE_SOURCE_DIR}/binaries/${CMAKE_BUILD_TYPE}/")
		add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
			COMMAND "${CMAKE_COMMAND}" -E copy
				"${CMAKE_SOURCE_DIR}/build/lib/spdlog/Debug/spdlogd.lib"
				"${CMAKE_SOURCE_DIR}/binaries/${CMAKE_BUILD_TYPE}/")
		add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
			COMMAND "${CMAKE_COMMAND}" -E copy
				"${CMAKE_SOURCE_DIR}/build/lib/Debug/imgui.lib"
				"${CMAKE_SOURCE_DIR}/binaries/${CMAKE_BUILD_TYPE}/")

		# FIXME: doesn't copy the binaries if MSVC is used (same for release mode)
		# TODO: copy zlib binaries and config.h
		# add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
		# 	COMMAND "${CMAKE_COMMAND}" -E copy
		# 		"${CMAKE_SOURCE_DIR}/build/lib/assimp/lib/Debug/assimp-vc143-mtd.lib"
		# 		"${CMAKE_SOURCE_DIR}/binaries/${CMAKE_BUILD_TYPE}/")
		# add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
		# 	COMMAND "${CMAKE_COMMAND}" -E copy
		# 		"${CMAKE_SOURCE_DIR}/build/lib/assimp/lib/Debug/assimp-vc143-mtd.pdb"
		# 		"${CMAKE_SOURCE_DIR}/binaries/${CMAKE_BUILD_TYPE}/")

	elseif(${CMAKE_BUILD_TYPE} MATCHES "Release" AND CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
		add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
			COMMAND "${CMAKE_COMMAND}" -E copy
				"${CMAKE_SOURCE_DIR}/build/lib/glfw/src/Release/glfw3.lib"
				"${CMAKE_SOURCE_DIR}/binaries/${CMAKE_BUILD_TYPE}/")
		add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
			COMMAND "${CMAKE_COMMAND}" -E copy
				"${CMAKE_SOURCE_DIR}/build/lib/spdlog/Release/spdlog.lib"
				"${CMAKE_SOURCE_DIR}/binaries/${CMAKE_BUILD_TYPE}/")
		add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
			COMMAND "${CMAKE_COMMAND}" -E copy
				"${CMAKE_SOURCE_DIR}/build/lib/Release/imgui.lib"
				"${CMAKE_SOURCE_DIR}/binaries/${CMAKE_BUILD_TYPE}/")

		# add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
		# 	COMMAND "${CMAKE_COMMAND}" -E copy
		# 		"${CMAKE_SOURCE_DIR}/build/lib/assimp/lib/Release/assimp-vc143-mt.lib"
		# 		"${CMAKE_SOURCE_DIR}/binaries/${CMAKE_BUILD_TYPE}/")
	endif()
endif()