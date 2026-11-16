import * as THREE from "three";
import { GLTFLoader } from "./vendor/GLTFLoader.js";
import { createTracker } from "./motion.js";

const EXPRESSIONS = {
  calm: "平静",
  blink: "眨眼",
  surprised: "惊讶",
  angry: "生气",
  dizzy: "头晕",
};

const lobby = document.querySelector("#lobby");
const stage = document.querySelector("#stage");
const c2d = document.querySelector("#c2d");
const c3d = document.querySelector("#c3d");
const ctx = c2d.getContext("2d");
const hudExpression = document.querySelector("#hud-expression");
const hudPose = document.querySelector("#hud-pose");
const hudPan = document.querySelector("#hud-pan");
const clock = document.querySelector("#clock");
const sensorLine = document.querySelector("#sensor");
const modeButtons = [...document.querySelectorAll(".card")];
const characterSelect = document.querySelector("#character");
const sensitivity = document.querySelector("#sensitivity");
const sensOut = document.querySelector("#sens-out");
const autoBox = document.querySelector("#auto");
const idleInput = document.querySelector("#idle");
const laptop = document.querySelector("#laptop");

const sim = {
  ax: 0, ay: 0, az: 9.81,
  targetAx: 0, targetAy: 0, targetAz: 9.81,
  impulse: 0,
  pushX: 0, pushY: 0, pushUntil: 0,
  holdPanX: 0, holdPanY: 0, holdUntil: 0,
  suppressPanUntil: 0,
};

let mode = "2d";
let tracker = createTracker();
let running = false;
let sensorOn = false;
let latestSensor = null;
let photo = null;
let shake = 0;
let shown = "";
let latched = "calm";
let latchUntil = 0;
let lastFrame = performance.now();
let lastInput = performance.now();
let enteredByAuto = false;

const world = { renderer: null, scene: null, camera: null, character: null, custom: null };

modeButtons.forEach((button) => {
  button.addEventListener("click", () => {
    mode = button.dataset.mode;
    modeButtons.forEach((item) => item.classList.toggle("selected", item === button));
  });
});

sensitivity.addEventListener("input", () => {
  sensOut.textContent = Number(sensitivity.value).toFixed(1);
});

document.querySelector("#photo").addEventListener("change", (event) => {
  const file = event.target.files[0];
  if (!file) return;
  const image = new Image();
  image.onload = () => {
    photo = image;
    characterSelect.value = "custom";
  };
  image.src = URL.createObjectURL(file);
});

document.querySelector("#model").addEventListener("change", async (event) => {
  const file = event.target.files[0];
  if (!file) return;
  const buffer = await file.arrayBuffer();
  const loader = new GLTFLoader();
  loader.parse(buffer, "", (gltf) => {
    world.custom = gltf.scene;
    fitModel(world.custom);
    characterSelect.value = "custom";
    if (running && mode === "3d") swapCharacter();
  }, (error) => {
    hudExpression.textContent = "模型没有载入";
    console.error(error);
  });
});

document.querySelector("#start").addEventListener("click", () => enter(false));
document.querySelector("#exit").addEventListener("click", leave);
window.addEventListener("keydown", (event) => {
  if (!running) return;
  if (event.key === "Escape") leave();
  if (event.key === "ArrowLeft") tilt(0, -5.2, 8.3);
  if (event.key === "ArrowRight") tilt(0, 5.2, 8.3);
  if (event.key === "ArrowUp") tilt(-4.6, 0, 8.6);
  if (event.key === "ArrowDown") tilt(4.6, 0, 8.6);
  if (event.key === "1") tap(10);
  if (event.key === "2") tap(14);
  if (event.key === "3") tap(26);
});

document.querySelector("#tilt-left").onclick = () => tilt(0, -5.2, 8.3);
document.querySelector("#tilt-right").onclick = () => tilt(0, 5.2, 8.3);
document.querySelector("#tilt-forward").onclick = () => tilt(-4.6, 0, 8.6);
document.querySelector("#tilt-back").onclick = () => tilt(4.6, 0, 8.6);
document.querySelector("#tilt-reset").onclick = () => tilt(0, 0, 9.81);
document.querySelector("#pan-left").onclick = () => push(0, -3.6);
document.querySelector("#pan-right").onclick = () => push(0, 3.6);
document.querySelector("#pan-forward").onclick = () => push(3.4, 0);
document.querySelector("#pan-back").onclick = () => push(-3.4, 0);
document.querySelector("#tap-light").onclick = () => tap(10);
document.querySelector("#tap-mid").onclick = () => tap(14);
document.querySelector("#tap-hard").onclick = () => tap(26);

