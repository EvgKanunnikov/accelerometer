import serial
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation

def Get_Data_From_Accel():
    ser = serial.Serial('COM4')
    data = []
    row_data = []

    #getting two start bits
    ce = ser.read()
    fa = ser.read()
    row_data.append(int.from_bytes(ce, signed=False))
    row_data.append(int.from_bytes(fa, signed=False))
    if (row_data[0] + row_data[1] == 456):

        #getting acceleration values along 3 axes X Y Z
        for _ in range(3):
            r1 = ser.read()
            r2 = ser.read()
            row_data.append(int.from_bytes(r1, signed=False))
            row_data.append(int.from_bytes(r2, signed=False))
            data.append(int.from_bytes(r1 + r2, signed=True))

        #a counter of received packets to understand how many losses there were
        count = ser.read()
        row_data.append(int.from_bytes(count, signed=False))
        data.append(int.from_bytes(count, signed=False))

        #CPR calculated a MCs
        crc = ser.read()
        data.append(int.from_bytes(crc, signed=False))

    ser.close()
    crc = Check_CRC(row_data)
    if crc == data[-1]:
        data[-1] = 0
    else:
        data[-1] = 1
    return data #[x, y, z, count, error]

def Check_CRC(data):
    #crc counting for 8 bits
    crc = 0x00
    poly = 0x07
    for byte in data:
        crc ^= byte
        for _ in range(8):
            if crc & 0x80:  
                crc = (crc << 1) ^ poly
            else:
                crc <<= 1
            crc &= 0xFF
    return crc


def Get_Average(data):
    #sample average
    sum = 0.
    for elem in data:
        sum += elem
    return round(sum / len(data), 3)

def Get_Sigma(data):
    #biased estimated variance
    aver = Get_Average(data)
    sum = 0.
    for elem in data:
        sum += (elem - aver) ** 2
    return round((sum / (len(data) - 1)) ** 0.5, 3) #variance ^ 0.5 = sigma


#creating a window and 3 axes
fig = plt.figure(layout='constrained')
ax1 = plt.subplot2grid((3, 3), (0, 0), colspan=3) 
ax2 = plt.subplot2grid((3, 3), (1, 0), colspan=3) 
ax3 = plt.subplot2grid((3, 3), (2, 0), colspan=3) 

#creating lists for storing accelerations and time for drawing graphs
t = [0]
gX = [0]
gY = [0]
gZ = [0]

#declaring names for graphs
valueX, = ax1.plot(t, gX)
ax1.set_title('X-axis acceleration')
valueY, = ax2.plot(t, gY, 'tab:orange')
ax2.set_title('Y-axis acceleration')
valueZ, = ax3.plot(t, gZ, 'tab:green')
ax3.set_title('Z-axis acceleration')

#designations of axis names
ax1.set_xlabel('Time (s)')
ax1.set_ylabel('Acceleration (g)')
ax2.set_xlabel('Time (s)')
ax2.set_ylabel('Acceleration (g)')
ax3.set_xlabel('Time (s)')
ax3.set_ylabel('Acceleration (g)')

#setting the boundaries for displaying graphs
ax1.axis([0, 600, -3, 3])
ax2.axis([0, 600, -3, 3])
ax3.axis([0, 600, -3, 3])

#displaying a grid on each graph
ax1.grid()
ax2.grid()
ax3.grid()

#Packing all the plots
plt.tight_layout() 

#creating windows to display acceleration
txt1 = ax1.text(0.95, 0.01, '0',
    verticalalignment='bottom', horizontalalignment='right',
    transform=ax1.transAxes,
    color='red', fontsize=12)
txt2 = ax2.text(0.95, 0.01, '0',
    verticalalignment='bottom', horizontalalignment='right',
    transform=ax2.transAxes,
    color='red', fontsize=12)
txt3 = ax3.text(0.95, 0.01, '0',
    verticalalignment='bottom', horizontalalignment='right',
    transform=ax3.transAxes,
    color='red', fontsize=12)

#a function that is called every frame for rendering
def update(frame):
    #obtaining and calculating acceleration from the accelerometer
    data = Get_Data_From_Accel()
    x = data[0] / 1000
    y = data[1] / 1000
    z = data[2] / 1000

    print(data[3])

    #updating lists for drawing graphs
    gX.append(x)
    gY.append(y)
    gZ.append(z)
    t.append(frame)

    #calculation of the lowest value and variance for the last 100 elements
    if len(t) % 100 == 0:
        #updating numerical acceleration values
        border = (int(len(t) / 100) - 1)  * 100

        #updating numerical acceleration values
        txt1.set_text('aver = {}g; sigma = {}'.format(Get_Average(gX[border : border + 100]), Get_Sigma(gX[border : border + 100])))
        txt2.set_text('aver = {}g; sigma = {}'.format(Get_Average(gY[border : border + 100]), Get_Sigma(gY[border : border + 100])))
        txt3.set_text('aver = {}g; sigma = {}'.format(Get_Average(gZ[border : border + 100]), Get_Sigma(gZ[border : border + 100])))

    #setting new values to display
    valueX.set_data(t, gX)
    valueY.set_data(t, gY)
    valueZ.set_data(t, gZ)

    #shifting the graph so that the graph is infinitely updated
    xmin, xmax = ax1.get_xlim()
    if len(t) >= xmax:
        ax1.set_xlim(xmax, 2 * xmax)
        ax1.figure.canvas.draw()
        ax2.set_xlim(xmax, 2 * xmax)
        ax2.figure.canvas.draw()
        ax3.set_xlim(xmax, 2 * xmax)
        ax3.figure.canvas.draw()
    
    return valueX,

#creating a graph animation
animation = FuncAnimation(fig, update, interval=50, save_count=50)
plt.show()