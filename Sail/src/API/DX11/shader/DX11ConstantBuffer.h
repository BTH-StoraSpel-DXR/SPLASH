#pragma once
#include "Sail/api/shader/ConstantBuffer.h"

struct ID3D11Buffer;

namespace ShaderComponent {

	class DX11ConstantBuffer : public ConstantBuffer {
	public:
		DX11ConstantBuffer(void* initData, unsigned int size, BIND_SHADER bindShader, unsigned int slot = 0);
		virtual ~DX11ConstantBuffer();

		virtual void updateData(const void* newData, unsigned int bufferSize, unsigned int offset = 0U) override;

		virtual void bind(void* cmdList) const override;

	private:
		ID3D11Buffer* m_buffer;
		BIND_SHADER m_bindShader;
		unsigned int m_slot;
		unsigned int m_bufferSize;
		void* m_data;
	};

}