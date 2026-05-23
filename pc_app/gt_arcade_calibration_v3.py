import re
import sys
import time
import threading
import queue
import tkinter as tk
from tkinter import ttk, messagebox

try:
    import serial
    import serial.tools.list_ports
except Exception:
    serial = None

BUTTON_BITS = {
    "YÖN YUKARI": 1 << 0,
    "YÖN AŞAĞI": 1 << 1,
    "YÖN SOL": 1 << 2,
    "YÖN SAĞ": 1 << 3,
    "BOMBA / SEÇ": 1 << 4,
    "TETİK": 1 << 5,
    "JARJÖR DEĞİŞTİR": 1 << 6,
    "COIN / KREDİ": 1 << 7,
    "START": 1 << 8,
    "TEST / SERVİS": 1 << 9,
    "NİŞANGAH AKTİF": 1 << 10,
}

STATE_RE = re.compile(r"STATE P=(\d+) RX=(\d+) RY=(\d+) X=(\d+) Y=(\d+) B=(\d+) CAL=(\d+),(\d+),(\d+),(\d+),(\d+)")
OPEN_RE = re.compile(r"OPEN_CAL P=(\d+)")

class PicoDevice:
    def __init__(self, port):
        self.port = port
        self.ser = None
        self.player = None
        self.state = None
        self.connected = False
        self.last_line = ""

    def open(self):
        self.ser = serial.Serial(self.port, 115200, timeout=0.1)
        self.connected = True
        time.sleep(0.2)
        self.write("PING\n")

    def close(self):
        self.connected = False
        try:
            if self.ser:
                self.ser.close()
        except Exception:
            pass

    def write(self, s):
        if self.ser and self.ser.is_open:
            self.ser.write(s.encode("ascii", errors="ignore"))