["pointerdown", "keydown", "wheel"].forEach((name) => {
  document.addEventListener(name, (event) => {
    if (running && sensorOn && event.target === c2d) leave();
    if (!running) lastInput = performance.now();
  });
});

setInterval(pollSensor, 80);
setInterval(checkIdle, 400);
window.addEventListener("resize", resize);
requestAnimationFrame(frame);
pollSensor();

function tilt(ax, ay, az) {
  sim.targetAx = ax;
  sim.targetAy = ay;
  sim.targetAz = az;
  sim.holdUntil = 0;
  sim.suppressPanUntil = performance.now() + 1400;
}

function push(ax, ay) {
  sim.holdPanX = ax === 0 ? 0 : Math.sign(ax) * 1.15;
  sim.holdPanY = ay === 0 ? 0 : Math.sign(ay) * 1.15;
  sim.holdUntil = performance.now() + 2600;
  sim.pushX = ax;
  sim.pushY = ay;
  sim.pushUntil = performance.now() + 700;
}

function tap(extra) {
  sim.impulse = extra;
}

function enter(auto) {
  enteredByAuto = auto;
  tracker = createTracker({ tapThreshold: 6.5 / Number(sensitivity.value) });
  latched = "calm";
  latchUntil = 0;
  sim.ax = sim.targetAx;
  sim.ay = sim.targetAy;
  sim.az = sim.targetAz;
  running = true;
  lobby.hidden = true;
  stage.hidden = false;
  resize();
  if (mode === "3d") ensureWorld();
  c2d.style.display = mode === "2d" ? "block" : "none";
  c3d.style.display = mode === "3d" ? "block" : "none";
  document.querySelector("#controls").hidden = sensorOn;
}

function leave() {
  running = false;
  stage.hidden = true;
  lobby.hidden = false;
  lastInput = performance.now();
}

function checkIdle() {
  if (running || !autoBox.checked) return;
  const wait = Math.max(5, Number(idleInput.value) || 90) * 1000;
  if (performance.now() - lastInput > wait) enter(true);
}

async function pollSensor() {
  try {
    const response = await fetch("/api/imu");
    const body = await response.json();
    sensorOn = Boolean(body.available);
    latestSensor = body.available ? body : null;
  } catch {
    sensorOn = false;
    latestSensor = null;
  }
  sensorLine.textContent = sensorOn
    ? "已连上这台电脑的加速度计。"
    : "这台电脑没有加速度计。屏保里可以用虚拟姿态：倾斜、平移和拍打。";
}

function sample(now) {
  if (sensorOn && latestSensor) {
    return { t: now / 1000, ax: latestSensor.ax, ay: latestSensor.ay, az: latestSensor.az };
  }
  const dt = 1 / 60;
  const ease = 1 - Math.exp(-dt / 0.22);
  sim.ax += (sim.targetAx - sim.ax) * ease;
  sim.ay += (sim.targetAy - sim.ay) * ease;
  sim.az += (sim.targetAz - sim.az) * ease;
  if (now > sim.pushUntil) {
    sim.pushX = 0;
    sim.pushY = 0;
  }
  const az = sim.az + sim.impulse;
  sim.impulse = 0;
  return { t: now / 1000, ax: sim.ax + sim.pushX, ay: sim.ay + sim.pushY, az };
}

function frame(now) {
  requestAnimationFrame(frame);
  const dt = Math.min(0.05, (now - lastFrame) / 1000);
  lastFrame = now;
  clock.textContent = new Date().toLocaleTimeString("zh-CN", { hour: "2-digit", minute: "2-digit" });
  if (!running) return;
  const state = tracker.step(sample(now));
  if (state.expression !== "calm") {
    latched = state.expression;
    latchUntil = now + 3200;
  } else if (now > latchUntil) {
    latched = "calm";
  }
  const view = viewPose(state, now);
  view.expression = latched;
  if (state.tap) shake = 18 + state.tap.intensity * 40;
  shake = Math.max(0, shake - dt * 22);
  paintHud(view);
  if (mode === "2d") paint2d(view, now / 1000);
  else paint3d(view, now / 1000);
}

