#pragma once
#include "Graphics/Post/PostEffect.h"

class BloomEffect : public PostEffect
{
public:
	void Init(unsigned width, unsigned height) override;

	void ApplyEffect(PostEffect* buffer) override;

	//Getters
	float GetTheDownscale() const;
	float GetTheThreshold() const;
	unsigned GetThePasses() const;

	//Setters
	void SetTheDownscale(float downscale);
	void SetTheThreshold(float threshold);
	void SetThePasses(unsigned passes);

private:

	float m_downscale = 2.0f;
	float m_threshold = 0.01f;
	unsigned m_passes = 10.0f;
	glm::vec2 direction;

};