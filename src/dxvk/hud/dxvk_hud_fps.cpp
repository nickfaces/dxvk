#include "dxvk_hud_fps.h"

#include <cmath>
#include <iomanip>
#include <chrono>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

using namespace std;

const int NUM_CPU_STATES = 10;

struct Cpus{
  size_t num;
  string name;
  float value;
  string output;
};

std::string m_averageFps;
std::string m_onePercentFps;
std::string m_pointOnePercentFps;

vector<size_t> recordedFrames;
size_t numCpuCores = dxvk::thread::hardware_concurrency();
size_t arraySize = numCpuCores + 1;
std::vector<Cpus> cpuArray;

void coreCounting(){
  cpuArray.push_back({0, "CPU:"});
  for (size_t i = 0; i < arraySize; i++) {
    size_t offset = i;
    stringstream ss;
    ss << "CPU" << offset << ":";
    string cpuNameString = ss.str();
    cpuArray.push_back({i+1 , cpuNameString});
  }
}

std::string m_cpuUtilizationString;

enum CPUStates
{
	S_USER = 0,
	S_NICE,
	S_SYSTEM,
	S_IDLE,
	S_IOWAIT,
	S_IRQ,
	S_SOFTIRQ,
	S_STEAL,
	S_GUEST,
	S_GUEST_NICE
};

typedef struct CPUData
{
	std::string cpu;
	size_t times[NUM_CPU_STATES];
} CPUData;

void ReadStatsCPU(std::vector<CPUData> & entries);

size_t GetIdleTime(const CPUData & e);
size_t GetActiveTime(const CPUData & e);

void PrintStats(const std::vector<CPUData> & entries1, const std::vector<CPUData> & entries2);

void ReadStatsCPU(std::vector<CPUData> & entries)
{
	std::ifstream fileStat("/proc/stat");

	std::string line;

	const std::string STR_CPU("cpu");
	const std::size_t LEN_STR_CPU = STR_CPU.size();
	const std::string STR_TOT("tot");

	while(std::getline(fileStat, line))
	{
		// cpu stats line found
		if(!line.compare(0, LEN_STR_CPU, STR_CPU))
		{
			std::istringstream ss(line);

			// store entry
			entries.emplace_back(CPUData());
			CPUData & entry = entries.back();

			// read cpu label
			ss >> entry.cpu;

			// remove "cpu" from the label when it's a processor number
			if(entry.cpu.size() > LEN_STR_CPU)
				entry.cpu.erase(0, LEN_STR_CPU);
			// replace "cpu" with "tot" when it's total values
			else
				entry.cpu = STR_TOT;

			// read times
			for(int i = 0; i < NUM_CPU_STATES; ++i)
				ss >> entry.times[i];
		}
	}
}

size_t GetIdleTime(const CPUData & e)
{
	return	e.times[S_IDLE] +
			e.times[S_IOWAIT];
}

size_t GetActiveTime(const CPUData & e)
{
	return	e.times[S_USER] +
			e.times[S_NICE] +
			e.times[S_SYSTEM] +
			e.times[S_IRQ] +
			e.times[S_SOFTIRQ] +
			e.times[S_STEAL] +
			e.times[S_GUEST] +
			e.times[S_GUEST_NICE];
}

void PrintStats(const std::vector<CPUData> & entries1, const std::vector<CPUData> & entries2)
{
	const size_t NUM_ENTRIES = entries1.size();

	for(size_t i = 0; i < NUM_ENTRIES; ++i)
	{
		const CPUData & e1 = entries1[i];
		const CPUData & e2 = entries2[i];

		const float ACTIVE_TIME	= static_cast<float>(GetActiveTime(e2) - GetActiveTime(e1));
		const float IDLE_TIME	= static_cast<float>(GetIdleTime(e2) - GetIdleTime(e1));
		const float TOTAL_TIME	= ACTIVE_TIME + IDLE_TIME;

    cpuArray[i].value = (truncf(100.f * ACTIVE_TIME / TOTAL_TIME) * 10 / 10);
	}
}

int getCpuUsage()
{
	std::vector<CPUData> entries1;
	std::vector<CPUData> entries2;

	// snapshot 1
	ReadStatsCPU(entries1);

	// 100ms pause
	std::this_thread::sleep_for(std::chrono::milliseconds(100));

	// snapshot 2
	ReadStatsCPU(entries2);

	// print output
	PrintStats(entries1, entries2);

	return 0;
}