function viewPose(state, now) {
  const held = !sensorOn && now < sim.holdUntil;
  const suppress = !sensorOn && !held && now < sim.suppressPanUntil;
  return {
    roll: state.roll,
    pitch: state.pitch,
    panX: held ? sim.holdPanX : suppress ? 0 : state.panX,
    panY: held ? sim.holdPanY : suppress ? 0 : state.panY,
    expression: state.expression,
    tap: state.tap,
  };
}

function paintHud(state) {
  const roll = state.roll * 180 / Math.PI;
  const pitch = state.pitch * 180 / Math.PI;
  const rollWord = roll > 4 ? "右倾" : roll < -4 ? "左倾" : "水平";
  const pitchWord = pitch > 4 ? "前倾" : pitch < -4 ? "后仰" : "平放";
  const panX = state.panX > 0.08 ? "前" : state.panX < -0.08 ? "后" : "";
  const panY = state.panY > 0.08 ? "右" : state.panY < -0.08 ? "左" : "";
  hudExpression.textContent = `表情：${EXPRESSIONS[state.expression] || state.expression}`;
  hudPose.textContent = `旋转：${rollWord} ${Math.abs(roll).toFixed(0)}° · ${pitchWord} ${Math.abs(pitch).toFixed(0)}°`;
  hudPan.textContent = `平移：${panX}${panY}` || "平移：居中";
  if (!panX && !panY) hudPan.textContent = "平移：居中";
  laptop.style.setProperty("--roll", `${roll}deg`);
  laptop.style.setProperty("--pitch", `${-pitch}deg`);
  laptop.style.setProperty("--panx", `${state.panY * -28}px`);
  laptop.style.setProperty("--pany", `${state.panX * -18}px`);
  stage.dataset.expression = state.expression;
  stage.dataset.roll = roll.toFixed(1);
  stage.dataset.pitch = pitch.toFixed(1);
}

function resize() {
  const width = window.innerWidth;
  const height = window.innerHeight;
  const ratio = Math.min(window.devicePixelRatio || 1, 2);
  c2d.width = Math.floor(width * ratio);
  c2d.height = Math.floor(height * ratio);
  if (world.renderer) {
    world.renderer.setSize(width, height);
    world.camera.aspect = width / height;
    world.camera.updateProjectionMatrix();
  }
}

function faceAt(expression, time) {
  const blink = expression === "calm" && time % 3.6 > 3.42;
  const name = blink ? "blink" : expression;
  if (name === "blink") return { open: 0.08, brow: 0, mouth: 0.3, blush: 0.15, spiral: false, angry: false };
  if (name === "surprised") return { open: 1, brow: -0.7, mouth: 0.9, blush: 0.2, spiral: false, angry: false };
  if (name === "angry") return { open: 0.55, brow: 0.9, mouth: -0.8, blush: 0.85, spiral: false, angry: true };
  if (name === "dizzy") return { open: 0.8, brow: 0.1, mouth: 0.15, blush: 0.3, spiral: true, angry: false };
  return { open: 0.62, brow: 0, mouth: 0.35, blush: 0.28, spiral: false, angry: false };
}

function paint2d(state, time) {
  const width = c2d.width;
  const height = c2d.height;
  ctx.setTransform(1, 0, 0, 1, 0, 0);
  const sky = ctx.createLinearGradient(0, 0, 0, height);
  sky.addColorStop(0, "#1a2233");
  sky.addColorStop(1, "#101218");
  ctx.fillStyle = sky;
  ctx.fillRect(0, 0, width, height);
  const glow = ctx.createRadialGradient(width * 0.5, height * 0.42, 20, width * 0.5, height * 0.42, width * 0.38);
  glow.addColorStop(0, "rgba(228, 177, 90, 0.22)");
  glow.addColorStop(1, "rgba(228, 177, 90, 0)");
  ctx.fillStyle = glow;
  ctx.fillRect(0, 0, width, height);

  const breath = 1 + Math.sin(time * 1.6) * 0.018;
  const who = characterSelect.value;
  ctx.save();
  ctx.translate(width / 2 + Math.sin(time * 42) * shake, height * 0.46);
  ctx.scale(breath, breath);
  const face = faceAt(state.expression, time);
  if (who === "custom" && photo) drawPhoto(face, time);
  else if (who === "page") drawPage(face, time);
  else drawPobo(face, time);
  ctx.restore();
  ctx.fillStyle = "#f6f1e7";
  ctx.font = `${Math.max(28, Math.floor(width * 0.035))}px "WenQuanYi Micro Hei", sans-serif`;
  ctx.textAlign = "center";
  ctx.fillText(EXPRESSIONS[state.expression] || state.expression, width / 2, height * 0.78);
}

