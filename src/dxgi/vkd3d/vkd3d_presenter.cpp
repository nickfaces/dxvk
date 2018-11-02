#include "vkd3d_presenter.h"

namespace dxvk {

  VkD3DPresenter::VkD3DPresenter(
            ID3D12CommandQueue*       pQueue,
            HWND                      hWnd,
            DXGI_SWAP_CHAIN_DESC1*    pDesc)
  : m_parent(pQueue),
    m_window(hWnd),
    m_desc  (*pDesc) {
    
  }


  VkD3DPresenter::~VkD3DPresenter() {

  }


  HRESULT STDMETHODCALLTYPE VkD3DPresenter::QueryInterface(
          REFIID                    riid,
          void**                    ppvObject) {
    InitReturnPtr(ppvObject);

    if (riid == __uuidof(IUnknown)
     || riid == __uuidof(IDXGIVkSwapChain)) {
      *ppvObject = ref(this);
      return S_OK;
    }

    Logger::warn("VkD3DPresenter::QueryInterface: Unknown interface query");
    return E_NOINTERFACE;
  }


  HRESULT STDMETHODCALLTYPE VkD3DPresenter::GetDesc(
          DXGI_SWAP_CHAIN_DESC1*    pDesc) {
    *pDesc = m_desc;
    return S_OK;
  }


  HRESULT STDMETHODCALLTYPE VkD3DPresenter::GetDevice(
          REFIID                    riid,
          void**                    ppDevice) {
    return m_parent->QueryInterface(riid, ppDevice);
  }

  
  HRESULT STDMETHODCALLTYPE VkD3DPresenter::GetImage(
          UINT                      BufferId,
          REFIID                    riid,
          void**                    ppBuffer) {
    
  }


  UINT STDMETHODCALLTYPE VkD3DPresenter::GetImageIndex() {

  }


  HRESULT STDMETHODCALLTYPE VkD3DPresenter::ChangeProperties(
    const DXGI_SWAP_CHAIN_DESC1*    pDesc) {

  }


  HRESULT STDMETHODCALLTYPE VkD3DPresenter::SetPresentRegion(
    const RECT*                     pRegion) {
      
  }


  HRESULT STDMETHODCALLTYPE VkD3DPresenter::SetGammaControl(
          UINT                      NumControlPoints,
    const DXGI_RGB*                 pControlPoints) {

  }


  HRESULT STDMETHODCALLTYPE VkD3DPresenter::Present(
          UINT                      SyncInterval,
          UINT                      PresentFlags,
    const DXGI_PRESENT_PARAMETERS*  pPresentParameters) {
    
  }

}