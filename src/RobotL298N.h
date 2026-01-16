#pragma once
#include <Arduino.h>

static inline int16_t clampSpeed(int32_t v) {
  if (v > 255) return 255;
  if (v < -255) return -255;
  return (int16_t)v;
}

struct MotorPins {
  uint8_t en;
  uint8_t in1;
  uint8_t in2;
};

class L298Motor {
public:
  L298Motor() = default;
  explicit L298Motor(MotorPins p) : _p(p) {}

  void begin() {
    pinMode(_p.en, OUTPUT);
    pinMode(_p.in1, OUTPUT);
    pinMode(_p.in2, OUTPUT);
    digitalWrite(_p.in1, LOW);
    digitalWrite(_p.in2, LOW);
    analogWrite(_p.en, 0);
    _target  = 0;
    _applied = 0;
  }

  void setInverted(bool inv) { _inv = inv; }
  bool inverted() const { return _inv; }

  void setTarget(int16_t sp) { _target = clampSpeed(sp); }
  int16_t target() const { return _target; }

  int16_t applied() const { return _applied; }

  void apply(int16_t sp, bool brakeIfZero);

private:
  MotorPins _p{};
  bool _inv = false;

  int16_t _target  = 0;
  int16_t _applied = 0;

  friend class PowerManager;
};

class L298Dual {
public:
  L298Dual(L298Motor& a, L298Motor& b) : A(a), B(b) {}

  void begin() { A.begin(); B.begin(); }

  void setTargetA(int16_t sp) { A.setTarget(sp); }
  void setTargetB(int16_t sp) { B.setTarget(sp); }

  void stop(bool brake = false) {
    A.setTarget(0);
    B.setTarget(0);
    A.apply(0, brake);
    B.apply(0, brake);
  }

  L298Motor& A;
  L298Motor& B;
};

class PowerManager {
public:
  void setPowerLimitPercent(uint8_t p) {
    if (p < 10) p = 10;
    if (p > 100) p = 100;
    _powerLimit = p;
  }
  uint8_t powerLimitPercent() const { return _powerLimit; }

  void setEcoMode(bool on) { _eco = on; }
  bool ecoMode() const { return _eco; }

  void setRampPwmPerSec(uint16_t v) {
    if (v < 50) v = 50;
    _ramp = v;
  }
  uint16_t rampPwmPerSec() const { return _ramp; }

  void setDerating(bool on) { _derating = on; }
  bool derating() const { return _derating; }

  void updateModule(L298Dual& mod, uint32_t dtMs, bool brakeIfZero, uint16_t& heatAccum);

private:
  uint8_t  _powerLimit = 100;
  bool     _eco = false;
  uint16_t _ramp = 900;
  bool     _derating = true;
};

class CrossDrive {
public:
  CrossDrive(L298Dual& driverX, L298Dual& driverY)
    : _x(driverX), _y(driverY) {}

  void begin() { _x.begin(); _y.begin(); }

  void setMotorInvert(uint8_t motorId, bool inv);

  void setSingleDriverMode(bool on) { _singleDriver = on; }
  bool singleDriverMode() const { return _singleDriver; }

  void setStrafeEnabled(bool on) { _strafeEnabled = on; }
  bool strafeEnabled() const { return _strafeEnabled; }

  PowerManager& power() { return _pm; }

  void command(int16_t forward, int16_t strafe, int16_t rotate);

  void stop(bool brake = false);

  void update(bool brakeIfZero = false);

private:
  L298Dual& _x;
  L298Dual& _y;

  PowerManager _pm;

  bool _singleDriver = true;
  bool _strafeEnabled = false;

  uint16_t _heatX = 0;
  uint16_t _heatY = 0;

  int16_t _m1 = 0;
  int16_t _m2 = 0;
  int16_t _m3 = 0;
  int16_t _m4 = 0;

  uint32_t _lastMs = 0;

  void applyTargets();
};
