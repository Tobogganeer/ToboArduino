
# Convert captured CAN data into something SavvyCAN is looking for (GVRET)

# from
# timestamp,bus,id,len,b7,b6,b5,b4,b3,b2,b1,b0
# 1751721260813,MS,0x50C,3,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x0C

# to
# Time Stamp,ID,Extended,Bus,LEN,D1,D2,D3,D4,D5,D6,D7,D8
# 166064000,0000021A,false,0,8,FE,36,12,FE,69,05,07,AD,

file = input("Filename: ")

with open(file, "r") as original:
    with open("conv_" + file, "w") as copy:
        original.readline() # Skip header line
        copy.write("Time Stamp,ID,Extended,Bus,LEN,D1,D2,D3,D4,D5,D6,D7,D8\n")
        
        line = original.readline()
        while line:
            split = line.split(",")
            timestamp = split[0]
            id = split[2].split("x")[1].rjust(8, "0") # remove 0x, then pad to 8 chars with 0s
            extended = "false" # Always false
            bus = 1 if split[1] == "MS" else 0 # MS = 1, HS = 0
            len = split[3]

            lenNum = int(len)
            output = f"{timestamp},{id},{extended},{bus},{len}"
            for i in range(lenNum):
                # split[11] is b0, so go backwards
                output += f",{split[11 - i].split("x")[1].strip()}"
            output += "\n"

            copy.write(output)
            line = original.readline()
