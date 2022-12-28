#include "Renderer.h"

namespace Utils
{
	static uint32_t ConvertToRGBA(const glm::vec4& color)
	{
		uint8_t r = (uint8_t)(color.r * 255.0f);
		uint8_t g = (uint8_t)(color.g * 255.0f);
		uint8_t b = (uint8_t)(color.b * 255.0f);
		uint8_t a = (uint8_t)(color.a * 255.0f);

		uint32_t result = (a << 24) | (b << 16) | (g << 8) | r;
		return result;
	}
}

void Renderer::Resize(uint32_t width, uint32_t height)
{
	if (m_FinalImage)
	{
		// No resize necessary
		if (m_FinalImage->GetWidth() == width && m_FinalImage->GetHeight() == height)
			return;

		m_FinalImage->Resize(width, height);
	}
	else
	{
		m_FinalImage = std::make_shared<Walnut::Image>(width, height, Walnut::ImageFormat::RGBA);
	}

	delete[] m_ImageData;
	m_ImageData = new uint32_t[width * height];

	m_AspectRatio = (float)width / (float)height;
}

void Renderer::Render()
{
	for (uint32_t y = 0; y < m_FinalImage->GetHeight(); ++y)
	{
		for (uint32_t x = 0; x < m_FinalImage->GetWidth(); ++x)
		{
			glm::vec2 screenCoordinates((float)x / (float)m_FinalImage->GetWidth(), (float)y / (float)m_FinalImage->GetHeight());
			screenCoordinates = screenCoordinates * 2.0f - 1.0f;
			screenCoordinates.x *= m_AspectRatio;

			glm::vec4 color = RenderPixel(screenCoordinates);
			color = glm::clamp(color, glm::vec4(0.0f), glm::vec4(1.0f));
			m_ImageData[y * m_FinalImage->GetWidth() + x] = Utils::ConvertToRGBA(color);
		}
	}

	m_FinalImage->SetData(m_ImageData);
}

glm::vec4 Renderer::RenderPixel(glm::vec2 coordinates)
{
	// Ray:
	// Vec3 a - ray origin (const)
	// Vec3 b - ray direction;

	glm::vec3 rayOrigin = { 0.0f, 0.0f, 5.0f };
	glm::vec3 rayDirection = { coordinates.x, coordinates.y, -1.0f };

	// Sphere:
	// Vec3 c - sphere origin (const)
	// float r - radius (const)

	glm::vec3 sphereOrigin = { 0.0f, 0.0f, 0.0f };
	float sphereRadius = 2.0f;

	// Light:
	// Vec3 l - light direction

	 glm::vec3 lightDirection = glm::normalize(glm::vec3(-1.0f, 0.0f, -1.0f));

	// Equation:
	// (bx^2 + by^2 + bz^2) * t^2 + 2 * (axbx + ayby + azbz) * t + (ax^2 + ay^2 + az^2 - r^2) = 0;
	// Where t - our intersection point, (bx^2 + by^2 + bz^2) is A, 2 * (axbx + ayby + azbz) is B, (ax^2 + ay^2 + az^2 - r^2) is C;

	/*float a = glm::dot(rayDirection, rayDirection);
	float b = 2.0f * glm::dot(rayOrigin, rayDirection);
	float c = glm::dot(rayOrigin, rayOrigin) - sphereRadius * sphereRadius;*/

	// Equation with sphere origin:
	// (bx^2 + by^2 + bz^2) * t^2 + 2 * (axbx + ayby + azbz - cxbx - cyby - czbz) * t +
	// + (ax^2 + ay^2 + az^2 + cx^2 + cy^2 + cz^2 - 2 * (cxax + cyay + czaz) - r^2);

	float a = glm::dot(rayDirection, rayDirection);
	float b = 2.0f * (glm::dot(rayOrigin, rayDirection) - glm::dot(sphereOrigin, rayDirection));
	float c = glm::dot(rayOrigin, rayOrigin) + glm::dot(sphereOrigin, sphereOrigin) - 2.0f * (glm::dot(sphereOrigin, rayOrigin)) - sphereRadius * sphereRadius;
		
	// Discriminant = b^2 - 4ac

	float discriminant = b * b - 4.0f * a * c;

	if (discriminant < 0.0f)
		return glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

	// (-b +- sqrt(discriminant)) / (2 * a)
	float closestT = (-b - sqrt(discriminant)) / (2.0f * a);

	glm::vec3 hitPoint = rayOrigin + rayDirection * closestT;
	glm::vec3 normal = glm::normalize(hitPoint - sphereOrigin);

	float d = glm::max(glm::dot(normal, -lightDirection), 0.0f); // == cos(angle)

	glm::vec3 sphereColor(1.0f, 0.0f, 1.0f);
	sphereColor *= d;
	return glm::vec4(sphereColor, 1.0f);
}
