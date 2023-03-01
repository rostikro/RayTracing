#include "Walnut/Application.h"
#include "Walnut/EntryPoint.h"

#include "Walnut/Image.h"
#include "Walnut/Random.h"
#include "Walnut/Timer.h"

#include "glm/gtc/type_ptr.hpp"

#include "Renderer.h"
#include "Camera.h"

using namespace Walnut;

class MainLayer : public Walnut::Layer
{
public:
	MainLayer()
		: m_Camera(45.0f, 0.1f, 100.0f)
	{
		// Materials definition
		Material& pink = m_Scene.Materials.emplace_back();
		pink.Albedo = { 1.0f, 0.0f, 1.0f };
		pink.Roughness = 0.0f;

		Material& blue = m_Scene.Materials.emplace_back();
		blue.Albedo = { 0.0f, 0.2f, 1.0f };
		blue.Roughness = 0.1f;

		Material& green = m_Scene.Materials.emplace_back();
		green.Albedo = { 0.0f, 1.0, 0.0f };
		green.Roughness = 0.3f;

		// Spheres definition
		{
			Sphere sphere;
			sphere.Position = { 0.0f, 0.0f, 0.0f };
			sphere.Radius = 1.0f;
			sphere.MaterialIndex = 0;
			m_Scene.Spheres.push_back(sphere);
		}

		{
			Sphere sphere;
			sphere.Position = { 5.0f, 0.5f, 0.0f };
			sphere.Radius = 1.5f;
			sphere.MaterialIndex = 2;
			m_Scene.Spheres.push_back(sphere);
		}

		{
			Sphere sphere;
			sphere.Position = { 0.0f, -101.0f, 0.0f };
			sphere.Radius = 100.0f;
			sphere.MaterialIndex = 1;
			m_Scene.Spheres.push_back(sphere);
		}
	}

	virtual void OnUpdate(float ts) override
	{
		if (m_Camera.OnUpdate(ts))
			m_Renderer.ResetFrameIndex();
	}

	virtual void OnUIRender() override
	{
		ImGui::Begin("Settings");
		ImGui::Text("Render time: %.3fms", m_RenderTime);
		ImGui::Text("Viewport size: %dx%d", m_ViewportWidth, m_ViewportHeight );

		ImGui::Checkbox("Accumulate", &m_Renderer.GetSettings().Accumulate);
		ImGui::Checkbox("Multithreding", &m_Renderer.GetSettings().Multithreding);

		if (ImGui::Button("Reset"))
			m_Renderer.ResetFrameIndex();

		ImGui::End();

		ImGui::Begin("Scene");
		for (size_t i=0; i<m_Scene.Spheres.size(); i++)
		{
			ImGui::PushID(i);

			Sphere& sphere = m_Scene.Spheres[i];
			ImGui::Text("Sphere #%u", i+1);
			ImGui::DragFloat3("Position", glm::value_ptr(sphere.Position), 0.1f);
			ImGui::DragFloat("Radius", &sphere.Radius, 0.1f);
			ImGui::DragInt("Material ID", &sphere.MaterialIndex, 1.0f, 0, (int)m_Scene.Materials.size() - 1);
			ImGui::Separator();

			ImGui::PopID();
		}
		ImGui::Separator();

		for (size_t i = 0; i < m_Scene.Materials.size(); i++)
		{
			ImGui::PushID(i);

			Material& material = m_Scene.Materials[i];
			ImGui::Text("Material ID: %u", i);
			ImGui::ColorEdit3("Color", glm::value_ptr(material.Albedo));
			ImGui::DragFloat("Roughness", &material.Roughness, 0.01f, 0.0f, 1.0f);
			ImGui::DragFloat("Metallic", &material.Metallic, 0.1f);
			ImGui::Separator();

			ImGui::PopID();
		}
		/*ImGui::DragFloat3("Position", glm::value_ptr(m_Scene.Spheres[0].Position), 0.1f);
		ImGui::DragFloat("Radius", &m_Scene.Spheres[0].Radius, 0.1f);
		ImGui::ColorPicker3("Color", glm::value_ptr(m_Scene.Spheres[0].Albedo));*/

		ImGui::End();

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("Viewport");

		m_ViewportWidth = ImGui::GetContentRegionAvail().x;
		m_ViewportHeight = ImGui::GetContentRegionAvail().y;

		auto image = m_Renderer.GetFinalImage();
		if(image)
			ImGui::Image(image->GetDescriptorSet(), { (float)image->GetWidth(), (float)image->GetHeight() }, ImVec2(0, 1), ImVec2(1, 0));

		ImGui::End();
		ImGui::PopStyleVar();

		Render();
	}

	void Render()
	{
		m_Timer.Reset();

		m_Renderer.Resize(m_ViewportWidth, m_ViewportHeight);
		m_Camera.OnResize(m_ViewportWidth, m_ViewportHeight);
		m_Renderer.Render(m_Scene, m_Camera);

		m_RenderTime = m_Timer.ElapsedMillis();
	}

private:
	uint32_t m_ViewportWidth = 0, m_ViewportHeight = 0;

	Renderer m_Renderer;
	Camera m_Camera;
	Scene m_Scene;

	Timer m_Timer;
	float m_RenderTime = 0.0f;
};

Walnut::Application* Walnut::CreateApplication(int argc, char** argv)
{
	Walnut::ApplicationSpecification spec;
	spec.Name = "Ray Tracing";

	Walnut::Application* app = new Walnut::Application(spec);
	app->PushLayer<MainLayer>();
	app->SetMenubarCallback([app]()
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Exit"))
			{
				app->Close();
			}
			ImGui::EndMenu();
		}
	});
	return app;
}