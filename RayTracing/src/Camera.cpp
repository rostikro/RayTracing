#include "Camera.h"

#include "Walnut/Input/Input.h";

Camera::Camera(float verticalFOV, float nearCLip, float farClip)
	: m_VerticalFOV(verticalFOV), m_NearClip(nearCLip), m_FarClip(farClip)
{
	m_Direction = glm::vec3(0.0f, 0.0f, -1.0f);
	m_Position = glm::vec3(0.0f, 0.0f, 5.0f);
}

void Camera::OnUpdate(float ts)
{
	glm::vec2 mousePos = Input
}

