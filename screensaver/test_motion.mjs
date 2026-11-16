import { createTracker, expressionFor } from "./motion.js";

function assert(cond, message) {
  if (!cond) throw new Error(message);
}

function settle(tracker, seconds = 0.4) {
  const n = Math.round(seconds * 60);
  let state;
  for (let i = 0; i < n; i++) {
    state = tracker.step({ t: i / 60, ax: 0, ay: 0, az: 9.81 });
  }
  return state;
}

const names = [];
function check(name, fn) {
  fn();
  names.push(name);
}

check("level laptop has no tilt and stays calm", () => {
  const tracker = createTracker();
  const state = settle(tracker, 0.5);
  assert(Math.abs(state.roll) < 0.02, `roll ${state.roll}`);
  assert(Math.abs(state.pitch) < 0.02, `pitch ${state.pitch}`);
  assert(state.expression === "calm", state.expression);
  assert(state.tap == null, "unexpected tap");
});

check("slow tilt changes roll without a tap", () => {
  const tracker = createTracker();
  settle(tracker, 0.3);
  let state;
  let ay = 0;
  for (let i = 0; i < 140; i++) {
    ay += 0.04;
    state = tracker.step({ t: 1 + i / 60, ax: 0, ay, az: 9.81 });
    assert(state.tap == null, `tap during slow tilt at shock ${state.shock}`);
  }
  assert(state.roll > 0.35, `roll ${state.roll}`);
});

check("forward tilt raises pitch", () => {
  const tracker = createTracker();
  settle(tracker, 0.3);
  let state;
  for (let i = 0; i < 180; i++) {
    state = tracker.step({ t: 2 + i / 60, ax: -4.2, ay: 0, az: 8.8 });
  }
  assert(state.pitch > 0.25, `pitch ${state.pitch}`);
  assert(state.tap == null, "tilt was detected as a tap");
});

check("a sideways push pans, then the view springs back", () => {
  const tracker = createTracker();
  settle(tracker, 0.3);
  let state;
  for (let i = 0; i < 30; i++) {
    state = tracker.step({ t: 1 + i / 60, ax: 0, ay: 3.2, az: 9.81 });
  }
  assert(Math.abs(state.panY) > 0.12, `panY ${state.panY}`);
  assert(state.tap == null, "pan was detected as a tap");
  for (let i = 0; i < 180; i++) {
    state = tracker.step({ t: 2 + i / 60, ax: 0, ay: 0, az: 9.81 });
  }
  assert(Math.abs(state.panY) < 0.08, `pan did not return: ${state.panY}`);
});

function hitFresh(extraZ) {
  const tracker = createTracker();
  const frames = 24;
  for (let i = 0; i < frames; i++) {
    tracker.step({ t: i / 60, ax: 0, ay: 0, az: 9.81 });
  }
  return tracker.step({ t: frames / 60, ax: 0, ay: 0, az: 9.81 + extraZ });
}

check("taps map vibration to expressions", () => {
  assert(expressionFor(0.1, 1) === "blink");
  assert(expressionFor(0.3, 1) === "surprised");
  assert(expressionFor(0.6, 1) === "angry");
  assert(expressionFor(0.9, 1) === "dizzy");
  assert(expressionFor(0.1, 5) === "dizzy");

  const light = hitFresh(10);
  assert(light.tap, "light tap missing");
  assert(light.expression === "blink", light.expression);

  const surprised = hitFresh(12);
  assert(surprised.expression === "surprised", surprised.expression);

  const angry = hitFresh(17);
  assert(angry.expression === "angry", angry.expression);

  const dizzy = hitFresh(24);
  assert(dizzy.expression === "dizzy", dizzy.expression);

  const tracker = createTracker();
  settle(tracker, 0.3);
  tracker.step({ t: 0.4, ax: 0, ay: 0, az: 9.81 + 24 });
  let state;
  for (let i = 0; i < 140; i++) {
    state = tracker.step({ t: 0.45 + i / 60, ax: 0, ay: 0, az: 9.81 });
  }
  assert(state.expression === "calm", `expression stuck at ${state.expression}`);
});

check("repeated taps escalate", () => {
  const tracker = createTracker();
  settle(tracker, 0.3);
  let hit;
  for (let i = 0; i < 3; i++) {
    hit = tracker.step({ t: 0.4 + i * 0.4, ax: 0, ay: 0, az: 9.81 + 12 });
    for (let k = 1; k <= 12; k++) {
      tracker.step({ t: 0.4 + i * 0.4 + k / 60, ax: 0, ay: 0, az: 9.81 });
    }
  }
  assert(hit.expression === "angry", hit.expression);
});

console.log(names.map((name) => `ok ${name}`).join("\n"));
