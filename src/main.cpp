#include <GLFW/glfw3.h>
#include <lvk/LVK.h>
#include <Utils.h>
#include <assimp/scene.h>
#include <assimp/cimport.h>
#include <assimp/postprocess.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>

int main(void) {
	minilog::initialize(nullptr, { .threadNames = false });
	int width = 960;
	int height = 540;
	GLFWwindow* window = lvk::initWindow(
		"Simple example", width, height
	);
	{
		std::unique_ptr<lvk::IContext> ctx =
			lvk::createVulkanContextWithSwapchain(
				window, width, height, {}
		);

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

		lvk::Holder<lvk::RenderPipelineHandle> pipelineSolid = 
			ctx->createRenderPipeline({
				.vertexInput	= vdesc,
				.smVert			= vert,
				.smFrag			= frag,
				.color			= { { .format = ctx->getSwapchainFormat() } },
				.depthFormat	= ctx->getFormat(depthTexture),
				.cullMode		= lvk::CullMode_Back
				});

		const uint32_t isWireFrame = 1;
		lvk::Holder<lvk::RenderPipelineHandle> pipelineWireframe = 
			ctx->createRenderPipeline({
				.vertexInput	= vdesc,
				.smVert			= vert,
				.smFrag			= frag,
				.specInfo		= {
									.entries = { {.constantId = 0,
												  .size = sizeof(uint32_t) } },
									.data = &isWireFrame,
									.dataSize = sizeof(isWireFrame) },
				.color			= { { .format = ctx->getSwapchainFormat() } },
				.depthFormat	= ctx->getFormat(depthTexture),
				.cullMode		= lvk::CullMode_Back,
				.polygonMode    = lvk::PolygonMode_Line
				});

		LVK_ASSERT(pipelineSolid.valid());
		LVK_ASSERT(pipelineWireframe.valid());

		while (!glfwWindowShouldClose(window)) {
			glfwPollEvents();
			glfwGetFramebufferSize(window, &width, &height);
			if (!width || !height) continue;

			const float ratio = width / (float)height;
			const glm::mat4 p = glm::perspective(45.0f, ratio, 0.1f, 1000.0f);
			const glm::mat4 m = glm::rotate(glm::mat4(1.0f), 
				glm::radians(-90.0f), glm::vec3(1, 0, 0));
			const glm::mat4 v = glm::rotate(glm::translate(
				glm::mat4(1.0f), glm::vec3(0.0f, -0.5f, -1.5f)), (float)glfwGetTime(),
				glm::vec3(0.0f, 1.0f, 0.0f));

			const lvk::RenderPass renderPass = {
				.color = { { .loadOp = lvk::LoadOp_Clear, .clearColor = {1, 1, 1, 1} } },
				.depth = { .loadOp = lvk::LoadOp_Clear, .clearDepth = 1. }
			};
			const lvk::Framebuffer framebuffer = {
				.color = { { .texture = ctx->getCurrentSwapchainTexture() } },
				.depthStencil = { .texture = depthTexture }
			};

			lvk::ICommandBuffer& buf = ctx->acquireCommandBuffer();
			buf.cmdBeginRendering(renderPass, framebuffer);

			buf.cmdBindVertexBuffer(0, vertexBuffer);
			buf.cmdBindIndexBuffer(indexBuffer, lvk::IndexFormat_UI32);
			buf.cmdBindRenderPipeline(pipelineSolid);
			buf.cmdBindDepthState({ .compareOp = lvk::CompareOp_Less, .isDepthWriteEnabled = true });
			buf.cmdPushConstants(p * v * m);
			buf.cmdDrawIndexed(indices.size());
			buf.cmdBindRenderPipeline(pipelineWireframe);
			buf.cmdSetDepthBiasEnable(true);
			buf.cmdSetDepthBias(0.0f, -1.0f, 0.0f);
			buf.cmdDrawIndexed(indices.size());

			buf.cmdEndRendering();

			ctx->submit(buf, ctx->getCurrentSwapchainTexture());
		}
	}

	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}