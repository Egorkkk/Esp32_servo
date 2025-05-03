// load-motor-state.js — загрузка параметров моторов из /get и обновление интерфейса

function loadMotorState(motorId) {
    fetch(`/get?id=${motorId}`)
      .then(res => res.json())
      .then(data => {
        const block = document.querySelector(`.motor[data-id="${motorId}"]`);
        if (!block) return;
  
        block.querySelector('input[type="range"][oninput*="syncAngle"]').value = data.angle;
        block.querySelector('input[type="number"][onchange*="syncAngleInput"]').value = data.angle;
  
        block.querySelector('input[type="range"][oninput*="syncOffset"]').value = data.offset;
        block.querySelector('input[type="number"][onchange*="syncOffsetInput"]').value = data.offset;
  
        block.querySelector('input[type="range"][oninput*="syncMin"]').value = data.min;
        block.querySelector('input[type="number"][onchange*="syncMinInput"]').value = data.min;
  
        block.querySelector('input[type="range"][oninput*="syncMax"]').value = data.max;
        block.querySelector('input[type="number"][onchange*="syncMaxInput"]').value = data.max;
      });
  }
  
  function loadAllMotors() {
    const motorIds = [
      "camL1", "camL2", "camL_YAW",
      "camC1", "camC2", "camC_YAW",
      "camR1", "camR2", "camR_YAW"
    ];
  
    motorIds.forEach(loadMotorState);
  }