class CalibrationWindow(tk.Toplevel):
    POINTS = [
        ("SOL ÜST", 0.08, 0.08),
        ("SAĞ ÜST", 0.92, 0.08),
        ("SAĞ ALT", 0.92, 0.92),
        ("SOL ALT", 0.08, 0.92),
    ]

    def __init__(self, app, player):
        super().__init__(app.root)
        self.app = app
        self.player = player
        self.captures = []
        self.index = 0
        self.title(f"GT Arcade Kalibrasyon - Oyuncu {player}")
        self.attributes("-fullscreen", True)
        self.configure(bg="#111111")
        self.bind("<Escape>", lambda e: self.cancel())
        self.canvas = tk.Canvas(self, bg="#111111", highlightthickness=0)
        self.canvas.pack(fill="both", expand=True)
        self.footer = tk.Frame(self, bg="#222222")
        self.footer.pack(fill="x")
        self.info = tk.Label(self.footer, text="", bg="#222222", fg="white", font=("Arial", 18, "bold"))
        self.info.pack(side="left", padx=20, pady=12)
        tk.Button(self.footer, text="Kaydet ve Çık", font=("Arial", 14, "bold"), command=self.save_and_exit).pack(side="right", padx=20, pady=10)
        tk.Button(self.footer, text="İptal", font=("Arial", 14), command=self.cancel).pack(side="right", padx=10, pady=10)
        self.after(100, self.redraw)
        self.after(80, self.watch_select)

    def redraw(self):
        self.canvas.delete("all")
        w = max(1, self.canvas.winfo_width())
        h = max(1, self.canvas.winfo_height())
        name, px, py = self.POINTS[min(self.index, len(self.POINTS)-1)]
        x, y = int(px*w), int(py*h)
        self.canvas.create_line(x-40, y, x+40, y, fill="white", width=4)
        self.canvas.create_line(x, y-40, x, y+40, fill="white", width=4)
        self.canvas.create_oval(x-18, y-18, x+18, y+18, outline="yellow", width=4)
        self.canvas.create_text(w//2, 60, text=f"OYUNCU {self.player} - 4 KÖŞE POTANS KALİBRASYONU", fill="white", font=("Arial", 28, "bold"))
        self.canvas.create_text(w//2, 105, text="Orta nokta yok. Silahı hedefe çevir, BOMBA / SEÇ butonuna bas.", fill="#cccccc", font=("Arial", 18))
        self.canvas.create_text(w//2, h//2, text=f"{self.index+1}/4  {name}", fill="yellow", font=("Arial", 54, "bold"))
        self.info.config(text=f"Son ham değer: X={self.app.raw_x(self.player)}  Y={self.app.raw_y(self.player)}")
        self.after(100, self.redraw)

    def watch_select(self):
        st = self.app.states.get(self.player)
        if st:
            b = st.get("buttons", 0)
            prev = self.app.prev_buttons_for_cal.get(self.player, 0)
            if (b & BUTTON_BITS["BOMBA / SEÇ"]) and not (prev & BUTTON_BITS["BOMBA / SEÇ"]):
                self.capture_point()
            self.app.prev_buttons_for_cal[self.player] = b
        self.after(60, self.watch_select)

    def capture_point(self):
        st = self.app.states.get(self.player)
        if not st:
            return
        self.captures.append((st["rx"], st["ry"]))
        self.index += 1
        if self.index >= len(self.POINTS):
            self.save_and_exit()

    def save_and_exit(self):
        if len(self.captures) < 4:
            messagebox.showwarning("Eksik", "4 köşe tamamlanmadan kaydedilmez.")
            return
        xs = [p[0] for p in self.captures]
        ys = [p[1] for p in self.captures]
        xmin, xmax = min(xs), max(xs)
        ymin, ymax = min(ys), max(ys)
        if xmax - xmin < 200 or ymax - ymin < 200:
            messagebox.showerror("Hata", "Potans aralığı çok dar. Bağlantıyı ve mekanik hareketi kontrol et.")
            return
        dev = self.app.devices_by_player.get(self.player)
        if dev:
            dev.write(f"SETCAL {xmin} {xmax} {ymin} {ymax} 0\n")
        self.destroy()

    def cancel(self):
        self.destroy()

class App:
    def __init__(self):
        self.root = tk.Tk()
        self.root.title("GT Arcade Pico Pro Calibration V3")
        self.root.geometry("980x680")
        self.q = queue.Queue()
        self.devices = []
        self.devices_by_player = {}
        self.states = {}
        self.prev_buttons_for_cal = {}
        self.setup_ui()
        self.root.after(300, self.scan_ports)
        self.root.after(50, self.process_queue)
        self.root.after(100, self.refresh_ui)

    def setup_ui(self):
        header = ttk.Frame(self.root, padding=10)
        header.pack(fill="x")
        ttk.Label(header, text="GT Arcade Pico Pro Calibration V3", font=("Arial", 22, "bold")).pack(side="left")
        ttk.Button(header, text="Portları Tara", command=self.scan_ports).pack(side="right")

        self.nb = ttk.Notebook(self.root)
        self.nb.pack(fill="both", expand=True, padx=10, pady=10)

        self.tab_live = ttk.Frame(self.nb, padding=15)
        self.tab_buttons = ttk.Frame(self.nb, padding=15)
        self.tab_notes = ttk.Frame(self.nb, padding=15)
        self.nb.add(self.tab_live, text="Canlı Nişangah Testi")
        self.nb.add(self.tab_buttons, text="Buton Testi")
        self.nb.add(self.tab_notes, text="Notlar")

        self.status = ttk.Label(self.tab_live, text="Pico aranıyor...", font=("Arial", 13))
        self.status.pack(anchor="w")
        self.live_canvas = tk.Canvas(self.tab_live, bg="#101010", height=420)
        self.live_canvas.pack(fill="both", expand=True, pady=10)
        btns = ttk.Frame(self.tab_live)
        btns.pack(fill="x")
        ttk.Button(btns, text="P1 Kalibrasyon Aç", command=lambda: self.open_cal(1)).pack(side="left", padx=5)
        ttk.Button(btns, text="P2 Kalibrasyon Aç", command=lambda: self.open_cal(2)).pack(side="left", padx=5)

        self.button_labels = {}
        for name in BUTTON_BITS:
            row = ttk.Frame(self.tab_buttons)
            row.pack(fill="x", pady=4)
            ttk.Label(row, text=name, font=("Arial", 15, "bold"), width=24).pack(side="left")
            p1 = ttk.Label(row, text="P1: BASILMADI", font=("Arial", 13), width=18)
            p2 = ttk.Label(row, text="P2: BASILMADI", font=("Arial", 13), width=18)
            p1.pack(side="left", padx=10)
            p2.pack(side="left", padx=10)
            self.button_labels[(1, name)] = p1
            self.button_labels[(2, name)] = p2

        notes = tk.Text(self.tab_notes, wrap="word", font=("Consolas", 11))
        notes.pack(fill="both", expand=True)
        notes.insert("1.0", NOTES_TEXT)
        notes.configure(state="disabled")

    def raw_x(self, player):
        return self.states.get(player, {}).get("rx", "-")

    def raw_y(self, player):
        return self.states.get(player, {}).get("ry", "-")

    def scan_ports(self):
        if serial is None:
            messagebox.showerror("Eksik modül", "pyserial kurulu değil. pc_app klasöründe: pip install -r requirements.txt")
            return
        existing = {d.port for d in self.devices if d.connected}
        for port in serial.tools.list_ports.comports():
            if port.device in existing:
                continue
            desc = f"{port.description} {port.manufacturer or ''} {port.product or ''}"
            if "Pico" in desc or "GT Arcade" in desc or "USB Serial" in desc or "CDC" in desc or "RP2" in desc:
                dev = PicoDevice(port.device)
                try:
                    dev.open()
                    self.devices.append(dev)
                    threading.Thread(target=self.reader_thread, args=(dev,), daemon=True).start()
                except Exception:
                    dev.close()
        self.status.config(text=f"Bağlı cihaz sayısı: {len(self.devices_by_player)}")

    def reader_thread(self, dev):
        while dev.connected:
            try:
                line = dev.ser.readline().decode("ascii", errors="ignore").strip()
                if line:
                    self.q.put((dev, line))
            except Exception:
                dev.close()
                break

    def process_queue(self):
        while True:
            try:
                dev, line = self.q.get_nowait()
            except queue.Empty:
                break
            dev.last_line = line
            m = STATE_RE.search(line)
            if m:
                p = int(m.group(1))
                dev.player = p
                self.devices_by_player[p] = dev
                self.states[p] = {
                    "rx": int(m.group(2)), "ry": int(m.group(3)),
                    "x": int(m.group(4)), "y": int(m.group(5)),
                    "buttons": int(m.group(6)),
                }
            m = OPEN_RE.search(line)
            if m:
                self.open_cal(int(m.group(1)))
        self.root.after(50, self.process_queue)

    def refresh_ui(self):
        self.draw_live()
        for player in (1, 2):
            b = self.states.get(player, {}).get("buttons", 0)
            for name, bit in BUTTON_BITS.items():
                lab = self.button_labels[(player, name)]
                lab.config(text=f"P{player}: {'BASILDI' if b & bit else 'BASILMADI'}")
        self.status.config(text=f"Bağlı: {', '.join('P'+str(p) for p in sorted(self.devices_by_player)) or 'Yok'}")
        self.root.after(100, self.refresh_ui)

    def draw_live(self):
        c = self.live_canvas
        c.delete("all")
        w = max(1, c.winfo_width())
        h = max(1, c.winfo_height())
        c.create_text(w//2, 28, text="Canlı nişangah testi", fill="white", font=("Arial", 18, "bold"))
        for p, color, ytext in [(1, "cyan", 60), (2, "orange", 90)]:
            st = self.states.get(p)
            if not st:
                c.create_text(20, ytext, anchor="w", text=f"P{p}: bağlı değil", fill="gray", font=("Arial", 13))
                continue
            x = int(st["x"] / 32767 * w)
            y = int(st["y"] / 32767 * h)
            c.create_oval(x-12, y-12, x+12, y+12, outline=color, width=3)
            c.create_line(x-25, y, x+25, y, fill=color, width=2)
            c.create_line(x, y-25, x, y+25, fill=color, width=2)
            c.create_text(20, ytext, anchor="w", text=f"P{p}: Ham X={st['rx']} Ham Y={st['ry']}  Kalibre X={st['x']} Kalibre Y={st['y']}", fill=color, font=("Arial", 13, "bold"))

    def open_cal(self, player):
        if player not in self.states:
            messagebox.showwarning("Cihaz yok", f"Oyuncu {player} Pico bağlı değil veya seri veri gelmiyor.")
            return
        CalibrationWindow(self, player)

    def run(self):
        self.root.mainloop()

NOTES_TEXT = """
GT Arcade Pico Pro Calibration V3 - Notlar

Kullanıcı ekranında GP pinleri yazmaz. Program butonları görev isimleriyle gösterir.

PİN LİSTESİ:
GP26 = X potans orta uç
GP27 = Y potans orta uç
Potans dış uçları = 3V3 ve GND

GP20 = Nişangah aktif
GP7  = Tetik
GP8  = Jarjör değiştir / Reload
GP6  = Bomba + Menü Select / Enter
GP2  = Menü yukarı
GP3  = Menü aşağı
GP4  = Menü sol
GP5  = Menü sağ
GP17 = Coin / Kredi
GP18 = Start
GP19 = Test / Kalibrasyon

KLAVYE KARŞILIKLARI:
P1 Tetik = Mouse Left Button
P1 Bomba / Select = Space ve Mouse Right Button
P1 Reload = R
P1 Coin = NumPad 5
P1 Start = NumPad 1
P1 Test = NumPad 7

P2 Tetik = Mouse Left Button, ama cihaz adı ayrıdır: GT Arcade Pico P2 Pro Gun
P2 Bomba / Select = Space ve Mouse Right Button
P2 Reload = T
P2 Coin = NumPad 6
P2 Start = NumPad 2
P2 Test = NumPad 7

Menü yönleri:
Menü yukarı = Up Arrow
Menü aşağı = Down Arrow
Menü sol = Left Arrow
Menü sağ = Right Arrow

KALİBRASYON:
GP19 / Test-Kalibrasyon butonuna basınca PC programı kalibrasyon ekranını otomatik açar.
4 köşe vardır: Sol üst, sağ üst, sağ alt, sol alt. Orta nokta yoktur.
Her köşede silahı hedefe çevir ve GP6 / Bomba-Seç butonuna bas.
4 köşe bitince kalibrasyon Pico içine kaydedilir ve ekran kapanır.
"""

if __name__ == "__main__":
    App().run()
