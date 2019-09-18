#include "pch.h"
#include <d3d11.h>
#include "DX11ConstantBuffer.h"
#include "Sail/Application.h"
#include "../DX11API.h"

namespace ShaderComponent {

	ConstantBuffer* ConstantBuffer::Create(void* initData, unsigned int size, BIND_SHADER bindShader, unsigned int slot) {
		return SAIL_NEW DX11ConstantBuffer(initData, size, bindShader, slot);
	}

	DX11ConstantBuffer::DX11ConstantBuffer(void* initData, unsigned int size, BIND_SHADER bindShader, unsigned int slot)
		: m_bindShader(bindShader)
		, m_slot(slot)
		, m_bufferSize(size)
	{

		D3D11_BUFFER_DESC desc;
		ZeroMemory(&desc, sizeof(desc));
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		desc.ByteWidth = size;
		desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

		D3D11_SUBRESOURCE_DATA data;
		data.pSysMem = initData;
		data.SysMemPitch = 0;
		data.SysMemSlicePitch = 0;

		ThrowIfFailed(Application::getInstance()->getAPI<DX11API>()->getDevice()->CreateBuffer(&desc, &data, &m_buffer));

		// Keep a copy of the data on the CPU
		// Reason being GPU buffer is write only, and updateData can write parts of the buffer at a time
		m_data = malloc(size);
		memcpy(m_data, initData, size);

	}
	DX11ConstantBuffer::~DX11ConstantBuffer() {
		Memory::SafeRelease(m_buffer);
		free(m_data);
	}

	void DX11ConstantBuffer::updateData(const void* newData, unsigned int bufferSize, unsigned int offset) {
		// Overwrite part of the locally stored data
		memcpy((char*)m_data + offset, newData, bufferSize);

		D3D11_MAPPED_SUBRESOURCE mappedResource;
		Application::getInstance()->getAPI<DX11API>()->getDeviceContext()->Map(m_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
		std::memcpy(mappedResource.pData, m_data, m_bufferSize);
		Application::getInstance()->getAPI<DX11API>()->getDeviceContext()->Unmap(m_buffer, 0);

	}

	void DX11ConstantBuffer::bind(void* cmdList) const {
		if (m_bindShader & ShaderComponent::VS)
			Application::getInstance()->getAPI<DX11API>()->getDeviceContext()->VSSetConstantBuffers(m_slot, 1, &m_buffer);
		if (m_bindShader & ShaderComponent::HS)
			Application::getInstance()->getAPI<DX11API>()->getDeviceContext()->HSSetConstantBuffers(m_slot, 1, &m_buffer);
		if (m_bindShader & ShaderComponent::DS)
			Application::getInstance()->getAPI<DX11API>()->getDeviceContext()->DSSetConstantBuffers(m_slot, 1, &m_buffer);
		if (m_bindShader & ShaderComponent::PS)
			Application::getInstance()->getAPI<DX11API>()->getDeviceContext()->PSSetConstantBuffers(m_slot, 1, &m_buffer);
		if (m_bindShader & ShaderComponent::GS)
			Application::getInstance()->getAPI<DX11API>()->getDeviceContext()->GSSetConstantBuffers(m_slot, 1, &m_buffer);
		if (m_bindShader & ShaderComponent::CS)
			Application::getInstance()->getAPI<DX11API>()->getDeviceContext()->CSSetConstantBuffers(m_slot, 1, &m_buffer);

	}

}