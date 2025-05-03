// Массив IP-адресов камер
let cameraIPs = [
    '192.168.0.201',
    '192.168.0.202',
    '192.168.0.203'
];

let fourthCameraEnabled = true;
let pingIntervalId = null;
let timecodeIntervalId = null;
let localTimecodeIntervalId = null;
let controlAllCameras = false;

const availableFpsOptions = [24, 25, 30, 50, 60];
const availableCodecOptions = ['BRaw:3_1', 'BRaw:5_1', 'BRaw:8_1', 'BRaw:12_1', 'BRaw:Q0', 'BRaw:Q1', 'BRaw:Q3', 'BRaw:Q5'];

const irisSteps = [2.5, 2.8, 3.2, 3.5, 4.0, 4.5, 5.0, 5.6, 6.3, 7.1, 8.0, 9.0, 10.0, 11.0, 13.0, 14.0, 16.0, 18.0, 20.0, 22.0];
const gainSteps = [-12, -6, 0, 6, 12, 18, 24, 30, 36];
const shutterSteps = [11, 22, 45, 50, 60, 90, 100, 120, 150, 180, 200, 240, 300, 360];
const whiteBalanceSteps = [2500, 2800, 3000, 3200, 3400, 3600, 4000, 4500, 4800, 5000, 5200, 5400, 5600, 6000, 6500, 7000, 7500, 8000];
const isoSteps = [100, 200, 400, 800, 1600, 3200, 6400, 12800, 25600];

// Основная структура хранения состояния всех камер
const allCameraStates = {};

function initializeCameraStates() {
    cameraIPs.forEach(ip => {
        if (!allCameraStates[ip]) {
            allCameraStates[ip] = {
                ip: ip,
                online: false,
                recording: false,
                timecode: null,
                frameCount: 0,
                lastUpdateTime: null,
                fps: 25,
                codec: null,
                desync: false,
                focus: 50,
                iris: 2.8,
                gain: 0,
                shutter: 180,
                whiteBalance: 5600,
                iso: 400
            };
        }
    });
}

function setupGlobalFormatControls() {
    const fpsSelect = document.getElementById('fpsSelect');
    const codecSelect = document.getElementById('codecSelect');
    const applyButton = document.getElementById('applyFormatBtn');

    if (!fpsSelect || !codecSelect || !applyButton) return;

    applyButton.onclick = async () => {
        const fps = parseInt(fpsSelect.value);
        const codec = codecSelect.value;
        applyButton.disabled = true;
    
        if (pingIntervalId !== null) clearInterval(pingIntervalId);
        if (timecodeIntervalId !== null) clearInterval(timecodeIntervalId);
        if (localTimecodeIntervalId !== null) clearInterval(localTimecodeIntervalId);
    
        await updateAllCamerasFormatCombined(fps, codec);
    
        setTimeout(() => {
            applyButton.disabled = false;
            pingIntervalId = setInterval(pingAll, 1000);
            timecodeIntervalId = setInterval(fetchAllTimecodes, 5000);
            startLocalTimecodeTimer();
        }, 2000);
    };
    const startAllBtn = document.getElementById('startAllBtn');
    const stopAllBtn = document.getElementById('stopAllBtn');

    if (startAllBtn) {
        startAllBtn.onclick = startRecordingAll;
    }
    if (stopAllBtn) {
        stopAllBtn.onclick = stopRecordingAll;
    }
}

async function updateAllCamerasFormatCombined(frameRate, codec) {
    const formatBody = {
        codec,
        frameRate: frameRate.toString(),
        maxOffSpeedFrameRate: 60,
        minOffSpeedFrameRate: 5,
        offSpeedEnabled: false,
        offSpeedFrameRate: 60,
        recordResolution: {
            height: 2160,
            width: 3840
        },
        sensorResolution: {
            height: 2160,
            width: 3840
        }
    };

    for (const ip of cameraIPs) {
        const url = `http://${ip}/control/api/v1/system/format`;
        console.log(`📤 Отправка на ${ip}:`, JSON.stringify(formatBody, null, 2));
        try {
            const response = await fetch(url, {
                method: 'PUT',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(formatBody)
            });
            if (response.ok) {
                allCameraStates[ip].fps = frameRate;
                allCameraStates[ip].codec = codec;
                console.log(`Updated format on ${ip}`, formatBody);
            } else {
                console.warn(`Failed to update format on ${ip}`);
            }
        } catch (err) {
            console.error(`Error updating format on ${ip}:`, err);
        }
    }
}

