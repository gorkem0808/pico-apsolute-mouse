import threading
import time
import tkinter as tk
from tkinter import ttk, messagebox
from dataclasses import dataclass, field

try:
    import serial
    import serial.tools.list_ports
except Exception:
    serial = None

APP_TITLE = "GT ARCADE KONTROL IO BOARD V0.01"

BUTTON_NAMES = [
    "YÖN YUKARI", "YÖN AŞAĞI", "YÖN SOL", "YÖN SAĞ",
    "BOMBA / SEÇ", "TETİK", "JARJÖR DEĞİŞTİR",
    "COIN / KREDİ", "START", "TEST / KALİBRASYON", "SİSTEM AKTİF / PASİF",
]

@dataclass
class PicoState:
    player: int
    port: str = ""
    ser: object = None
    connected: bool = False
    raw_x: int = 0
    raw_y: int = 0
    cal_x: int = 0
    cal_y: int = 0
    buttons: list = field(default_factory=lambda: [0]*11)
    filter_value: int = 60

class SerialManager:
    def __init__(self, on_update, on_calreq):
        self.states = {1: PicoState(1), 2: PicoState(2)}
        self.on_update = on_update
        self.on_calreq = on_calreq
        self.running = True
        self.thread = threading.Thread(target=self.worker, daemon=True)
        self.thread.start()

    def find_ports(self):
        if serial is None:
            return []
        return list(serial.tools.list_ports.comports())

    def worker(self):
        while self.running:
            try:
                self.scan_connect()
                self.read_all()
            except Exception:
                pass
            time.sleep(0.03)

    def scan_connect(self):
        if serial is None:
            return
        used = {s.port for s in self.states.values() if s.connected}
        for p in self.find_ports():
            if p.device in used:
                continue
            # Pico CDC can have generic name; try ping.
            try:
                ser = serial.Serial(p.device, 115200, timeout=0.02)
                ser.write(b"PING\n")
                time.sleep(0.08)
                data = ser.read(200).decode(errors="ignore")
                if "PONG,P1" in data:
                    self.attach(1, p.device, ser)
                    used.add(p.device)
                elif "PONG,P2" in data:
                    self.attach(2, p.device, ser)
                    used.add(p.device)
                else:
                    ser.close()
            except Exception:
                try:
                    ser.close()
                except Exception:
                    pass

    def attach(self, player, port, ser):
        old = self.states[player].ser
        try:
            if old: old.close()
        except Exception:
            pass
        st = self.states[player]
        st.port = port
        st.ser = ser
        st.connected = True
        self.on_update()

    def read_all(self):
        for st in self.states.values():
            if not st.connected or not st.ser:
                continue
            try:
                while st.ser.in_waiting:
                    line = st.ser.readline().decode(errors="ignore").strip()
                    if line:
                        self.parse_line(st, line)
            except Exception:
                st.connected = False
                try: st.ser.close()
                except Exception: pass
                self.on_update()

    def parse_line(self, st, line):
        if line.startswith("CALREQ,P"):
            try:
                p = int(line.split("P",1)[1])
                self.on_calreq(p)
            except Exception:
                pass
            return
        if line.startswith("GTIO,"):
            parts = line.split(",")
            try:
                p = int(parts[1].replace("P", ""))
                if p != st.player:
                    return
                # GTIO,P1,RAW,x,y,CAL,x,y,BTN,010...,FILTER,n
                raw_i = parts.index("RAW")
                cal_i = parts.index("CAL")
                btn_i = parts.index("BTN")
                flt_i = parts.index("FILTER")
                st.raw_x = int(parts[raw_i+1]); st.raw_y = int(parts[raw_i+2])
                st.cal_x = int(parts[cal_i+1]); st.cal_y = int(parts[cal_i+2])
                bits = parts[btn_i+1].strip()
                st.buttons = [1 if c == "1" else 0 for c in bits[:11].ljust(11,"0")]
                st.filter_value = int(parts[flt_i+1])
                self.on_update()
            except Exception:
                pass

    def send(self, player, text):
        st = self.states[player]
        if st.connected and st.ser:
            try:
                st.ser.write((text.strip()+"\n").encode())
            except Exception:
                pass