function drawPobo(face, time) {
  const unit = Math.min(c2d.width, c2d.height) * 0.18;
  ctx.fillStyle = "rgba(0,0,0,0.25)";
  ctx.beginPath();
  ctx.ellipse(0, unit * 1.35, unit * 0.85, unit * 0.18, 0, 0, Math.PI * 2);
  ctx.fill();
  ear(-unit * 0.55, -unit * 0.85, unit * 0.28, "#f2c14e");
  ear(unit * 0.55, -unit * 0.85, unit * 0.28, "#f2c14e");
  const body = ctx.createRadialGradient(-unit * 0.2, -unit * 0.3, unit * 0.2, 0, 0, unit);
  body.addColorStop(0, "#ffe7ad");
  body.addColorStop(1, "#e2a63a");
  ctx.fillStyle = body;
  ctx.beginPath();
  ctx.arc(0, 0, unit, 0, Math.PI * 2);
  ctx.fill();
  eyes(face, unit * 0.34, unit * 0.42, time);
  mouth(face, unit * 0.34);
  blush(face, unit * 0.62, unit * 0.18);
}

function drawPage(face, time) {
  const unit = Math.min(c2d.width, c2d.height) * 0.16;
  roundRect(-unit * 0.95, -unit * 1.15, unit * 1.9, unit * 2.15, unit * 0.45, "#f4efe4");
  ctx.fillStyle = "#2c3954";
  ctx.beginPath();
  ctx.ellipse(0, -unit * 0.95, unit * 0.78, unit * 0.38, 0, Math.PI, 0);
  ctx.fill();
  ctx.fillStyle = "#d15d5d";
  roundRect(-unit * 0.7, unit * 0.15, unit * 1.4, unit * 0.28, 8, "#d15d5d");
  eyes(face, unit * 0.32, unit * 0.38, time);
  mouth(face, unit * 0.28);
  blush(face, unit * 0.55, unit * 0.16);
}

function drawPhoto(face, time) {
  const unit = Math.min(c2d.width, c2d.height) * 0.22;
  ctx.save();
  ctx.beginPath();
  ctx.arc(0, 0, unit, 0, Math.PI * 2);
  ctx.clip();
  const scale = Math.max((unit * 2) / photo.width, (unit * 2) / photo.height);
  ctx.drawImage(photo, -photo.width * scale / 2, -photo.height * scale / 2, photo.width * scale, photo.height * scale);
  ctx.restore();
  ctx.lineWidth = 10;
  ctx.strokeStyle = face.angry ? "#d15d5d" : "#e4b15a";
  ctx.beginPath();
  ctx.arc(0, 0, unit + 8, 0, Math.PI * 2);
  ctx.stroke();
  if (face.spiral) stars(unit * 1.35, time);
  if (face.open > 0.9) marks(unit * 1.15);
}

function ear(x, y, radius, color) {
  ctx.fillStyle = color;
  ctx.beginPath();
  ctx.arc(x, y, radius, 0, Math.PI * 2);
  ctx.fill();
  ctx.fillStyle = "#f7d7c4";
  ctx.beginPath();
  ctx.arc(x, y, radius * 0.45, 0, Math.PI * 2);
  ctx.fill();
}

