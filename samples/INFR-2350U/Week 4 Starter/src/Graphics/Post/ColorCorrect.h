#pragma once

#include "Graphics/Post/PostEffect.h"

class ColorCorrect : public PostEffect
{
public:

	void Init(unsigned width, unsigned height) override;

	void ApplyEffect(PostEffect* buffer) override;

	float GetIntensity() const;

	void SetInvensity(float intensity);

private:
	float _intensity = 1.0f;

};