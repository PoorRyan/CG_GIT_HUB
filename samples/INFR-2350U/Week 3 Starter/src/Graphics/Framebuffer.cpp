#include "Framebuffer.h"

GLuint Framebuffer::_fullscreenQuadVAO = 0;
GLuint Framebuffer::_fullscreenQuadVBO = 0;

int Framebuffer::_maxColorAttachments = 0;
bool Framebuffer::_isInitFSQ = false;

DepthTarget::~DepthTarget()
{
	//Unloads the depth target
	Unload();
}

void DepthTarget::Unload()
{
	//Deltes the texture at the specified handle
	glDeleteTextures(1, &_texture.GetHandle());
}

ColorTarget::~ColorTarget()
{
	//Unloads color target
	Unload();
}

void ColorTarget::Unload()
{
	glDeleteTextures(_numAttachments, &_textures[0].GetHandle());
}

Framebuffer::Framebuffer()
{
}

Framebuffer::~Framebuffer()
{
	Unload();
}

void Framebuffer::Unload()
{
	//Deletes the framebuffer
	glDeleteFramebuffers(1, &_FB0);
	//sets init to false
	_isInit = false;
}

void Framebuffer::Init(unsigned width, unsigned height)
{
	//Sets the size to width and height
	SetSize(width, height);
	//Inits framebuffer
	Init();
}

void Framebuffer::Init()
{
	//Generates FB0
	glGenFramebuffers(1, &_FB0);
	//Bind it
	glBindFramebuffer(GL_FRAMEBUFFER, _FB0);

	if (_depthActive)
	{
		_clearFlag = GL_DEPTH_BUFFER_BIT;
		//Generates  the texture
		glGenTextures(1, &_depth._texture.GetHandle());
		glBindTexture(GL_TEXTURE_2D, _depth._texture.GetHandle());
		glTexStorage2D(GL_TEXTURE_2D, 1, GL_DEPTH_COMPONENT24, _width, _height);

		//set texture parameters
		glTextureParameteri(_depth._texture.GetHandle(), GL_TEXTURE_MIN_FILTER, _filter);
		glTextureParameteri(_depth._texture.GetHandle(), GL_TEXTURE_MAG_FILTER, _filter);
		glTextureParameteri(_depth._texture.GetHandle(), GL_TEXTURE_WRAP_S, _wrap);
		glTextureParameteri(_depth._texture.GetHandle(), GL_TEXTURE_WRAP_T, _wrap);

		//sets up as a framebuffer texture
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, _depth._texture.GetHandle(), 0);

		glBindTexture(GL_TEXTURE_2D, GL_NONE);
	}

	//If there is more then zero color attachments
	//we create them
	if (_color._numAttachments)
	{
		//Because we have a color target we include a color buffer bit into clear flag
		_clearFlag |= GL_COLOR_BUFFER_BIT;
		//Creates the GLUints to hold the new texture handles
		GLuint* textureHandles = new GLuint[_color._numAttachments];

		glGenTextures(_color._numAttachments, textureHandles);

		//loops through them
		for (unsigned i = 0; i < _color._numAttachments; i++)
		{
			_color._textures[i].GetHandle() = textureHandles[i];

			//Binds the texture
			glBindTexture(GL_TEXTURE_2D, _color._textures[i].GetHandle());
			//Sets texture storage
			glTexStorage2D(GL_TEXTURE_2D, 1, _color._formats[i], _width, _height);

			//Sets texture parameters
			glTextureParameteri(_color._textures[i].GetHandle(), GL_TEXTURE_MIN_FILTER, _filter);
			glTextureParameteri(_color._textures[i].GetHandle(), GL_TEXTURE_MAG_FILTER, _filter);
			glTextureParameteri(_color._textures[i].GetHandle(), GL_TEXTURE_WRAP_S, _wrap);
			glTextureParameteri(_color._textures[i].GetHandle(), GL_TEXTURE_WRAP_T, _wrap);

			//Sets up as a frame buffer
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, _color._textures[i].GetHandle(), 0);
		}
		delete[] textureHandles;
	}
	//Make sure it's set up right
	CheckFBO();
	//Unbind buffer
	glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
	//Setinint to true
	_isInit = true;
}

void Framebuffer::AddDepthTarget()
{
	//if there is a handle already, unload it
	if (_depth._texture.GetHandle())
	{
		_depth.Unload();
	}
	//make depth active true
	_depthActive = true;
}

void Framebuffer::AddColorTarget(GLenum format)
{
	//Resizes the textures to number of attachments
	_color._textures.resize(_color._numAttachments + 1);
	//Adds	the format
	_color._formats.push_back(format);
	_color._buffers.push_back(GL_COLOR_ATTACHMENT0 + _color._numAttachments);
	_color._numAttachments++;
}

