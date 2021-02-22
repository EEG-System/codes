# Created by: Jose C. Dominguez

import matplotlib.pyplot as plt
import array as arr


# to open the text document containing the ADS1299-ADCs
file = open('sampleText.txt', 'rb')
# two dimension array 2x8 where to save every channel data
channel_data = [[],[],[],[],[],[],[],[]]

one_channel = 0
three_bytes = 0
LSB = (2*4500/24)/16777216

data = 0

# 2D array
SAMPLES = 10
CHANNELS = 8 
moving_avg_ch = [ [0]*SAMPLES for i in range(CHANNELS)]
n=0
count=0
mo_avg_value=0


# loop to read every element of the file
char = file.read()
for c in char:

    data = (data << 8 ) | int(c)
    
    three_bytes+=1
    if three_bytes == 3: # checks when the 3 bytes are together
        three_bytes = 0
        
        if count == SAMPLES:
            for i in range(CHANNELS):
                for j in range(SAMPLES):
                    mo_avg_value += moving_avg_ch[i][j]
                for j in range(SAMPLES):
                    channel_data[i].append((mo_avg_value/SAMPLES)*LSB)
                mo_avg_value=0
            count=0
  
        moving_avg_ch[one_channel][count] = data
  
        data = 0 
        one_channel+=1
        if one_channel == CHANNELS:  # checks when all the channel are being used
            count+=1
            one_channel=0


    
file.close()


# plotting ...
fig, axs = plt.subplots(8, sharex='col')
fig.suptitle('DATA ACQUIRED (reading 24 by 24 bits)')
fig.text(0.5, 0.04, 'Samples', ha='center')
fig.text(0.04, 0.5, 'Volts', va='center', rotation='vertical')

for i in range(8):
    axs[i].plot(channel_data[i], label = 'ch{}'.format(i+1), color='C{}'.format(i+1))
    axs[i].legend()
    axs[i].spines["top"].set_visible(False)
    axs[i].spines["right"].set_visible(False)
    axs[i].spines["bottom"].set_visible(False)

plt.show()