initializeCameraStates();

function decodeBCDToFrameCount(bcd, fps) {
    function bcdToDec(b) {
        return ((b >> 4) * 10 + (b & 0x0F));
    }
    const hh = bcdToDec((bcd >> 24) & 0xFF);
    const mm = bcdToDec((bcd >> 16) & 0xFF);
    const ss = bcdToDec((bcd >> 8) & 0xFF);
    const ff = bcdToDec(bcd & 0xFF);
    return (((hh * 60 + mm) * 60 + ss) * fps + ff);
}

function frameCountToTimecode(fc, fps) {
    const totalSeconds = Math.floor(fc / fps);
    const ff = fc % fps;
    const ss = totalSeconds % 60;
    const mm = Math.floor(totalSeconds / 60) % 60;
    const hh = Math.floor(totalSeconds / 3600);
    return `${hh.toString().padStart(2, '0')}:${mm.toString().padStart(2, '0')}:${ss.toString().padStart(2, '0')}:${ff.toString().padStart(2, '0')}`;
}

function startLocalTimecodeTimer() {
    if (localTimecodeIntervalId !== null) clearInterval(localTimecodeIntervalId);
    localTimecodeIntervalId = setInterval(() => {
        const now = performance.now();
        cameraIPs.forEach(ip => {
            const state = allCameraStates[ip];
            if (!state.online || state.lastUpdateTime === null) return;

            const elapsed = (now - state.lastUpdateTime) / 1000;
            const estimatedFrames = Math.floor(state.frameCount + elapsed * state.fps);
            const displayTimecode = frameCountToTimecode(estimatedFrames, state.fps);
            updateCameraTimecodeInDOM(ip, displayTimecode);
        });
    }, 1000 / 25); // обновление 25 раз в секунду
}
/*
async function pingAll() {
    cameraIPs.forEach(async ip => {
        const url = `http://${ip}/control/api/v1/system`;
        try {
            const response = await fetch(url);
            allCameraStates[ip].online = response.status === 204;
        } catch (err) {
            allCameraStates[ip].online = false;
        }

        // ⏺️ Проверка состояния записи
        if (allCameraStates[ip].online) {
            try {
                const recResponse = await fetch(`http://${ip}/control/api/v1/transports/0/record`);
                if (recResponse.ok) {
                    const data = await recResponse.json();
                    allCameraStates[ip].recording = data.recording === true;
                } else {
                    allCameraStates[ip].recording = false;
                }
            } catch (err) {
                allCameraStates[ip].recording = false;
            }
        } else {
            allCameraStates[ip].recording = false;
        }
    });

    rebuildCameraBlocks();
}
    */

