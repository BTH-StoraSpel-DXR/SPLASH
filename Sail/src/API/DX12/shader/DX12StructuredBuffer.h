#pragma once

#include "Sail/api/shader/StructuredBuffer.h"
#include "../DX12API.h"
#include "../resources/DescriptorHeap.h"

namespace ShaderComponent {

	class DX12StructuredBuffer : public StructuredBuffer {
	public:
		DX12StructuredBuffer(void* initData, unsigned int numElements, unsigned int stride);
		DX12StructuredBuffer(void* initData, unsigned int size, unsigned int numElements, unsigned int stride, BIND_SHADER bindShader, unsigned int slot = 0);
		~DX12StructuredBuffer();

		virtual void updateData(const void* newData, unsigned int numElements, int meshIndex = 0, unsigned int offset = 0) override;
		virtual void bind(void* cmdList) const override;
		void bind_new(void* cmdList, int meshIndex) const;

		ID3D12Resource* getBuffer(int bufferIndex = -1) const;

	private:
		void createBuffers(unsigned int numElements);

	private:
		DX12API* m_context;

		static const unsigned int MAX_ELEMENTS = 10000;
		static const unsigned int MAX_MESHES_PER_FRAME = 20;

		std::vector<unsigned int> m_resourceHeapSize;
		unsigned int m_numElements;
		unsigned int m_stride;
		unsigned int m_elementByteSize;

		unsigned int m_register;
		std::vector<wComPtr<ID3D12Resource1>> m_bufferUploadHeap;
		std::vector<UINT8*> m_cbGPUAddress;

		std::unique_ptr<DescriptorHeap> m_srvHeap;
		std::vector<D3D12_CPU_DESCRIPTOR_HANDLE> m_srvCDHs;

	};

}