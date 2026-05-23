import json
import queue
import threading
import time
import tkinter as tk
from tkinter import ttk, messagebox

try:
    import serial
    import serial.tools.list_ports
except Exception:
    serial = None

BUTTON_MASKS = {
    "YÖN YUKARI": 1 << 2,
    "YÖN AŞAĞI": 1 << 3,
    "YÖN SOL": 1 << 4,
    "YÖN SAĞ": 1 << 5,
    "BOMBA / SEÇ": 1 << 6,
    "TETİK": 1 << 7,
    "JARJÖR DEĞİŞTİR": 1 << 8,
    "COIN / KREDİ": 1 << 17,
    "START": 1 << 18,
    "TEST / SERVİS": 1 << 19,
    "NİŞANGAH AKTİF": 1 << 20,
}

DEFAULT_SETTINGS = {
    "deadzone": 8,
    "smoothing": 80,
    "samples": 24,
    "min_move": 4,
}

class SerialWorker(threading.Thread):
    def __init__(self, out_q):
        super().__init__(daemon=True)
        self.out_q = out_q
        self.ser = None
        self.running = True

    def find_port(self):
        if serial is None:
            return None
        for p in serial.tools.list_ports.comports():
            text = f"{p.description} {p.manufacturer} {p.hwid}".lower()
            if "gt arcade" in text or "pico" in text or "usb serial" in text:
                return p.device
        ports = list(serial.tools.list_ports.comports())
        return ports[0].device if ports else None

    def run(self):
        while self.running:
            try:
                if self.ser is None:
                    port = self.find_port()
                    if port:
                        self.ser = serial.Serial(port, 115200, timeout=0.2)
                        self.out_q.put(("status", f"Bağlandı: {port}"))
                        self.write("PING\n")
                    else:
                        self.out_q.put(("status", "Pico bekleniyor..."))
                        time.sleep(1)
                        continue
                line = self.ser.readline().decode(errors="ignore").strip()
                if line:
                    self.out_q.put(("line", line))
            except Exception as e:
                self.out_q.put(("status", f"Bağlantı koptu: {e}"))
                try:
                    if self.ser:
                        self.ser.close()
                except Exception:
                    pass
                self.ser = None
                time.sleep(1)

    def write(self, text):
        try:
            if self.ser:
                self.ser.write(text.encode())
                self.ser.flush()
                return True
        except Exception as e:
            self.out_q.put(("status", f"Gönderme hatası: {e}"))
        return False

