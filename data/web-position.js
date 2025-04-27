// web-position.js — команды управления вкладкой POSITION

function enableMotors() {
    fetch("/enable")
      .then(res => res.text())
      .then(text => alert("Сервы включены"));
      updateInterfaceLockState();
  }
  
  function saveAll() {
    document.querySelectorAll(".motor").forEach(motor => {
      const id = motor.dataset.id;
      const min = motor.querySelector('input[oninput*="syncMin"]').value;
      const max = motor.querySelector('input[oninput*="syncMax"]').value;
      const offset = motor.querySelector('input[oninput*="syncOffset"]').value;
  
      fetch(`/set?id=${id}&min=${min}&max=${max}&offset=${offset}&save=true`);
    });
  }
  
  function loadSettings() {
    fetch('/debug')
      .then(res => res.text())
      .then(text => {
        document.getElementById('settingsOutput').innerText = text;
      });
  }
  
  function liveUpdateAngle() {
    const group = document.getElementById("liveGroup").value;
    const angle = document.getElementById("liveAngle").value;
    groupPitch(group, angle);
  }

  /*
  function updateInterfaceLockState() {
    fetch("/status")
      .then(res => res.text())
      .then(enabled => {
        const isEnabled = enabled === "true";
  
        // Блокируем/разблокируем все колёса
        document.querySelectorAll(".wheel").forEach(wheel => {
          wheel.style.pointerEvents = isEnabled ? "auto" : "none";
          wheel.style.opacity = isEnabled ? "1" : "0.4";
        });
  
        // Блокируем/разблокируем все input-поля и кнопки настроек
        document.querySelectorAll(".motor input, .motor button").forEach(el => {
          el.disabled = !isEnabled;
        });
      });
  }
*/

function updateInterfaceLockState() {
  fetch("/status")
    .then(res => res.text())
    .then(enabled => {
      const isEnabled = enabled === "true";

      // Блокируем/разблокируем все штурвалы
      document.querySelectorAll(".wheel").forEach(wheel => {
        wheel.style.pointerEvents = isEnabled ? "auto" : "none";
        wheel.style.opacity = isEnabled ? "1" : "0.4";
      });

      // Блокируем/разблокируем все input и button, кроме кнопки "Включить моторы"
      document.querySelectorAll(
        '#POSITION input, #POSITION button, #SETTINGS input, #SETTINGS button'
      ).forEach(el => {
        if (el.id !== "enableMotorsButton") {
          el.disabled = !isEnabled;
        }
      });

      // Визуально затемняем вкладки
      const positionTab = document.getElementById("POSITION");
      const settingsTab = document.getElementById("SETTINGS");
      if (positionTab && settingsTab) {
        if (!isEnabled) {
          positionTab.classList.add("disabled");
          settingsTab.classList.add("disabled");
        } else {
          positionTab.classList.remove("disabled");
          settingsTab.classList.remove("disabled");
        }
      }
    });
}
  