import glob
from typing import TypedDict

import numpy as np
import matplotlib.pyplot as plt
import os
import csv
import seaborn as sns
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches

from os import listdir
from os.path import isfile, join

script_dir = os.path.dirname(os.path.realpath(__file__))


# # T110 Times
# # T110; type; name; number-of-calls; timing (rdtsc ticks)
# T110; 0; unknown; 0; 0
# T110; 1; BENCH_DEVICE_OPEN; 1; 15345448
# T110; 2; BENCH_DEVICE_CLOSE; 1; 110757096
# T110; 3; BENCH_DEVICE_IOCTL_QUERY; 4; 22908
# T110; 4; BENCH_DEVICE_IOCTL_MEMALLOC; 5; 1326616
# T110; 5; BENCH_DEVICE_IOCTL_FREE; 5; 729288
# T110; 6; BENCH_DEVICE_IOCTL_VIRTGET; 0; 0
# T110; 7; BENCH_DEVICE_IOCTL_SYNC; 198; 152722
# T110; 8; BENCH_DEVICE_IOCTL_TUNE; 1; 614502
# T110; 9; BENCH_DEVICE_IOCTL_HOST_TO_DEV; 4; 1855684
# T110; 10; BENCH_DEVICE_IOCTL_DEV_TO_HOST; 3; 2505660
# T110; 11; BENCH_DEVICE_IOCTL_LAUNCH; 198; 34874302
# T110; 12; BENCH_DEVICE_IOCTL_BARRIER; 5; 540444
# T110; 13; unknown; 0; 0
# T110; 14; unknown; 0; 0
# T111; total rdtsc ticks; 168724670
class Benchmark:
    def __init__(self, traces, name=''):
        self.traces: [GpuTrace] = traces
        self.name = name

    def get_n(self):
        return len(self.traces)


    def get_cycle_count(self):
        cycles = []
        cycle_map = {}
        count_map = {}
        for t in self.traces:
            cycles.append(t.total_cycles)

            for trace in t.t110_exec:
                tsc_key = 'rdtsc-' + trace.name
                num_key = 'num-' + trace.name
                if trace.name not in cycle_map:
                    cycle_map[tsc_key] = []
                cycle_map[tsc_key].append(trace.cycles)

                if trace.name not in count_map:
                    count_map[num_key] = []
                count_map[num_key].append(trace.exec_num)

        for k in cycle_map:
            cycle_map[k] = np.mean(cycle_map[k])

        for k in count_map:
            count_map[k] = np.mean(count_map[k])

        res = {'rdtsc-total': np.mean(cycles)}
        res.update(cycle_map)
        res.update(count_map)

        return res


    def get_exec_total_cycles(self):
        cycles = []
        for t in self.traces:
            cycles.append(t.total_cycles)

        return np.mean(cycles)

    def get_exec_cycles(self):
        data_cycles = dict()
        data_num = dict()
        for t in self.traces:
            for exec in t.t110_exec:
                key = exec.name
                cycles = exec.cycles
                num = exec.exec_num

                if not data_cycles.get(key):
                    data_cycles[key] = []
                if not data_num.get(key):
                    data_num[key] = []

                data_cycles[key].append(cycles)
                data_num[key].append(num)

        for key in data_cycles.keys():
            data_cycles[key] = np.median(data_cycles[key])

        for key in data_num.keys():
            data_num[key] = np.median(data_num[key])

        return data_cycles, data_num

class T110ExecTrace:
    name: str
    id: int
    exec_num: int
    cycles: int


class GpuTrace:
    total_cycles: int
    key: str
    def __init__(self):
        self.t110_exec: [T110ExecTrace] = []
        self.key = ''
        self.total_cycles = 0


