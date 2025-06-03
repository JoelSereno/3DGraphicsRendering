#include <GLFW/glfw3.h>
#include <lvk/LVK.h>
#include <lvk/HelpersImGui.h>
#include <Utils.h>
#include <assimp/scene.h>
#include <assimp/cimport.h>
#include <assimp/postprocess.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <stb/stb_image.h>
#include <imgui/misc/single_file/imgui_single_file.h>

int main(void) {
	minilog::initialize(nullptr, { .threadNames = false });
	int width = -95;
	int height = -90;
	GLFWwindow* window = lvk::initWindow(
		"Simple example", width, height
	);
	{
		std::unique_ptr<lvk::IContext> ctx =
			lvk::createVulkanContextWithSwapchain(
				window, width, height, {}
		);

		std::unique_ptr<lvk::ImGuiRenderer> imgui =
			std::make_unique<lvk::ImGuiRenderer>(
				*ctx, "data/OpenSans-Light.ttf", 30.0f
			);

		glfwSetCursorPosCallback(window,
			[](auto* window, double x, double y) {
				ImGui::GetIO().MousePos = ImVec2(x, y);
			});
		glfwSetMouseButtonCallback(window,
			[](auto* window, int button, int action, int mods) {
				double xpos, ypos;
				glfwGetCursorPos(window, &xpos, &ypos);
				const ImGuiMouseButton_ imguiButton =
					(button == GLFW_MOUSE_BUTTON_LEFT) ?
					ImGuiMouseButton_Left : (
						button == GLFW_MOUSE_BUTTON_RIGHT ?
						ImGuiMouseButton_Right :
						ImGuiMouseButton_Middle);
				ImGuiIO& io = ImGui::GetIO();
				io.MousePos = ImVec2((float)xpos, (float)ypos);
				io.MouseDown[imguiButton] = action == GLFW_PRESS;
			});

		const aiScene* scene = aiImportFile("data/rubber_duck/scene.gltf", aiProcess_Triangulate);
		const aiMesh* mesh = scene->mMeshes[0];
		std::vector<glm::vec3> positions;
		std::vector<uint32_t> indices;
		for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
			const aiVector3D v = mesh->mVertices[i];
			positions.push_back(glm::vec3(v.x, v.y, v.z));
		}
		for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
			for (unsigned int j = 0; j < 3; j++) {
				indices.push_back(mesh->mFaces[i].mIndices[j]);
			}
		}
		aiReleaseImport(scene);

		lvk::Holder<lvk::BufferHandle> vertexBuffer = ctx->createBuffer({
			.usage		= lvk::BufferUsageBits_Vertex,
			.storage	= lvk::StorageType_Device,
			.size		= sizeof(glm::vec3) * positions.size(),
			.data		= positions.data(),
			.debugName	= "Buffer: vertex"
			});
		lvk::Holder<lvk::BufferHandle> indexBuffer = ctx->createBuffer({
			.usage		= lvk::BufferUsageBits_Index,
			.storage	= lvk::StorageType_Device,
			.size		= sizeof(uint32_t) * indices.size(),
			.data		= indices.data(),
			.debugName	= "Buffer: index"
			});

		lvk::Holder<lvk::TextureHandle> depthTexture = ctx->createTexture({
			.type		= lvk::TextureType_2D,
			.format		= lvk::Format_Z_F32,
			.dimensions	= {(uint32_t)width, (uint32_t)height},
			.usage		= lvk::TextureUsageBits_Attachment,
			.debugName  = "Depth Buffer"
			});

		const lvk::VertexInput vdesc = {
			.attributes		= { { .location = 0,
								  .format = lvk::VertexFormat::Float3 } },
			.inputBindings  = { { .stride = sizeof(glm::vec3) } }
		};
		
		lvk::Holder<lvk::ShaderModuleHandle> vert = loadShaderModule(ctx, "src/main.vert");
		lvk::Holder<lvk::ShaderModuleHandle> frag = loadShaderModule(ctx, "src/main.frag");

		lvk::Holder<lvk::RenderPipelineHandle> pipeline = ctx->createRenderPipeline({
			.topology = lvk::Topology_TriangleStrip,
			.smVert   = vert,
			.smFrag   = frag,
			.color    = { { .format = ctx->getSwapchainFormat() } },
			.cullMode = lvk::CullMode_Back,
		});

		LVK_ASSERT(pipeline.valid());

		int w, h, comp;
		const stbi_uc* img = stbi_load(
			"data/wood.jpg", &w, &h, &comp, 4
		);

		lvk::Holder<lvk::TextureHandle> texture = ctx->createTexture({
			.type		= lvk::TextureType_2D,
			.format		= lvk::Format_RGBA_UN8,
			.dimensions = { (uint32_t)w, (uint32_t)h },
			.usage		= lvk::TextureUsageBits_Sampled,
			.data		= img,
			.debugName  = "STB.jpg"
			});
		stbi_image_free((void*)img);

		while (!glfwWindowShouldClose(window)) {
			glfwPollEvents();
			glfwGetFramebufferSize(window, &width, &height);
			if (!width || !height) continue;

			const float ratio = width / (float)height;
			const glm::mat4 m = glm::rotate(glm::mat4(1.0f), (float)glfwGetTime(), glm::vec3(0.0f, 0.0f, 1.0f));
			const glm::mat4 p = glm::ortho(-ratio, ratio, -1.f, 1.f, 1.f, -1.f);

			const struct PerFrameData {
				glm::mat4 mvp;
				uint32_t textureId;
			} pc = {
				.mvp		= p * m,
				.textureId	= texture.index()
			};

			lvk::ICommandBuffer& buf = ctx->acquireCommandBuffer();
			const lvk::Framebuffer framebuffer = {
				.color = { {.texture = ctx->getCurrentSwapchainTexture()} }
			};
			buf.cmdBeginRendering(
				{ .color = { {.loadOp = lvk::LoadOp_Clear, .clearColor = { 1.0f, 1.0f, 1.0f, 1.0f } } } },
				framebuffer);
			buf.cmdPushDebugGroupLabel("Quad", 0xff0000ff);
			buf.cmdBindRenderPipeline(pipeline);
			buf.cmdPushConstants(pc);
			buf.cmdDraw(4);
			buf.cmdPopDebugGroupLabel();
			imgui->beginFrame(framebuffer);
			ImGui::Begin("Texture Viewer", nullptr,
				ImGuiWindowFlags_AlwaysAutoResize);
			ImGui::Image(texture.index(), ImVec2(512, 512));
			ImGui::ShowDemoWindow();
			ImGui::End();
			imgui->endFrame(buf);
			buf.cmdEndRendering();

			ctx->submit(buf, ctx->getCurrentSwapchainTexture());
		}
	}

	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}