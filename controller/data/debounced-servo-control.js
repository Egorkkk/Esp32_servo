// debounced-servo-control.js — ограничение частоты отправки запросов /set

const requestQueue = new Map();

function debounceSet(motorId, param, value, delay = 60) {
  const key = `${motorId}-${param}`;
  if (requestQueue.has(key)) {
    clearTimeout(requestQueue.get(key));
  }

  const timeout = setTimeout(() => {
    fetch(`/set?id=${motorId}&${param}=${value}`);
    requestQueue.delete(key);
  }, delay);

  requestQueue.set(key, timeout);
}

function updateAngle(slider) {
  const motorId = slider.closest('.motor').dataset.id;
  debounceSet(motorId, "angle", slider.value);
}

function updateOffset(slider) {
  const motorId = slider.closest('.motor').dataset.id;
  debounceSet(motorId, "offset", slider.value);
}

function updateMin(slider) {
  const motorId = slider.closest('.motor').dataset.id;
  debounceSet(motorId, "min", slider.value);
}

function updateMax(slider) {
  const motorId = slider.closest('.motor').dataset.id;
  debounceSet(motorId, "max", slider.value);
}

function groupPitch(cameraPrefix, value) {
  debounceSet(`${cameraPrefix}1`, "angle", value);
  debounceSet(`${cameraPrefix}2`, "angle", value);
}