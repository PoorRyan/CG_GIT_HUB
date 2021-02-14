//Just a simple handler for simple initialization stuffs
#include "Utilities/BackendHandler.h"

#include <filesystem>
#include <json.hpp>
#include <fstream>

#include <Texture2D.h>
#include <Texture2DData.h>
#include <MeshBuilder.h>
#include <MeshFactory.h>
#include <NotObjLoader.h>
#include <ObjLoader.h>
#include <VertexTypes.h>
#include <ShaderMaterial.h>
#include <RendererComponent.h>
#include <TextureCubeMap.h>
#include <TextureCubeMapData.h>

#include <Timing.h>
#include <GameObjectTag.h>
#include <InputHelpers.h>

#include <IBehaviour.h>
#include <CameraControlBehaviour.h>
#include <FollowPathBehaviour.h>
#include <SimpleMoveBehaviour.h>

int main() {
	int frameIx = 0;
	float fpsBuffer[128];
	float minFps, maxFps, avgFps;
	int selectedVao = 0; // select cube by default
	std::vector<GameObject> controllables;

	BackendHandler::InitAll();

	// Let OpenGL know that we want debug output, and route it to our handler function
	glEnable(GL_DEBUG_OUTPUT);
	glDebugMessageCallback(BackendHandler::GlDebugMessage, nullptr);

	// Enable texturing
	glEnable(GL_TEXTURE_2D);

	// Push another scope so most memory should be freed *before* we exit the app
	{
#pragma region Shader and ImGui
		Shader::sptr passthroughShader = Shader::Create();
		passthroughShader->LoadShaderPartFromFile("shaders/passthrough_vert.glsl", GL_VERTEX_SHADER);
		passthroughShader->LoadShaderPartFromFile("shaders/passthrough_frag.glsl", GL_FRAGMENT_SHADER);
		passthroughShader->Link();

		Shader::sptr shader = Shader::Create();
		shader->LoadShaderPartFromFile("shaders/vertex_shader.glsl", GL_VERTEX_SHADER);
		shader->LoadShaderPartFromFile("shaders/frag_blinn_phong_textured.glsl", GL_FRAGMENT_SHADER);
		shader->Link();

		glm::vec3 lightPos = glm::vec3(0.0f, 0.0f, 5.0f);
		glm::vec3 lightCol = glm::vec3(0.9f, 0.85f, 0.5f);
		float     lightAmbientPow = 0.7f;
		float     lightSpecularPow = 1.0f;
		glm::vec3 ambientCol = glm::vec3(1.0f);
		float     ambientPow = 0.1f;
		float     lightLinearFalloff = 0.09f;
		float     lightQuadraticFalloff = 0.032f;
		float strength = 4.0f;
		float Threshold = 0.1f;

		// These are our application / scene level uniforms that don't necessarily update
		// every frame
		shader->SetUniform("u_LightPos", lightPos);
		shader->SetUniform("u_LightCol", lightCol);
		shader->SetUniform("u_AmbientLightStrength", lightAmbientPow);
		shader->SetUniform("u_SpecularLightStrength", lightSpecularPow);
		shader->SetUniform("u_AmbientCol", ambientCol);
		shader->SetUniform("u_AmbientStrength", ambientPow);
		shader->SetUniform("u_LightAttenuationConstant", 1.0f);
		shader->SetUniform("u_LightAttenuationLinear", lightLinearFalloff);
		shader->SetUniform("u_LightAttenuationQuadratic", lightQuadraticFalloff);
		shader->SetUniform("u_Strength", strength);
		shader->SetUniform("u_Threshold", Threshold);

		PostEffect* basicEffect;

		int activeEffect = 0;
		std::vector<PostEffect*> effects;

		SepiaEffect* sepiaEffect;
		GreyscaleEffect* greyscaleEffect;
		BloomEffect* theBloomEffect;

		// We'll add some ImGui controls to control our shader
		int noLighting = 0;
		int ambienceLighting = 0;
		int specularLighting = 0;
		int specularAmbLighting = 0;
		int NoTextures = 0;
		int BloomEffectInt = 0;


		shader->SetUniform("u_LightingIsOff", noLighting);
		shader->SetUniform("u_AmbientLighting", ambienceLighting);
		shader->SetUniform("u_SpecularLighting", specularLighting);
		shader->SetUniform("u_AmbientAndSpecularLighting", specularAmbLighting);
		shader->SetUniform("u_NoTextures", NoTextures);

		BackendHandler::imGuiCallbacks.push_back([&]() {

			if (ImGui::CollapsingHeader("Effect Controls"))
			{
				(ImGui::SliderInt("Chosen Effect", &activeEffect, 0, effects.size() - 1));

				if (activeEffect == 0)
				{
					ImGui::Text("Active Effect: Sepia Effect");
					SepiaEffect* temp = (SepiaEffect*)effects[activeEffect];
					float intensity = temp->GetIntensity();

					if (ImGui::SliderFloat("Intensity", &intensity, 0.0f, 1.0f))
					{
						temp->SetInvensity(intensity);
					}
				}
				
				if (activeEffect == 1)
				{
					ImGui::Text("Active Effect: Greyscale Effect");
					GreyscaleEffect* temp = (GreyscaleEffect*)effects[activeEffect];
					float intensity = temp->GetIntensity();

					if (ImGui::SliderFloat("Intensity", &intensity, 0.0f, 1.0f))
					{
						temp->SetInvensity(intensity);
					}
				}

				if (activeEffect == 2)
				{
					ImGui::Text("Active Effect: Bloom Effect");
					BloomEffect* temp = (BloomEffect*)effects[activeEffect];
					
					float threshold = temp->GetTheThreshold();

					if (ImGui::SliderFloat("Threshold", &threshold, 0.0f, 1.0f))
					{
						temp->SetTheThreshold(threshold);
					}
					
					
					BloomEffect* tempB = (BloomEffect*)effects[activeEffect];
					float passes = tempB->GetThePasses();

					if (ImGui::SliderFloat("Blur", &passes, 0.0f, 10.0f))
					{
						tempB->SetThePasses(passes);
					}
				}
			}

			if (ImGui::CollapsingHeader("Environment generation"))
			{
				if (ImGui::Button("Regenerate Environment", ImVec2(200.0f, 40.0f)))
				{
					EnvironmentGenerator::RegenerateEnvironment();
				}
			}
			if (ImGui::CollapsingHeader("Scene Level Lighting Settings"))
			{
				if (ImGui::ColorPicker3("Ambient Color", glm::value_ptr(ambientCol))) {
					shader->SetUniform("u_AmbientCol", ambientCol);
				}
				if (ImGui::SliderFloat("Fixed Ambient Power", &ambientPow, 0.01f, 1.0f)) {
					shader->SetUniform("u_AmbientStrength", ambientPow);
				}
			}
			if (ImGui::CollapsingHeader("Light Level Lighting Settings"))
			{
				if (ImGui::DragFloat3("Light Pos", glm::value_ptr(lightPos), 0.01f, -10.0f, 10.0f)) {
					shader->SetUniform("u_LightPos", lightPos);
				}
				if (ImGui::ColorPicker3("Light Col", glm::value_ptr(lightCol))) {
					shader->SetUniform("u_LightCol", lightCol);
				}
				if (ImGui::SliderFloat("Light Ambient Power", &lightAmbientPow, 0.0f, 1.0f)) {
					shader->SetUniform("u_AmbientLightStrength", lightAmbientPow);
				}
				if (ImGui::SliderFloat("Light Specular Power", &lightSpecularPow, 0.0f, 1.0f)) {
					shader->SetUniform("u_SpecularLightStrength", lightSpecularPow);
				}
				if (ImGui::DragFloat("Light Linear Falloff", &lightLinearFalloff, 0.01f, 0.0f, 1.0f)) {
					shader->SetUniform("u_LightAttenuationLinear", lightLinearFalloff);
				}
				if (ImGui::DragFloat("Light Quadratic Falloff", &lightQuadraticFalloff, 0.01f, 0.0f, 1.0f)) {
					shader->SetUniform("u_LightAttenuationQuadratic", lightQuadraticFalloff);
				}
			}

			if (ImGui::CollapsingHeader("Midterm Toggles - Ryan Chang"))
			{
				if (ImGui::Button("No Lighting!"))
				{
					shader->SetUniform("u_LightingIsOff", noLighting = 1);
					shader->SetUniform("u_AmbientLighting", ambienceLighting = 0);
					shader->SetUniform("u_SpecularLighting", specularLighting = 0);
					shader->SetUniform("u_AmbientAndSpecularLighting", specularAmbLighting = 0);
					shader->SetUniform("u_BloomOn", BloomEffectInt = 0);
					shader->SetUniform("u_NoTextures", NoTextures = 0);
				}

				if (ImGui::Button("Ambient Lighting Activated!"))
				{
					shader->SetUniform("u_LightingIsOff", noLighting = 0);
					shader->SetUniform("u_AmbientLighting", ambienceLighting = 1);
					shader->SetUniform("u_SpecularLighting", specularLighting = 0);
					shader->SetUniform("u_AmbientAndSpecularLighting", specularAmbLighting = 0);
					shader->SetUniform("u_BloomOn", BloomEffectInt = 0);
					shader->SetUniform("u_NoTextures", NoTextures = 0);
				}

				if (ImGui::Button("Specular Lighting Activated!"))
				{
					shader->SetUniform("u_LightingIsOff", noLighting = 0);
					shader->SetUniform("u_AmbientLighting", ambienceLighting = 0);
					shader->SetUniform("u_SpecularLighting", specularLighting = 1);
					shader->SetUniform("u_AmbientAndSpecularLighting", specularAmbLighting = 0);
					shader->SetUniform("u_BloomOn", BloomEffectInt = 0);
					shader->SetUniform("u_NoTextures", NoTextures = 0);
				}

				if (ImGui::Button("Ambient + Specular Lighting Activated!"))
				{
					shader->SetUniform("u_LightingIsOff", noLighting = 0);
					shader->SetUniform("u_AmbientLighting", ambienceLighting = 0);
					shader->SetUniform("u_SpecularLighting", specularLighting = 0);
					shader->SetUniform("u_AmbientAndSpecularLighting", specularAmbLighting = 1);
					shader->SetUniform("u_BloomOn", BloomEffectInt = 0);
					shader->SetUniform("u_NoTextures", NoTextures = 0);
				}

				if (ImGui::Button("Textures ON/OFF"))
				{
					shader->SetUniform("u_LightingIsOff", noLighting = 0);
					shader->SetUniform("u_AmbientLighting", ambienceLighting = 0);
					shader->SetUniform("u_SpecularLighting", specularLighting = 0);
					shader->SetUniform("u_AmbientAndSpecularLighting", specularAmbLighting = 0);
					shader->SetUniform("u_BloomOn", BloomEffectInt = 0);
					shader->SetUniform("u_NoTextures", NoTextures = 1);
				}
			}

			auto name = controllables[selectedVao].get<GameObjectTag>().Name;
			ImGui::Text(name.c_str());
			auto behaviour = BehaviourBinding::Get<SimpleMoveBehaviour>(controllables[selectedVao]);
			ImGui::Checkbox("Relative Rotation", &behaviour->Relative);

			ImGui::Text("Q/E -> Yaw\nLeft/Right -> Roll\nUp/Down -> Pitch\nY -> Toggle Mode");

			minFps = FLT_MAX;
			maxFps = 0;
			avgFps = 0;
			for (int ix = 0; ix < 128; ix++) {
				if (fpsBuffer[ix] < minFps) { minFps = fpsBuffer[ix]; }
				if (fpsBuffer[ix] > maxFps) { maxFps = fpsBuffer[ix]; }
				avgFps += fpsBuffer[ix];
			}
			ImGui::PlotLines("FPS", fpsBuffer, 128);
			ImGui::Text("MIN: %f MAX: %f AVG: %f", minFps, maxFps, avgFps / 128.0f);
			});

#pragma endregion 

		// GL states
		glEnable(GL_DEPTH_TEST);
		//glEnable(GL_CULL_FACE);
		glDepthFunc(GL_LEQUAL); // New 

#pragma region TEXTURE LOADING

// Load some textures from files
		Texture2D::sptr stone = Texture2D::LoadFromFile("images/Stone_001_Diffuse.png");
		Texture2D::sptr stoneSpec = Texture2D::LoadFromFile("images/Stone_001_Specular.png");
		Texture2D::sptr grass = Texture2D::LoadFromFile("images/grass.jpg");
		Texture2D::sptr noSpec = Texture2D::LoadFromFile("images/grassSpec.png");
		Texture2D::sptr box = Texture2D::LoadFromFile("images/box.bmp");
		Texture2D::sptr boxSpec = Texture2D::LoadFromFile("images/box-reflections.bmp");
		Texture2D::sptr simpleFlora = Texture2D::LoadFromFile("images/SimpleFlora.png");
		Texture2D::sptr leaves = Texture2D::LoadFromFile("images/TreeLeaves.jpg");
		Texture2D::sptr leavesSpec = Texture2D::LoadFromFile("images/grassSpec.png");

		Texture2D::sptr DarkBlueText = Texture2D::LoadFromFile("images/DarkBlueTexture.jpg");
		Texture2D::sptr DarkBlueSpec = Texture2D::LoadFromFile("images/grassSpec.png");

		
		LUT3D testCube("cubes/test.cube");

		// Load the cube map
		//TextureCubeMap::sptr environmentMap = TextureCubeMap::LoadFromImages("images/cubemaps/skybox/sample.jpg");
		TextureCubeMap::sptr environmentMap = TextureCubeMap::LoadFromImages("images/cubemaps/skybox/ToonSky.jpg");

		// Creating an empty texture
		Texture2DDescription desc = Texture2DDescription();
		desc.Width = 1;
		desc.Height = 1;
		desc.Format = InternalFormat::RGB8;
		Texture2D::sptr texture2 = Texture2D::Create(desc);
		// Clear it with a white colour
		texture2->Clear();

#pragma endregion

		///////////////////////////////////// Scene Generation //////////////////////////////////////////////////
#pragma region Scene Generation

// We need to tell our scene system what extra component types we want to support
		GameScene::RegisterComponentType<RendererComponent>();
		GameScene::RegisterComponentType<BehaviourBinding>();
		GameScene::RegisterComponentType<Camera>();

		// Create a scene, and set it to be the active scene in the application
		GameScene::sptr scene = GameScene::Create("test");
		Application::Instance().ActiveScene = scene;

		// We can create a group ahead of time to make iterating on the group faster
		entt::basic_group<entt::entity, entt::exclude_t<>, entt::get_t<Transform>, RendererComponent> renderGroup =
			scene->Registry().group<RendererComponent>(entt::get_t<Transform>());

		// Create a material and set some properties for it
		ShaderMaterial::sptr stoneMat = ShaderMaterial::Create();
		stoneMat->Shader = shader;
		stoneMat->Set("s_Diffuse", stone);
		stoneMat->Set("s_Specular", stoneSpec);
		stoneMat->Set("u_Shininess", 2.0f);
		stoneMat->Set("u_TextureMix", 0.0f);

		ShaderMaterial::sptr grassMat = ShaderMaterial::Create();
		grassMat->Shader = shader;
		grassMat->Set("s_Diffuse", grass);
		grassMat->Set("s_Specular", noSpec);
		grassMat->Set("u_Shininess", 2.0f);
		grassMat->Set("u_TextureMix", 0.0f);

		ShaderMaterial::sptr boxMat = ShaderMaterial::Create();
		boxMat->Shader = shader;
		boxMat->Set("s_Diffuse", box);
		boxMat->Set("s_Specular", boxSpec);
		boxMat->Set("u_Shininess", 8.0f);
		boxMat->Set("u_TextureMix", 0.0f);

		ShaderMaterial::sptr simpleFloraMat = ShaderMaterial::Create();
		simpleFloraMat->Shader = shader;
		simpleFloraMat->Set("s_Diffuse", simpleFlora);
		simpleFloraMat->Set("s_Specular", noSpec);
		simpleFloraMat->Set("u_Shininess", 8.0f);
		simpleFloraMat->Set("u_TextureMix", 0.0f);

		ShaderMaterial::sptr leavesMat = ShaderMaterial::Create();
		leavesMat->Shader = shader;
		leavesMat->Set("s_Diffuse", leaves);
		leavesMat->Set("s_Specular", leavesSpec);
		leavesMat->Set("u_Shininess", 2.0f);
		leavesMat->Set("u_TextureMix", 0.0f);

		ShaderMaterial::sptr darkBlueMat = ShaderMaterial::Create();
		darkBlueMat->Shader = shader;
		darkBlueMat->Set("s_Diffuse", DarkBlueText);
		darkBlueMat->Set("s_Specular", DarkBlueSpec);
		darkBlueMat->Set("u_Shininess", 2.0f);

		leavesMat->Set("u_TextureMix", 0.0f);
		GameObject obj1 = scene->CreateEntity("Ground");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/plane.obj");
			obj1.emplace<RendererComponent>().SetMesh(vao).SetMaterial(grassMat);
		}

		GameObject obj2 = scene->CreateEntity("monkey_quads");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/WatchTower.obj");
			obj2.emplace<RendererComponent>().SetMesh(vao).SetMaterial(boxMat);
			obj2.get<Transform>().SetLocalPosition(0.0f, -6.0f, 0.5f);
			obj2.get<Transform>().SetLocalRotation(90.0f, 0.0f, 0.0f);
			obj2.get<Transform>().SetLocalScale(0.5f, 0.5f, 0.5f);
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(obj2);
		}

		GameObject obj3 = scene->CreateEntity("TreeBranch");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/TreeBranch.obj");
			obj3.emplace<RendererComponent>().SetMesh(vao).SetMaterial(boxMat);
			obj3.get<Transform>().SetLocalPosition(2.5f, -4.0f, 0.3f);
			obj3.get<Transform>().SetLocalRotation(90.0f, 0.0f, 140.0f);
			obj3.get<Transform>().SetLocalScale(0.5f, 0.5f, 0.5f);
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(obj3);
		}

		GameObject obj4 = scene->CreateEntity("TreeLeaves");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/TreeLeaves.obj");
			obj4.emplace<RendererComponent>().SetMesh(vao).SetMaterial(leavesMat);
			obj4.get<Transform>().SetLocalPosition(2.5f, -4.0f, 0.3f);
			obj4.get<Transform>().SetLocalRotation(90.0f, 0.0f, 140.0f);
			obj4.get<Transform>().SetLocalScale(0.5f, 0.5f, 0.5f);
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(obj4);
		}

		GameObject obj5 = scene->CreateEntity("TreeBranch2");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/TreeBranch.obj");
			obj5.emplace<RendererComponent>().SetMesh(vao).SetMaterial(boxMat);
			obj5.get<Transform>().SetLocalPosition(-3.0f, -4.5f, 0.3f);
			obj5.get<Transform>().SetLocalRotation(90.0f, 0.0f, -140.0f);
			obj5.get<Transform>().SetLocalScale(0.5f, 0.5f, 0.5f);
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(obj5);
		}

		GameObject obj6 = scene->CreateEntity("TreeLeaves2");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/TreeLeaves.obj");
			obj6.emplace<RendererComponent>().SetMesh(vao).SetMaterial(leavesMat);
			obj6.get<Transform>().SetLocalPosition(-3.0f, -4.5f, 0.3f);
			obj6.get<Transform>().SetLocalRotation(90.0f, 0.0f, -140.0f);
			obj6.get<Transform>().SetLocalScale(0.5f, 0.5f, 0.5f);
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(obj6);
		}

		GameObject obj7 = scene->CreateEntity("TreeBranch3");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/TreeBranch.obj");
			obj7.emplace<RendererComponent>().SetMesh(vao).SetMaterial(boxMat);
			obj7.get<Transform>().SetLocalPosition(-2.5f, -8.5f, 0.2f);
			obj7.get<Transform>().SetLocalRotation(90.0f, 0.0f, -140.0f);
			obj7.get<Transform>().SetLocalScale(0.5f, 0.5f, 0.5f);
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(obj7);
		}

		GameObject obj8 = scene->CreateEntity("TreeLeaves3");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/TreeLeaves.obj");
			obj8.emplace<RendererComponent>().SetMesh(vao).SetMaterial(leavesMat);
			obj8.get<Transform>().SetLocalPosition(-2.5f, -8.5f, 0.2f);
			obj8.get<Transform>().SetLocalRotation(90.0f, 0.0f, -140.0f);
			obj8.get<Transform>().SetLocalScale(0.5f, 0.5f, 0.5f);
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(obj8);
		}


		GameObject obj9 = scene->CreateEntity("TreeBranch4");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/TreeBranch.obj");
			obj9.emplace<RendererComponent>().SetMesh(vao).SetMaterial(boxMat);
			obj9.get<Transform>().SetLocalPosition(-5.0f, -6.0f, 0.2f);
			obj9.get<Transform>().SetLocalRotation(90.0f, 0.0f, -140.0f);
			obj9.get<Transform>().SetLocalScale(0.5f, 0.5f, 0.5f);
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(obj9);
		}

		GameObject obj10 = scene->CreateEntity("TreeLeaves4");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/TreeLeaves.obj");
			obj10.emplace<RendererComponent>().SetMesh(vao).SetMaterial(leavesMat);
			obj10.get<Transform>().SetLocalPosition(-5.0f, -6.0f, 0.2f);
			obj10.get<Transform>().SetLocalRotation(90.0f, 0.0f, -140.0f);
			obj10.get<Transform>().SetLocalScale(0.5f, 0.5f, 0.5f);
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(obj10);
		}

		GameObject obj11 = scene->CreateEntity("TreeBranch5");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/TreeBranch.obj");
			obj11.emplace<RendererComponent>().SetMesh(vao).SetMaterial(boxMat);
			obj11.get<Transform>().SetLocalPosition(4.5f, -5.0f, 0.3f);
			obj11.get<Transform>().SetLocalRotation(90.0f, 0.0f, 140.0f);
			obj11.get<Transform>().SetLocalScale(0.5f, 0.5f, 0.5f);
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(obj11);
		}

		GameObject obj12 = scene->CreateEntity("TreeLeaves5");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/TreeLeaves.obj");
			obj12.emplace<RendererComponent>().SetMesh(vao).SetMaterial(leavesMat);
			obj12.get<Transform>().SetLocalPosition(4.5f, -5.0f, 0.3f);
			obj12.get<Transform>().SetLocalRotation(90.0f, 0.0f, 140.0f);
			obj12.get<Transform>().SetLocalScale(0.5f, 0.5f, 0.5f);
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(obj12);
		}

		GameObject obj13 = scene->CreateEntity("TreeBranch6");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/TreeBranch.obj");
			obj13.emplace<RendererComponent>().SetMesh(vao).SetMaterial(boxMat);
			obj13.get<Transform>().SetLocalPosition(2.5f, -7.0f, 0.3f);
			obj13.get<Transform>().SetLocalRotation(90.0f, 0.0f, 140.0f);
			obj13.get<Transform>().SetLocalScale(0.5f, 0.5f, 0.5f);
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(obj13);
		}

		GameObject obj14 = scene->CreateEntity("TreeLeaves6");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/TreeLeaves.obj");
			obj14.emplace<RendererComponent>().SetMesh(vao).SetMaterial(leavesMat);
			obj14.get<Transform>().SetLocalPosition(2.5f, -7.0f, 0.3f);
			obj14.get<Transform>().SetLocalRotation(90.0f, 0.0f, 140.0f);
			obj14.get<Transform>().SetLocalScale(0.5f, 0.5f, 0.5f);
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(obj14);
		}

		GameObject obj15 = scene->CreateEntity("FireplaceLogs");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/FireplaceLogs.obj");
			obj15.emplace<RendererComponent>().SetMesh(vao).SetMaterial(boxMat);
			obj15.get<Transform>().SetLocalPosition(-0.3f, -1.0f, -0.2f);
			obj15.get<Transform>().SetLocalRotation(90.0f, 0.0f, 0.0f);
			obj15.get<Transform>().SetLocalScale(0.6f, 0.6f, 0.6f);
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(obj15);
		}

		GameObject obj16 = scene->CreateEntity("FireplaceRocks");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/FireplaceRocks.obj");
			obj16.emplace<RendererComponent>().SetMesh(vao).SetMaterial(stoneMat);
			obj16.get<Transform>().SetLocalPosition(-0.3f, -1.0f, -0.2f);
			obj16.get<Transform>().SetLocalRotation(90.0f, 0.0f, 0.0f);
			obj16.get<Transform>().SetLocalScale(0.6f, 0.6f, 0.6f);
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(obj16);
		}

		GameObject obj17 = scene->CreateEntity("Robot Running");
		{
			VertexArrayObject::sptr vao = ObjLoader::LoadFromFile("models/ROBOT_RUNNING.obj");
			obj17.emplace<RendererComponent>().SetMesh(vao).SetMaterial(darkBlueMat);
			obj17.get<Transform>().SetLocalPosition(0.0f, 1.0f, -0.2f);
			obj17.get<Transform>().SetLocalRotation(90.0f, 0.0f, 0.0f);
			obj17.get<Transform>().SetLocalScale(1.5f, 1.5f, 1.5f);
			BehaviourBinding::BindDisabled<SimpleMoveBehaviour>(obj17);

			auto pathing = BehaviourBinding::Bind<FollowPathBehaviour>(obj17);
			// Set up a path for the object to follow
			pathing->Points.push_back({ 1.0f, 1.0f, -0.2f });
			pathing->Points.push_back({ -1.0f, 1.0f, -0.2f });
			pathing->Points.push_back({ -1.0f, 5.0f, -0.2f });
			pathing->Points.push_back({ 1.0f, 5.0f, -0.2f });
			pathing->Speed = 2.0f;
		}

		
		/*
		std::vector<glm::vec2> allAvoidAreasFrom = { glm::vec2(-4.0f, -4.0f) };
		std::vector<glm::vec2> allAvoidAreasTo = { glm::vec2(4.0f, 4.0f) };

		std::vector<glm::vec2> rockAvoidAreasFrom = { glm::vec2(-3.0f, -3.0f), glm::vec2(-19.0f, -19.0f), glm::vec2(5.0f, -19.0f),
														glm::vec2(-19.0f, 5.0f), glm::vec2(-19.0f, -19.0f) };
		std::vector<glm::vec2> rockAvoidAreasTo = { glm::vec2(3.0f, 3.0f), glm::vec2(19.0f, -5.0f), glm::vec2(19.0f, 19.0f),
														glm::vec2(19.0f, 19.0f), glm::vec2(-5.0f, 19.0f) };
		glm::vec2 spawnFromHere = glm::vec2(-19.0f, -19.0f);
		glm::vec2 spawnToHere = glm::vec2(19.0f, 19.0f);

		EnvironmentGenerator::AddObjectToGeneration("models/simplePine.obj", simpleFloraMat, 150,
			spawnFromHere, spawnToHere, allAvoidAreasFrom, allAvoidAreasTo);
		EnvironmentGenerator::AddObjectToGeneration("models/simpleTree.obj", simpleFloraMat, 150,
			spawnFromHere, spawnToHere, allAvoidAreasFrom, allAvoidAreasTo);
		EnvironmentGenerator::AddObjectToGeneration("models/simpleRock.obj", simpleFloraMat, 40,
			spawnFromHere, spawnToHere, rockAvoidAreasFrom, rockAvoidAreasTo);
		EnvironmentGenerator::GenerateEnvironment();
		*/

		// Create an object to be our camera
		GameObject cameraObject = scene->CreateEntity("Camera");
		{
			cameraObject.get<Transform>().SetLocalPosition(0, 3, 3).LookAt(glm::vec3(0, 0, 0));

			// We'll make our camera a component of the camera object
			Camera& camera = cameraObject.emplace<Camera>();// Camera::Create();
			camera.SetPosition(glm::vec3(0, 3, 3));
			camera.SetUp(glm::vec3(0, 0, 1));
			camera.LookAt(glm::vec3(0));
			camera.SetFovDegrees(90.0f); // Set an initial FOV
			camera.SetOrthoHeight(3.0f);
			BehaviourBinding::Bind<CameraControlBehaviour>(cameraObject);
		}
		int width, height;
		glfwGetWindowSize(BackendHandler::window, &width, &height);
	
		GameObject framebufferObject = scene->CreateEntity("Basic Buffer");
		{
			basicEffect = &framebufferObject.emplace<PostEffect>();
			basicEffect->Init(width, height);
		}

		GameObject sepiaEffectObject = scene->CreateEntity("Sepia Effect");
		{
			sepiaEffect = &sepiaEffectObject.emplace<SepiaEffect>();
			sepiaEffect->Init(width, height);
		}
		effects.push_back(sepiaEffect);

		GameObject greyScaleObject = scene->CreateEntity("Greyscale Effect");
		{
			greyscaleEffect = &greyScaleObject.emplace<GreyscaleEffect>();
			greyscaleEffect->Init(width, height);
		}
		effects.push_back(greyscaleEffect);
		
		GameObject bloomEffectObject = scene->CreateEntity("Bloom Effect");
		{
			theBloomEffect = &bloomEffectObject.emplace<BloomEffect>();
			theBloomEffect ->Init(width, height);
		}

		effects.push_back(theBloomEffect);