async function pingAll() {
    for (const ip of cameraIPs) {
        const state = allCameraStates[ip];
        const block = document.querySelector(`.camera-block[data-ip="${ip}"]`);
        if (!block) continue;

        // === Проверка состояния подключения ===
        try {
            const systemResp = await fetch(`http://${ip}/control/api/v1/system`);
            const online = systemResp.status === 204;

            if (state.online !== online) {
                state.online = online;
                block.classList.toggle('offline', !online);

                // Обновляем все input и button
                const controls = block.querySelectorAll('input, button');
                controls.forEach(el => {
                    el.disabled = !online;
                });
            }
        } catch {
            if (state.online) {
                state.online = false;
                block.classList.add('offline');

                // Отключаем элементы управления
                const controls = block.querySelectorAll('input, button');
                controls.forEach(el => {
                    el.disabled = true;
                });
            }
        }

        // === Проверка состояния записи ===
        if (state.online) {
            try {
                const recResp = await fetch(`http://${ip}/control/api/v1/transports/0/record`);
                if (recResp.ok) {
                    const data = await recResp.json();
                    const isRecording = data.recording === true;
                    if (state.recording !== isRecording) {
                        state.recording = isRecording;
                        block.classList.toggle('recording', isRecording);
                    }
                }
            } catch {
                // Не меняем state.recording при ошибке
            }
        } else {
            if (state.recording) {
                state.recording = false;
                block.classList.remove('recording');
            }
        }

        // === Обновление текста статуса (Connected / Not connected) ===
        const statusSpan = block.querySelector('h3 span');
        if (statusSpan) {
            const baseStatus = state.online ? 'Connected' : 'Not connected';
            statusSpan.textContent = `(${baseStatus})`;
            statusSpan.style.color = state.online ? 'green' : 'red';
        }
    }
}

function connectAllCameras() {
    if (pingIntervalId !== null) clearInterval(pingIntervalId);
    if (timecodeIntervalId !== null) clearInterval(timecodeIntervalId);
    if (localTimecodeIntervalId !== null) clearInterval(localTimecodeIntervalId);

    pingAll();
    fetchAllTimecodes();

    pingIntervalId = setInterval(pingAll, 1000);
    timecodeIntervalId = setInterval(fetchAllTimecodes, 5000);
    startLocalTimecodeTimer();
}

function fetchAllTimecodes() {
    cameraIPs.forEach(async ip => {
        if (!allCameraStates[ip].online) return;
        const url = `http://${ip}/control/api/v1/transports/0/timecode`;
        try {
            const response = await fetch(url);
            if (response.ok) {
                const data = await response.json();
                const bcd = data.timecode;
                const state = allCameraStates[ip];
                state.frameCount = decodeBCDToFrameCount(bcd, state.fps);
                state.lastUpdateTime = performance.now();
                state.timecode = frameCountToTimecode(state.frameCount, state.fps);
                updateCameraTimecodeInDOM(ip, state.timecode);
            } else {
                allCameraStates[ip].timecode = null;
                updateCameraTimecodeInDOM(ip, null);
            }
        } catch (err) {
            allCameraStates[ip].timecode = null;
            updateCameraTimecodeInDOM(ip, null);
        }
    });
}

function updateCameraTimecodeInDOM(ip, displayTimecode) {
    const elementId = `timecode-${ip}`;
    //console.log(`[updateTimecode] IP: ${ip}, Element ID: ${elementId}, Value: ${displayTimecode}`);

    const tcElement = document.getElementById(elementId);
    if (tcElement) {
        tcElement.innerHTML = `<strong>${displayTimecode || '__:__:__:__'}</strong>`;
        //console.log(`[updateTimecode] ✅ Обновлён DOM элемент #${elementId}`);
    } else {
        //console.warn(`[updateTimecode] ❌ Элемент #${elementId} не найден`);
    }
}
function decodeBCDTimecode(bcd) {
    function bcdToDec(b) {
        return ((b >> 4) * 10 + (b & 0x0F));
    }
    const hh = bcdToDec((bcd >> 24) & 0xFF);
    const mm = bcdToDec((bcd >> 16) & 0xFF);
    const ss = bcdToDec((bcd >> 8) & 0xFF);
    const ff = bcdToDec(bcd & 0xFF);
    return `${hh.toString().padStart(2, '0')}:${mm.toString().padStart(2, '0')}:${ss.toString().padStart(2, '0')}:${ff.toString().padStart(2, '0')}`;
}

