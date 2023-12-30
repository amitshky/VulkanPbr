#include "core/input.h"

#include <GLFW/glfw3.h>
#include "engine/engine.h"


bool Input::IsKeyPressed(Key keycode)
{
	GLFWwindow* window = Engine::GetWindowHandle();
	auto status = glfwGetKey(window, keycode);
	return status == GLFW_PRESS || status == GLFW_REPEAT;
}

bool Input::IsKeyReleased(Key keycode)
{
	GLFWwindow* window = Engine::GetWindowHandle();
	auto status = glfwGetKey(window, keycode);
	return status == GLFW_RELEASE;
}

bool Input::IsMouseButtonPressed(Mouse button)
{
	GLFWwindow* window = Engine::GetWindowHandle();
	auto status = glfwGetMouseButton(window, button);
	return status == GLFW_PRESS;
}

bool Input::IsMouseButtonReleased(Mouse button)
{
	GLFWwindow* window = Engine::GetWindowHandle();
	auto status = glfwGetMouseButton(window, button);
	return status == GLFW_RELEASE;
}

std::pair<float, float> Input::GetMousePosition()
{
	GLFWwindow* window = Engine::GetWindowHandle();
	double xpos = 0.0;
	double ypos = 0.0;
	glfwGetCursorPos(window, &xpos, &ypos);
	return { static_cast<float>(xpos), static_cast<float>(ypos) };
}

float Input::GetMouseX()
{
	auto [x, y] = Input::GetMousePosition();
	return x;
}

float Input::GetMouseY()
{
	auto [x, y] = Input::GetMousePosition();
	return y;
}