#pragma endregion 
		//////////////////////////////////////////////////////////////////////////////////////////

		/////////////////////////////////// SKYBOX ///////////////////////////////////////////////
		{
			// Load our shaders
			Shader::sptr skybox = std::make_shared<Shader>();
			skybox->LoadShaderPartFromFile("shaders/skybox-shader.vert.glsl", GL_VERTEX_SHADER);
			skybox->LoadShaderPartFromFile("shaders/skybox-shader.frag.glsl", GL_FRAGMENT_SHADER);
			skybox->Link();

			ShaderMaterial::sptr skyboxMat = ShaderMaterial::Create();
			skyboxMat->Shader = skybox;
			skyboxMat->Set("s_Environment", environmentMap);
			skyboxMat->Set("u_EnvironmentRotation", glm::mat3(glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(1, 0, 0))));
			skyboxMat->RenderLayer = 100;

			MeshBuilder<VertexPosNormTexCol> mesh;
			MeshFactory::AddIcoSphere(mesh, glm::vec3(0.0f), 1.0f);
			MeshFactory::InvertFaces(mesh);
			VertexArrayObject::sptr meshVao = mesh.Bake();

			GameObject skyboxObj = scene->CreateEntity("skybox");
			skyboxObj.get<Transform>().SetLocalPosition(0.0f, 0.0f, 0.0f);
			skyboxObj.get_or_emplace<RendererComponent>().SetMesh(meshVao).SetMaterial(skyboxMat);
		}
		////////////////////////////////////////////////////////////////////////////////////////


		// We'll use a vector to store all our key press events for now (this should probably be a behaviour eventually)
		std::vector<KeyPressWatcher> keyToggles;
		{
			// This is an example of a key press handling helper. Look at InputHelpers.h an .cpp to see
			// how this is implemented. Note that the ampersand here is capturing the variables within
			// the scope. If you wanted to do some method on the class, your best bet would be to give it a method and
			// use std::bind
			keyToggles.emplace_back(GLFW_KEY_T, [&]() { cameraObject.get<Camera>().ToggleOrtho(); });

			controllables.push_back(obj2);
			controllables.push_back(obj3);
			controllables.push_back(obj4);
			controllables.push_back(obj5);
			controllables.push_back(obj6);
			controllables.push_back(obj7);
			controllables.push_back(obj8);
			controllables.push_back(obj9);
			controllables.push_back(obj10); 
			controllables.push_back(obj15);
			controllables.push_back(obj16);

			keyToggles.emplace_back(GLFW_KEY_KP_ADD, [&]() {
				BehaviourBinding::Get<SimpleMoveBehaviour>(controllables[selectedVao])->Enabled = false;
				selectedVao++;
				if (selectedVao >= controllables.size())
					selectedVao = 0;
				BehaviourBinding::Get<SimpleMoveBehaviour>(controllables[selectedVao])->Enabled = true;
				});
			keyToggles.emplace_back(GLFW_KEY_KP_SUBTRACT, [&]() {
				BehaviourBinding::Get<SimpleMoveBehaviour>(controllables[selectedVao])->Enabled = false;
				selectedVao--;
				if (selectedVao < 0)
					selectedVao = controllables.size() - 1;
				BehaviourBinding::Get<SimpleMoveBehaviour>(controllables[selectedVao])->Enabled = true;
				});

			keyToggles.emplace_back(GLFW_KEY_Y, [&]() {
				auto behaviour = BehaviourBinding::Get<SimpleMoveBehaviour>(controllables[selectedVao]);
				behaviour->Relative = !behaviour->Relative;
				});
		}

		// Initialize our timing instance and grab a reference for our use
		Timing& time = Timing::Instance();
		time.LastFrame = glfwGetTime();

		///// Game loop /////
		while (!glfwWindowShouldClose(BackendHandler::window)) {
			glfwPollEvents();

			// Update the timing
			time.CurrentFrame = glfwGetTime();
			time.DeltaTime = static_cast<float>(time.CurrentFrame - time.LastFrame);

			time.DeltaTime = time.DeltaTime > 1.0f ? 1.0f : time.DeltaTime;

			// Update our FPS tracker data
			fpsBuffer[frameIx] = 1.0f / time.DeltaTime;
			frameIx++;
			if (frameIx >= 128)
				frameIx = 0;

			// We'll make sure our UI isn't focused before we start handling input for our game
			if (!ImGui::IsAnyWindowFocused()) {
				// We need to poll our key watchers so they can do their logic with the GLFW state
				// Note that since we want to make sure we don't copy our key handlers, we need a const
				// reference!
				for (const KeyPressWatcher& watcher : keyToggles) {
					watcher.Poll(BackendHandler::window);
				}
			}

			// Iterate over all the behaviour binding components
			scene->Registry().view<BehaviourBinding>().each([&](entt::entity entity, BehaviourBinding& binding) {
				// Iterate over all the behaviour scripts attached to the entity, and update them in sequence (if enabled)
				for (const auto& behaviour : binding.Behaviours) {
					if (behaviour->Enabled) {
						behaviour->Update(entt::handle(scene->Registry(), entity));
					}
				}
				});

			// Clear the screen
			basicEffect->Clear();
			//greyscaleEffect->Clear();
			//sepiaEffect->Clear();
			
			for (int i = 0; i < effects.size(); i++)
			{
				effects[i]->Clear(); 
			}

			glClearColor(0.08f, 0.17f, 0.31f, 1.0f);
			glEnable(GL_DEPTH_TEST);
			glClearDepth(1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			// Update all world matrices for this frame
			scene->Registry().view<Transform>().each([](entt::entity entity, Transform& t) {
				t.UpdateWorldMatrix();
				});

			// Grab out camera info from the camera object
			Transform& camTransform = cameraObject.get<Transform>();
			glm::mat4 view = glm::inverse(camTransform.LocalTransform());
			glm::mat4 projection = cameraObject.get<Camera>().GetProjection();
			glm::mat4 viewProjection = projection * view;

			// Sort the renderers by shader and material, we will go for a minimizing context switches approach here,
			// but you could for instance sort front to back to optimize for fill rate if you have intensive fragment shaders
			renderGroup.sort<RendererComponent>([](const RendererComponent& l, const RendererComponent& r) {
				// Sort by render layer first, higher numbers get drawn last
				if (l.Material->RenderLayer < r.Material->RenderLayer) return true;
				if (l.Material->RenderLayer > r.Material->RenderLayer) return false;

				// Sort by shader pointer next (so materials using the same shader run sequentially where possible)
				if (l.Material->Shader < r.Material->Shader) return true;
				if (l.Material->Shader > r.Material->Shader) return false;

				// Sort by material pointer last (so we can minimize switching between materials)
				if (l.Material < r.Material) return true;
				if (l.Material > r.Material) return false;

				return false;
				});

			// Start by assuming no shader or material is applied
			Shader::sptr current = nullptr;
			ShaderMaterial::sptr currentMat = nullptr;

			basicEffect->BindBuffer(0);

			// Iterate over the render group components and draw them
			renderGroup.each([&](entt::entity e, RendererComponent& renderer, Transform& transform) {
				// If the shader has changed, set up it's uniforms
				if (current != renderer.Material->Shader) {
					current = renderer.Material->Shader;
					current->Bind();
					BackendHandler::SetupShaderForFrame(current, view, projection);
				}
				// If the material has changed, apply it
				if (currentMat != renderer.Material) {
					currentMat = renderer.Material;
					currentMat->Apply();
				}
				// Render the mesh
				BackendHandler::RenderVAO(renderer.Material->Shader, renderer.Mesh, viewProjection, transform);
				});

			basicEffect->UnBindBuffer();

			effects[activeEffect]->ApplyEffect(basicEffect);
			effects[activeEffect]->DrawToScreen();

			// Draw our ImGui content
			BackendHandler::RenderImGui();

			scene->Poll();
			glfwSwapBuffers(BackendHandler::window);
			time.LastFrame = time.CurrentFrame;
		}

		// Nullify scene so that we can release references
		Application::Instance().ActiveScene = nullptr;
		//Clean up the environment generator so we can release references
		EnvironmentGenerator::CleanUpPointers();
		BackendHandler::ShutdownImGui();
	}

	// Clean up the toolkit logger so we don't leak memory
	Logger::Uninitialize();
	return 0;
}