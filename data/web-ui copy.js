/*      Blackmagic Camera Control WebUI
        WebUI Script functions
        (c) Dylan Speiser 2024              
        github.com/DylanSpeiser
*/


/* Global variables */
var cameras = [];       // Array to store all of the camera objects
var ci = 0;             // Index into this array for the currently selected camera.
// cameras[ci] is used to reference the currently selected camera object

var WBMode = 0;         // 0: balance, 1: tint

var defaultControlsHTML;

var unsavedChanges = [];

var isCamConnected = [];


function sendCommandToallCameras(commandFunc) {
    if (controlAll) {
        allCameras.forEach(camera => {
            if (camera !== null) commandFunc(camera);
        });
    } else {
        // При передаче через обработчик всегда передаём индекс камеры
        // Поэтому здесь ничего менять не надо
    }
}

function onFocusChange(index, value) {
    if (controlAll) {
        sendCommandToallCameras(camera => camera.setFocus(parseInt(value)));
    } else {
        if (allCameras[index] !== null) {
            allCameras[index].setFocus(parseInt(value));
        }
    }
}

function onIrisChange(index, value) {
    if (controlAll) {
        sendCommandToallCameras(camera => camera.setIris(parseInt(value)));
    } else {
        if (allCameras[index] !== null) {
            allCameras[index].setIris(parseInt(value));
        }
    }
}

// Аналогично для Gain, Shutter, White Balance...

function startRecording(index) {
    if (controlAll) {
        sendCommandToallCameras(camera => camera.startRecording());
    } else {
        if (allCameras[index] !== null) {
            allCameras[index].startRecording();
        }
    }
}

function stopRecording(index) {
    if (controlAll) {
        sendCommandToallCameras(camera => camera.stopRecording());
    } else {
        if (allCameras[index] !== null) {
            allCameras[index].stopRecording();
        }
    }
}





// Set everything up
function bodyOnLoad() {
    defaultControlsHTML = document.getElementById("allCamerasContainer").innerHTML;
}

// Checks the hostname, if it replies successfully then a new BMCamera object
//  is made and gets put in the array at ind
function initCamera(cam_num, ipInput) {
    // Get hostname from Hostname text field
    let hostname = document.getElementById(ipInput).value;
    let security = 0;// document.getElementById("secureCheckbox").checked;

    try {
        // Check if the hostname is valid
        let response = sendRequest("GET", (security ? "https://" : "http://")+hostname+"/control/api/v1/system","");

        if (response.status < 300) {
            // Success, make a new camera, get all relevant info, and populate the UI
            cameras[cam_num] = new BMCamera(hostname, security);

            cameras[cam_num].updateUI = updateUIAll;

            cameras[cam_num].active = true;

            document.getElementById("connectionErrorSpan").innerHTML = "Connected";
            document.getElementById("connectionErrorSpan").setAttribute("style","color: #6e6e6e;");
            document.getElementById("buttonLEFT").innerHTML = "Reconnect";
        } else {
            // Something has gone wrong, tell the user
            document.getElementById("connectionErrorSpan").innerHTML = response.statusText;
        }
    } catch (error) {
        // Something has gone wrong, tell the user
        document.getElementById("connectionErrorSpan").title = error;
        document.getElementById("connectionErrorSpan").innerHTML = "Error "+error.code+": "+error.name+" (Your hostname is probably incorrect, hover for more details)";
    }

    unsavedChanges = unsavedChanges.filter((e) => {return e !== "Hostname"});
}

// =============================== UI Updater ==================================
// =============================================================================

