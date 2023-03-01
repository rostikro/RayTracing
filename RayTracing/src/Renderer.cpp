#include "Renderer.h"

#include <iostream>
#include <execution>

#include <Walnut/Random.h>

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

	ResetFrameIndex();

	delete[] m_ImageData;
	m_ImageData = new uint32_t[width * height];

	delete[] m_AccumulationData;
	m_AccumulationData = new glm::vec4[width * height];

	m_ImageHorizontalIt.resize(width);
	for (uint32_t i = 0; i < width; i++)
		m_ImageHorizontalIt[i] = i;
	m_ImageVerticalIt.resize(height);
	for (uint32_t i = 0; i < height; i++)
		m_ImageVerticalIt[i] = i;
}

void Renderer::Render(const Scene& scene, const Camera& camera)
{
	m_ActiveScene = &scene;
	m_ActiveCamera = &camera;

	if (m_FrameIndex == 1)
		memset(m_AccumulationData, 0, m_FinalImage->GetHeight() * m_FinalImage->GetWidth() * sizeof(glm::vec4));

	/*if (m_FrameIndex > 20)
		return;*/
	if (m_Settings.Multithreding)
	{
		std::for_each(std::execution::par, m_ImageVerticalIt.begin(), m_ImageVerticalIt.end(),
			[this](uint32_t y)
			{
				/*std::for_each(std::execution::par, m_ImageHorizontalIt.begin(), m_ImageHorizontalIt.end(),
				[this, y](uint32_t x)
					{

					});*/
				for (uint32_t x = 0; x < m_FinalImage->GetWidth(); x++)
				{
					glm::vec4 color = RenderPixel(x, y);
					m_AccumulationData[y * m_FinalImage->GetWidth() + x] += color;

					glm::vec4 accumulatedColor = m_AccumulationData[y * m_FinalImage->GetWidth() + x];
					accumulatedColor /= (float)m_FrameIndex;

					accumulatedColor = glm::clamp(accumulatedColor, glm::vec4(0.0f), glm::vec4(1.0f));
					m_ImageData[y * m_FinalImage->GetWidth() + x] = Utils::ConvertToRGBA(accumulatedColor);
				}
			});
	}
	else
	{
		for (uint32_t y = 0; y < m_FinalImage->GetHeight(); y++)
		{
			for (uint32_t x = 0; x < m_FinalImage->GetWidth(); x++)
			{
				glm::vec4 color = RenderPixel(x, y);
				m_AccumulationData[y * m_FinalImage->GetWidth() + x] += color;

				glm::vec4 accumulatedColor = m_AccumulationData[y * m_FinalImage->GetWidth() + x];
				accumulatedColor /= (float)m_FrameIndex;

				accumulatedColor = glm::clamp(accumulatedColor, glm::vec4(0.0f), glm::vec4(1.0f));
				m_ImageData[y * m_FinalImage->GetWidth() + x] = Utils::ConvertToRGBA(accumulatedColor);
			}
		}
	}

	m_FinalImage->SetData(m_ImageData);

	if (m_Settings.Accumulate)
		m_FrameIndex++;
	else
		m_FrameIndex = 1;
}