namespace dxvk::hud {

  HudFps::HudFps(HudElements elements)
  : m_elements  (elements),
    m_fpsString ("FPS: "),
    m_prevFpsUpdate(Clock::now()),
    m_prevFtgUpdate(Clock::now()) {

  }


  HudFps::~HudFps() {

  }

  void printToLog(std::string file, string m_fpsString, string cpuUtil, string gpuUtil, string m_frameTimes) {
    fstream f(file, f.out | f.app);
    f << m_fpsString << "," << "Frametime: " << m_frameTimes << "," << cpuUtil << "," << gpuUtil;
	}

  string getCpuTemp(string var){
    string cpuTemp;
    ifstream ifs (var);
    getline (ifs, cpuTemp, (char) ifs.eof());
    int value = stoi(cpuTemp) / 1000;
    return std::to_string(value);
  }

  void updateCpuStrings(){
    for (size_t i = 0; i < arraySize; i++) {
      size_t spacing = 10;
      string value = to_string(cpuArray[i].value);
      value.erase( value.find_last_not_of('0') + 1, std::string::npos );
      size_t correctionValue = (spacing - cpuArray[i].name.length()) - value.length();
      string correction = "";
      for (size_t i = 0; i < correctionValue; i++) {
        correction.append(" ");
      }
      stringstream ss;
      if (i < 11) {
        if (i == 0) {
          ss << cpuArray[i].name << correction << cpuArray[i].value << "%";
        } else {
          ss << cpuArray[i].name << correction << cpuArray[i].value << "%";
        }
      } else {
        ss << cpuArray[i].name << correction << cpuArray[i].value << "%";
      }
      cpuArray[i].output = ss.str();
    }
  }

  void recordingFrames(int fps){
    recordedFrames.push_back(fps);
    // Removing 10 minute old frames
    if (recordedFrames.size() > 1200){
      recordedFrames.erase(recordedFrames.begin());
    }
    vector<size_t> sortedFrames;
    sortedFrames = recordedFrames;
    sort(begin(sortedFrames),end(sortedFrames));
    size_t arrayLength = sortedFrames.size();
    size_t sum = 0;
    size_t average;
    size_t minOnePercent;
    size_t minPointOnePercent;
    size_t max = sortedFrames[sortedFrames.size()];
    size_t min = sortedFrames[0];

    // average
    for (size_t i = 0; i < arrayLength; i++) {
      sum = sum + sortedFrames[i];
    }
    average = sum / sortedFrames.size();

    // 1% min
    sum = 0;
    for (size_t i = 0; i < arrayLength * 0.1; i++) {
      sum = sum + sortedFrames[i];
    }
    minOnePercent = sum / (arrayLength * 0.1);

    // .1% min
    sum = 0;
    for (size_t i = 0; i < arrayLength * 0.01; i++) {
      sum = sum + sortedFrames[i];
    }
    minPointOnePercent = sum / (arrayLength * 0.01);

    m_averageFps = str::format("FPS Avg: ", average / 10, ".", average % 10);
    m_onePercentFps = str::format("FPS 1%: ", minOnePercent / 10, ".", minOnePercent % 10);
    m_pointOnePercentFps = str::format("FPS 0.1%: ", minPointOnePercent / 10, ".", minPointOnePercent % 10);

    // console debugging
    // cout << "0.1%: " << minPointOnePercent / 10 << "." << minPointOnePercent % 10 << " " << "1%: " << minOnePercent / 10 << "." << minOnePercent % 10 << " " << "average: " << average / 10 << "." << average % 10 << endl;
  }

