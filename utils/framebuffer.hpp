#pragma once

#include <glad/glad.h>
#include <ogl_material_factory.hpp>
#include "texture.hpp"

struct CADescription {
	GLenum format;
	GLenum type;
	GLint internalFormat;
};


class Framebuffer {
public:
	Framebuffer(
		int aWidth,
		int aHeight,
		const std::vector<CADescription> &aColorAttachmentDescriptions)
		: mWidth(aWidth)
		, mHeight(aHeight)
		, mFramebuffer(createFramebuffer())
		, mColorAttachmentDescriptions(aColorAttachmentDescriptions)
	{
		init();
	}

	void bind() {
		GL_CHECK(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, mFramebuffer.get()));
	}

	void unbind() {
		GL_CHECK(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0));
	}

	void init() {
		bind();
		mColorAttachments.resize(mColorAttachmentDescriptions.size());
		for (int i = 0; i < mColorAttachmentDescriptions.size(); ++i) {
			auto texture = createColorAttachment(
					i,
					mWidth,
					mHeight,
					mColorAttachmentDescriptions[i].internalFormat,
					mColorAttachmentDescriptions[i].format,
					mColorAttachmentDescriptions[i].type);
			mColorAttachments[i] = std::make_shared<OGLTexture>(std::move(texture));
		}
		
		// Create depth texture instead of renderbuffer for DoF
		mDepthTexture = std::make_shared<OGLTexture>(createDepthTexture(mWidth, mHeight));
		
		checkStatus();
		unbind();
	}

	OpenGLResource createColorAttachment(
			int aAttachmentIndex,
			int aWidth,
			int aHeight,
			GLint aInternalFormat = GL_RGBA,
			GLenum aFormat = GL_RGBA,
			GLenum aType = GL_FLOAT)
	{
		auto textureID = createColorTexture(aWidth, aHeight, aInternalFormat, aFormat, aType);

		// Attach the texture to the FBO
		GL_CHECK(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + aAttachmentIndex, GL_TEXTURE_2D, textureID.get(), 0));
		return textureID;
	}

	OpenGLResource createDepthAndStencilBuffers(
			int aWidth,
			int aHeight)
	{
		auto rbo = createRenderBuffer();
		GL_CHECK(glBindRenderbuffer(GL_RENDERBUFFER, rbo.get()));
		GL_CHECK(glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, aWidth, aHeight));
		GL_CHECK(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo.get()));
		return rbo;
	}

	OpenGLResource createDepthTexture(int aWidth, int aHeight) {
		auto textureID = createColorTexture(aWidth, aHeight, GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, GL_FLOAT);
		
		// Attach the depth texture to the framebuffer
		GL_CHECK(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, textureID.get(), 0));
		
		return textureID;
	}

	void setDrawBuffers() {
		std::vector<GLenum> drawBuffers;
		drawBuffers.resize(mColorAttachmentDescriptions.size());
		for (int i = 0; i < mColorAttachmentDescriptions.size(); ++i) {
			drawBuffers[i] = GL_COLOR_ATTACHMENT0 + i;
		}
		glDrawBuffers(drawBuffers.size(), drawBuffers.data());
	}

	void checkStatus() {
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
			throw OpenGLError("Framebuffer is not complete!");
		}
	}

	std::shared_ptr<OGLTexture> getColorAttachment(int aIdx) {
		if (aIdx < 0 || aIdx >= mColorAttachments.size()) {
			throw OpenGLError("Framebuffer - invalid color attachment index.");
		}
		return mColorAttachments[aIdx];
	}

	std::shared_ptr<OGLTexture> getDepthTexture() {
		return mDepthTexture;
	}

	int mWidth;
	int mHeight;
	OpenGLResource mFramebuffer;
	std::vector<CADescription> mColorAttachmentDescriptions;
	std::vector<std::shared_ptr<OGLTexture>> mColorAttachments;
	OpenGLResource mDepthBuffer;
	std::shared_ptr<OGLTexture> mDepthTexture;
};
