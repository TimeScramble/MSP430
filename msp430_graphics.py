from os import write
import threading
from typing import Counter
from matplotlib import pyplot as plt
from matplotlib.widgets import Button, Slider
import matplotlib.dates as dates
import csv
import time
import serial
import numpy as np
from datetime import datetime
import pandas as pd


plt.ion()
fig, ax = plt.subplots()

uart_msg = {
    "gain1x": 0b11001100,
    "gain4x": 0b11001101,
    "gain16x": 0b11001110,
    "gain64x": 0b11001111,
}


def main():

    ser = serial.Serial("COM5", 9600, timeout=1)
    stop_event = threading.Event()
    stop_reading = threading.Event()

    var3 = np.array(np.zeros(1))
    var4 = np.array(np.zeros(1))
    var5 = np.array(np.zeros(1))

    open("test_data.csv", "w").close()

    time_label = []
    plt.xlabel("Time")
    plt.title("APDS-9960 Reading")
    plt.gcf().autofmt_xdate()
    #plt.tight_layout()
    
    plt.subplots_adjust(bottom=0.25)
    """
    SLIDERS
    """
    axcolor = 'lightgoldenrodyellow'
    ax_slider_gain = plt.axes([0.25, 0.1, 0.65, 0.03], facecolor=axcolor)
    ax_slider_freq = plt.axes([0.25, 0.15, 0.65, 0.03], facecolor=axcolor)

    sxgain = Slider(ax_slider_gain, 'Gain', 1, 4, valinit=4, valstep=1)
    sxfreq = Slider(ax_slider_freq, 'Sampling rate',
                    1, 4, valinit=4, valstep=1)

    def update(val):
        if sxgain.val == 1:
            serial_write(ser, "gain1x")
        elif sxgain.val == 2:
            serial_write(ser, "gain4x")
        elif sxgain.val == 3:
            serial_write(ser, "gain16x")
        elif sxgain.val == 4:
            serial_write(ser, "gain64x")
    sxgain.on_changed(update)           #on_changed -> on_release?
    # sxfreq.on_changed(update)

    """
    BUTTONS
    """
    resetax = plt.axes([0.8, 0.025, 0.1, 0.04])
    closeax = plt.axes([0.2, 0.025, 0.1, 0.04])
    stopax = plt.axes([0.5, 0.025, 0.1, 0.04])

    reset_button = Button(resetax, 'Reset', color=axcolor, hovercolor='0.975')
    close_button = Button(closeax, 'Close', color=axcolor, hovercolor='0.975')
    stop_button = Button(stopax, 'Pause', color=axcolor, hovercolor='0.975')

    def reset(event):
        sxgain.reset()
        sxfreq.reset()
    reset_button.on_clicked(reset)

    def close(event):
        stop_event.set()
    close_button.on_clicked(close)

    def stop(event):
        if stop_reading.is_set():
            stop_reading.clear()
        else:
            stop_reading.set()
    stop_button.on_clicked(stop)

    #threading.Thread(target=serial_loop, args=[ser, stop_event], daemon = True).start()

    """
    MAIN LOOP
    """
    while not stop_event.is_set():

        numbers = []  # reset numbers[] to write new values
        now = datetime.now().strftime("%H:%M:%S")

        if not stop_reading.is_set():
            msg = ser.read_until(expected=serial.LF)  # read serial-port
            
            numbers = read_data(msg, now)

            if len(numbers) == 5:  # save values in graph_parameters
                var3 = np.append(var3, numbers[2])
                var4 = np.append(var4, numbers[3])
                var5 = np.append(var5, numbers[4])


            #line.set_ydata(var[0], var[1], var[2], var[3], var[4])

            #ani = FuncAnimation(plt.gcf(), animate, 1000)

        draw(var3, var4, var5, stop_reading)
        #plt.legend(loc = "upper left")

    print("main loop closed")



"""
            #var1 = np.append(var1, numbers[0])
            #var1 = var1[1:plot_window + 1]

            #var2 = np.append(var2, numbers[1])
            #var2 = var2[1:plot_window + 1]
"""

"""
def serial_loop(ser, stop_event):


    while not stop_event.is_set():
        inp = input("? ")
        if inp == "x":
            stop_event.set()
        elif inp == "r":
            serial_write(ser, "l100\n")
        elif inp == "g":
            serial_write(ser, "l010\n")
        elif inp == "b":
            serial_write(ser, "l001\n")
"""


def read_data(data, now):
    values = []

    if len(data) == 0:
        return values

    temp = data.decode("ascii")

    if 'GG' in temp:  # Check for command-identifier 'GG'

        print(temp.strip("GG"))  # print command_messages without identifier
        return values

    msg = temp.rstrip("\r\n")
    msg = msg.split(" ")
    

    for x in msg:
        if not x.isupper() and not x == '=' and not x == '':
            hex = int(x, 16)
            values.append(hex)

    
    with open("test_data.csv", "a") as f:  # save read values in test_data.csv-file
        writer = csv.writer(f, delimiter=",")
        if len(values) == 5:
            writer.writerow(
                [now, values[0], values[1], values[2], values[3], values[4]])

    return values

"""
def animate():
    
    with open("test_data.csv", newline='') as f:
        reader = csv.reader(f, delimiter=",")
        for row in reader:
            data.append(row)
    
    temp = pd.read_csv("test_data.csv",header = None, usecols = [0,2,3,4])
    x_values = temp[0]
    y_values = temp[2,3,4]
    plt.cla()
    plt.plot(x_values, y_values)
    plt.xlabel("Time")
    plt.title("APDS-9960 Reading")
    plt.gcf().autofmt_xdate()
    plt.tight_layout()
    plt.show()
"""




def draw(v3, v4, v5, stop_reading):
    """
    DRAWING
    """
    #saved_time = list(dates.datestr2num(time))

    if not stop_reading.is_set():
        ax.clear()
        ax.autoscale_view()

        ax.plot(v3, c= "r")
        ax.plot(v4, c= "g")
        ax.plot(v5, c= "b")
    
    #plt.legend([line1], ["RDATA"])

    fig.canvas.draw()
    fig.canvas.flush_events()

    # ax[0].clear()
    # ax[0].plot(var1)
    # ax[0].plot(var2)

    # ax.relim()


def serial_write(ser, msg):

    if msg in uart_msg:

        ser.write(uart_msg[msg])
    else:
        print("no possible message found")


if __name__ == "__main__":
    main()