  void HudFps::update() {

    m_frameCount += 1;

    TimePoint now = Clock::now();
    TimeDiff elapsedFps = std::chrono::duration_cast<TimeDiff>(now - m_prevFpsUpdate);
    TimeDiff elapsedFtg = std::chrono::duration_cast<TimeDiff>(now - m_prevFtgUpdate);
    m_prevFtgUpdate = now;

    // Update FPS string

    // Fetching nvidia utilization
    if (elapsedFps.count() >= UpdateInterval) {
      coreCounting();
      dxvk::thread([this] () { getCpuUsage();});
      updateCpuStrings();
      ifstream ifs ("/tmp/nvidia-smi");
      getline (ifs, gpuUtil, (char) ifs.eof());
      // Using latest value in case of lost value (not sure why it happends, probably unsynced)
      if(gpuUtil == ""){
        gpuUtil = lastGpuUtil;
      }else{
        lastGpuUtil = gpuUtil;
      }
      string thermal_zone = "/sys/class/thermal/thermal_zone2/temp";
      char const* thermal_zone_env;
      thermal_zone_env = getenv("DXVK_THERMAL_ZONE");
      if(!thermal_zone_env == 0){
        thermal_zone = thermal_zone_env;
      }
      cpuTemp = getCpuTemp(thermal_zone);
      // m_cpuUtilizationString = str::format("CPU: ", truncf(getCurrentValue() * 10) / 10, "%");
      m_gpuUtilizationString = str::format("GPU: ", gpuUtil);
      m_cpuTempString        = str::format("CPU T: ", cpuTemp + "C");

      m_frameTimes = str::format(us);
      const int64_t fps = (10'000'000ll * m_frameCount) / elapsedFps.count();
      recordingFrames(fps);
      m_fpsString = str::format("FPS: ", fps / 10, ".", fps % 10);
      m_prevFpsUpdate = now;
      m_frameCount = 0;

      // Logging
      char const* logging;
      logging = getenv("DXVK_LOG_TO_FILE");
      if (!logging == 0){
        printToLog(logging, m_fpsString, m_cpuUtilizationString, m_gpuUtilizationString, m_frameTimes);
      }
    }

    // Update frametime stuff
    m_dataPoints[m_dataPointId] = float(elapsedFtg.count());
    m_dataPointId = (m_dataPointId + 1) % NumDataPoints;
  }


  HudPos HudFps::render(
    const Rc<DxvkContext>&  context,
          HudRenderer&      renderer,
          HudPos            position) {
    if (m_elements.test(HudElement::GpuLoad)) {
      position = this->renderGpuUtilText(
        context, renderer, position);
    }
    if (m_elements.test(HudElement::CpuLoad)) {
      position = this->renderCpuUtilText(
        context, renderer, position);
    }
    if (m_elements.test(HudElement::CpuCoreLoad)) {
      position = this->renderCpuCoreUtilText(
        context, renderer, position);
    }
    if (m_elements.test(HudElement::CpuTemp)) {
      position = this->renderCpuTempText(
        context, renderer, position);
    }
    if (m_elements.test(HudElement::Framerate)) {
      position = this->renderFpsText(
        context, renderer, position);
    }
    if (m_elements.test(HudElement::AverageFps)) {
      position = this->renderAverageFps(
        context, renderer, position);
    }
    if (m_elements.test(HudElement::OnePercentLow)) {
      position = this->renderOnePercentLow(
        context, renderer, position);
    }
    if (m_elements.test(HudElement::PointOnePercentLow)) {
      position = this->renderPointOnePercentLow(
        context, renderer, position);
    }
    if (m_elements.test(HudElement::Frametimes)) {
      position = this->renderFrametimeGraph(
        context, renderer, position);
    }


    return position;
  }


  HudPos HudFps::renderFpsText(
    const Rc<DxvkContext>&  context,
          HudRenderer&      renderer,
          HudPos            position) {
    renderer.drawText(context, 16.0f,
      { position.x, position.y },
      { 1.0f, 1.0f, 1.0f, 1.0f },
      m_fpsString);

    return HudPos { position.x, position.y + 24 };
  }

  HudPos HudFps::renderAverageFps(
    const Rc<DxvkContext>&  context,
          HudRenderer&      renderer,
          HudPos            position) {
    renderer.drawText(context, 16.0f,
      { position.x, position.y },
      { 1.0f, 1.0f, 1.0f, 1.0f },
      m_averageFps);

    return HudPos { position.x, position.y + 24 };
  }

  HudPos HudFps::renderOnePercentLow(
    const Rc<DxvkContext>&  context,
          HudRenderer&      renderer,
          HudPos            position) {
    renderer.drawText(context, 16.0f,
      { position.x, position.y },
      { 1.0f, 1.0f, 1.0f, 1.0f },
      m_onePercentFps);

    return HudPos { position.x, position.y + 24 };
  }

