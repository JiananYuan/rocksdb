//
// Created by yjn on 23-5-24.
//

#include "stats.h"
#include <cstdio>

namespace adgMod {

Stats* Stats::singleton = nullptr;

Stats::Stats() : timers(20, Timer{}), initial_time(system_clock::now()) {
  // timers[0].setName("plr train segments");
  // timers[1].setName("build second layer");
  // timers[2].setName("build root node");
  // timers[3].setName("root node search");
  // timers[4].setName("second layer search");
  // timers[5].setName("last mile search");
  // timers[6].setName("linear model predict");
  // timers[7].setName("root bucket search");
  // timers[8].setName("second bucket search");
  // timers[9].setName("pointer retrieval");

  // timers[0].setName("LevelModel");
  // timers[1].setName("FileModel");
  // timers[2].setName("Baseline");
  // timers[3].setName("Succeeded");
  // timers[4].setName("FalseInternal");
  // timers[5].setName("Compaction");
  // timers[6].setName("Learn");
  // timers[7].setName("SuccessTime");
  // timers[8].setName("FalseTime");
  // timers[9].setName("FilteredLookup");
  // timers[10].setName("PutWait");
  // timers[11].setName("FileLearn");

  timers[0].setName("FindFiles");
  timers[1].setName("LoadIB+FB");
  timers[2].setName("SearchIB+SearchDB");
  timers[3].setName("SearchFB");
  timers[4].setName("LoadDB");
  timers[5].setName("ReadValue");
}

Stats* Stats::GetInstance() {
  if (!singleton) singleton = new Stats();
  return singleton;
}

void Stats::StartTimer(uint32_t id) {
  Timer& timer = timers[id];
  timer.Start();
}

uint8_t Stats::get_timer_size() {
  return timers.size();
}

std::pair<uint64_t, uint64_t> Stats::PauseTimer(uint32_t id, bool record) {
  Timer& timer = timers[id];
  return timer.Pause(record);
}

void Stats::ResetTimer(uint32_t id) {
  Timer& timer = timers[id];
  timer.Reset();
}

uint64_t Stats::ReportTime(uint32_t id, uint32_t count) {
  Timer& timer = timers[id];
  // convert to ns
  printf("Timer %d(%s) \t %f\n", id, timer.Name().data(), timer.Time() * 1.0 / count);
  return timer.Time();
}

void Stats::ReportTime() {
  for (size_t i = 0; i < timers.size(); ++i) {
    printf("Timer %lu: %lu\n", i, timers[i].Time());
  }
}

uint64_t Stats::ReportTime(uint32_t id) {
  Timer& timer = timers[id];
  return timer.Time();
}

Stats::~Stats() {
  ReportTime();
}

uint64_t Stats::GetInitTimeDuration() const {
  return duration<uint64_t , std::nano>(system_clock::now() - initial_time).count();
}

void Stats::ResetAll() {
  for (Timer& t: timers) t.Reset();
  initial_time = system_clock::now();
}

}
