// motor-control-sync.js — логика синхронных слайдеров и полей

function syncAngle(slider) {
    const motorId = slider.closest('.motor').dataset.id;
    const value = slider.value;
    const numberInput = slider.nextElementSibling;
    if (numberInput) numberInput.value = value;
    debounceSet(motorId, "angle", value);
  }
  
  function syncAngleInput(numberInput) {
    const slider = numberInput.previousElementSibling;
    if (slider && slider.type === "range") {
      slider.value = numberInput.value;
      slider.dispatchEvent(new Event('input'));
    }
  }
  
  function syncOffset(slider) {
    const motorId = slider.closest('.motor').dataset.id;
    const value = slider.value;
    const numberInput = slider.nextElementSibling;
    if (numberInput) numberInput.value = value;
    debounceSet(motorId, "offset", value);
  }
  
  function syncOffsetInput(numberInput) {
    const slider = numberInput.previousElementSibling;
    if (slider && slider.type === "range") {
      slider.value = numberInput.value;
      slider.dispatchEvent(new Event('input'));
    }
  }
  
  function syncMin(slider) {
    const motorId = slider.closest('.motor').dataset.id;
    const value = slider.value;
    const numberInput = slider.nextElementSibling;
    if (numberInput) numberInput.value = value;
    debounceSet(motorId, "min", value);
  }
  
  function syncMinInput(numberInput) {
    const slider = numberInput.previousElementSibling;
    if (slider && slider.type === "range") {
      slider.value = numberInput.value;
      slider.dispatchEvent(new Event('input'));
    }
  }
  
  function syncMax(slider) {
    const motorId = slider.closest('.motor').dataset.id;
    const value = slider.value;
    const numberInput = slider.nextElementSibling;
    if (numberInput) numberInput.value = value;
    debounceSet(motorId, "max", value);
  }
  
  function syncMaxInput(numberInput) {
    const slider = numberInput.previousElementSibling;
    if (slider && slider.type === "range") {
      slider.value = numberInput.value;
      slider.dispatchEvent(new Event('input'));
    }
  }
  