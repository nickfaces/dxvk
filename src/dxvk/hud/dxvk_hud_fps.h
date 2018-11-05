#pragma once

#include <chrono>

#include "dxvk_hud_config.h"
#include "dxvk_hud_renderer.h"


namespace dxvk::hud {

  /**
   * \brief FPS display for the HUD
   *
   * Displays the current frames per second.
   */
  class HudFps {
    using Clock     = std::chrono::high_resolution_clock;
    using TimeDiff  = std::chrono::microseconds;
    using TimePoint = typename Clock::time_point;

    constexpr static uint32_t NumDataPoints  = 300;
    constexpr static int64_t  UpdateInterval = 500'000;
  public:

    HudFps(HudElements elements);
    ~HudFps();

    void update();

    HudPos render(
      const Rc<DxvkContext>&  context,
            HudRenderer&      renderer,
            HudPos            position);

  private:

    const HudElements m_elements;

    std::string m_fpsString;

    // Variables used for my extra Hud elements.

    std::string cpuUtil;
    std::string m_gpuUtilizationString;
    std::string gpuUtil;
    std::string lastGpuUtil;
    std::string m_frameTimes;
    std::string frametime;
    std::string cpuTemp;
    std::string m_cpuTempString;

    // Moved defining of us here to more easily access it.
    float us;

    TimePoint m_prevFpsUpdate;
    TimePoint m_prevFtgUpdate;
    int64_t   m_frameCount = 0;

    std::array<float, NumDataPoints>  m_dataPoints  = {};
    uint32_t                          m_dataPointId = 0;

    HudPos renderFpsText(
      const Rc<DxvkContext>&  context,
            HudRenderer&      renderer,
            HudPos            position);

    HudPos renderAverageFps(
      const Rc<DxvkContext>&  context,
            HudRenderer&      renderer,
            HudPos            position);

    HudPos renderOnePercentLow(
      const Rc<DxvkContext>&  context,
            HudRenderer&      renderer,
            HudPos            position);

    HudPos renderPointOnePercentLow(
      const Rc<DxvkContext>&  context,
            HudRenderer&      renderer,
            HudPos            position);

    HudPos renderCpuUtilText(
      const Rc<DxvkContext>&  context,
            HudRenderer&      renderer,
            HudPos            position);

    HudPos renderCpuCoreUtilText(
      const Rc<DxvkContext>&  context,
            HudRenderer&      renderer,
            HudPos            position);

    HudPos renderCpuTempText(
      const Rc<DxvkContext>&  context,
            HudRenderer&      renderer,
            HudPos            position);

    HudPos renderGpuUtilText(
      const Rc<DxvkContext>&  context,
            HudRenderer&      renderer,
            HudPos            position);

    HudPos renderFrametimeGraph(
      const Rc<DxvkContext>&  context,
            HudRenderer&      renderer,
            HudPos            position);

  };

}
