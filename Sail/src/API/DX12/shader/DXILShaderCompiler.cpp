#include "pch.h"
#include "DXILShaderCompiler.h"
#include "../../../Sail/utils/Utils.h"
#include <atlstr.h>

DXILShaderCompiler::DXILShaderCompiler()
	: m_linker(nullptr)
	, m_library(nullptr)
	, m_includeHandler(nullptr)
	, m_compiler(nullptr) {

}

DXILShaderCompiler::~DXILShaderCompiler() {
	Memory::SafeRelease(m_compiler);
	Memory::SafeRelease(m_library);
	Memory::SafeRelease(m_includeHandler);
	Memory::SafeRelease(m_linker);
}

HRESULT DXILShaderCompiler::init() {
#ifdef _WIN64
	HMODULE dll = LoadLibraryA("dxcompiler_x64.dll");
	if (!dll) MessageBox(0, L"dxcompiler_x64.dll is missing", L"Error", 0);
#else
	HMODULE dll = LoadLibraryA("dxcompiler_x86.dll");
	if (!dll) MessageBox(0, L"dxcompiler_x86.dll is missing", L"Error", 0);
#endif

	DxcCreateInstanceProc pfnDxcCreateInstance = DxcCreateInstanceProc(GetProcAddress(dll, "DxcCreateInstance"));

	HRESULT hr = E_FAIL;

	if (SUCCEEDED(hr = pfnDxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&m_compiler)))) {
		if (SUCCEEDED(hr = pfnDxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&m_library)))) {
			if (SUCCEEDED(m_library->CreateIncludeHandler(&m_includeHandler))) {
				if (SUCCEEDED(hr = pfnDxcCreateInstance(CLSID_DxcLinker, IID_PPV_ARGS(&m_linker)))) {

				}
			}
		}
	}

	return hr;
}

HRESULT DXILShaderCompiler::compile(Desc* desc, IDxcBlob** ppResult) {
	HRESULT hr = E_FAIL;

	if (desc) {
		IDxcBlobEncoding* source = nullptr;
		if (desc->source) {
			// Compile from source allocated on stack
			hr = m_library->CreateBlobWithEncodingFromPinned(desc->source, desc->sourceSize, CP_UTF8, &source);
		} else {
			// Compile from file path
			hr = m_library->CreateBlobFromFile(desc->filePath, nullptr, &source);
		}

		if (SUCCEEDED(hr)) {
			IDxcOperationResult* pResult = nullptr;
			if (SUCCEEDED(hr = m_compiler->Compile(
				source,									// program text
				desc->filePath,							// file name, mostly for error messages
				desc->entryPoint,						// entry point function
				desc->targetProfile,					// target profile
				desc->compileArguments.data(),          // compilation arguments
				(UINT)desc->compileArguments.size(),	// number of compilation arguments
				desc->defines.data(),					// define arguments
				(UINT)desc->defines.size(),				// number of define arguments
				m_includeHandler,						// handler for #include directives
				&pResult))) {
				HRESULT hrCompile = E_FAIL;
				if (SUCCEEDED(hr = pResult->GetStatus(&hrCompile))) {
					if (SUCCEEDED(hrCompile)) {
						if (ppResult) {
							pResult->GetResult(ppResult);
							hr = S_OK;
						} else {
							hr = E_FAIL;
						}
					} else {
						IDxcBlobEncoding* pPrintBlob = nullptr;
						if (SUCCEEDED(pResult->GetErrorBuffer(&pPrintBlob))) {
							// We can use the library to get our preferred encoding.
							IDxcBlobEncoding* pPrintBlob16 = nullptr;
							m_library->GetBlobAsUtf16(pPrintBlob, &pPrintBlob16);

							OutputDebugStringW((LPCWSTR)pPrintBlob16->GetBufferPointer());
							MessageBox(0, (LPCWSTR)pPrintBlob16->GetBufferPointer(), L"", 0);


							Memory::SafeRelease(pPrintBlob);
							Memory::SafeRelease(pPrintBlob16);
						}
					}

					Memory::SafeRelease(pResult);
				}
			}
		}
	}

	if (hr == HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND) || hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
		std::string errMsg("Missing shader: " + std::string(CW2A(desc->filePath)) + "\n");
		MessageBoxA(0, errMsg.c_str(), "DXILShaderCompiler Error", 0);
		OutputDebugStringA(errMsg.c_str());

	}

	return hr;
}
