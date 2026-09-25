#include "game_session.h"

void session_init(GameSession& s) {
  s.speed = kSpeedStart;
  s.distance_px = 0.0f;
  s.score = 0;
  s.night = false;
  s.last_milestone = 0;
  s.last_night_mark = 0;
}

uint32_t session_update(GameSession& s, float frames) {
  uint32_t evt = 0;
  s.distance_px += s.speed * frames;
  if (s.speed < kSpeedMax) {
    s.speed += kAccel * frames;
    if (s.speed > kSpeedMax) s.speed = kSpeedMax;
  }
  s.score = (uint32_t)(s.distance_px * kScoreCoef);

  uint32_t milestone = s.score / 100;
  if (milestone > s.last_milestone) {
    s.last_milestone = milestone;
    evt |= kEvtMilestone;
  }
  uint32_t night_mark = s.score / kNightPeriod;
  if (night_mark > s.last_night_mark) {
    s.last_night_mark = night_mark;
    s.night = !s.night;
    evt |= kEvtNightFlip;
  }
  return evt;
}
