"""
TODO: describe this file
"""

import argparse
import re
import matplotlib.pyplot as plt

def main():
    args = parse_args()
    
    
    temps = []
    humidities = []
    
    def to_numbers(a,b):
        return (float(a),float(b))
    
    with open(args.filename, "r") as file:
        for line in file:
            time, log = re.match("\[([^[]*)\](.*)", line).groups()
            temp = re.search("SHT4X: (.*) Temp. \[C\]", log)
            humidity = re.search("SHT4X: (.*) RH. \[%\]", log)
            if temp: temps.append(to_numbers(time, temp.group(1)))
            if humidity: humidities.append(to_numbers(time, humidity.group(1)))
    
    print(len(temps))
    print(len(humidities))
    
    fig,ax1 = plt.subplots()
    ax1.set_xlabel('Time (s)')
    ax1.set_ylabel('Temperature [C]')
    ax1.plot(*zip(*temps), color='tab:red')
    
    ax2 = ax1.twinx()
    ax2.set_ylabel("Relative Humidity [%]")
    ax2.plot(*zip(*humidities), color='tab:blue')

    fig.tight_layout()
    plt.show()


def parse_args():
    description = None
    parser = argparse.ArgumentParser(description=description)
    parser.add_argument("filename", default="log.txt", nargs='?')

    args = parser.parse_args()
    return args

if __name__ == "__main__":
    main()