function toggleFourthCamera(enabled) {
    fourthCameraEnabled = enabled;
    const ip = '192.168.0.204';

    if (enabled) {
        if (!cameraIPs.includes(ip)) {
            cameraIPs.push(ip);
        }
        if (!allCameraStates[ip]) {
            allCameraStates[ip] = {
                ip: ip,
                online: false,
                recording: false,
                timecode: null,
                desync: false,
                focus: 50,
                iris: 2.8,
                gain: 0,
                shutter: 180,
                whiteBalance: 5600,
                iso: 400
            };
        }
    } else {
        cameraIPs = cameraIPs.filter(camIp => camIp !== ip);
    }

    rebuildCameraBlocks();
}

function attachStepperHandlers() {
    const ups = document.querySelectorAll('.stepper-up');
    const downs = document.querySelectorAll('.stepper-down');

    console.log(`🔍 Найдено ${ups.length} кнопок up, ${downs.length} кнопок down`);

    ups.forEach(btn => {
        btn.addEventListener('click', () => {
            const param = btn.dataset.param;
            const index = parseInt(btn.dataset.index);
            console.log(`▶ ${param}[${index}] +1`);
            changeStep(param, index, 1);
        });
    });

    downs.forEach(btn => {
        btn.addEventListener('click', () => {
            const param = btn.dataset.param;
            const index = parseInt(btn.dataset.index);
            console.log(`◀ ${param}[${index}] -1`);
            changeStep(param, index, -1);
        });
    });
}

function rebuildCameraBlocks() {
    const container = document.getElementById('cameraControlsContainer');
    container.innerHTML = '';

    cameraIPs.forEach((ip, index) => {
        const state = allCameraStates[ip];

        const block = document.createElement('div');
        block.className = 'camera-block';
        block.dataset.ip = ip;
        if (!state.online) block.classList.add('offline');
        if (state.recording) block.classList.add('recording');
        if (state.desync) block.classList.add('desync');

        const statusText = state.online ? 'Connected' : 'Not connected';
        const statusColor = state.online ? 'green' : 'red';
        const timecode = state.timecode ? ` — ${state.timecode}` : '';
        const timecodeDisplay = state.timecode || '__:__:__:__';

        block.innerHTML = `
        <h3>
            Камера ${index + 1} 
            <span style="color:${statusColor}; font-weight:normal; font-size:0.8em">(${statusText})</span>
        </h3>
        <div class="camera-timecode" id="timecode-${ip}"><strong>${timecodeDisplay}</strong></div>

            <div class="control-block">
                <label for="focus-${index}">Focus:</label><br>
                <input type="range" id="focus-${index}" min="0" max="100" value="${state.focus}" ${!state.online ? 'disabled' : ''}>
            </div>

            ${buildStepper('Iris', 'iris', irisSteps, index, state.iris, !state.online)}
            ${buildStepper('Gain', 'gain', gainSteps, index, state.gain, !state.online)}
            ${buildStepper('ISO', 'iso', isoSteps, index, state.iso, !state.online)}
            ${buildStepper('Shutter', 'shutter', shutterSteps, index, state.shutter, !state.online)}
            ${buildStepper('White Balance', 'whitebalance', whiteBalanceSteps, index, state.whiteBalance, !state.online)}
            

            <div class="control-block">
                <button onclick="startRecording(${index})" ${!state.online ? 'disabled' : ''}>Start Rec</button>
                <button onclick="stopRecording(${index})" ${!state.online ? 'disabled' : ''}>Stop Rec</button>
            </div>

            <hr>
        `;
        container.appendChild(block);

        const focusInput = block.querySelector(`#focus-${index}`);
        if (focusInput) {
            focusInput.addEventListener('input', () => {
                const newValue = parseInt(focusInput.value);
        
                const targets = controlAllCameras
                    ? cameraIPs.filter(ip => allCameraStates[ip]?.online)
                    : [ip];
        
                targets.forEach(targetIp => {
                    allCameraStates[targetIp].focus = newValue;
                    updateFocus(targetIp, newValue);
        
                    const targetIndex = cameraIPs.indexOf(targetIp);
                    const otherInput = document.getElementById(`focus-${targetIndex}`);
                    if (otherInput && otherInput !== focusInput) {
                        otherInput.value = newValue;
                    }
                });
            });
        }
    });
    attachStepperHandlers(); // ← в самом конце rebuildCameraBlocks
}

