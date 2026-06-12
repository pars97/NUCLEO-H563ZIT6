import serial 
import threading
import pyqtgraph as pg 
from pyqtgraph.Qt import QtWidgets, QtCore 
from collections import deque 
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
#adc2_ch2 = deque(maxlen=N) 
buffer = deque(maxlen=2000) 
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
    try:
         parts = line.split("|") 
         a1 = list(map(int, parts[0].split("=")[1].split())) 
         a2 = list(map(int, parts[1].split("=")[1].split())) 
         return a1[0], a1[1], a2[0], a2[1]#, #a2[2] 
    except: 
        return None
    
#----------Send
def send_uart():
    text = input_box.text().strip()
    print(text)
    if text:
        ser.write((text+'\n').encode())
        log_box.appendPlainText(">> " + text)
        input_box.clear()

# ---------------- QT APP ----------------
app = QtWidgets.QApplication([])
win = pg.GraphicsLayoutWidget(show=True, title="STM32 ADC Oscilloscope")
win.resize(1000, 600) 
plot = win.addPlot(title="ADC Streaming") 
plot.setYRange(0, 4096) 
plot.setXRange(0, N) 
plot.addLegend() 

log_win = QtWidgets.QWidget()
log_win.setWindowTitle("UART Terminal")

layout = QtWidgets.QVBoxLayout()

log_box = QtWidgets.QPlainTextEdit()
log_box.setReadOnly(True)

input_box = QtWidgets.QLineEdit()
input_box.setPlaceholderText("Type command and press Enter...")

input_box.returnPressed.connect(send_uart)

layout.addWidget(log_box)
layout.addWidget(input_box)

log_win.setLayout(layout)
log_win.resize(600, 400)
log_win.show()

curve1 = plot.plot(pen=pg.mkPen('y', width=5), name="ADC1 CH0") 
curve2 = plot.plot(pen=pg.mkPen('r', width=5), name="ADC1 CH1") 
curve3 = plot.plot(pen=pg.mkPen('g', width=5), name="ADC2 CH0") 
curve4 = plot.plot(pen=pg.mkPen('b', width=5), name="ADC2 CH1")
#curve5 = plot.plot(pen=pg.mkPen('m', width=5), name="ADC2 CH2")
# ---------------- UPDATE FUNCTION ---------------- 
def update():
    with lock:
        while buffer:
            line = buffer.popleft()

            data = parse(line)

            if data is not None:
                a, b, c, d = data

                adc1_ch0.append(a)
                adc1_ch1.append(b)
                adc2_ch0.append(c)
                adc2_ch1.append(d)
                #adc2_ch2.append(e)

            else:
                log_box.appendPlainText(line)

    curve1.setData(adc1_ch0)
    curve2.setData(adc1_ch1)
    curve3.setData(adc2_ch0)
    curve4.setData(adc2_ch1)
    #curve5.setData(adc2_ch2)



# ---------------- TIMER ---------------- 
timer = QtCore.QTimer() 
timer.timeout.connect(update) 
timer.start(20) # 50 FPS update loop 
# ---------------- START ---------------- 
app.exec_()