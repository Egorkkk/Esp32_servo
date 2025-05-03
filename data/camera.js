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

const availableFpsOptions = [24, 25, 30, 50, 60];
const availableCodecOptions = ['BRaw:3_1', 'BRaw:5_1', 'BRaw:8_1', 'BRaw:12_1', 'BRaw:Q0', 'BRaw:Q1', 'BRaw:Q3', 'BRaw:Q5'];

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
//setupGlobalFormatControls();

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

function updateCameraTimecodeInDOM(ip, timecode) {
    const block = document.querySelector(`.camera-block[data-ip="${ip}"]`);
    if (!block) return;

    const span = block.querySelector('h3 span');
    if (!span) return;

    const state = allCameraStates[ip];
    const baseLabel = state.online ? 'Connected' : 'Not connected';
    const display = timecode ? `(${baseLabel} — ${timecode})` : `(${baseLabel})`;

    if (span.textContent !== display) {
        span.textContent = display;
    }
}

function pingAll() {
    cameraIPs.forEach(async (ip) => {
        const state = allCameraStates[ip];
        const nowOnline = await pingCamera(ip);
        state.online = nowOnline;

        const block = document.querySelector(`.camera-block[data-ip="${ip}"]`);
        if (!block) return;

        const header = block.querySelector('h3 span');
        if (header) {
            const tc = state.timecode ? ` — ${state.timecode}` : '';
            header.textContent = nowOnline ? `(Connected${tc})` : '(Not connected)';
            header.style.color = nowOnline ? 'green' : 'red';
        }

        block.classList.toggle('offline', !nowOnline);

        block.querySelectorAll('input, button').forEach(el => {
            if (el.type === 'range' || el.tagName === 'BUTTON') {
                el.disabled = !nowOnline;
            }
        });
    });
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

function updateCameraTimecodeInDOM(ip, timecode) {
    const block = document.querySelector(`.camera-block[data-ip="${ip}"]`);
    if (!block) return;

    const state = allCameraStates[ip];
    const header = block.querySelector('h3 span');
    if (header) {
        const tc = timecode ? ` — ${timecode}` : '';
        header.textContent = state.online ? `(Connected${tc})` : '(Not connected)';
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

const irisSteps = [2.0, 2.2, 2.5, 2.8, 3.2, 3.5, 4.0, 4.5, 5.0, 5.6, 6.3, 7.1, 8.0, 9.0, 10.0, 11.0, 13.0, 14.0, 16.0, 18.0, 20.0, 22.0];
const gainSteps = [-12, -9, -6, -3, 0, 3, 6, 9, 12];
const shutterSteps = [11, 22, 45, 50, 60, 90, 100, 120, 150, 180, 200, 240, 300, 360];
const whiteBalanceSteps = [2500, 2800, 3000, 3200, 3400, 3600, 4000, 4500, 4800, 5000, 5200, 5400, 5600, 6000, 6500, 7000, 7500, 8000];
const isoSteps = [100, 200, 400, 800, 1600, 3200, 6400, 12800];

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

        block.innerHTML = `
            <h3>Камера ${index + 1} <span style="color:${statusColor}; font-weight:normal; font-size:0.8em">(${statusText}${timecode})</span></h3>

            <div class="control-block">
                <label for="focus-${index}">Focus:</label><br>
                <input type="range" id="focus-${index}" min="0" max="100" value="${state.focus}" ${!state.online ? 'disabled' : ''}>
            </div>

            ${buildStepper('Iris', 'iris', irisSteps, index, state.iris, !state.online)}
            ${buildStepper('Gain', 'gain', gainSteps, index, state.gain, !state.online)}
            ${buildStepper('Shutter', 'shutter', shutterSteps, index, state.shutter, !state.online)}
            ${buildStepper('White Balance', 'whitebalance', whiteBalanceSteps, index, state.whiteBalance, !state.online)}
            ${buildStepper('ISO', 'iso', isoSteps, index, state.iso, !state.online)}

            <div class="control-block">
                <button onclick="startRecording(${index})" ${!state.online ? 'disabled' : ''}>Start Rec</button>
                <button onclick="stopRecording(${index})" ${!state.online ? 'disabled' : ''}>Stop Rec</button>
            </div>

            <hr>
        `;
        container.appendChild(block);
    });
}


function toggleControlMode() {
    controlAll = document.getElementById('controlAllCheckbox').checked;
}


function buildStepper(label, prefix, stepsArray, index, value, disabled) {
    return `
        <div class="control-block">
            <label for="${prefix}-${index}">${label}:</label><br>
            <button class="${prefix}-down" data-index="${index}" ${disabled ? 'disabled' : ''}>◀</button>
            <input type="text" id="${prefix}-${index}" value="${value}" readonly style="width:50px; text-align:center;">
            <button class="${prefix}-up" data-index="${index}" ${disabled ? 'disabled' : ''}>▶</button>
        </div>
    `;
}

function changeStep(stepsArray, inputId, direction, ip, paramName) {
    const input = document.getElementById(inputId);
    if (!input) return;

    let current = parseFloat(input.value);
    let index = stepsArray.indexOf(current);
    if (index === -1) index = 0;
    index += direction;
    if (index < 0) index = 0;
    if (index >= stepsArray.length) index = stepsArray.length - 1;
    const newValue = stepsArray[index];
    input.value = newValue;
    allCameraStates[ip][paramName] = newValue;
    // Optionally: send change to camera
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
    const ip = cameraIPs[index];
    const url = `http://${ip}/control/api/v1/transports/0/record`;
    try {
        const response = await fetch(url, { method: 'POST' });
        if (response.ok) {
            allCameraStates[ip].recording = true;
            rebuildCameraBlocks();
        } else {
            console.warn(`Ошибка запуска записи на ${ip}`);
        }
    } catch (err) {
        console.error(`Ошибка сети при запуске записи на ${ip}:`, err);
    }
}

async function stopRecording(index) {
    const ip = cameraIPs[index];
    const url = `http://${ip}/control/api/v1/transports/0/stop`;
    try {
        const response = await fetch(url, { method: 'POST' });
        if (response.ok) {
            allCameraStates[ip].recording = false;
            rebuildCameraBlocks();
        } else {
            console.warn(`Ошибка остановки записи на ${ip}`);
        }
    } catch (err) {
        console.error(`Ошибка сети при остановке записи на ${ip}:`, err);
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