class App:
    def __init__(self):
        self.root = tk.Tk()
        self.root.title(APP_TITLE)
        self.root.geometry("980x680")
        self.root.minsize(900, 620)
        self.manager = SerialManager(self.schedule_update, self.open_calibration)
        self.update_pending = False
        self.build_ui()
        self.schedule_update()

    def build_ui(self):
        header = ttk.Frame(self.root, padding=10)
        header.pack(fill="x")
        ttk.Label(header, text=APP_TITLE, font=("Segoe UI", 18, "bold")).pack(side="left")
        self.status = ttk.Label(header, text="Pico aranıyor...", font=("Segoe UI", 10))
        self.status.pack(side="right")

        nb = ttk.Notebook(self.root)
        nb.pack(fill="both", expand=True, padx=10, pady=10)
        self.tabs = {}
        for name in ["Canlı Nişangah Testi", "Buton Testi", "Titreşimi Azalt", "Manuel Kalibrasyon"]:
            f = ttk.Frame(nb, padding=10)
            nb.add(f, text=name)
            self.tabs[name] = f
        self.build_live_tab(self.tabs["Canlı Nişangah Testi"])
        self.build_buttons_tab(self.tabs["Buton Testi"])
        self.build_filter_tab(self.tabs["Titreşimi Azalt"])
        self.build_manual_cal_tab(self.tabs["Manuel Kalibrasyon"])

    def build_live_tab(self, parent):
        self.live_canvas = {}
        for p in [1,2]:
            box = ttk.LabelFrame(parent, text=f"OYUNCU {p}", padding=10)
            box.pack(side="left", fill="both", expand=True, padx=6, pady=6)
            c = tk.Canvas(box, bg="#111", highlightthickness=1, highlightbackground="#888")
            c.pack(fill="both", expand=True)
            lbl = ttk.Label(box, text="RAW X/Y: -    KALİBRE X/Y: -")
            lbl.pack(pady=5)
            self.live_canvas[p] = (c, lbl)

    def build_buttons_tab(self, parent):
        self.button_labels = {1: [], 2: []}
        for p in [1,2]:
            box = ttk.LabelFrame(parent, text=f"OYUNCU {p} BUTON TESTİ", padding=10)
            box.pack(side="left", fill="both", expand=True, padx=6, pady=6)
            for name in BUTTON_NAMES:
                row = ttk.Frame(box)
                row.pack(fill="x", pady=3)
                ttk.Label(row, text=name, width=24).pack(side="left")
                lbl = tk.Label(row, text="BASILMADI", width=14, bg="#333", fg="white", font=("Segoe UI", 10, "bold"))
                lbl.pack(side="right")
                self.button_labels[p].append(lbl)

    def build_filter_tab(self, parent):
        self.filter_vars = {}
        ttk.Label(parent, text="Titreşimi Azalt", font=("Segoe UI", 16, "bold")).pack(anchor="w", pady=6)
        ttk.Label(parent, text="Az  ← sürgüyü sağa aldıkça titreme azalır →  Çok").pack(anchor="w")
        for p in [1,2]:
            box = ttk.LabelFrame(parent, text=f"OYUNCU {p}", padding=12)
            box.pack(fill="x", pady=10)
            var = tk.IntVar(value=60)
            self.filter_vars[p] = var
            scale = ttk.Scale(box, from_=0, to=100, orient="horizontal", variable=var)
            scale.pack(fill="x", padx=4)
            row = ttk.Frame(box)
            row.pack(fill="x", pady=4)
            ttk.Button(row, text="Kaydet", command=lambda pp=p: self.save_filter(pp)).pack(side="left")
            ttk.Button(row, text="İkisine de uygula", command=self.save_filter_both).pack(side="left", padx=6)

    def build_manual_cal_tab(self, parent):
        ttk.Label(parent, text="Manuel Kalibrasyon", font=("Segoe UI", 16, "bold")).pack(anchor="w")
        ttk.Label(parent, text="Normal kullanımda Pico üzerindeki TEST / KALİBRASYON butonuna basınca tam ekran otomatik açılır.").pack(anchor="w", pady=4)
        row = ttk.Frame(parent)
        row.pack(anchor="w", pady=20)
        ttk.Button(row, text="Oyuncu 1 Kalibrasyonu Aç", command=lambda: self.open_calibration(1)).pack(side="left", padx=5)
        ttk.Button(row, text="Oyuncu 2 Kalibrasyonu Aç", command=lambda: self.open_calibration(2)).pack(side="left", padx=5)

    def schedule_update(self):
        if not self.update_pending:
            self.update_pending = True
            self.root.after(50, self.refresh_ui)

    def refresh_ui(self):
        self.update_pending = False
        s1 = self.manager.states[1]
        s2 = self.manager.states[2]
        self.status.config(text=f"P1: {'BAĞLI '+s1.port if s1.connected else 'YOK'}   |   P2: {'BAĞLI '+s2.port if s2.connected else 'YOK'}")
        for p, st in self.manager.states.items():
            c, lbl = self.live_canvas[p]
            c.delete("all")
            w = max(c.winfo_width(), 10); h = max(c.winfo_height(), 10)
            x = int(st.cal_x / 32767 * w) if st.cal_x else w//2
            y = int(st.cal_y / 32767 * h) if st.cal_y else h//2
            c.create_line(x-20, y, x+20, y, fill="white", width=2)
            c.create_line(x, y-20, x, y+20, fill="white", width=2)
            c.create_oval(x-6, y-6, x+6, y+6, outline="white", width=2)
            lbl.config(text=f"RAW X/Y: {st.raw_x} / {st.raw_y}    KALİBRE X/Y: {st.cal_x} / {st.cal_y}")
            for i, b in enumerate(st.buttons[:len(self.button_labels[p])]):
                lab = self.button_labels[p][i]
                if b:
                    lab.config(text="BASILDI", bg="#148a28")
                else:
                    lab.config(text="BASILMADI", bg="#333")
            if p in self.filter_vars and not self.filter_vars[p].get():
                self.filter_vars[p].set(st.filter_value)
        self.root.after(100, self.refresh_ui)

    def save_filter(self, p):
        val = int(self.filter_vars[p].get())
        self.manager.send(p, f"FILTER,{val}")

    def save_filter_both(self):
        val = int(self.filter_vars[1].get())
        self.filter_vars[2].set(val)
        self.manager.send(1, f"FILTER,{val}")
        self.manager.send(2, f"FILTER,{val}")

    def open_calibration(self, player):
        st = self.manager.states.get(player)
        if not st or not st.connected:
            messagebox.showwarning("Bağlantı yok", f"Oyuncu {player} Pico bağlı değil.")
            return
        CalWindow(self, player)

    def run(self):
        self.root.mainloop()

