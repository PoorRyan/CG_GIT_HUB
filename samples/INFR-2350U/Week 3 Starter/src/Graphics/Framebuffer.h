#pragma once
#include <vector>
#include <Texture2D.h>
#include <Shader.h>

struct DepthTarget
{
	//Deconstructor for Depth Target
	//*Unloads texture
	~DepthTarget();
	//Deletes the texture of the depth target
	void Unload();
	//Holds the depth texture
	Texture2D _texture;
};

struct ColorTarget
{
	//Deconstructor for Color Target
	//*Unloads all the color targets
	~ColorTarget();
	//Deletes the texture of the color target
	void Unload();
	//Holds the color textures
	std::vector<Texture2D> _textures;
	std::vector<GLenum> _formats;
	std::vector<GLenum> _buffers;
	//Stores the number of color attachments the target has
	unsigned int _numAttachments = 0;
};

class Framebuffer
{
public:
	Framebuffer();
	~Framebuffer();

	//Unloads the framebuffer
	void Unload();

	//Initialize framebuffer with size
	virtual void Init(unsigned width, unsigned height);

	//Initlizes frame buffer
	void Init();
	//Add depth target
	//**ONLY EVER ONE**//
	void AddDepthTarget();

	//Adds color target
	//**You can have as many as you want**//
	void AddColorTarget(GLenum format);

	//Binds our depth buffer as a texture to specified slot
	void BindDepthAsTexture(int textureSlot) const;
	//Binds color buffer as a texure in a specified spot
	void BindColorAsTexture(unsigned colorBuffer, int textureSlot) const;
	//Unbinds texture from a specific texture slot
	void UnbindTexture(int textureSlot) const;
	//Reshapes the framebuffer
	void Reshape(unsigned width, unsigned height);
	//Set size of the framebufer
	void SetSize(unsigned width, unsigned height);
	//Set Viewport to full screen (using the size of framebuffer)
	void SetViewport() const;

	//Binds the framebuffer
	void Bind() const;
	//Unbind the framebuffer
	void Unbind() const;
	//Renders the framebuffer to fullscreen
	void RenderToFSQ() const;

	//Draws the contents of the framebuffer to the back buffer
	void DrawToBackbuffer();

	//
	void Clear();
	//Checks to make sure our framebuffer...is okay
	bool CheckFBO();

	//Initilizes fullscreen quad
	//Creates VAO for full screen quad
	//*covers -1 to 1 range
	static void InitFullscreenQuad();
	//Draws the fullscreen quad
	static void DrawFullScreenQuad();

	//Initial width and height is 0
	unsigned int _height = 0;
	unsigned int _width = 0;

protected:
	//OpenGL framebuffer handle
	GLuint _FB0;
	//Depth attachment (either one or none)
	DepthTarget _depth;
	//Color attachments (can be either 1 or above)
	ColorTarget _color;

	//Default filer is GL_Nearest
	GLenum _filter = GL_NEAREST;
	//Default filler is GL_CLAMP_TO_EDGE
	GLenum _wrap = GL_CLAMP_TO_EDGE;
	//Clearflag is nothing on default
	GLbitfield _clearFlag = 0;
	//Is the framebuffer initialized
	bool _isInit = false;
	//Depth attachment?
	bool _depthActive = false;

	static GLuint _fullscreenQuadVBO;
	static GLuint _fullscreenQuadVAO;

	//Maximum of color attachments
	static int _maxColorAttachments;
	//Is the fullscreen quad initialized
	static bool _isInitFSQ;


};