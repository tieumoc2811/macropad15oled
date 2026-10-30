# Macropad15 – ZMK (Pro Micro RP2040) + OLED

Port từ bản Vial `macropad15oledtest` sang **ZMK** (board `sparkfun_pro_micro_rp2040//zmk`, chỉ USB, có ZMK Studio).

## Phần cứng (khớp bản Vial mới)

```
            (ENC1)     P0  P1  P2  P3  P4
       (ENC2)  (ENC3)  P5  P6  P7  P8  P9
                       P10 P11 P12 P13 P14
```
- Ma trận **5 cột × 4 hàng, diode ROW2COL** (hàng → công tắc → anode diode, cathode/vạch về đường **cột**). P(n) ở hàng n%4, cột n/4; R3C3 để trống.
- Nút nhấn encoder: ENC1→R0C4, ENC2→R1C4, ENC3→R2C4 (3 chân còn lại nối chung qua 1 diode vào C4).
- Chân: C0..C4 = GP7, GP8, GP29, GP28, GP4 · R0..R3 = GP1, GP0, GP2, GP3 · encoder (A,B) = (GP5,GP6), (GP26,GP27), (GP22,GP23).
- OLED SSD1306 I2C0: **SDA = D14 (GP20), SCL = D10 (GP21)**, VCC 3V3, GND.
- ZMK quét hàng làm output, còn QMK quét cột làm output, nhưng chiều diode giống nhau nên **dùng chung dây** với bản Vial.

## Đối chiếu Vial → ZMK

| Vial | ZMK |
|---|---|
| 8 layer: Base (A..O), Layer1, Numpad, Media, Nokia, FN, 2 trống | `config/macropad15.keymap` – 8 layer, mỗi layer 18 binding |
| `LCYCLE` (Base→Numpad→Media→Nokia) trên nút ENC1 | `&layer_cycle` (`cycle-layers = <0 2 3 4>`) |
| Nút nhấn ENC2 = Backspace, ENC3 = Tab | giữ nguyên |
| `TH_MIN` / `TH_PLS` (nhấn `-`, giữ `/`; nhấn `+`, giữ `*`) | `&ht_op` (hold-tap tap-preferred 200 ms) |
| `SQ_F1F2`, `SQ_F11PU`, `SQ_F11PD` | macro `&f1f2`, `&f11pu`, `&f11pd` (bấm lần lượt) |
| Nokia multi-tap, Caps Word, Shift one-shot | `&td_*` (`src/multitap.c`), `&caps_word`, `&sk LSHFT` |
| Encoder map (vol / trái-phải / lên-xuống; Media: bài trước-sau, PgUp/PgDn) | `sensor-bindings` từng layer |
| FN: F1..F12, RESET, BOOT, MUTE | `&kp F1..F12`, `&sys_reset`, `&bootloader`, `&kp C_MUTE` |
| OLED: tên layer, NUM/CAPS, mod; tên phím chữ to khi bấm; tắt sau 60 s | `src/oled_status.c` (màn hình LVGL tùy biến) |
| Vial GUI + Unlock | ZMK Studio; `&studio_unlock` đặt ở nút nhấn **ENC2 của layer FN** |

Thay đổi Nokia theo bản Vial mới: Shift (sticky/Caps Word) **chỉ áp dụng cho chữ cái**; chữ số, space, `.` `,` không bị shift; `?` `!` luôn có shift.

## Build

1. Push repo lên GitHub → tab **Actions** → tải artifact `.uf2` (firmware + `settings_reset`).
2. Giữ BOOT khi cắm USB (hoặc FN + phím bootloader) → ổ `RPI-RP2` → thả `.uf2` vào.
3. Lỗi lưu trữ/keymap cũ: nạp `settings_reset`, rồi nạp lại firmware.

Local: `west build -s app -b sparkfun_pro_micro_rp2040//zmk -S studio-rpc-usb-uart -- -DSHIELD=macropad15 -DZMK_CONFIG=<repo>/config -DZMK_EXTRA_MODULES=<repo>`

## Khác biệt so với bản Vial (cần biết)

- **Hold-tap**: bản Vial chỉ xuất ký tự khi *nhả* phím; ZMK xuất `/` hoặc `*` ngay khi giữ quá 200 ms (và lặp khi giữ).
- **Nokia**: ZMK kết thúc chuỗi khi bấm bất kỳ phím nào khác (tránh xóa nhầm); bản Vial chỉ so sánh với phím Nokia bấm trước đó.
- **Layer1** (giống C3) không có phím chuyển tới ngoài `&to 1`; từ Layer1 về Base bằng `&to 0` (P10). Gán `&to 1` ở đâu đó trong Studio nếu cần.
- **OLED**: tên phím lấy từ mã HID thực tế được gửi (nên phím layer như FN, LCYCLE không hiện chữ to); không có chỉ báo Caps Word (ZMK không có sự kiện công khai cho việc này) — NUM/CAPS lấy từ đèn LED của máy tính.
- ZMK Studio không thêm/xóa layer giống Vial; 2 layer trống (Spare1/2) giữ cho đủ 8 layer.

## Chỉnh nhanh
- Encoder ngược chiều: đổi cặp CW/CCW trong `sensor-bindings`, hoặc đổi `a-gpios`/`b-gpios` trong overlay.
- 1 nấc nhảy 2 lần: `resolution = <2>` trong overlay.
- Màn 128x32: `height = <32>`, `multiplex-ratio = <31>` trong overlay (và chỉnh lại vị trí chữ trong `oled_status.c`).
- Thời gian multi-tap: `tapping-term-ms` trong macro `MULTITAP`; hold-tap: `&ht_op`.

## Nếu build lỗi
Mình chưa build được trong môi trường tạo file (không có mạng), nên các điểm dễ lệch theo phiên bản ZMK/Zephyr:
- **OLED không lên/lỗi build màn hình**: tạm bỏ dòng `CONFIG_ZMK_DISPLAY_STATUS_SCREEN_CUSTOM=y` trong `config/macropad15.conf` để dùng màn hình mặc định của ZMK (file `oled_status.c` sẽ không được biên dịch).
- Symbol Kconfig font `CONFIG_LV_FONT_*` hoặc `LV_COLOR_DEPTH` báo không tồn tại: chỉnh theo phiên bản LVGL của ZMK bạn dùng.
- `i2c0`/`pinctrl`: nếu board đã định nghĩa sẵn, xóa node `i2c0_oled` và dùng pinctrl của board cho GP20/GP21.
- Pin: đối chiếu bảng GPIO với sơ đồ chân module Pro Micro RP2040 của bạn.