class App(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("GT Arcade Pico Pro Calibration V3.2")
        self.geometry("980x680")
        self.configure(bg="#111")
        self.q = queue.Queue()
        self.worker = SerialWorker(self.q)
        self.worker.start()
        self.raw_x = 0
        self.raw_y = 0
        self.cal_x = 0
        self.cal_y = 0
        self.buttons = 0
        self.cal_points = []
        self.settings = DEFAULT_SETTINGS.copy()
        self.create_ui()
        self.after(50, self.poll_queue)
        self.after(100, self.redraw_live)

    def create_ui(self):
        top = tk.Frame(self, bg="#111")
        top.pack(fill="x", padx=12, pady=8)
        tk.Label(top, text="GT ARCADE PICO PRO V3.2", fg="white", bg="#111", font=("Arial", 20, "bold")).pack(side="left")
        self.status = tk.Label(top, text="Pico bekleniyor...", fg="#ddd", bg="#111", font=("Arial", 12))
        self.status.pack(side="right")

        self.nb = ttk.Notebook(self)
        self.nb.pack(fill="both", expand=True, padx=12, pady=8)

        self.tab_cal = tk.Frame(self.nb, bg="#1a1a1a")
        self.tab_live = tk.Frame(self.nb, bg="#1a1a1a")
        self.tab_buttons = tk.Frame(self.nb, bg="#1a1a1a")
        self.tab_filter = tk.Frame(self.nb, bg="#1a1a1a")
        self.nb.add(self.tab_cal, text="POTANS KALİBRASYONU")
        self.nb.add(self.tab_live, text="CANLI NİŞANGAH TESTİ")
        self.nb.add(self.tab_buttons, text="BUTON TESTİ")
        self.nb.add(self.tab_filter, text="TİTREŞİM AZALTMA")

        self.build_cal_tab()
        self.build_live_tab()
        self.build_buttons_tab()
        self.build_filter_tab()

    def build_cal_tab(self):
        tk.Label(self.tab_cal, text="4 KÖŞE KALİBRASYON", fg="white", bg="#1a1a1a", font=("Arial", 24, "bold")).pack(pady=12)
        self.cal_text = tk.Label(self.tab_cal, text="Sol üst köşeye çevir ve KÖŞEYİ KAYDET bas.", fg="#fff", bg="#1a1a1a", font=("Arial", 16))
        self.cal_text.pack(pady=8)
        self.cal_canvas = tk.Canvas(self.tab_cal, width=760, height=380, bg="black", highlightthickness=1, highlightbackground="#555")
        self.cal_canvas.pack(pady=12)
        row = tk.Frame(self.tab_cal, bg="#1a1a1a")
        row.pack(pady=8)
        tk.Button(row, text="KÖŞEYİ KAYDET", font=("Arial", 14, "bold"), width=18, command=self.save_corner).pack(side="left", padx=8)
        tk.Button(row, text="KAYDET VE ÇIK", font=("Arial", 14, "bold"), width=18, command=self.finish_cal).pack(side="left", padx=8)
        tk.Button(row, text="SIFIRLA", font=("Arial", 14, "bold"), width=12, command=self.reset_cal).pack(side="left", padx=8)
        self.cal_steps = ["SOL ÜST", "SAĞ ÜST", "SAĞ ALT", "SOL ALT"]
        self.draw_cal_points()

    def build_live_tab(self):
        self.live_canvas = tk.Canvas(self.tab_live, width=850, height=480, bg="black", highlightthickness=1, highlightbackground="#555")
        self.live_canvas.pack(pady=20)
        self.live_info = tk.Label(self.tab_live, text="X: 0   Y: 0", fg="white", bg="#1a1a1a", font=("Consolas", 16))
        self.live_info.pack()

    def build_buttons_tab(self):
        tk.Label(self.tab_buttons, text="BUTON TESTİ", fg="white", bg="#1a1a1a", font=("Arial", 24, "bold")).pack(pady=12)
        grid = tk.Frame(self.tab_buttons, bg="#1a1a1a")
        grid.pack(pady=12)
        self.button_labels = {}
        for i, name in enumerate(BUTTON_MASKS.keys()):
            r = i // 2
            c = i % 2
            box = tk.Label(grid, text=f"{name}: BASILMADI", width=34, height=2, fg="white", bg="#333", font=("Arial", 14, "bold"))
            box.grid(row=r, column=c, padx=10, pady=8)
            self.button_labels[name] = box
        tk.Label(self.tab_buttons, text="Bu ekranda pin numarası yazmaz. Sadece görev isimleri görünür.", fg="#ccc", bg="#1a1a1a", font=("Arial", 12)).pack(pady=10)

    def build_filter_tab(self):
        tk.Label(self.tab_filter, text="TİTREŞİMİ AZALT", fg="white", bg="#1a1a1a", font=("Arial", 26, "bold")).pack(pady=18)
        tk.Label(self.tab_filter, text="Az  —  Çok", fg="#ddd", bg="#1a1a1a", font=("Arial", 16, "bold")).pack(pady=(12, 0))
        self.titresim_var = tk.IntVar(value=60)
        self.titresim_scale = tk.Scale(
            self.tab_filter, from_=0, to=100, orient="horizontal", variable=self.titresim_var,
            length=760, bg="#1a1a1a", fg="white", troughcolor="#444",
            highlightthickness=0, font=("Arial", 12)
        )
        self.titresim_scale.pack(pady=18)
        self.titresim_label = tk.Label(self.tab_filter, text="Seviye: 60", fg="white", bg="#1a1a1a", font=("Arial", 16, "bold"))
        self.titresim_label.pack(pady=6)
        self.titresim_scale.configure(command=lambda v: self.titresim_label.config(text=f"Seviye: {int(float(v))}"))
        row = tk.Frame(self.tab_filter, bg="#1a1a1a")
        row.pack(pady=18)
        tk.Button(row, text="KAYDET", font=("Arial", 16, "bold"), width=16, command=self.send_filter).pack(side="left", padx=10)
        tk.Button(row, text="ORTA", font=("Arial", 16, "bold"), width=10, command=lambda: self.set_filter_level(60)).pack(side="left", padx=10)
        tk.Button(row, text="ÇOK", font=("Arial", 16, "bold"), width=10, command=lambda: self.set_filter_level(85)).pack(side="left", padx=10)
        tk.Label(self.tab_filter, text="Titreme fazlaysa sağa doğru artır. Tepki ağırlaşırsa biraz azalt.", fg="#ccc", bg="#1a1a1a", font=("Arial", 13)).pack(pady=10)

    def set_filter_level(self, value):
        self.titresim_var.set(value)
        self.titresim_label.config(text=f"Seviye: {value}")

    def filter_values_from_level(self, level):
        level = max(0, min(100, int(level)))
        deadzone = 2 + int(level * 0.18)      # 2..20
        smoothing = 45 + int(level * 0.45)    # 45..90
        samples = 8 + int(level * 0.32)       # 8..40
        min_move = 1 + int(level * 0.08)      # 1..9
        return deadzone, smoothing, samples, min_move

    def send_filter(self):
        level = self.titresim_var.get()
        dz, sm, sam, mm = self.filter_values_from_level(level)
        ok = self.worker.write(f"SET,{dz},{sm},{sam},{mm}\n")
        if ok:
            self.status.config(text=f"Titreşim azaltma kaydedildi: Seviye {level}")
        else:
            messagebox.showwarning("Bağlantı yok", "Pico bağlantısı bulunamadı. Program demo modunda olabilir.")

    def save_corner(self):
        if len(self.cal_points) >= 4:
            return
        self.cal_points.append((self.raw_x, self.raw_y))
        self.draw_cal_points()
        if len(self.cal_points) < 4:
            self.cal_text.config(text=f"{self.cal_steps[len(self.cal_points)]} köşeye çevir ve KÖŞEYİ KAYDET bas.")
        else:
            self.cal_text.config(text="4 köşe alındı. KAYDET VE ÇIK bas.")

    def reset_cal(self):
        self.cal_points.clear()
        self.cal_text.config(text="SOL ÜST köşeye çevir ve KÖŞEYİ KAYDET bas.")
        self.draw_cal_points()

    def finish_cal(self):
        if len(self.cal_points) < 4:
            messagebox.showwarning("Eksik", "Önce 4 köşeyi kaydet.")
            return
        xs = [p[0] for p in self.cal_points]
        ys = [p[1] for p in self.cal_points]
        xmin, xmax = min(xs), max(xs)
        ymin, ymax = min(ys), max(ys)
        self.worker.write(f"CAL,{xmin},{xmax},{ymin},{ymax}\n")
        self.status.config(text="Kalibrasyon Pico'ya gönderildi. Ekran kapanıyor.")
        self.after(1200, self.withdraw)

    def draw_cal_points(self):
        self.cal_canvas.delete("all")
        w, h = 760, 380
        corners = [(30,30), (w-30,30), (w-30,h-30), (30,h-30)]
        labels = ["SOL ÜST", "SAĞ ÜST", "SAĞ ALT", "SOL ALT"]
        for i, (x,y) in enumerate(corners):
            color = "lime" if i < len(self.cal_points) else "red"
            self.cal_canvas.create_oval(x-12,y-12,x+12,y+12, fill=color)
            self.cal_canvas.create_text(x, y+25, text=labels[i], fill="white", font=("Arial", 11, "bold"))

    def redraw_live(self):
        self.live_canvas.delete("all")
        w, h = 850, 480
        x = int(self.cal_x / 32767 * w)
        y = int(self.cal_y / 32767 * h)
        self.live_canvas.create_line(x-20, y, x+20, y, fill="white", width=2)
        self.live_canvas.create_line(x, y-20, x, y+20, fill="white", width=2)
        self.live_canvas.create_oval(x-8, y-8, x+8, y+8, outline="white", width=2)
        self.live_info.config(text=f"HAM X: {self.raw_x:4d}   HAM Y: {self.raw_y:4d}   KALİBRE X: {self.cal_x:5d}   KALİBRE Y: {self.cal_y:5d}")
        self.update_buttons()
        self.after(60, self.redraw_live)

    def update_buttons(self):
        for name, mask in BUTTON_MASKS.items():
            pressed = bool(self.buttons & mask)
            self.button_labels[name].config(
                text=f"{name}: {'BASILDI' if pressed else 'BASILMADI'}",
                bg="#087a16" if pressed else "#333"
            )

    def poll_queue(self):
        try:
            while True:
                typ, data = self.q.get_nowait()
                if typ == "status":
                    self.status.config(text=data)
                elif typ == "line":
                    self.handle_line(data)
        except queue.Empty:
            pass
        self.after(50, self.poll_queue)

    def handle_line(self, line):
        if line.startswith("DATA,"):
            parts = line.split(",")
            if len(parts) >= 7:
                try:
                    self.raw_x = int(parts[2])
                    self.raw_y = int(parts[3])
                    self.cal_x = int(parts[4])
                    self.cal_y = int(parts[5])
                    self.buttons = int(parts[6])
                except ValueError:
                    pass
        elif line == "EVENT,CALIBRATE":
            self.deiconify()
            self.lift()
            self.nb.select(self.tab_cal)
            self.status.config(text="Kalibrasyon butonu algılandı")
        else:
            self.status.config(text=line)

    def on_close(self):
        self.worker.running = False
        self.destroy()

if __name__ == "__main__":
    app = App()
    app.protocol("WM_DELETE_WINDOW", app.on_close)
    app.mainloop()