class CalWindow:
    POINTS = ["SOL ÜST", "SAĞ ÜST", "SAĞ ALT", "SOL ALT"]
    def __init__(self, app, player):
        self.app = app
        self.player = player
        self.step = 0
        self.values = []
        self.win = tk.Toplevel(app.root)
        self.win.title(f"Oyuncu {player} Kalibrasyon")
        self.win.attributes("-fullscreen", True)
        self.win.configure(bg="black")
        self.canvas = tk.Canvas(self.win, bg="black", highlightthickness=0)
        self.canvas.pack(fill="both", expand=True)
        self.win.bind("<Escape>", lambda e: self.close())
        self.win.bind("<space>", lambda e: self.save_point())
        self.win.bind("<Return>", lambda e: self.save_point())
        self.win.after(100, self.loop)
        self.draw()

    def draw(self):
        self.canvas.delete("all")
        w = self.canvas.winfo_width() or self.win.winfo_screenwidth()
        h = self.canvas.winfo_height() or self.win.winfo_screenheight()
        margin = 80
        positions = [(margin, margin), (w-margin, margin), (w-margin, h-margin), (margin, h-margin)]
        x, y = positions[self.step]
        self.canvas.create_text(w//2, 45, fill="white", font=("Segoe UI", 28, "bold"), text=f"OYUNCU {self.player} KALİBRASYON")
        self.canvas.create_text(w//2, 95, fill="white", font=("Segoe UI", 18), text=f"Silahı {self.POINTS[self.step]} köşeye çevir ve BOMBA / SEÇ butonuna bas")
        self.canvas.create_line(x-35, y, x+35, y, fill="red", width=5)
        self.canvas.create_line(x, y-35, x, y+35, fill="red", width=5)
        self.canvas.create_oval(x-20, y-20, x+20, y+20, outline="red", width=5)
        self.canvas.create_text(w//2, h-70, fill="gray", font=("Segoe UI", 16), text="GP6 = Köşeyi kaydet    GP8 / ESC = İptal")

    def loop(self):
        st = self.app.manager.states[self.player]
        # GP6 = index 4, GP8 reload = index 6
        if st.buttons[4]:
            self.save_point()
            time.sleep(0.25)
        if st.buttons[6]:
            self.close()
            return
        self.draw()
        self.win.after(100, self.loop)

    def save_point(self):
        st = self.app.manager.states[self.player]
        self.values.append((st.raw_x, st.raw_y))
        self.step += 1
        if self.step >= 4:
            xs = [v[0] for v in self.values]
            ys = [v[1] for v in self.values]
            x_min, x_max = min(xs), max(xs)
            y_min, y_max = min(ys), max(ys)
            self.app.manager.send(self.player, f"SETCAL,{x_min},{x_max},{y_min},{y_max}")
            self.canvas.delete("all")
            w = self.canvas.winfo_width(); h = self.canvas.winfo_height()
            self.canvas.create_text(w//2, h//2, fill="white", font=("Segoe UI", 34, "bold"), text="KALİBRASYON KAYDEDİLDİ")
            self.win.after(900, self.close)

    def close(self):
        try: self.win.destroy()
        except Exception: pass

if __name__ == "__main__":
    if serial is None:
        root = tk.Tk(); root.withdraw()
        messagebox.showerror("Eksik modül", "pyserial kurulu değil. CMD/PowerShell: py -m pip install pyserial")
        raise SystemExit
    App().run()
