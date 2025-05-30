#include <GLFW/glfw3.h>
#include <lvk/LVK.h>
#include <Utils.h>

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

		lvk::Holder<lvk::ShaderModuleHandle> vert = loadShaderModule(ctx, "src/main.vert");
		lvk::Holder<lvk::ShaderModuleHandle> frag = loadShaderModule(ctx, "src/main.frag");

		lvk::Holder<lvk::RenderPipelineHandle> rpTriangle = ctx->createRenderPipeline({
			.smVert = vert,
			.smFrag = frag,
			.color = { {.format = ctx->getSwapchainFormat() } },
			});

		while (!glfwWindowShouldClose(window)) {
			glfwPollEvents();
			glfwGetFramebufferSize(window, &width, &height);
			if (!width || !height) continue;
			lvk::ICommandBuffer& buf = ctx->acquireCommandBuffer();

			buf.cmdBeginRendering(
				{ .color = { {.loadOp = lvk::LoadOp_Clear, .clearColor = { 1.0f, 1.0f, 1.0f, 1.0f } } } },
				{ .color = { {.texture = ctx->getCurrentSwapchainTexture() } } });
			buf.cmdBindRenderPipeline(rpTriangle);
			buf.cmdPushDebugGroupLabel("Render Triangle", 0xff0000ff);
			buf.cmdDraw(3);
			buf.cmdPopDebugGroupLabel();
			buf.cmdEndRendering();

			ctx->submit(buf, ctx->getCurrentSwapchainTexture());
		}
	}

	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}