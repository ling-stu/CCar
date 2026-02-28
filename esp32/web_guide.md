# `web.h` 功能教學（按鈕行為與影像接收）


## 1) 影像接收流程（`toggle` 開關）

**對應區塊**：
- HTML：`<input type="checkbox" id="toggle">`
- JS：`togChk.addEventListener('change', ...)`

**運作方式**：
- 當使用者勾選「啟動串流」：
  - 設定 `img#stream` 的 `src` 為 `http://192.168.4.20/stream`
  - 顯示影像元素
- 當使用者取消勾選：
  - 清空 `src`
  - 隱藏影像元素

**重點**：
- 影像來源為 ESP32‑CAM 的 `/stream` 路徑（MJPEG 串流）。
- 必須確保 ESP32‑CAM 的 IP（預設 `192.168.4.20`）與實際設定一致。

**常見問題**：
- **影像黑屏**：檢查 ESP32‑CAM 是否成功連線且 IP 正確。
- **顯示延遲**：可降低 ESP32‑CAM 解析度或調整 `jpeg_quality`。

---

## 2) 按鈕觸發行為（前端送指令）

**對應區塊**：
- HTML：`button` 元素（例如 `BTN_F`、`BTN_L`、`BTN_R`、`BTN_B`）
- JS：`pointerdown`、`pointerup`、`pointerleave`

**按鍵邏輯（核心概念）**：
1. **按下 (`pointerdown`) 立刻送一次指令**
2. **按住時每 100ms 重送一次指令（持續控制）**
3. **放開 (`pointerup`) 停止重送**
4. **滑出按鈕區域 (`pointerleave`)**
   - 先送停止指令（例如 `S` 或 `SCam`）
   - 再停止重送

**實作邏輯範例（概念）**：
- 以 `setInterval` 建立重送
- 以 `clearInterval` 停止重送

**已定義的指令對應**（送到 WebSocket）：
- `BTNF`：前進
- `BTNB`：後退
- `BTNR`：右移
- `BTNL`：左移
- `BTNC`：右旋
- `BTNCC`：左旋
- `BTNCamR`：鏡頭右旋
- `BTNCamL`：鏡頭左旋
- `S`：停止移動
- `SCam`：停止鏡頭（視 `esp32.ino` 接收邏輯而定）

**常見問題**：
- **按鈕無反應**：請確認 WebSocket 是否連線成功（瀏覽器 console 顯示 `WS connected`）。
- **反應延遲**：可以把 `setInterval` 的 100ms 改大或改小；頻率過高會導致延遲累積。

---

## 3) WebSocket 接收與控制對應

**對應區塊**：
- JS：`const ws = new WebSocket(...)`
- 後端（ESP32）：`ws.onEvent(...)`

**流程**：
1. 前端按鍵 → 送出字串（例如 `BTNF`）
2. ESP32 `wsEvent()` 收到字串
3. 依字串執行車體或鏡頭動作

**常見問題**：
- **前端送了但車不動**：確認 ESP32 `wsEvent` 內的字串判斷是否一致。

---

## 4) CSS 說明

- `button.tcp` 定義按鈕外觀（顏色、圓角、字體大小）
- `.switch` 與 `.slider` 定義串流開關樣式
- `#stream` 設定影像大小與裁切

---

## 5) 如何自行加入與控制方向按鈕具有相同外觀及功能的按鈕

以下步驟可新增一個**外觀一致、按住連續送指令**的新按鈕（例如：`BTN_X`）：

### 步驟 A：新增 HTML 按鈕
在 `web.h` 的 HTML 區塊中加入：

```html
<button id="BTN_X" class="tcp">X</button>
```

### 步驟 B：取得按鈕 DOM
在 JS 區塊加入：

```javascript
const btn_x = document.getElementById('BTN_X');
```

### 步驟 C：新增重送計時器
在 JS 區塊加入：

```javascript
let btnxInterval = null;
```

### 步驟 D：新增按下／放開／離開事件
加入以下事件處理，保持與方向按鈕一致的行為：

```javascript
btn_x.addEventListener('pointerdown', () => {
  send('BTNX'); // 馬上送一次
  btnxInterval = setInterval(() => {
    send('BTNX');
  }, 100);
});

btn_x.addEventListener('pointerup', () => {
  clearInterval(btnxInterval);
});

btn_x.addEventListener('pointerleave', () => {
  send('S');
  clearInterval(btnxInterval);
});
```

### 步驟 E：ESP32 端接收指令
在 `esp32.ino` 的 `wsEvent()` 裡新增：

```cpp
} else if (msg == "BTNX") {
  // 在這裡放你要的控制行為
}
```

---

完成以上步驟後，新按鈕就會具有與方向按鈕**相同外觀與相同的連續觸發行為**。
