# Set the minimum CMake version
cmake_minimum_required(VERSION 3.10)

project(04_dof)

add_executable(04_dof
	main.cpp
	renderer.hpp
	scene_definition.hpp
)
target_sources(04_dof PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/../glad/src/glad.c
)
target_link_libraries(04_dof utils glm::glm glfw OpenGL::GL)
target_include_directories(04_dof PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/../glad/include
	${CMAKE_CURRENT_SOURCE_DIR}/../utils
	${CMAKE_CURRENT_SOURCE_DIR}/..
	${CMAKE_CURRENT_SOURCE_DIR}
)