# Sample trace
# # T110 Times
# # T110; type; name; number-of-calls; timing (rdtsc ticks)
# T110; 0; unknown; 0; 0
# T110; 1; BENCH_DEVICE_OPEN; 1; 16230656
# T110; 2; BENCH_DEVICE_CLOSE; 1; 86092032
# T110; 3; BENCH_DEVICE_IOCTL_QUERY; 4; 51608
# T110; 4; BENCH_DEVICE_IOCTL_MEMALLOC; 17; 3588584
# T110; 5; BENCH_DEVICE_IOCTL_FREE; 8; 986012
# T110; 6; BENCH_DEVICE_IOCTL_VIRTGET; 0; 0
# T110; 7; BENCH_DEVICE_IOCTL_SYNC; 204; 216496
# T110; 8; BENCH_DEVICE_IOCTL_TUNE; 1; 694892
# T110; 9; BENCH_DEVICE_IOCTL_HOST_TO_DEV; 12; 3967544
# T110; 10; BENCH_DEVICE_IOCTL_DEV_TO_HOST; 3; 2524140
# T110; 11; BENCH_DEVICE_IOCTL_LAUNCH; 204; 31940948
# T110; 12; BENCH_DEVICE_IOCTL_BARRIER; 14; 1562728
# T110; 13; unknown; 0; 0
# T110; 14; unknown; 0; 0
# T111; total rdtsc ticks; 147855640
def parse_gpu_trace(file):
    t110_exec = []
    key = file
    total_cycles = 0

    with open(file, 'r') as csvfile:
        plots = csv.reader(csvfile, delimiter=';')
        for row in plots:
            try:
                if len(row) == 0:
                    continue
                if str(row[0]).startswith("#"):
                    continue

                type = row[0]
                if type == "T110":
                    id = int(row[1].strip())
                    name = str(row[2].strip())
                    exec_num = int(row[3].strip())
                    cycles = int(row[4])

                    entry = T110ExecTrace()
                    entry.id = id
                    entry.cycles = cycles
                    entry.exec_num = exec_num
                    entry.name = name

                    t110_exec.append(entry)
                    pass

                elif type == "T111":
                    # T111', ' total rdtsc ticks', ' 152236572'
                    # dont need total explicitly
                    total_cycles = int(row[2].strip())
                    pass
                else:
                    print(f"ERROR: Unknown type in {row}")

            except Exception as e:
                print(f"skipping row {row}", e)
            pass

        trace = GpuTrace()
        trace.key = key
        trace.total_cycles = total_cycles
        trace.t110_exec = t110_exec
        return trace


def parse_set(name, folder_dir, file_ext=".txt"):
    if not os.path.exists(folder_dir):
        raise Exception(f"not found: {folder_dir}")

    print(glob.escape(folder_dir) + "*" + file_ext)
    files = glob.glob(glob.escape(folder_dir) + "*" + file_ext)
    print(files)

    traces = []
    for f in files:
        traces.append(parse_gpu_trace(f))

    b = Benchmark(traces, name)
    return b

def parse_file(name, file):
    traces = []
    traces.append(parse_gpu_trace(file))
    b = Benchmark(traces, name)
    return b


def plot_sample():
    import matplotlib.pyplot as plt
    import numpy as np

    devmem = parse_set('devmem', script_dir + '/data/gpu/armcca_gpu/devmem/')
    enc = parse_set('enc', script_dir + '/data/gpu/armcca_gpu/enc/')

    import matplotlib.pyplot as plt

    fig, ax = plt.subplots()

    operation = ['devmem', 'enc']
    counts = [devmem.get_exec_total_cycles(), enc.get_exec_total_cycles()]

    ax.bar(operation, counts)
    ax.set_ylabel('rtdsc')
    ax.set_title('Total cycle comparision gpu')

    plt.show()

if __name__ == '__main__':
    b = parse_file('test', '/home/b/2.5bay/mthesis-unsync/projects/trusted-periph/src/benchmarking/plot/data/sample/gpu_trace_2023-04-17_14-11-14.txt')
    print(b.get_cycle_count())