function toggleControlMode() {
    controlAll = document.getElementById('controlAllCheckbox').checked;
}

function getStepArrayForParam(paramName) {
    switch (paramName) {
        case 'iris': return irisSteps;
        case 'gain': return gainSteps;
        case 'shutter': return shutterSteps;
        case 'whitebalance': return whiteBalanceSteps;
        case 'iso': return isoSteps;
        default: return [];
    }
}

function buildStepper(label, paramName, steps, index, currentValue, disabled) {
    const value = currentValue ?? steps[0];

    return `
    <div class="stepper-control">
      <label>${label}:</label>
      <button class="stepper-down ${paramName}-down stepper-btn" data-param="${paramName}" data-index="${index}" ${disabled ? 'disabled' : ''}>◀</button>
      <input type="text" id="${paramName}-${index}" value="${value}" readonly style="width:50px; text-align:center;" ${disabled ? 'disabled' : ''}>
      <button class="stepper-up ${paramName}-up stepper-btn" data-param="${paramName}" data-index="${index}" ${disabled ? 'disabled' : ''}>▶</button>
    </div>
  `;
}

function changeStep(paramName, index, direction) {
    const targets = controlAllCameras
        ? cameraIPs.filter(ip => allCameraStates[ip]?.online)
        : [cameraIPs[index]];

    targets.forEach(ip => {
        const state = allCameraStates[ip];
        const steps = getStepArrayForParam(paramName);
        const currentValue = state[paramName];
        const currentIndex = steps.indexOf(currentValue);

        let newIndex = currentIndex + direction;
        newIndex = Math.max(0, Math.min(steps.length - 1, newIndex));
        const newValue = steps[newIndex];

        state[paramName] = newValue;

        // Обновить поле в интерфейсе
        const uiIndex = cameraIPs.indexOf(ip); // нужен для поиска правильного поля
        const input = document.getElementById(`${paramName}-${uiIndex}`);
        if (input) {
            input.value = newValue;
        }

        // Отправить на камеру
        if (paramName === 'iris') {
            updateIris(ip, newValue);
        } else if (paramName === 'gain') {
            updateGain(ip, newValue);
        } else if (paramName === 'shutter') {
            updateShutter(ip, newValue);
        } else if (paramName === 'whitebalance') {
            updateWhiteBalance(ip, newValue);
        } else if (paramName === 'iso') {
            updateISO(ip, newValue);
        }
    });
}

async function pingCamera(ip) {
    const url = `http://${ip}/control/api/v1/system`;

    try {
        const response = await fetch(url, { method: 'GET' });
        const status = response.status;
        console.log(`Ответ от ${ip}: ${status}`);
        return status === 200 || status === 204;
    } catch (err) {
        console.error(`Ошибка соединения с ${ip}:`, err);
        return false;
    }
}

async function startRecording(index) {
    const selectedIPs = getTargetIPs(index);
    for (const ip of selectedIPs) {
        const url = `http://${ip}/control/api/v1/transports/0/record`;
        try {
            const response = await fetch(url, {
                method: 'PUT',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ recording: true })
            });
            if (response.ok) {
                allCameraStates[ip].recording = true;
                rebuildCameraBlocks();
            } else {
                console.warn(`Ошибка запуска записи на ${ip}: статус ${response.status}`);
            }
        } catch (err) {
            console.error(`Ошибка соединения при запуске записи на ${ip}:`, err);
        }
    }
}

async function stopRecording(index) {
    const selectedIPs = getTargetIPs(index);
    for (const ip of selectedIPs) {
        const url = `http://${ip}/control/api/v1/transports/0/record`;
        try {
            const response = await fetch(url, {
                method: 'PUT',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ recording: false })
            });
            if (response.ok) {
                allCameraStates[ip].recording = false;
                rebuildCameraBlocks();
            } else {
                console.warn(`Ошибка остановки записи на ${ip}: статус ${response.status}`);
            }
        } catch (err) {
            console.error(`Ошибка соединения при остановке записи на ${ip}:`, err);
        }
    }
}

