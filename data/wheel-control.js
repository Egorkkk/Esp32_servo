// Управление виртуальными штурвалами

// ИДЕИ НА БУДУЩЕЕ:
// - Добавить подтверждение перед центровкой всех камер
// - Кнопка "Сохранить текущее положение" всех моторов
// - Поддержка пользовательских пресетов положений (например, "вид на дорогу", "вид на салон")

let isDragging = false;
let lastAngle = 0;
let targetId = null;
let sensitivity = 0.1;
const motorLimits = {}; // ограничения углов из prefs
const angles = {}; // текущие значения всех моторов
const lastSentAngles = {}; // последнее отправленное значение
const motorIds = [
  "camL1", "camL_YAW",
  "camC1", "camC_YAW",
  "camR1", "camR_YAW"
];

const motorConfig = {
  camL1:     { gearRatio: 14/24, cameraZero: 90 },
  camL_YAW:  { gearRatio: 14/24, cameraZero: 90 },
  camC1:     { gearRatio: 14/24, cameraZero: 0 },
  camC_YAW:  { gearRatio: 14/24, cameraZero: 0 },
  camR1:     { gearRatio: 14/24, cameraZero: -90 },
  camR_YAW:  { gearRatio: 14/24, cameraZero: -90 }
};

function loadMotorLimits() {
  motorIds.forEach(id => {
    fetch(`/get?id=${id}`)
      .then(res => res.json())
      .then(data => {
        motorLimits[id] = {
          min: data.min - 90,
          max: data.max - 90
        };
      });
  });
}
function setSensitivity(val) {
  sensitivity = parseFloat(val);
}

function getAngle(e, element) {
  const rect = element.getBoundingClientRect();
  const cx = rect.left + rect.width / 2;
  const cy = rect.top + rect.height / 2;
  const dx = (e.touches ? e.touches[0].clientX : e.clientX) - cx;
  const dy = (e.touches ? e.touches[0].clientY : e.clientY) - cy;
  return Math.atan2(dy, dx) * (180 / Math.PI);
}

function onWheelDown(e) {
  const wheel = e.target.closest(".wheel");
  if (!wheel) return;
  targetId = wheel.dataset.id;
  isDragging = true;
  lastAngle = getAngle(e, wheel);
  e.preventDefault();
}

function onWheelMove(e) {
  if (!isDragging || !targetId) return;
  const wheel = document.querySelector(`.wheel[data-id="${targetId}"]`);
  const newAngle = getAngle(e, wheel);
  let delta = newAngle - lastAngle;
  if (delta > 180) delta -= 360;
  if (delta < -180) delta += 360;
  lastAngle = newAngle;

  // обновляем локальный угол
  angles[targetId] = (angles[targetId] || 0) + delta * sensitivity;

  // ограничения из prefs
  const limits = motorLimits[targetId] || { min: -30, max: 30 };
  angles[targetId] = Math.max(limits.min, Math.min(limits.max, angles[targetId]));

  // вычисляем сервоугол и реальный угол камеры
  const gear = motorConfig[targetId]?.gearRatio || 1;
  const zero = motorConfig[targetId]?.cameraZero || 0;
  const servoOut = Math.round(90 + angles[targetId]);
  const cameraOut = Math.round((servoOut - 90) * gear + zero);

  // обновляем UI
  document.getElementById(`display-${targetId}`).innerText = `${targetId.includes("YAW") ? "Yaw" : "Pitch"}: ${cameraOut}°`;
  document.querySelector(`.wheel[data-id="${targetId}"] .indicator`).style.transform = `translateX(-50%) rotate(${angles[targetId]}deg)`;

  // отправляем сервоугол, если изменился
  if (lastSentAngles[targetId] !== servoOut) {
    fetch(`/set?id=${targetId}&angle=${servoOut}`);
    lastSentAngles[targetId] = servoOut;
  }
}

function onWheelUp() {
  isDragging = false;
  targetId = null;
}

function resetAngle(id) {
  angles[id] = 0;
  lastSentAngles[id] = 90;
  document.getElementById(`display-${id}`).innerText = `${id.includes("YAW") ? "Yaw" : "Pitch"}: 90°`;
  document.querySelector(`.wheel[data-id="${id}"] .indicator`).style.transform = `translateX(-50%) rotate(0deg)`;
  fetch(`/set?id=${id}&angle=90&save=true`);
}

function centerAllMotors() {
  motorIds.forEach(id => {
    resetAngle(id);
  });
}

function disableMotors() {
  fetch("/disable").then(() => {
    centerAllMotors();
    alert("Моторы отключены, штурвалы сброшены в центр.");
    updateInterfaceLockState();
  });
}

document.addEventListener("DOMContentLoaded", () => {
  document.querySelectorAll(".wheel").forEach(wheel => {
    wheel.addEventListener("mousedown", onWheelDown);
    wheel.addEventListener("touchstart", onWheelDown);
  });
  window.addEventListener("mousemove", onWheelMove);
  window.addEventListener("touchmove", onWheelMove);
  window.addEventListener("mouseup", onWheelUp);
  window.addEventListener("touchend", onWheelUp);

  loadMotorLimits();
  
  // Центруем все колёса и камеры при запуске
  updateInterfaceLockState();
  centerAllMotors();
});

function servoToCamera(servoAngle, gearRatio, cameraZero) {
  return Math.round((servoAngle - 90) * gearRatio + cameraZero);
}

function cameraToServo(cameraAngle, gearRatio, cameraZero) {
  return Math.round((cameraAngle - cameraZero) / gearRatio + 90);
}