function updateUIAll() {
    // ========== Camera Name ==========

    document.getElementById("cameraName").innerHTML = cameras[ci].name;

    // ========== Hostname ==========

    if (!unsavedChanges.includes("Hostname")) {
        document.getElementById("hostnameInput").value = cameras[ci].hostname;
    }

    // ========== Format ==========

    document.getElementById("formatCodec").innerHTML = cameras[ci].propertyData['/system/format']?.codec.toUpperCase().replace(":"," ").replace("_",":");
    
    let resObj = cameras[ci].propertyData['/system/format']?.recordResolution;
    document.getElementById("formatResolution").innerHTML = resObj?.width + "x" + resObj?.height;
    document.getElementById("formatFPS").innerHTML = cameras[ci].propertyData['/system/format']?.frameRate+" fps";

    // ========== Recording State ==========

    if (cameras[ci].propertyData['/transports/0/record']?.recording) {
        document.getElementById("cameraControlHeadContainer").classList.add("liveCam");
        document.getElementById("cameraControlExpandedHeadContainer").classList.add("liveCam");
    } else {
        document.getElementById("cameraControlHeadContainer").classList.remove("liveCam");
        document.getElementById("cameraControlExpandedHeadContainer").classList.remove("liveCam");
    }
/*
    // ========== Playback Loop State ==========
    let loopState = cameras[ci].propertyData['/transports/0/playback']?.loop;
    let singleClipState = cameras[ci].propertyData['/transports/0/playback']?.singleClip;

    let loopButton = document.getElementById("loopButton");
    let singleClipButton = document.getElementById("singleClipButton");

    if (loopState) {
        loopButton.classList.add("activated");
    } else {
        loopButton.classList.remove("activated");
    }

    if (singleClipState) {
        singleClipButton.classList.add("activated");
    } else {
        singleClipButton.classList.remove("activated");
    }
*/
    // ========== Timecode ==========

    document.getElementById("timecodeLabel").innerHTML = parseTimecode(cameras[ci].propertyData['/transports/0/timecode']?.timecode);

    // ========== Presets Dropdown ==========
/*
    if (!unsavedChanges.includes("presets")) {
    
        var presetsList = document.getElementById("presetsDropDown");

        presetsList.innerHTML = "";

        cameras[ci].propertyData['/presets']?.presets.forEach((presetItem) => {
            let presetName = presetItem.split('.', 1);

            let textNode = document.createTextNode(presetName);
            let optionNode = document.createElement("option");
            optionNode.setAttribute("name", "presetOption"+presetName);
            optionNode.appendChild(textNode);
            document.getElementById("presetsDropDown").appendChild(optionNode);
        });

    // ========== Active Preset ==========

        var presetsList = document.getElementById("presetsDropDown");

        presetsList.childNodes.forEach((child) => {
            if (child.nodeName == 'OPTION' && (child.value+".cset") == cameras[ci].propertyData['/presets/active']?.preset) {
                child.selected=true
            } else {
                child.selected=false
            }
        })

    }
*/
    // ========== Iris ==========

    document.getElementById("irisRange").value = cameras[ci].propertyData['/lens/iris']?.normalised;
    document.getElementById("apertureStopsLabel").innerHTML = cameras[ci].propertyData['/lens/iris']?.apertureStop.toFixed(1);

    // ========== Zoom ==========
/*
    document.getElementById("zoomRange").value = cameras[ci].propertyData['/lens/zoom']?.normalised;
    document.getElementById("zoomMMLabel").innerHTML = cameras[ci].propertyData['/lens/zoom']?.focalLength +"mm";
*/
    // ========== Focus ==========

    document.getElementById("focusRange").value = cameras[ci].propertyData['/lens/focus']?.normalised;


    // ========== GAIN ==========

    if (!unsavedChanges.includes("Gain")) {
        let gainString = "";
        let gainInt = cameras[ci].propertyData['/video/gain']?.gain

        if (gainInt >= 0) {
            gainString = "+"+gainInt+"db"
        } else {
            gainString = gainInt+"db"
        }

        document.getElementById("gainSpan").innerHTML = gainString;
        document.getElementById("isoSpan").innerHTML = cameras[ci].propertyData['/video/iso']?.iso;
    }

    // ========== WHITE BALANCE ===========

    if (!unsavedChanges.includes("WB")) {
        document.getElementById("whiteBalanceSpan").innerHTML = cameras[ci].propertyData['/video/whiteBalance']?.whiteBalance+"K";
    }
    

    // ============ Shutter =====================

    if (!unsavedChanges.includes("Shutter")) {
        let shutterString = "SS"
        let shutterObj = cameras[ci].propertyData['/video/shutter'];

        if (shutterObj?.shutterSpeed) {
            shutterString = "1/"+shutterObj.shutterSpeed
        } else if (shutterObj?.shutterAngle) {
            var shangleString = (shutterObj.shutterAngle / 100).toFixed(1).toString()
            if (shangleString.indexOf(".0") > 0) {
                shutterString = parseFloat(shangleString).toFixed(0)+"°";
            } else {
                shutterString = shangleString+"°";
            }
        }

        document.getElementById("shutterSpan").innerHTML = shutterString;
    }


 
    // ============ Footer Links ===============
    //document.getElementById("documentationLink").href = (cameras[ci].useHTTPS ? "https://" : "http://")+cameras[ci].hostname+"/control/documentation.html";
    //document.getElementById("mediaManagerLink").href = (cameras[ci].useHTTPS ? "https://" : "http://")+cameras[ci].hostname;
}


