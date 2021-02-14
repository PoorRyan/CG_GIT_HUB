#pragma once

#include "Graphics/Framebuffer.h"
#include "Shader.h"

class PostEffect
{
public:
	//Initialize this effects (will be over ridden in each derived class)
	virtual void Init(unsigned width, unsigned height);

	//Applies the effect
	virtual void ApplyEffect(PostEffect* previousbuffer);
	virtual void DrawToScreen();
	
	//Reshapes the buffer
	virtual void Reshape(unsigned width, unsigned height);

	//Clears the screen
	void Clear();
	
	//Unloads all the buffers
	void Unload();

	void BindBuffer(int index);
	void UnBindBuffer();

	//Bind textures
	void BindColorAsTexture(int index, int colorBuffer, int textureSlot);
	void BindDepthAsTexture(int index, int textureSlot);
	void UnbindTexture(int textureSlot);

	//bind shaders
	void BindShader(int index);
	void UnbindShader();

protected:
	//Holds all of our buffers for effects
	std::vector<Framebuffer*>_buffers;
	//Holds all of our shaders for effects
	std::vector<Shader::sptr> _shaders;

};