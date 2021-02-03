#Created by: Jose C. Dominguez

import matplotlib.pyplot as plt
import matplotlib.animation as anim
import numpy as np
import time, sys, os


# skeleton for plot figure ---- {
fig = plt.figure()
axs = fig.subplots(8)

fig.suptitle('DATA ACQUIRED (reading 24 by 24 bits)')
fig.text(0.5, 0.04, 'Samples', ha='center')
fig.text(0.04, 0.5, 'Volts', va='center', rotation='vertical')
for i in range(8):
    axs[i].spines["top"].set_visible(False)
    axs[i].spines["right"].set_visible(False)
    axs[i].spines["bottom"].set_visible(False)
# -------------------------------- }

# terminate program functions ---- {
def on_close(event):
    print("Closing program...")
    os._exit(1)
    
fig.canvas.mpl_connect('close_event', on_close)
# -------------------------------- }

def follow(thefile):

    plt.ion()
    plt.show()
    
   
    thefile.seek(0,2)
    while True:
        # two dimension array 2x8 where to save every channel data

        # clearing the plots every second ---- {
        time.sleep(1)
        for i in range(8):
            axs[i].clear()
            
        # ------------------------------------ }
   
        # process to fill data in a 1x8 array ---- {
        channel_data = [[],[],[],[],[],[],[],[]]
        one_channel = 0
        another_channel = 0

        LSB = (2*4500/24)/16777216
        # loop to read every element of the file
        char = thefile.read()
        for c in char:
            # filling array ...
            channel_data[another_channel].append(float(c*LSB))
            
            
            # every three character appended to the array means 24 bits of data 
            one_channel += 1
            if one_channel == 3:      
                one_channel = 0
                
                # once the 8th channel is appended 24 bits, will return to the 1st channel
                another_channel += 1
                if another_channel == 8:
                    another_channel = 0
        #yield channel_data
        # ---------------------------------------- }
        
        # plotting and redrawing---------- {
        for i in range(8):
            axs[i].plot(channel_data[i], color='C{}'.format(i+1))

        plt.draw()
        plt.pause(0.0001)
        # -------------------------------- }

if __name__ == '__main__':  
    
    file = open("test.txt","rb")
    logchar = follow(file)