  HudPos HudFps::renderPointOnePercentLow(
    const Rc<DxvkContext>&  context,
          HudRenderer&      renderer,
          HudPos            position) {
    renderer.drawText(context, 16.0f,
      { position.x, position.y },
      { 1.0f, 1.0f, 1.0f, 1.0f },
      m_pointOnePercentFps);

    return HudPos { position.x, position.y + 24 };
  }

  HudPos HudFps::renderCpuUtilText(
    const Rc<DxvkContext>&  context,
          HudRenderer&      renderer,
          HudPos            position) {
    renderer.drawText(context, 16.0f,
      { position.x, position.y },
      { 1.0f, 1.0f, 1.0f, 1.0f },
      cpuArray[0].output);

    return HudPos { position.x, position.y + 24 };
  }

HudPos HudFps::renderCpuCoreUtilText(
  const Rc<DxvkContext>&  context,
        HudRenderer&      renderer,
        HudPos            position) {
  for (size_t i = 0; i < numCpuCores; i++) {
    renderer.drawText(context, 16.0f,
      { position.x, position.y + i * 24},
      { 1.0f, 1.0f, 1.0f, 1.0f },
      cpuArray[i + 1].output);
    }

  return HudPos { position.x, position.y + (24 * numCpuCores)
  };
}

  HudPos HudFps::renderCpuTempText(
  const Rc<DxvkContext>&  context,
        HudRenderer&      renderer,
        HudPos            position) {
  renderer.drawText(context, 16.0f,
    { position.x, position.y },
    { 1.0f, 1.0f, 1.0f, 1.0f },
    m_cpuTempString);

  return HudPos { position.x, position.y + 24 };
}

HudPos HudFps::renderGpuUtilText(
  const Rc<DxvkContext>&  context,
        HudRenderer&      renderer,
        HudPos            position) {
  renderer.drawText(context, 16.0f,
    { position.x, position.y },
    { 1.0f, 1.0f, 1.0f, 1.0f },
    m_gpuUtilizationString);

  return HudPos { position.x, position.y + 24 };
}


  HudPos HudFps::renderFrametimeGraph(
    const Rc<DxvkContext>&  context,
          HudRenderer&      renderer,
          HudPos            position) {
    std::array<HudVertex, NumDataPoints * 2> vData;

    // 60 FPS = optimal, 10 FPS = worst
    const float targetUs =  16'666.6f;
    const float minUs    =   5'000.0f;
    const float maxUs    = 100'000.0f;

    // Ten times the maximum/minimum number
    // of milliseconds for a single frame
    uint32_t minMs = 0xFFFFFFFFu;
    uint32_t maxMs = 0x00000000u;

    // Paint the time points
    for (uint32_t i = 0; i < NumDataPoints; i++) {
      us = m_dataPoints[(m_dataPointId + i) % NumDataPoints];

      minMs = std::min(minMs, uint32_t(us / 100.0f));
      maxMs = std::max(maxMs, uint32_t(us / 100.0f));

      float r = std::min(std::max(-1.0f + us / targetUs, 0.0f), 1.0f);
      float g = std::min(std::max( 3.0f - us / targetUs, 0.0f), 1.0f);
      float l = std::sqrt(r * r + g * g);

      HudTexCoord tc = { 0u, 0u };
      HudColor color = { r / l, g / l, 0.0f, 1.0f };

      float x = position.x + float(i);
      float y = position.y + 24.0f;

      float hVal = std::log2(std::max((us - minUs) / targetUs + 1.0f, 1.0f))
                 / std::log2((maxUs - minUs) / targetUs);
      float h = std::min(std::max(40.0f * hVal, 2.0f), 40.0f);

      vData[2 * i + 0] = HudVertex { { x, y     }, tc, color };
      vData[2 * i + 1] = HudVertex { { x, y - h }, tc, color };
    }

    renderer.drawLines(context, vData.size(), vData.data());

    // Paint min/max frame times in the entire window
    renderer.drawText(context, 14.0f,
      { position.x, position.y + 44.0f },
      { 1.0f, 1.0f, 1.0f, 1.0f },
      str::format("min: ", minMs / 10, ".", minMs % 10));

    renderer.drawText(context, 14.0f,
      { position.x + 150.0f, position.y + 44.0f },
      { 1.0f, 1.0f, 1.0f, 1.0f },
      str::format("max: ", maxMs / 10, ".", maxMs % 10));

    return HudPos { position.x, position.y + 66.0f };
  }

}