glm::vec4 Renderer::RenderPixel(uint32_t x, uint32_t y)
{
	Ray ray;
	ray.Origin = m_ActiveCamera->GetPosition();
	ray.Direction = m_ActiveCamera->GetRayDirections()[x + y * m_FinalImage->GetWidth()];

	glm::vec3 color(0.0f, 0.0f, 0.0f);
	float multiplier = 1.0f;

	int bounces = 5;

	for (int i = 0; i < bounces; i++)
	{
		Renderer::HitPayload payload = TraceRay(ray);
		if (payload.ObjectIndex == -1)
		{
			glm::vec3 skyColor = glm::vec3(0.6f, 0.7f, 0.9f);
			color += skyColor * multiplier;
			break;
		}

		glm::vec3 lightDirection = glm::normalize(glm::vec3(-1.0f, -1.0f, -1.0f));
		float lightIntensity = glm::max(glm::dot(payload.WorldNormal, -lightDirection), 0.0f); // == cos(angle)

		const Sphere& sphere = m_ActiveScene->Spheres[payload.ObjectIndex];
		const Material& material = m_ActiveScene->Materials[sphere.MaterialIndex];

		glm::vec3 sphereColor = material.Albedo;
		sphereColor *= lightIntensity;
		color += sphereColor * multiplier;

		multiplier *= 0.5f;

		ray.Origin = payload.WorldPosition + payload.WorldNormal * 0.00001f;
		ray.Direction = glm::reflect(ray.Direction, 
			payload.WorldNormal + material.Roughness * Walnut::Random::Vec3(-0.5f, 0.5f));
	}

	// Trace second ray

	/*Ray ray2;
	ray2.Origin = payload.WorldPosition + payload.WorldNormal * 0.00001f;
	ray2.Direction = glm::reflect(ray.Direction, payload.WorldNormal);

	Renderer::HitPayload payload2 = TraceRay(ray2);

	if (payload2.ObjectIndex != -1)
	{
		const Sphere& sphere2 = m_ActiveScene->Spheres[payload.ObjectIndex];

		sphereColor = sphere2.Albedo;
		sphereColor *= lightIntensity;
	}*/

	return glm::vec4(color, 1.0f);
}

Renderer::HitPayload Renderer::TraceRay(const Ray& ray)
{
	// Ray:
	// Vec3 a - ray origin (const)
	// Vec3 b - ray direction;

	// Sphere:
	// Vec3 c - sphere origin (const)
	// float r - radius (const)

	// Light:
	// Vec3 l - light direction

	// Equation:
	// (bx^2 + by^2 + bz^2) * t^2 + 2 * (axbx + ayby + azbz) * t + (ax^2 + ay^2 + az^2 - r^2) = 0;
	// Where t - our intersection point, (bx^2 + by^2 + bz^2) is A, 2 * (axbx + ayby + azbz) is B, (ax^2 + ay^2 + az^2 - r^2) is C;

	int closestSphereIndex = -1;
	float hitDistance = FLT_MAX;

	for (size_t i=0; i<m_ActiveScene->Spheres.size(); i++)
	{
		const Sphere& sphere = m_ActiveScene->Spheres[i];
		glm::vec3 origin = ray.Origin - sphere.Position;

		float a = glm::dot(ray.Direction, ray.Direction);
		float b = 2.0f * glm::dot(origin, ray.Direction);
		float c = glm::dot(origin, origin) - sphere.Radius * sphere.Radius;

		// Discriminant = b^2 - 4ac

		float discriminant = b * b - 4.0f * a * c;

		if (discriminant < 0.0f)
			continue;

		// Ray hitting processing
		// (-b +- sqrt(discriminant)) / (2 * a)
		float closestT = (-b - sqrt(discriminant)) / (2.0f * a);

		if (closestT > 0.0f && closestT < hitDistance)
		{
			hitDistance = closestT;
			closestSphereIndex = (int)i;
		}
	}

	if (closestSphereIndex == -1)
		return Miss(ray);

	return ClosestHit(ray, hitDistance, closestSphereIndex);
}

Renderer::HitPayload Renderer::ClosestHit(const Ray& ray, float hitDistance, int objectIndex)
{
	Renderer::HitPayload payload;
	payload.HitDistance = hitDistance;
	payload.ObjectIndex = objectIndex;

	const Sphere& closestSphere = m_ActiveScene->Spheres[objectIndex];

	glm::vec3 origin = ray.Origin - closestSphere.Position;
	payload.WorldPosition = origin + ray.Direction * hitDistance;
	payload.WorldNormal = glm::normalize(payload.WorldPosition);

	payload.WorldPosition += closestSphere.Position;

	return payload;
}

Renderer::HitPayload Renderer::Miss(const Ray& ray)
{
	Renderer::HitPayload payload;
	payload.ObjectIndex = -1;
	return payload;
}