// ==============================================================================

// Called when the user changes tabs to a different camera
function switchCamera(index) {
    if (cameras[ci]) {
        cameras[ci].active = false;
    }

    ci = index;

    // Reset the Controls
    document.getElementById("allCamerasContainer").innerHTML = defaultControlsHTML;

    // Update the UI

    for (var i = 0; i < 4; i++) {
        if (i == ci) {
            document.getElementsByClassName("cameraSwitchLabel")[i].classList.add("selectedCam");
        } else {
            document.getElementsByClassName("cameraSwitchLabel")[i].classList.remove("selectedCam");
        }
    }

    document.getElementById("cameraNumberLabel").innerHTML = "CAM"+(ci+1);
    document.getElementById("cameraName").innerHTML = "CAMERA NAME";

    if (cameras[ci]) {
        cameras[ci].active = true;
    }
}

// For not-yet-implemented Color Correction UI
function setCCMode(mode) {
    if (mode == 0) {
        // Lift

    } else if (mode == 1) {
        // Gamma

    } else {
        // Gain

    }

    for (var i = 0; i < 3; i++) {
        if (i == mode) {
            document.getElementsByClassName("ccTabLabel")[i].classList.add("selectedTab");
        } else {
            document.getElementsByClassName("ccTabLabel")[i].classList.remove("selectedTab");
        }
    }
}

// Allows for changing WB/Tint displayed in the UI
function swapWBMode() {
    if (WBMode == 0) {
        // Balance
        document.getElementById("WBLabel").innerHTML = "TINT";
        document.getElementById("WBValueContainer").classList.add("dNone");
        document.getElementById("WBTintValueContainer").classList.remove("dNone");
        
        WBMode = 1;
    } else {
        //Tint
        document.getElementById("WBLabel").innerHTML = "BALANCE";
        document.getElementById("WBValueContainer").classList.remove("dNone");
        document.getElementById("WBTintValueContainer").classList.add("dNone");

        WBMode = 0;
    }
}

// Triggered by the button by those text boxes. Reads the info from the inputs and sends it to the camera.
function manualAPICall() {
    const requestRadioGET = document.getElementById("requestTypeGET");

    const requestEndpointText = document.getElementById("manualRequestEndpointLabel").value;
    let requestData = "";

    try {
        requestData = JSON.parse(document.getElementById("manualRequestBodyLabel").value);
    } catch (err) {
        document.getElementById("manualRequestResponseP").innerHTML = err;
    }

    const requestMethod = (requestRadioGET.checked ? "GET" : "PUT");
    const requestURL = cameras[ci].APIAddress+requestEndpointText;

    let response = sendRequest(requestMethod,requestURL,requestData);
    
    document.getElementById("manualRequestResponseP").innerHTML = JSON.stringify(response);
}

