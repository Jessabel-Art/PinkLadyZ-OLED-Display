const canvas = document.getElementById("oled");
const ctx = canvas.getContext("2d");

const SCREEN_WIDTH = 128;
const SCREEN_HEIGHT = 64;

const STATES = {
  BOOT: "BOOT",
  DASHBOARD: "DASHBOARD",
  CRUISE_IDLE: "CRUISE_IDLE",
  NIGHT_IDLE: "NIGHT_IDLE",
  GARAGE_IDLE: "GARAGE_IDLE",
  SHUTDOWN: "SHUTDOWN"
};

let currentState = STATES.BOOT;
let stateStartTime = performance.now();
let frame = 0;

let currentTimeText = "09:32AM";
let weatherText = "CLEAR";
let tempText = "72F";

let isNight = false;
let isRaining = false;
let isCold = false;
let isStorming = false;

function millis() {
  return performance.now();
}

function elapsed() {
  return millis() - stateStartTime;
}

function changeState(newState) {
  currentState = newState;
  stateStartTime = millis();
  frame = 0;

  document.querySelectorAll("button[data-state]").forEach((button) => {
    button.classList.toggle("active", button.dataset.state === newState);
  });
}

function clearDisplay() {
  ctx.fillStyle = "black";
  ctx.fillRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
}

function drawPixel(x, y, color = "white") {
  if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) return;
  ctx.fillStyle = color;
  ctx.fillRect(Math.round(x), Math.round(y), 1, 1);
}

function drawLine(x0, y0, x1, y1) {
  x0 = Math.round(x0); y0 = Math.round(y0);
  x1 = Math.round(x1); y1 = Math.round(y1);

  const dx = Math.abs(x1 - x0);
  const sx = x0 < x1 ? 1 : -1;
  const dy = -Math.abs(y1 - y0);
  const sy = y0 < y1 ? 1 : -1;
  let err = dx + dy;

  while (true) {
    drawPixel(x0, y0);
    if (x0 === x1 && y0 === y1) break;
    const e2 = 2 * err;
    if (e2 >= dy) {
      err += dy;
      x0 += sx;
    }
    if (e2 <= dx) {
      err += dx;
      y0 += sy;
    }
  }
}

function drawFastHLine(x, y, w) {
  for (let i = 0; i < w; i++) drawPixel(x + i, y);
}

function fillRect(x, y, w, h, color = "white") {
  ctx.fillStyle = color;
  ctx.fillRect(x, y, w, h);
}

function fillCircle(cx, cy, r, color = "white") {
  ctx.fillStyle = color;
  ctx.beginPath();
  ctx.arc(cx, cy, r, 0, Math.PI * 2);
  ctx.fill();
}

function drawBitmap(bitmap, x = 0, y = 0) {
  const bytesPerRow = Math.ceil(bitmap.width / 8);

  for (let row = 0; row < bitmap.height; row++) {
    for (let colByte = 0; colByte < bytesPerRow; colByte++) {
      const byte = bitmap.data[row * bytesPerRow + colByte] || 0;

      for (let bit = 0; bit < 8; bit++) {
        const px = colByte * 8 + bit;
        if (px >= bitmap.width) continue;

        if (byte & (0x80 >> bit)) {
          drawPixel(x + px, y + row);
        }
      }
    }
  }
}

function drawText(text, x, y, size = 1) {
  ctx.fillStyle = "white";
  ctx.font = `${size * 8}px monospace`;
  ctx.textBaseline = "top";
  ctx.fillText(text, x, y);
}

function centerText(text, y, size = 1) {
  ctx.font = `${size * 8}px monospace`;
  const width = ctx.measureText(text).width;
  drawText(text, (SCREEN_WIDTH - width) / 2, y, size);
}

function drawScanlines() {
  for (let y = 0; y < SCREEN_HEIGHT; y += 4) {
    drawFastHLine(0, y, SCREEN_WIDTH);
  }
}

function drawStars(offset) {
  for (let i = 0; i < 18; i++) {
    const x = (i * 17 + offset) % SCREEN_WIDTH;
    const y = (i * 9 + 3) % 30;
    drawPixel(x, y);
  }
}

function drawRain(offset) {
  for (let i = 0; i < 16; i++) {
    const x = (i * 13 + offset) % SCREEN_WIDTH;
    const y = (i * 9 + offset) % SCREEN_HEIGHT;
    drawLine(x, y, x - 2, y + 5);
  }
}

function drawSnow(offset) {
  for (let i = 0; i < 14; i++) {
    const x = (i * 13 + offset) % SCREEN_WIDTH;
    const y = (i * 8 + offset) % SCREEN_HEIGHT;
    drawPixel(x, y);
    drawPixel(x + 1, y);
  }
}

function drawMoon() {
  fillCircle(108, 12, 8, "white");
  fillCircle(112, 10, 8, "black");
}

function drawLightningFlash() {
  if ((frame % 45) < 4) {
    drawLine(76, 0, 62, 18);
    drawLine(62, 18, 70, 18);
    drawLine(70, 18, 54, 42);
    drawLine(62, 18, 52, 28);
    drawLine(70, 18, 84, 31);
  }
}

function drawVaporGrid() {
  drawFastHLine(0, 48, 128);
  drawLine(0, 63, 64, 48);
  drawLine(128, 63, 64, 48);

  for (let x = 16; x < 128; x += 16) {
    drawLine(x, 63, 64, 48);
  }

  for (let y = 52; y < 64; y += 4) {
    drawFastHLine(0, y, 128);
  }
}

