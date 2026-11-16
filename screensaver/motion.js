// Laptop motion → view pose and tap expression.
// ax, ay, az are linear acceleration in m/s^2.
// ax: +screen-up (tilt back), ay: +right, az: +out of the screen.

export function createTracker(options = {}) {
  const gravityTau = options.gravityTau ?? 0.4;
  const panGain = options.panGain ?? 0.95;
  const panDamping = options.panDamping ?? 2.6;
  const panSpring = options.panSpring ?? 2.4;
  const maxPan = options.maxPan ?? 1.8;
  const tapThreshold = options.tapThreshold ?? 6.5;
  const tapCooldown = options.tapCooldown ?? 0.28;

  let gravity = { x: 0, y: 0, z: 9.81 };
  let linear = { x: 0, y: 0, z: 0 };
  let velocity = { x: 0, y: 0 };
  let pan = { x: 0, y: 0 };
  let lastT = null;
  let cooldown = 0;
  let expression = "calm";
  let expressionUntil = 0;
  const taps = [];

  function step(sample) {
    const t = sample.t;
    const dt = lastT == null ? 1 / 60 : clamp(t - lastT, 0.001, 0.1);
    lastT = t;

    const follow = 1 - Math.exp(-dt / gravityTau);
    gravity.x += (sample.ax - gravity.x) * follow;
    gravity.y += (sample.ay - gravity.y) * follow;
    gravity.z += (sample.az - gravity.z) * follow;
    linear = {
      x: sample.ax - gravity.x,
      y: sample.ay - gravity.y,
      z: sample.az - gravity.z,
    };

    velocity.x = velocity.x * Math.exp(-panDamping * dt) + linear.x * dt * panGain;
    velocity.y = velocity.y * Math.exp(-panDamping * dt) + linear.y * dt * panGain;
    velocity.x += -panSpring * pan.x * dt;
    velocity.y += -panSpring * pan.y * dt;
    pan.x = clamp(pan.x + velocity.x * dt, -maxPan, maxPan);
    pan.y = clamp(pan.y + velocity.y * dt, -maxPan, maxPan);

    const roll = Math.atan2(gravity.y, gravity.z);
    const pitch = Math.atan2(-gravity.x, Math.hypot(gravity.y, gravity.z));
    const shock = Math.hypot(linear.x, linear.y, linear.z);

    cooldown = Math.max(0, cooldown - dt);
    let tap = null;
    if (shock > tapThreshold && cooldown <= 0) {
      const intensity = clamp((shock - tapThreshold) / 18, 0, 1);
      tap = { t, intensity, shock };
      cooldown = tapCooldown;
      taps.push(t);
    }
    while (taps.length && t - taps[0] > 3) taps.shift();

    if (tap) {
      expression = expressionFor(tap.intensity, taps.length);
      expressionUntil = t + 0.55 + tap.intensity * 1.15;
    } else if (t > expressionUntil) {
      expression = "calm";
    }

    return {
      roll,
      pitch,
      panX: pan.x,
      panY: pan.y,
      shock,
      expression,
      tap,
    };
  }

  function reset(t = 0) {
    gravity = { x: 0, y: 0, z: 9.81 };
    linear = { x: 0, y: 0, z: 0 };
    velocity = { x: 0, y: 0 };
    pan = { x: 0, y: 0 };
    lastT = t;
    cooldown = 0;
    expression = "calm";
    expressionUntil = 0;
    taps.length = 0;
  }

  return { step, reset };
}

export function expressionFor(intensity, burst) {
  if (intensity >= 0.78 || burst >= 5) return "dizzy";
  if (intensity >= 0.48 || burst >= 3) return "angry";
  if (intensity >= 0.22) return "surprised";
  return "blink";
}

function clamp(value, min, max) {
  return Math.max(min, Math.min(max, value));
}