async function updateRecordingState(ip, state) {
    const url = `http://${ip}/control/api/v1/transports/0/record`;
    try {
        const response = await fetch(url, {
            method: 'PUT',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ recording: state })
        });
        if (response.ok) {
            allCameraStates[ip].recording = state;
            rebuildCameraBlocks();
        } else {
            console.warn(`Не удалось обновить запись на ${ip}: статус ${response.status}`);
        }
    } catch (err) {
        console.error(`Ошибка обновления записи на ${ip}:`, err);
    }
}

function startRecordingAll() {
    cameraIPs.forEach(ip => {
        if (allCameraStates[ip]?.online) {
            updateRecordingState(ip, true);
        }
    });
}

function stopRecordingAll() {
    cameraIPs.forEach(ip => {
        if (allCameraStates[ip]?.online) {
            updateRecordingState(ip, false);
        }
    });
}

async function updateIris(ip, value) {
    const url = `http://${ip}/control/api/v1/lens/iris`;
    try {
        const response = await fetch(url, {
            method: 'PUT', // <-- ВАЖНО
            headers: {
                'Content-Type': 'application/json'
            },
            body: JSON.stringify({ apertureStop: value })
        });
        if (!response.ok) {
            console.warn(`Не удалось обновить iris на ${ip}: статус ${response.status}`);
        }
    } catch (err) {
        console.error(`Ошибка при обновлении iris на ${ip}:`, err);
    }
}

function shutterAngleToSpeedValue(angle, fps) {
    const allowedSpeeds = [25, 30, 50, 60, 100, 125, 200, 250, 500, 1000, 2000];

    // Вычисляем фактическую выдержку в секундах
    const targetShutterTime = (angle / 360) / fps;

    // Фильтруем те выдержки, которые допустимы (N >= fps)
    const validSpeeds = allowedSpeeds.filter(n => n >= fps);

    // Находим ближайшее значение
    let bestMatch = validSpeeds[0];
    let smallestDiff = Math.abs((1 / bestMatch) - targetShutterTime);

    for (let i = 1; i < validSpeeds.length; i++) {
        const n = validSpeeds[i];
        const diff = Math.abs((1 / n) - targetShutterTime);
        if (diff < smallestDiff) {
            smallestDiff = diff;
            bestMatch = n;
        }
    }

    return bestMatch; // Например, 50 означает 1/50
}

async function updateShutter(ip, angle) {
    const fps = allCameraStates[ip].fps || 25; // fallback
    const speedValue = shutterAngleToSpeedValue(angle, fps); // например, 50
    const url = `http://${ip}/control/api/v1/video/shutter`;

    console.log(`[${ip}] shutter angle ${angle}° → 1/${speedValue} sec (fps: ${fps})`);

    try {
        const response = await fetch(url, {
            method: 'PUT',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ shutterSpeed: speedValue })
        });
        if (!response.ok) {
            console.warn(`Не удалось обновить shutter на ${ip}: статус ${response.status}`);
        }
    } catch (err) {
        console.error(`Ошибка при обновлении shutter на ${ip}:`, err);
    }
}

async function updateGain(ip, value) {
    const url = `http://${ip}/control/api/v1/video/gain`;

    try {
        const response = await fetch(url, {
            method: 'PUT',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ gain: value })
        });

        if (response.ok) {
            allCameraStates[ip].gain = value;
            const iso = gainToIso(value);
            allCameraStates[ip].iso = iso;

            // Обновим поле ISO в интерфейсе
            const index = cameraIPs.indexOf(ip);
            const isoInput = document.getElementById(`iso-${index}`);
            if (isoInput) {
                isoInput.value = iso;
            }

            console.log(`[${ip}] Gain set to ${value}, ISO updated to ${iso}`);
        } else {
            console.warn(`Не удалось обновить gain на ${ip}: статус ${response.status}`);
        }
    } catch (err) {
        console.error(`Ошибка при обновлении gain на ${ip}:`, err);
    }
}