/*  Control Calling Functions   */
/*    Makes the HTML cleaner.   */

function decreaseND() {
    cameras[ci].PUTdata("/video/ndFilter",{stop: cameras[ci].propertyData['/video/ndFilter'].stop-2});
}

function increaseND() {
    cameras[ci].PUTdata("/video/ndFilter",{stop: cameras[ci].propertyData['/video/ndFilter'].stop+2});
}

function decreaseGain(cam_num) {
    cameras[cam_num].PUTdata("/video/gain",{gain: cameras[cam_num].propertyData['/video/gain'].gain-2});
}

function increaseGain(cam_num) {
    cameras[cam_num].PUTdata("/video/gain",{gain: cameras[cam_num].propertyData['/video/gain'].gain+2});
}





function decreaseShutter() {
    let cam = cameras[ci];

    if ('shutterSpeed' in cam.propertyData['/video/shutter']) {
        cam.PUTdata("/video/shutter", {"shutterSpeed": cam.propertyData['/video/shutter'].shutterSpeed+10});
    } else {
        cam.PUTdata("/video/shutter", {"shutterAngle": cam.propertyData['/video/shutter'].shutterAngle-1000});
    }
}

function increaseShutter() {
    let cam = cameras[ci];

    if ('shutterSpeed' in cam.propertyData['/video/shutter']) {
        cam.PUTdata("/video/shutter", {"shutterSpeed": cam.propertyData['/video/shutter'].shutterSpeed-10});
    } else {
        cam.PUTdata("/video/shutter", {"shutterAngle": cam.propertyData['/video/shutter'].shutterAngle+1000});
    }
}

function handleShutterInput() {
    let inputString = document.getElementById("shutterSpan").innerHTML;

    if (event.key === 'Enter') {
        let cam = cameras[ci];

        if ('shutterSpeed' in cam.propertyData['/video/shutter']) {
            if (inputString.indexOf("1/") >= 0) {
                cam.PUTdata("/video/shutter", {"shutterSpeed" :parseInt(inputString.substring(2))});
            } else {
                cam.PUTdata("/video/shutter", {"shutterSpeed" :parseInt(inputString)});
            }
            
        } else {
            cam.PUTdata("/video/shutter", {"shutterAngle": parseInt(parseFloat(inputString)*100)});
        }
        
        unsavedChanges = unsavedChanges.filter((e) => {return e !== "Shutter"});
    } else {
        unsavedChanges.push('Shutter');
    }
}

function decreaseWhiteBalance() {
    cameras[ci].PUTdata("/video/whiteBalance", {whiteBalance: cameras[ci].propertyData['/video/whiteBalance'].whiteBalance-50});
}

function increaseWhiteBalance() {
    cameras[ci].PUTdata("/video/whiteBalance", {whiteBalance: cameras[ci].propertyData['/video/whiteBalance'].whiteBalance+50});
}

function decreaseWhiteBalanceTint() {
    cameras[ci].PUTdata("/video/whiteBalanceTint", {whiteBalanceTint: cameras[ci].propertyData['/video/whiteBalanceTint'].whiteBalanceTint-1});
}

function increaseWhiteBalanceTint() {
    cameras[ci].PUTdata("/video/whiteBalanceTint", {whiteBalanceTint: cameras[ci].propertyData['/video/whiteBalanceTint'].whiteBalanceTint+1});
}

function presetInputHandler() {
    let selectedPreset = document.getElementById("presetsDropDown").value;

    cameras[ci].PUTdata("/presets/active", {preset: selectedPreset+".cset"});

    unsavedChanges = unsavedChanges.filter((e) => {return e !== "presets"});
}

