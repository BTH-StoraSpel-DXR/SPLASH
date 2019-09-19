#include "pch.h"
#include "DX12InputLayout.h"
#include "Sail/utils/Utils.h"

InputLayout* InputLayout::Create() {
	return new DX12InputLayout();
}

DX12InputLayout::DX12InputLayout() {

}

DX12InputLayout::~DX12InputLayout() {

}

void DX12InputLayout::pushFloat(InputType inputType, LPCSTR semanticName, UINT semanticIndex, InputClassification inputSlotClass /*= PER_VERTEX_DATA*/, UINT instanceDataStepRate /*= 0*/) {
	push(DXGI_FORMAT_R32_FLOAT, sizeof(float), semanticName, semanticIndex, inputSlotClass, instanceDataStepRate);
	InputLayout::pushFloat(inputType, semanticName, semanticIndex, inputSlotClass, instanceDataStepRate);
}

void DX12InputLayout::pushVec2(InputType inputType, LPCSTR semanticName, UINT semanticIndex, InputClassification inputSlotClass /*= PER_VERTEX_DATA*/, UINT instanceDataStepRate /*= 0*/) {
	push(DXGI_FORMAT_R32G32_FLOAT, sizeof(glm::vec2), semanticName, semanticIndex, inputSlotClass, instanceDataStepRate);
	InputLayout::pushVec2(inputType, semanticName, semanticIndex, inputSlotClass, instanceDataStepRate);
}

void DX12InputLayout::pushVec3(InputType inputType, LPCSTR semanticName, UINT semanticIndex, InputClassification inputSlotClass /*= PER_VERTEX_DATA*/, UINT instanceDataStepRate /*= 0*/) {
	push(DXGI_FORMAT_R32G32B32_FLOAT, sizeof(glm::vec3), semanticName, semanticIndex, inputSlotClass, instanceDataStepRate);
	InputLayout::pushVec3(inputType, semanticName, semanticIndex, inputSlotClass, instanceDataStepRate);
}

void DX12InputLayout::pushVec4(InputType inputType, LPCSTR semanticName, UINT semanticIndex, InputClassification inputSlotClass /*= PER_VERTEX_DATA*/, UINT instanceDataStepRate /*= 0*/) {
	push(DXGI_FORMAT_R32G32B32A32_FLOAT, sizeof(float), semanticName, semanticIndex, inputSlotClass, instanceDataStepRate);
	InputLayout::pushVec4(inputType, semanticName, semanticIndex, inputSlotClass, instanceDataStepRate);
}

void DX12InputLayout::create(void* vertexShaderBlob) {

	m_inputLayoutDesc.pInputElementDescs = m_inputElementDescs.data();
	m_inputLayoutDesc.NumElements = static_cast<UINT>(m_inputElementDescs.size());

}

void DX12InputLayout::bind() const {
	// Do nothing
	// Already bound through the pipeline state
}

const D3D12_INPUT_LAYOUT_DESC& DX12InputLayout::getDesc() const {
	return m_inputLayoutDesc;
}

int DX12InputLayout::convertInputClassification(InputClassification inputSlotClass) {
	switch (inputSlotClass) {
	case InputLayout::PER_VERTEX_DATA:
		return D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
		break;
	case InputLayout::PER_INSTANCE_DATA:
		return D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
		break;
	default:
		Logger::Error("Invalid input classifier specified");
		return 0;
		break;
	}
}

void DX12InputLayout::push(DXGI_FORMAT format, UINT typeSize, LPCSTR semanticName, UINT semanticIndex, InputClassification inputSlotClass, UINT instanceDataStepRate) {
	UINT alignedByteOffset = (m_inputElementDescs.empty()) ? 0 : D3D12_APPEND_ALIGNED_ELEMENT;
	auto convertedInputClass = (D3D12_INPUT_CLASSIFICATION)convertInputClassification(inputSlotClass);
	m_inputElementDescs.push_back({ semanticName, semanticIndex, format, 0, alignedByteOffset,	convertedInputClass, instanceDataStepRate });

	if (convertedInputClass == D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA)
		InstanceSize += typeSize;
	else
		VertexSize += typeSize;
}