function runBootSequence() {
  const e = elapsed();

  clearDisplay();

  if (e < 1000) {
    if ((Math.floor(e / 250)) % 2 === 0) {
      fillRect(62, 30, 5, 8);
    }
  } else if (e < 2500) {
    centerText("INITIALIZING", 22);
    centerText("SYSTEM...", 36);

    if ((Math.floor(e / 200)) % 2 === 0) {
      fillRect(101, 36, 5, 7);
    }
  } else if (e < 3500) {
    drawScanlines();
    centerText("SYSTEM CHECK", 28);
  } else if (e < 4500) {
    drawBitmap(BITMAPS.z32_side);
  } else if (e < 5500) {
    drawBitmap(BITMAPS.z32_angle);
  } else if (e < 6500) {
    drawBitmap(BITMAPS.z32_front);
  } else if (e < 7300) {
    drawBitmap(BITMAPS.z32_front);

    if ((Math.floor(e / 120)) % 2 === 0) {
      fillCircle(42, 39, 3);
      fillCircle(86, 39, 3);
    }
  } else if (e < 8100) {
    drawBitmap(BITMAPS.glitch_scene);
  } else if (e < 8600) {
    drawBitmap(BITMAPS.glitch_scene_2);
  } else if (e < 9100) {
    drawBitmap(BITMAPS.glitch_scene_3);
  } else if (e < 10800) {
    drawBitmap(BITMAPS.hello_pinkladyz);
  } else {
    changeState(STATES.DASHBOARD);
  }
}

function runDashboard() {
  const e = elapsed();

  clearDisplay();
  drawBitmap(BITMAPS.dashboard_frame);

  drawText(currentTimeText, 6, 6);
  drawText(tempText, 92, 6);
  centerText("PINKLADY-Z", 20);
  drawText(`WX:${weatherText}`, 12, 36);
  drawText(`MODE:${isNight ? "NIGHT" : "DAY"}`, 12, 48);

  if (e > 4500) {
    changeState(isNight ? STATES.NIGHT_IDLE : STATES.CRUISE_IDLE);
  }
}

function runCruiseIdle() {
  clearDisplay();

  const offset = frame * 2;
  drawStars(offset);
  drawBitmap(BITMAPS.z32_side);

  if ((Math.floor(frame / 20)) % 2 === 0) {
    fillCircle(104, 42, 2);
  }

  if (elapsed() > 12000) {
    changeState(STATES.GARAGE_IDLE);
  }
}

function runNightIdle() {
  clearDisplay();

  const offset = frame * 2;
  drawMoon();
  drawStars(offset);

  if (isRaining) drawRain(offset);
  if (isCold) drawSnow(offset);
  if (isStorming) drawLightningFlash();

  drawVaporGrid();
  drawBitmap(BITMAPS.z32_side);

  if (elapsed() > 12000) {
    changeState(STATES.GARAGE_IDLE);
  }
}

function runGarageIdle() {
  clearDisplay();

  drawBitmap(BITMAPS.garage_open);

  if ((Math.floor(frame / 12)) % 2 === 0) {
    drawBitmap(BITMAPS.z32_rear_lights_on);
  } else {
    drawBitmap(BITMAPS.z32_rear_lights_off);
  }

  if (elapsed() > 10000) {
    changeState(isNight ? STATES.NIGHT_IDLE : STATES.CRUISE_IDLE);
  }
}

function runShutdownAnimation() {
  const e = elapsed();

  clearDisplay();

  if (e < 1200) {
    centerText("PARKING...", 28);
  } else if (e < 3500) {
    drawBitmap(BITMAPS.garage_open);

    if ((Math.floor(e / 250)) % 2 === 0) {
      drawBitmap(BITMAPS.z32_rear_lights_on);
    } else {
      drawBitmap(BITMAPS.z32_rear_lights_off);
    }
  } else if (e < 5000) {
    drawBitmap(BITMAPS.garage_half);
    drawBitmap(BITMAPS.z32_rear_lights_off);
  } else if (e < 6500) {
    drawBitmap(BITMAPS.garage_closed);
  } else if (e < 8500) {
    centerText("GOODNIGHT", 20);
    centerText("PINKLADYZ", 36);
  }
}

function render() {
  if (currentState === STATES.BOOT) runBootSequence();
  if (currentState === STATES.DASHBOARD) runDashboard();
  if (currentState === STATES.CRUISE_IDLE) runCruiseIdle();
  if (currentState === STATES.NIGHT_IDLE) runNightIdle();
  if (currentState === STATES.GARAGE_IDLE) runGarageIdle();
  if (currentState === STATES.SHUTDOWN) runShutdownAnimation();

  frame++;
  setTimeout(() => requestAnimationFrame(render), 80);
}

document.querySelectorAll("button[data-state]").forEach((button) => {
  button.addEventListener("click", () => {
    if (button.dataset.state === "NIGHT_IDLE") isNight = true;
    if (button.dataset.state === "CRUISE_IDLE") isNight = false;
    changeState(button.dataset.state);
  });
});

document.getElementById("rainToggle").addEventListener("change", (event) => {
  isRaining = event.target.checked;
  if (isRaining) {
    weatherText = "RAIN";
    isNight = true;
  }
});

document.getElementById("snowToggle").addEventListener("change", (event) => {
  isCold = event.target.checked;
  if (isCold) {
    weatherText = "SNOW";
    isNight = true;
  }
});

document.getElementById("stormToggle").addEventListener("change", (event) => {
  isStorming = event.target.checked;
  if (isStorming) {
    weatherText = "STORM";
    isRaining = true;
    document.getElementById("rainToggle").checked = true;
    isNight = true;
  }
});

changeState(STATES.BOOT);
render();