function hostnameInputHandler(cam_num) {
    let newHostname = document.getElementById("hostnameInput").value;

    if (event.key === 'Enter') {
        event.preventDefault;
        unsavedChanges = unsavedChanges.filter((e) => {return e !== "Hostname"});
        initCamera(cam_num);
    } else {
        unsavedChanges.push('Hostname');
    }
}

function AEmodeInputHandler() {
    let AEmode = document.getElementById("AEmodeDropDown").value;
    let AEtype = document.getElementById("AEtypeDropDown").value;

    cameras[ci].PUTdata("/video/autoExposure", {mode: AEmode, type: AEtype});

    unsavedChanges = unsavedChanges.filter((e) => {return e !== "AutoExposure"});
}

function ISOInputHandler() {
    let ISOInput = document.getElementById("ISOInput");

    if (event.key === 'Enter') {
        event.preventDefault;
        cameras[ci].PUTdata("/video/iso", {iso: parseInt(document.getElementById("isoSpan").innerHTML)})
        //cameras[ci].PUTdata("/video/iso", {iso: parseInt(ISOInput.value)})
        unsavedChanges = unsavedChanges.filter((e) => {return e !== "ISO"});
    } else {
        unsavedChanges.push('ISO');
    }
}

// 0: lift, 1: gamma, 2: gain, 3: offset, 4: contrast, 5: color & LC
function CCInputHandler(which) {
    if (event.key === 'Enter') {
        event.preventDefault;
        setCCFromUI(which);
    } else {
        unsavedChanges.push('CC'+which);
    }
}

function NDFilterInputHandler() {
    if (event.key === 'Enter') {
        event.preventDefault;
        cameras[ci].PUTdata("/video/ndFilter", {stop: parseInt(document.getElementById("ndFilterSpan").innerHTML)})
        unsavedChanges = unsavedChanges.filter((e) => {return e !== "ND"});
    } else {
        unsavedChanges.push('ND');
    }
}

function GainInputHandler() {
    if (event.key === 'Enter') {
        event.preventDefault;
        cameras[ci].PUTdata("/video/gain", {gain: parseInt(document.getElementById("gainSpan").innerHTML)})
        unsavedChanges = unsavedChanges.filter((e) => {return e !== "Gain"});
    } else {
        unsavedChanges.push('Gain');
    }
}




function WBInputHandler() {
    if (event.key === 'Enter') {
        event.preventDefault;
        cameras[ci].PUTdata("/video/whiteBalance", {whiteBalance: parseInt(document.getElementById("whiteBalanceSpan").innerHTML)})
        unsavedChanges = unsavedChanges.filter((e) => {return e !== "WB"});
    } else {
        unsavedChanges.push('WB');
    }
}

function WBTInputHandler() {
    if (event.key === 'Enter') {
        event.preventDefault;
        cameras[ci].PUTdata("/video/whiteBalanceTint", {whiteBalanceTint: parseInt(document.getElementById("whiteBalanceTintSpan").innerHTML)})
        unsavedChanges = unsavedChanges.filter((e) => {return e !== "WBT"});
    } else {
        unsavedChanges.push('WBT');
    }
}

