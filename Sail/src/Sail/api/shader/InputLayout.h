#pragma once

#include <vector>

class InputLayout {
public:
	enum InputType {
		POSITION,
		POSITION2D,
		TEXCOORD,
		NORMAL,
		TANGENT,
		BITANGENT
	};
	enum InputClassification {
		PER_VERTEX_DATA,
		PER_INSTANCE_DATA
	};
	
public:
	static InputLayout* InputLayout::Create();
	InputLayout();
	virtual ~InputLayout();

	virtual void pushFloat(InputType inputType, LPCSTR semanticName, UINT semanticIndex, InputClassification inputSlotClass = PER_VERTEX_DATA, UINT instanceDataStepRate = 0);
	virtual void pushVec2(InputType inputType, LPCSTR semanticName, UINT semanticIndex, InputClassification inputSlotClass = PER_VERTEX_DATA, UINT instanceDataStepRate = 0);
	virtual void pushVec3(InputType inputType, LPCSTR semanticName, UINT semanticIndex, InputClassification inputSlotClass = PER_VERTEX_DATA, UINT instanceDataStepRate = 0);
	virtual void pushVec4(InputType inputType, LPCSTR semanticName, UINT semanticIndex, InputClassification inputSlotClass = PER_VERTEX_DATA, UINT instanceDataStepRate = 0);
	
	virtual void create(void* vertexShaderBlob) = 0;
	virtual void bind() const = 0;

	const std::vector<InputType>& getOrderedInputs() const;
	UINT getVertexSize() const;
	UINT getInstanceSize() const;

protected:
	virtual int convertInputClassification(InputClassification inputSlotClass) = 0;

protected:
	std::vector<InputType> InputOrder;
	UINT VertexSize;
	UINT InstanceSize;

};