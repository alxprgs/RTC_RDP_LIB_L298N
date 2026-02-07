#include "RobotL298N.h"

static inline uint8_t u8abs16(int16_t v) {
  int16_t a = (v < 0) ? (int16_t)(-v) : v;
  if (a > 255) a = 255;
  return (uint8_t)a;
}

static inline int16_t rampTo(int16_t cur, int16_t tgt, uint16_t step) {
  if (cur == tgt) return cur;
  return (cur < tgt) ? (cur + step) : (cur - step);
}

void L298Motor::apply(int16_t sp, bool brakeIfZero) {
  sp = clampSpeed(sp);

  if (_inv) sp = (int16_t)(-sp);

  _applied = sp;

  const uint8_t pwm = u8abs16(sp);

  if (sp > 0) {
    digitalWrite(_p.in1, HIGH);
    digitalWrite(_p.in2, LOW);
    analogWrite(_p.en, pwm);
    return;
  }

  if (sp < 0) {
    digitalWrite(_p.in1, LOW);
    digitalWrite(_p.in2, HIGH);
    analogWrite(_p.en, pwm);
    return;
  }

  if (brakeIfZero) {
    digitalWrite(_p.in1, HIGH);
    digitalWrite(_p.in2, HIGH);
    analogWrite(_p.en, 255);
  } else {
    digitalWrite(_p.in1, LOW);
    digitalWrite(_p.in2, LOW);
    analogWrite(_p.en, 0);
  }
}

void PowerManager::updateModule(L298Dual& mod, uint32_t dtMs, bool brakeIfZero, uint16_t& heatAccum) {
  if (dtMs == 0) dtMs = 1;

  if (dtMs > 200) dtMs = 200;

  uint16_t limit = (uint16_t)(255UL * (uint32_t)_powerLimit / 100UL); // 0..255

  if (_eco) {
    limit = (uint16_t)(limit * 75UL / 100UL);
  }

  if (_derating) {
    uint16_t loadA = (uint16_t)u8abs16(mod.A.target());
    uint16_t loadB = (uint16_t)u8abs16(mod.B.target());
    uint16_t load  = (loadA > loadB) ? loadA : loadB;

    if (load > 20) {
      uint32_t add = (uint32_t)load * (uint32_t)dtMs / 12UL;
      uint32_t h = (uint32_t)heatAccum + add;
      heatAccum = (h > 60000UL) ? 60000UL : (uint16_t)h;
    } else {
      int32_t cool = (int32_t)heatAccum - (int32_t)dtMs * 8;
      heatAccum = (cool < 0) ? 0 : (uint16_t)cool;
    }

    uint16_t deratePct = 100;
    if (heatAccum > 5000) {
      uint32_t x = (uint32_t)(heatAccum - 5000);
      uint32_t span = 55000UL;
      if (x > span) x = span;
      deratePct = (uint16_t)(100 - (45UL * x / span));
    }

    limit = (uint16_t)(limit * (uint32_t)deratePct / 100UL);

    if (limit < 40) limit = 40;
  }

  if (limit > 255) limit = 255;

  uint32_t step32 = (uint32_t)_ramp * (uint32_t)dtMs / 1000UL;
  step32 = constrain(step32, 1, 255);
  uint16_t step = (uint16_t)step32;

  auto applyOne = [&](L298Motor& m) {
    int16_t tgt = m.target();

    if (tgt > (int16_t)limit) tgt = (int16_t)limit;
    if (tgt < -(int16_t)limit) tgt = -(int16_t)limit;

    int16_t next = rampTo(m._applied, tgt, step);
    m.apply(next, brakeIfZero);
  };

  applyOne(mod.A);
  applyOne(mod.B);
}

void CrossDrive::setMotorInvert(uint8_t motorId, bool inv) {
  switch (motorId) {
    case 1: _y.A.setInverted(inv); break;
    case 2: _x.A.setInverted(inv); break;
    case 3: _y.B.setInverted(inv); break;
    case 4: _x.B.setInverted(inv); break;
    default: break;
  }
}

void CrossDrive::applyTargets() {
  _y.setTargetA(_m1);
  _y.setTargetB(_m3);

  _x.setTargetA(_m2);
  _x.setTargetB(_m4);
}

void CrossDrive::command(int16_t forward, int16_t strafe, int16_t rotate) {
  forward = clampSpeed(forward);
  strafe  = clampSpeed(strafe);
  rotate  = clampSpeed(rotate);

  if (!_strafeEnabled) {
    strafe = 0;
  }

  const int16_t STRAFE_ON = 20;
  const bool strafeActive = (abs(strafe) >= STRAFE_ON);

  if (_singleDriver) {
    if (_strafeEnabled && strafeActive) {
      _m1 = 0;
      _m3 = 0;

      _m2 = clampSpeed((int32_t)strafe + rotate);
      _m4 = clampSpeed((int32_t)(-strafe) + rotate);
    } else {
      _m2 = 0;
      _m4 = 0;

      _m1 = clampSpeed((int32_t)forward + rotate);
      _m3 = clampSpeed((int32_t)(-forward) + rotate);
    }
  } else {
    _m1 = clampSpeed((int32_t)forward + rotate);
    _m3 = clampSpeed((int32_t)(-forward) + rotate);

    if (_strafeEnabled) {
      _m2 = clampSpeed((int32_t)strafe + rotate);
      _m4 = clampSpeed((int32_t)(-strafe) + rotate);
    } else {
      _m2 = rotate;
      _m4 = rotate;
    }
  }

  applyTargets();
}

void CrossDrive::stop(bool brake) {
  _m1 = _m2 = _m3 = _m4 = 0;
  applyTargets();

  _x.stop(brake);
  _y.stop(brake);
}

void CrossDrive::update(bool brakeIfZero) {
  const uint32_t now = millis();

  if (_lastMs == 0) {
    _lastMs = now;
    return;
  }

  uint32_t dt = now - _lastMs;
  _lastMs = now;

  _pm.updateModule(_x, dt, brakeIfZero, _heatX);
  _pm.updateModule(_y, dt, brakeIfZero, _heatY);
}