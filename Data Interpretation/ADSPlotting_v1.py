import matplotlib.pyplot as plt


# to open the text document containing the ADS1299-ADCs
file = open('test.txt', 'rb')
# two dimension array 2x8 where to save every channel data
channel_data = [[],[],[],[],[],[],[],[]]

one_channel = 0
another_channel = 0

LSB = (2*4500/24)/16777216

# loop to read every element of the file
char = file.read()
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
            
            
        

file.close()

#print(channel_data)

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

