#pragma once

#include "Sail/api/ComputeShaderDispatcher.h"
#include "DX12API.h"

class DX12ComputeShaderDispatcher : public ComputeShaderDispatcher {
public:
	DX12ComputeShaderDispatcher();
	~DX12ComputeShaderDispatcher();

	virtual void begin(void* cmdList = nullptr) override;
	virtual Shader::ComputeShaderOutput& dispatch(Shader& computeShader, Shader::ComputeShaderInput& input, void* cmdList) override;

private:
	DX12API* m_context;

};