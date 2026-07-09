import serial 
import threading
import pyqtgraph as pg 
from pyqtgraph.Qt import QtWidgets, QtCore 
from collections import deque 
import re
import numpy as np
# ---------------- CONFIG ---------------- 
PORT = "COM3" 
BAUDRATE = 115200 
N = 1000 # more points = smoother trace 
ser = serial.Serial(PORT, BAUDRATE, timeout=0.1)
 # ---------------- DATA BUFFERS ---------------- 
adc1_ch0 = deque(maxlen=N) 
adc1_ch1 = deque(maxlen=N) 
adc2_ch0 = deque(maxlen=N) 
adc2_ch1 = deque(maxlen=N) 
dac = deque(maxlen=N) 
il = deque(maxlen=N) 
buffer = deque(maxlen=2000) 
il_max = None
il_min = None
lock = threading.Lock() 
# ---------------- SERIAL THREAD ---------------- 
def read_serial(): 
    while True: 
        line = ser.readline().decode("utf-8", errors="ignore").strip() 
        if line: 
            with lock: 
                buffer.append(line) 
threading.Thread(target=read_serial, daemon=True).start()
# ---------------- PARSER ---------------- 
def parse(line):
    m = re.match(
        r"ADC1=(\d+)\s+(\d+)\s*\|\s*"
        r"ADC2=(\d+)\s+(\d+)\s*\|\s*"
        r"DAC=(\d+)\s+IL=(\d+)",
        line.strip()
    )

    if not m:
        return None

    adc1_ch0, adc1_ch1, adc2_ch0, adc2_ch1, dac, il = map(int, m.groups())

    return adc1_ch0, adc1_ch1, adc2_ch0, adc2_ch1, dac, il
    
#----------Send
def send_uart():
    text = input_box.text().strip()
    print(text)
    if text:
        ser.write((text+'\n').encode())
        log_box.appendPlainText(">> " + text)
        input_box.clear()
#------------------------
def reset_il_extrema():
    global il_max, il_min

    il_max = None
    il_min = None

    curve_max.clear()
    curve_min.clear()

    delta_label.setText("ΔIL = --")

# ---------------- QT APP ----------------
app = QtWidgets.QApplication([])
win = pg.GraphicsLayoutWidget(show=True, title="STM32 ADC Oscilloscope")
win.resize(1500, 800) 
plot = win.addPlot(title="ADC Streaming") 
plot.setYRange(0, 4096) 
plot.setXRange(0, N) 
plot.addLegend() 

log_win = QtWidgets.QWidget()
log_win.setWindowTitle("UART Terminal")

layout = QtWidgets.QVBoxLayout()

log_box = QtWidgets.QPlainTextEdit()
log_box.setReadOnly(True)
delta_label = QtWidgets.QLabel("ΔIL = --")
delta_label.setStyleSheet(
    "font-size: 14pt; font-weight: bold;"
)

reset_button = QtWidgets.QPushButton("Reset IL Max/Min")
reset_button.clicked.connect(reset_il_extrema)

input_box = QtWidgets.QLineEdit()
input_box.setPlaceholderText("Type command and press Enter...")

input_box.returnPressed.connect(send_uart)

layout.addWidget(delta_label)
layout.addWidget(reset_button)
layout.addWidget(log_box)
layout.addWidget(input_box)

log_win.setLayout(layout)
log_win.resize(600, 400)
log_win.show()

curve1 = plot.plot(pen=pg.mkPen('y', width=5), name="ADC1 CH0") 
curve2 = plot.plot(pen=pg.mkPen('r', width=5), name="ADC1 CH1") 
curve3 = plot.plot(pen=pg.mkPen('g', width=5), name="ADC2 CH0") 
curve4 = plot.plot(pen=pg.mkPen('b', width=5), name="ADC2 CH1")
curve5 = plot.plot(pen=pg.mkPen('magenta', width=5), name ='DAC')

win2 = pg.GraphicsLayoutWidget(show=True, title="IL tracker")
win2.resize(1500, 800) 
plot2 = win2.addPlot(title="IL tracker") 
plot2.setYRange(0, -6) 
plot2.setXRange(0, N) 
plot2.addLegend() 

curve = plot2.plot(pen=pg.mkPen('r', width=5), name ='IL')
curve_max = plot2.plot(
    pen=pg.mkPen('g', width=5, style=QtCore.Qt.PenStyle.DashLine),
    name='IL Max'
)

curve_min = plot2.plot(
    pen=pg.mkPen('magenta', width=5, style=QtCore.Qt.PenStyle.DashLine),
    name='IL Min'
)
#curve5 = plot.plot(pen=pg.mkPen('m', width=5), name="ADC2 CH2")
# ---------------- UPDATE FUNCTION ---------------- 
def update():
    with lock:
        while buffer:
            line = buffer.popleft()

            data = parse(line)

            if data is not None:
                a, b, c, d,e,f = data

                adc1_ch0.append(a)
                adc1_ch1.append(b)
                adc2_ch0.append(c)
                adc2_ch1.append(d)
                dac.append(e)
                il_value = np.log(f / 65535)
                il.append(il_value)

                global il_max, il_min

                if il_max is None or il_value > il_max:
                    il_max = il_value

                if il_min is None or il_value < il_min:
                    il_min = il_value

            else:
                log_box.appendPlainText(line)

    curve1.setData(adc1_ch0)
    curve2.setData(adc1_ch1)
    curve3.setData(adc2_ch0)
    curve4.setData(adc2_ch1)
    curve5.setData(dac)
    curve.setData(il)
    if il_max is not None and len(il):
        curve_max.setData([il_max] * len(il))
    else:
        curve_max.clear()

    if il_min is not None and len(il):
        curve_min.setData([il_min] * len(il))
    else:
        curve_min.clear()

    if il_max is not None and il_min is not None:
        delta_label.setText(
        f"IL Max = {il_max:.4f}    IL Min = {il_min:.4f}    ΔIL = {il_max - il_min:.4f}"
        )



# ---------------- TIMER ---------------- 
timer = QtCore.QTimer() 
timer.timeout.connect(update) 
timer.start(20) # 50 FPS update loop 
# ---------------- START ---------------- 
app.exec_()