function isoToGain(iso) {
    const map = {
        100: -12, 200: -6, 400: 0, 800: 6,
        1600: 12, 3200: 18, 6400: 24,
        12800: 30, 25600: 36
    };
    return map[iso];
}

function gainToIso(gain) {
    const reverseMap = {
        '-12': 100, '-6': 200, '0': 400, '6': 800,
        '12': 1600, '18': 3200, '24': 6400,
        '30': 12800, '36': 25600
    };
    return reverseMap[gain] || 400;
}

async function updateISO(ip, isoValue) {
    const gainValue = isoToGain(isoValue);
    if (gainValue === undefined) return;

    const url = `http://${ip}/control/api/v1/video/gain`;

    console.log(`[${ip}] ISO ${isoValue} → gain ${gainValue}`);

    try {
        const response = await fetch(url, {
            method: 'PUT',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ gain: gainValue })
        });

        if (response.ok) {
            allCameraStates[ip].iso = isoValue;
            allCameraStates[ip].gain = gainValue;

            const index = cameraIPs.indexOf(ip);

            // Обновить поле ISO
            const isoInput = document.getElementById(`iso-${index}`);
            if (isoInput) {
                isoInput.value = isoValue;
            }

            // Обновить поле Gain
            const gainInput = document.getElementById(`gain-${index}`);
            if (gainInput) {
                gainInput.value = gainValue;
            }

        } else {
            console.warn(`Не удалось обновить ISO на ${ip}`);
        }
    } catch (err) {
        console.error(`Ошибка при обновлении ISO на ${ip}:`, err);
    }
}

async function updateWhiteBalance(ip, value) {
    const url = `http://${ip}/control/api/v1/video/whiteBalance`;
    try {
        const response = await fetch(url, {
            method: 'PUT',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ whiteBalance: value })
        });
        if (!response.ok) {
            console.warn(`Не удалось обновить whiteBalance на ${ip}: статус ${response.status}`);
        }
    } catch (err) {
        console.error(`Ошибка при обновлении whiteBalance на ${ip}:`, err);
    }
}

async function updateFocus(ip, value) {
    const normalized = value / 100;
    const url = `http://${ip}/control/api/v1/lens/focus`;

    //console.log(`[${ip}] Focus slider: ${value} → normalized: ${normalized.toFixed(4)}`);

    try {
        const response = await fetch(url, {
            method: 'PUT',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ normalised: normalized })
        });
        if (!response.ok) {
            console.warn(`Не удалось обновить фокус на ${ip}: статус ${response.status}`);
        }
    } catch (err) {
        console.error(`Ошибка при обновлении фокуса на ${ip}:`, err);
    }
}

function toggleControlMode() {
    const checkbox = document.getElementById('controlAllCheckbox');
    controlAllCameras = checkbox.checked;
}

function getTargetIPs(index) {
    if (controlAllCameras) {
        return cameraIPs.filter(ip => allCameraStates[ip]?.online);
    } else {
        const ip = cameraIPs[index];
        return allCameraStates[ip]?.online ? [ip] : [];
    }
}

function openTab(evt, tabName) {
    // Declare all variables
    var i, tabcontent, tablinks;
  
    // Get all elements with class="tabcontent" and hide them
    tabcontent = document.getElementsByClassName("tabcontent");
    for (i = 0; i < tabcontent.length; i++) {
      tabcontent[i].style.display = "none";
    }
  
    // Get all elements with class="tablinks" and remove the class "active"
    tablinks = document.getElementsByClassName("tablinks");
    for (i = 0; i < tablinks.length; i++) {
      tablinks[i].className = tablinks[i].className.replace(" active", "");
    }
  
    // Show the current tab, and add an "active" class to the button that opened the tab
    document.getElementById(tabName).style.display = "block";
    evt.currentTarget.className += " active";
}