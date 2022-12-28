#pragma once

#include "Walnut/Image.h"

#include "glm/glm.hpp"
#include <memory>

class Renderer
{
public:
	void Resize(uint32_t width, uint32_t height);
	void Render();
	std::shared_ptr<Walnut::Image> GetFinalImage() const { return m_FinalImage; }

private:
	glm::vec4 RenderPixel(glm::vec2 coordinates);

private:
	std::shared_ptr<Walnut::Image> m_FinalImage;
	float m_AspectRatio;

	uint32_t* m_ImageData = nullptr;
};