void Framebuffer::BindDepthAsTexture(int textureSlot) const
{
	_depth._texture.Bind(textureSlot);
}

void Framebuffer::BindColorAsTexture(unsigned colorBuffer, int textureSlot) const
{
	_color._textures[colorBuffer].Bind(textureSlot);
}

void Framebuffer::UnbindTexture(int textureSlot) const
{
	glActiveTexture(GL_TEXTURE0 + textureSlot);
	glBindTexture(GL_TEXTURE_2D, GL_NONE);
}

void Framebuffer::Reshape(unsigned width, unsigned height)
{
	//Set size
	SetSize(width, height);
	//Loads framebuffer -> depth target -> color target
	Unload();
	_depth.Unload();
	_color.Unload();
	//Inits the fram buffer
	Init();
}

void Framebuffer::SetSize(unsigned width, unsigned height)
{
	_width = width;
	_height = height;

}

void Framebuffer::SetViewport() const
{
	glViewport(0, 0, _width, _height);
}

void Framebuffer::Bind() const
{
	glBindFramebuffer(GL_FRAMEBUFFER, _FB0);

	if (_color._numAttachments)
	{
		glDrawBuffers(_color._numAttachments, &_color._buffers[0]);
	}
}

void Framebuffer::Unbind() const
{
	glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
}

void Framebuffer::RenderToFSQ() const
{
	SetViewport();
	Bind();
	DrawFullScreenQuad();
	Unbind();
}

void Framebuffer::DrawToBackbuffer()
{
	glBindFramebuffer(GL_READ_FRAMEBUFFER, _FB0);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, GL_NONE);

	//Blits the framebuffer to the back buffer
	glBlitFramebuffer(0, 0, _width, _height, 0, 0, _width, _height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, GL_NONE);

}

void Framebuffer::Clear()
{
	glBindFramebuffer(GL_FRAMEBUFFER, _FB0);
	glClear(_clearFlag);
	glBindFramebuffer(GL_FRAMEBUFFER, GL_NONE);
}

bool Framebuffer::CheckFBO()
{
	Bind();
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		printf("Lol the program failed");
		return false;
	}
	return true;
}

void Framebuffer::InitFullscreenQuad()
{
	//A vbo with Uvs and verts from
	//-1 to 1 for verts
	//0 to 1 for UVs
	float VBO_DATA[]
	{
		-1.f, -1.f, 0.f,
		1.f, -1.f, 0.f,
		-1.f, 1.f, 0.f,

		1.f, 1.f, 0.f,
		-1.f, 1.f, 0.f,
		1.f, -1.f, 0.f,

		0.f, 0.f,
		1.f, 0.f,
		0.f, 1.f,

		1.f, 1.f,
		0.f, 1.f,
		1.f, 0.f
	};
	//Vertex size is 6pts * 3 data points * sizeof (float)
	int vertexSize = 6 * 3 * sizeof(float);
	//texcoord size = 6pts * 2 data points * sizeof(float)
	int texCoordSize = 6 * 2 * sizeof(float);

	//Generates vertex array
	glGenVertexArrays(1, &_fullscreenQuadVAO);
	//Binds VAO
	glBindVertexArray(_fullscreenQuadVAO);

	//Enables 2 vertex attrib array slots
	glEnableVertexAttribArray(0); //Vertices
	glEnableVertexAttribArray(1); //UVS

	//Generates VBO
	glGenBuffers(1, &_fullscreenQuadVBO);

	//Binds the VBO
	glBindBuffer(GL_ARRAY_BUFFER, _fullscreenQuadVBO);
	//Buffers the vbo data
	glBufferData(GL_ARRAY_BUFFER, vertexSize + texCoordSize, VBO_DATA, GL_STATIC_DRAW);

#pragma warning(push)
#pragma warning(disable : 4312)
	//Sets first attrib array to point to the beginning of the data
	glVertexAttribPointer((GLuint)0, 3, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<void*>(0));
	//Sets the second attrib array to point to an offset in the data
	glVertexAttribPointer((GLuint)1, 2, GL_FLOAT, GL_FALSE, 0, reinterpret_cast<void*>(vertexSize));
#pragma warning(pop)

	glBindBuffer(GL_ARRAY_BUFFER, GL_NONE);
	glBindVertexArray(GL_NONE);
}

void Framebuffer::DrawFullScreenQuad()
{
	glBindVertexArray(_fullscreenQuadVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(GL_NONE);
}

void Framebuffer::DrawFullScreenQuad()
{
}