function eyes(face, spread, radius, time) {
  const lift = face.brow * radius * 0.45;
  [-1, 1].forEach((side) => {
    ctx.strokeStyle = face.angry ? "#8d3a32" : "#5c4632";
    ctx.lineWidth = Math.max(6, radius * 0.18);
    ctx.beginPath();
    ctx.moveTo(side * spread - radius * 0.7, -radius * 1.15 + lift * side);
    ctx.lineTo(side * spread + radius * 0.7, -radius * 1.15 - lift * side);
    ctx.stroke();
    const open = Math.max(0.08, face.open);
    ctx.fillStyle = "#fffaf3";
    ctx.beginPath();
    ctx.ellipse(side * spread, 0, radius, radius * open, 0, 0, Math.PI * 2);
    ctx.fill();
    if (face.spiral) {
      spiral(side * spread, 0, radius * 0.55, time);
    } else if (open > 0.2) {
      ctx.fillStyle = "#241c16";
      ctx.beginPath();
      ctx.arc(side * spread, radius * 0.08, radius * 0.38, 0, Math.PI * 2);
      ctx.fill();
    }
  });
}

function mouth(face, width) {
  ctx.strokeStyle = "#6a4332";
  ctx.lineWidth = 7;
  ctx.beginPath();
  if (face.mouth > 0.7) {
    ctx.ellipse(0, width * 0.85, width * 0.28, width * 0.36, 0, 0, Math.PI * 2);
    ctx.stroke();
    return;
  }
  const bend = face.mouth * width;
  ctx.moveTo(-width * 0.45, width * 0.75);
  ctx.quadraticCurveTo(0, width * 0.75 + bend, width * 0.45, width * 0.75);
  ctx.stroke();
}

function blush(face, x, radius) {
  ctx.fillStyle = `rgba(214, 96, 96, ${0.15 + face.blush * 0.45})`;
  [-1, 1].forEach((side) => {
    ctx.beginPath();
    ctx.ellipse(side * x, radius * 2.2, radius * 1.3, radius, 0, 0, Math.PI * 2);
    ctx.fill();
  });
}

function spiral(x, y, radius, time) {
  ctx.strokeStyle = "#241c16";
  ctx.lineWidth = 3;
  ctx.beginPath();
  for (let step = 0; step < 20; step += 1) {
    const angle = step * 0.55 + time * 8;
    const rr = radius * step / 20;
    const px = x + Math.cos(angle) * rr;
    const py = y + Math.sin(angle) * rr;
    if (step === 0) ctx.moveTo(px, py);
    else ctx.lineTo(px, py);
  }
  ctx.stroke();
}

function stars(radius, time) {
  ctx.fillStyle = "#f2c14e";
  for (let i = 0; i < 5; i += 1) {
    const angle = time * 3 + i * 1.25;
    ctx.beginPath();
    ctx.arc(Math.cos(angle) * radius, Math.sin(angle) * radius * 0.55 - 20, 7, 0, Math.PI * 2);
    ctx.fill();
  }
}

function marks(radius) {
  ctx.strokeStyle = "#f6f1e7";
  ctx.lineWidth = 4;
  [-1, 1].forEach((side) => {
    ctx.beginPath();
    ctx.moveTo(side * radius, -radius * 0.2);
    ctx.lineTo(side * radius * 1.25, -radius * 0.55);
    ctx.stroke();
  });
}

function roundRect(x, y, w, h, r, color) {
  ctx.fillStyle = color;
  ctx.beginPath();
  ctx.roundRect(x, y, w, h, r);
  ctx.fill();
}

function ensureWorld() {
  if (world.renderer) return;
  world.renderer = new THREE.WebGLRenderer({ canvas: c3d, antialias: true, alpha: true });
  world.renderer.setPixelRatio(Math.min(window.devicePixelRatio || 1, 2));
  world.scene = new THREE.Scene();
  world.camera = new THREE.PerspectiveCamera(38, 1, 0.1, 50);
  world.scene.add(new THREE.HemisphereLight(0xd9e4f5, 0x2a2118, 1.1));
  const key = new THREE.DirectionalLight(0xfff1d2, 2.2);
  key.position.set(3, 5, 4);
  world.scene.add(key);
  const rim = new THREE.DirectionalLight(0x8fb7d8, 0.8);
  rim.position.set(-4, 2, -3);
  world.scene.add(rim);
  const floor = new THREE.Mesh(
    new THREE.CircleGeometry(3.2, 48),
    new THREE.MeshStandardMaterial({ color: 0x171b24, roughness: 1 })
  );
  floor.rotation.x = -Math.PI / 2;
  floor.position.y = -1.35;
  world.scene.add(floor);
  world.character = buildPobo();
  world.scene.add(world.character);
  resize();
}

