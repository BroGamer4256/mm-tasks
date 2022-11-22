#include "helpers.h"
#include <cstdio>
#include <d3d11.h>
#include <dxgi.h>
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>
#include <iostream>
#include <vector>

extern LRESULT ImGui_ImplWin32_WndProcHandler (HWND hWnd, UINT msg,
											   WPARAM wParam, LPARAM lParam);

ID3D11DeviceContext *pContext = NULL;
ID3D11RenderTargetView *mainRenderTargetView = NULL;
WNDPROC oWndProc;

LRESULT __stdcall WndProc (const HWND hWnd, UINT uMsg, WPARAM wParam,
						   LPARAM lParam) {
	if (ImGui_ImplWin32_WndProcHandler (hWnd, uMsg, wParam, lParam))
		return true;
	if (ImGui::GetCurrentContext () != 0 && ImGui::GetIO ().WantCaptureMouse) {
		switch (uMsg) {
		case WM_LBUTTONDOWN:
		case WM_LBUTTONDBLCLK:
		case WM_LBUTTONUP:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONDBLCLK:
		case WM_RBUTTONUP:
		case WM_MOUSEWHEEL:
			return true;
		}
	}

	return CallWindowProc (oWndProc, hWnd, uMsg, wParam, lParam);
}

#pragma pack(1)
typedef struct task {
	void *vftable;
	u64 padding[4];
	u32 padding2;
	u16 padding3;
	char name[32];
} task;

std::vector<task *> tasks;

HOOK (void, __stdcall, RunTask, 0x1402c9a80, task *task) {
	tasks.push_back (task);
	originalRunTask (task);
}

#ifdef __cplusplus
extern "C" {
#endif
__declspec(dllexport) void init () { INSTALL_HOOK (RunTask); }

__declspec(dllexport) void D3DInit (IDXGISwapChain *swapChain,
									ID3D11Device *device,
									ID3D11DeviceContext *deviceContext) {
	pContext = deviceContext;

	DXGI_SWAP_CHAIN_DESC sd;
	swapChain->GetDesc (&sd);
	ID3D11Texture2D *pBackBuffer;
	swapChain->GetBuffer (0, __uuidof(ID3D11Texture2D),
						  (LPVOID *)&pBackBuffer);
	device->CreateRenderTargetView (pBackBuffer, NULL, &mainRenderTargetView);
	pBackBuffer->Release ();
	HWND window = sd.OutputWindow;
	oWndProc
		= (WNDPROC)SetWindowLongPtrA (window, GWLP_WNDPROC, (LONG_PTR)WndProc);
	ImGui::CreateContext ();
	ImGuiIO &io = ImGui::GetIO ();
	io.ConfigFlags = ImGuiConfigFlags_NoMouseCursorChange;
	ImGui_ImplWin32_Init (window);
	ImGui_ImplDX11_Init (device, pContext);
}

__declspec(dllexport) void onFrame (IDXGISwapChain *chain) {
	ImGui_ImplDX11_NewFrame ();
	ImGui_ImplWin32_NewFrame ();
	ImGui::NewFrame ();

	ImGui::SetNextWindowSize (ImVec2 (700, 70), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowPos (ImVec2 (0, 0), ImGuiCond_FirstUseEver);
	if (ImGui::Begin ("Tasks", 0, 0)) {
		ImGui::BeginTable ("TasksTable", 3,
						   ImGuiTableFlags_Borders
							   | ImGuiTableFlags_Resizable);
		ImGui::TableSetupColumn ("Name");
		ImGui::TableSetupColumn ("data address");
		ImGui::TableSetupColumn ("vftable address");
		ImGui::TableHeadersRow ();
		for (task *task : tasks) {
			ImGui::TableNextColumn ();
			ImGui::Text ("%s", task->name);
			ImGui::TableNextColumn ();
			ImGui::Text ("%llx", (u64)task);
			ImGui::TableNextColumn ();
			ImGui::Text ("%llx", (u64)task->vftable);
		}
		ImGui::EndTable ();
	}

	ImGui::End ();

	ImGui::EndFrame ();
	ImGui::Render ();
	pContext->OMSetRenderTargets (1, &mainRenderTargetView, NULL);
	ImGui_ImplDX11_RenderDrawData (ImGui::GetDrawData ());
	tasks.clear ();
}
#ifdef __cplusplus
}
#endif