// 0: lift, 1: gamma, 2: gain, 3: offset
function setCCFromUI(which) {
    if (which < 4) {
        var lumaFloat = parseFloat(document.getElementsByClassName("CClumaLabel")[which].innerHTML);
        var redFloat = parseFloat(document.getElementsByClassName("CCredLabel")[which].innerHTML);
        var greenFloat = parseFloat(document.getElementsByClassName("CCgreenLabel")[which].innerHTML);
        var blueFloat = parseFloat(document.getElementsByClassName("CCblueLabel")[which].innerHTML);
        
        var ccobject = {"red": redFloat, "green": greenFloat, "blue": blueFloat, "luma": lumaFloat};
    }

    if (which == 0) {
        cameras[ci].PUTdata("/colorCorrection/lift", ccobject);
    } else if (which == 1) {
        cameras[ci].PUTdata("/colorCorrection/gamma", ccobject);
    } else if (which == 2) {
        cameras[ci].PUTdata("/colorCorrection/gain", ccobject);
    } else if (which == 3) {
        cameras[ci].PUTdata("/colorCorrection/offset", ccobject);
    } else if (which == 4) {
        let pivotFloat = parseFloat(document.getElementById("CCcontrastPivotLabel").innerHTML);
        let adjustInt = parseInt(document.getElementById("CCcontrastAdjustLabel").innerHTML);
        
        let adjustFloat = adjustInt/50.0;

        cameras[ci].PUTdata("/colorCorrection/contrast", {pivot: pivotFloat, adjust: adjustFloat});
    } else {
        let hueInt = parseInt(document.getElementById("CCcolorHueLabel").innerHTML);
        let satInt = parseInt(document.getElementById("CCcolorSatLabel").innerHTML);
        let lumCoInt = parseInt(document.getElementById("CCcolorLCLabel").innerHTML);
        
        let hueFloat = (hueInt/180.0) - 1.0;
        let satFloat = satInt/50.0;
        let lumCoFloat = lumCoInt/100.0;

        cameras[ci].PUTdata("/colorCorrection/color", {hue: hueFloat, saturation: satFloat});
        cameras[ci].PUTdata("/colorCorrection/lumaContribution", {lumaContribution: lumCoFloat});
    }

    unsavedChanges = unsavedChanges.filter((e) => {return !e.includes("CC"+which)});
}

// Reset Color Correction Values
// 0: lift, 1: gamma, 2: gain, 3: offset, 4: contrast, 5: color & LC
function resetCC(which) {
    if (which == 0) {
        cameras[ci].PUTdata("/colorCorrection/lift", {"red": 0.0, "green": 0.0, "blue": 0.0, "luma": 0.0});
    } else if (which == 1) {
        cameras[ci].PUTdata("/colorCorrection/gamma", {"red": 0.0, "green": 0.0, "blue": 0.0, "luma": 0.0});
    } else if (which == 2) {
        cameras[ci].PUTdata("/colorCorrection/gain", {"red": 1.0, "green": 1.0, "blue": 1.0, "luma": 1.0});
    } else if (which == 3) {
        cameras[ci].PUTdata("/colorCorrection/offset", {"red": 0.0, "green": 0.0, "blue": 0.0, "luma": 0.0});
    } else if (which == 4) {
        cameras[ci].PUTdata("/colorCorrection/contrast", {"pivot": 0.5, "adjust": 1.0});
    } else if (which == 5) {
        cameras[ci].PUTdata("/colorCorrection/color", {"hue": 0.0, "saturation": 1.0});
        cameras[ci].PUTdata("/colorCorrection/lumaContribution", {"lumaContribution": 1.0});
    }

    unsavedChanges = unsavedChanges.filter((e) => {return !e.includes("CC"+which)});
}

// Triggered by the Loop and Single Clip buttons
function loopHandler(callerString) {
    let playbackState = cameras[ci].propertyData['/transports/0/playback'];
    
    if (callerString === "Loop") {
        playbackState.loop = !playbackState.loop;
    } else if (callerString === "Single Clip") {
        playbackState.singleClip = !playbackState.singleClip;
    }

    cameras[ci].PUTdata("/transports/0/playback", playbackState);
}

/*  Helper Functions   */
function parseTimecode(timecodeBCD) {
    let noDropFrame = timecodeBCD & 0b01111111111111111111111111111111;     // The first bit of the timecode is 1 if "Drop Frame Timecode" is on. We don't want to include that in the display.
    let decimalTCInt = parseInt(noDropFrame.toString(16), 10);              // Convert the BCD number into base ten
    let decimalTCString = decimalTCInt.toString().padStart(8, '0');         // Convert the base ten number to a string eight characters long
    let finalTCString = decimalTCString.match(/.{1,2}/g).join(':');         // Put colons between every two characters
    return finalTCString;
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