function buildPobo() {
  const group = new THREE.Group();
  const skin = new THREE.MeshStandardMaterial({ color: 0xf2c14e, roughness: 0.48 });
  const belly = new THREE.MeshStandardMaterial({ color: 0xffe3a8, roughness: 0.6 });
  const dark = new THREE.MeshStandardMaterial({ color: 0x241c16, roughness: 0.4 });
  const body = new THREE.Mesh(new THREE.SphereGeometry(1, 40, 28), skin);
  body.scale.set(1, 1.05, 0.92);
  group.add(body);
  const tum = new THREE.Mesh(new THREE.SphereGeometry(0.48, 24, 18), belly);
  tum.position.set(0, -0.12, 0.62);
  group.add(tum);
  const eyeGeo = new THREE.SphereGeometry(0.16, 20, 16);
  [-0.28, 0.28].forEach((x) => {
    const white = new THREE.Mesh(eyeGeo, new THREE.MeshStandardMaterial({ color: 0xfffaf3 }));
    white.position.set(x, 0.18, 0.78);
    white.scale.set(1, 1.05, 0.55);
    group.add(white);
    const pupil = new THREE.Mesh(new THREE.SphereGeometry(0.07, 16, 12), dark);
    pupil.position.set(x, 0.16, 0.9);
    group.add(pupil);
  });
  [-0.48, 0.62].forEach((x, index) => {
    const ear = new THREE.Mesh(new THREE.SphereGeometry(0.28, 20, 16), skin);
    ear.position.set(x, 0.78, 0);
    group.add(ear);
    if (index === 1) {
      const ribbon = new THREE.Mesh(
        new THREE.SphereGeometry(0.12, 16, 12),
        new THREE.MeshStandardMaterial({ color: 0xd15d5d })
      );
      ribbon.position.set(x + 0.16, 0.68, 0.16);
      group.add(ribbon);
    }
  });
  const nose = new THREE.Mesh(new THREE.SphereGeometry(0.08, 12, 10), new THREE.MeshStandardMaterial({ color: 0xc9844a }));
  nose.position.set(0.18, 0.02, 0.92);
  group.add(nose);
  group.position.y = 0.15;
  return group;
}

function fitModel(model) {
  const box = new THREE.Box3().setFromObject(model);
  const size = box.getSize(new THREE.Vector3());
  const center = box.getCenter(new THREE.Vector3());
  const scale = 2.2 / Math.max(size.x, size.y, size.z);
  model.scale.setScalar(scale);
  model.position.sub(center.multiplyScalar(scale));
  model.position.y -= 0.4;
}

function swapCharacter() {
  if (!world.scene) ensureWorld();
  if (world.character) world.scene.remove(world.character);
  if (characterSelect.value === "custom" && world.custom) {
    world.character = world.custom;
  } else if (characterSelect.value === "page") {
    world.character = buildPobo();
    world.character.traverse((child) => {
      if (child.material && child.material.color && child.material.color.getHex() === 0xf2c14e) {
        child.material = child.material.clone();
        child.material.color.set(0xf4efe4);
      }
    });
  } else {
    world.character = buildPobo();
  }
  world.character.userData.baseY = world.character.position.y;
  world.scene.add(world.character);
}

function paint3d(state, time) {
  ensureWorld();
  const distance = 6.4;
  const yaw = state.roll;
  const pitch = Math.max(-0.9, Math.min(0.9, state.pitch));
  world.camera.position.set(
    Math.sin(yaw) * distance * Math.cos(pitch) + state.panY * 1.6,
    0.7 + Math.sin(pitch) * distance * 0.75 + state.panX * 0.7,
    Math.cos(yaw) * distance * Math.cos(pitch)
  );
  world.camera.lookAt(state.panY * 0.8, 0.25 + state.panX * 0.25, 0);
  const who = characterSelect.value === "custom" && world.custom ? "custom" : characterSelect.value;
  if (shown !== who) {
    swapCharacter();
    shown = who;
  }
  const baseY = world.character.userData.baseY ?? 0.15;
  world.character.position.y = baseY + Math.sin(time * 1.5) * 0.04;
  world.renderer.render(world.scene, world.camera);
}
