#pragma once

#include "Sail/api/shader/ConstantBuffer.h"
#include "../DX12API.h"
#include <mutex>

namespace ShaderComponent {

	// TODO: think about renaming this class to DX12ResourceHeap as it a cbuffer for each mesh in the scene
	class DX12ConstantBuffer : public ConstantBuffer {
	public:
		DX12ConstantBuffer(void* initData, unsigned int size, BIND_SHADER bindShader, unsigned int slot = 0);
		~DX12ConstantBuffer();

		/*[deprecated]. Use updateData_new() for thread safety.*/
		virtual void updateData(const void* newData, unsigned int bufferSize, unsigned int offset = 0U) override;
		virtual void updateData_new(const void* newData, unsigned int bufferSize, int meshIndex, unsigned int offset = 0U);
		/*[deprecated]. Use bind_new() for thread safety.*/
		virtual void bind(void* cmdList) const override;
		virtual void bind_new(void* cmdList, int meshIndex, bool useComputeShader = false) const;

		void checkBufferSize(unsigned int nMeshes);

		ID3D12Resource* getBuffer(int bufferIndex = -1) const;

	private:
		void createBuffers();

	private:
		DX12API* m_context;
		std::mutex m_mutex_bufferExpander;
		bool expanding = false;

		//void* m_newData;
		//bool* m_needsUpdate;

		// The index specifies which offset it should write/read from in the resource heap
		// This is used since multiple meshes store their cbuffers in a single resource heap
		unsigned int m_resourceHeapMeshIndex;
		unsigned int m_byteAlignedSize;
		unsigned int m_resourceHeapSize;

		unsigned int m_register;
		std::vector<wComPtr<ID3D12Resource1>> m_constantBufferUploadHeap;
		std::vector<UINT8*> m_cbGPUAddress;

	};

}