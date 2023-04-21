import glob
import pathlib
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


class T10IpCount(TypedDict):
    core: int
    count: int


class T15IpMarker(TypedDict):
    core: int
    count: int


def marker_translate(marker):
    if marker is not str:
        marker = str(marker)

    if marker == '0x1011': return 'init stop'
    if marker == '0x1012': return 'memalloc'
    if marker == '0x1013': return 'memalloc stop'
    if marker == '0x1014': return 'exec'
    if marker == '0x1015': return 'exec stop'
    if marker == '0x1016': return 'close'
    if marker == '0x1017': return 'close sotp'
    if marker == '0x1018': return 'h to d'
    if marker == '0x1019': return 'h to d stop'
    if marker == '0x101A': return 'd to h'
    if marker == '0x101B': return 'd to h stop'

    if marker == '0x01': return 'start bench'
    if marker == '0x02': return 'stop bench'
    if marker == '0x100': return 'dma read # pages'
    if marker == '0x101': return 'dma write # pages'
    if marker == '0x102': return 'mmap # pages'
    if marker == '0x103': return 'dma # page alloc'
    if marker == '0x104': return 'rsi call devmem delegate 1 page per call'

    return marker


class Benchmark:
    def __init__(self, traces, name=''):
        self.traces = traces
        self.name = name

    def get_n(self):
        return len(self.traces)

    def get_ip_count(self):
        core_map = {}
        for trace in self.traces:
            total = 0
            for ip_count in trace.t10_ip_count:
                core = 'instr-core-' + str(ip_count['core'])
                count = ip_count['count']

                if not core_map.get(core):
                    core_map[core] = []

                core_map[core].append(count)
                total += count

        for key in core_map.keys():
            core_map[key] = np.mean(core_map[key])

        # total of means of all cores
        total = 0
        for k in core_map.keys():
            total += core_map[k]

        core_map['instr-total'] = total
        return {self.name:
                    dict(sorted(core_map.items()))
                }

    def get_world_switches(self):
        core_map = {}
        total = {}
        for trace in self.traces:
            for ip_count in trace.t50_world_switch:
                core = ip_count['core']
                count = ip_count['count']
                cmd = ip_count['cmd']
                key = cmd + '-core-' + str(core)

                if not core_map.get(key):
                    core_map[key] = []
                if not total.get(cmd):
                    total[cmd] = []

                core_map[key].append(count)
                total[cmd].append(count)

        for key in core_map.keys():
            core_map[key] = np.mean(core_map[key])
        for key in total.keys():
            total[key] = np.mean(total[key])

        res = {
        }
        res.update(total)
        res.update(core_map)

        return {self.name:
                    dict(sorted(res.items()))
                }

    def get_exception_mode(self):
        total = {}
        for trace in self.traces:
            # 'mode': mode,
            # 'world': world,
            # 'core': core,
            # 'count': count
            for ip_count in trace.t40_mode_switch:
                core = ip_count['core']
                count = ip_count['count']
                world = ip_count['world'].strip()
                mode = ip_count['mode'].strip()

                total_key = world + '-' + mode

                if not total.get(total_key):
                    total[total_key] = []

                total[total_key].append(count)

        for key in total.keys():
            total[key] = np.mean(total[key])

        res = {}
        res.update(total)

        return {self.name:
                    dict(sorted(res.items()))
                }

    def get_ip_marker(self):
        marker_map = {}

    def print_marker_values(self):
        for t in self.traces:
            events = {}
            for event in t.t20_event_marker:
                core = event['core']
                name = event['event']
                count = event['count']

                if name not in events:
                    events[name] = 0

                events[name] += count

            print(t.key)
            for k in events.keys():
                print(f'\t{k}={events[k]}')


class GenericTrace:
    t10_ip_count: [T10IpCount]

    def __init__(self):
        self.t10_ip_count = []
        self.t15_ip_marker = []
        self.t20_event_marker = []
        self.t30_instr_distribution = []
        self.t40_mode_switch = []
        self.t50_world_switch = []
        self.key = ''


def parse_generic_trace(file):
    t10_ip_count = []
    t15_ip_marker = []
    t20_event_marker = []
    t30_instr_distribution = []
    t40_mode_switch = []
    t50_world_switch = []
    key = file

    def get_from_str(c, key):
        extract = c.replace(key, '').strip()
        assert extract != ''
        return int(extract)

    def get_count_from_str(c):
        return get_from_str(c, 'count')

    def get_core_from_str(c):
        return get_from_str(c, 'core')

    with open(file, 'r') as csvfile:
        plots = csv.reader(csvfile, delimiter=';')
        for row in plots:
            try:
                if len(row) == 0:
                    continue
                if str(row[0]).startswith("#"):
                    continue

                type = row[0]
                if type == "T03":
                    # file name
                    # T03; '/tmp/arm_cca_trace/trace-15-04-2023_22-41-41.txt'
                    pass
                elif type == "T10":
                    # T10; core 0; count 339472
                    core = get_core_from_str(row[1])
                    count = get_count_from_str(row[2])

                    t10_ip_count.append({
                        'core': core,
                        'count': count
                    })
                elif type == "T15":
                    # T15; core 2; 0x1010; count 1
                    core = get_core_from_str(row[1])
                    marker = row[2].strip()
                    count = get_count_from_str(row[3])

                    t15_ip_marker.append({
                        'core': core,
                        'marker': marker,
                        'count': count
                    })
                elif type == "T20":
                    # T20; core 2; 'MOV      xzr,#2'; count 1
                    core = get_core_from_str(row[1])
                    marker = str(row[2])
                    count = get_count_from_str(row[3])

                    # remove assembly from value
                    marker_value = marker.lower().replace(' ', '').replace('movxzr,#', '')

                    t20_event_marker.append({
                        'event': marker_value,
                        'core': core,
                        'count': count
                    })
                elif type == "T30":
                    # T30; core 0; inst LDSETA; count 1
                    core = get_core_from_str(row[1])
                    instr = str(row[2]).strip().replace('inst ', '')
                    count = get_count_from_str(row[3])

                    t30_instr_distribution.append({
                        'instr': instr,
                        'count': count,
                        'core': core
                    })
                elif type == "T40":
                    # T40; core 0; ns; EL0t_to_EL2h; 39
                    core = get_core_from_str(row[1])
                    world = str(row[2]).strip()
                    mode = str(row[3])
                    count = get_count_from_str(row[4])

                    t40_mode_switch.append({
                        'mode': mode,
                        'world': world,
                        'core': core,
                        'count': count
                    })
                    pass
                elif type == "T50":
                    # write_trace("T50; core %d; cmd %s; count %ld \n", core, cmd.c_str(), inst_count);
                    core = get_core_from_str(row[1])
                    cmd = str(row[2]).strip().replace('cmd ', '')
                    count = get_count_from_str(row[3])
                    t50_world_switch.append({
                        'core': core,
                        'cmd': cmd,
                        'count': count
                    })
                    pass
                else:
                    print(f"ERROR: Unknown type in {row}")

            except Exception as e:
                print(f"skipping row {row}", e)
            pass

        trace = GenericTrace()
        trace.t50_world_switch = t50_world_switch
        trace.t40_mode_switch = t40_mode_switch
        trace.t30_instr_distribution = t30_instr_distribution
        trace.t20_event_marker = t20_event_marker
        trace.t15_ip_marker = t15_ip_marker
        trace.t10_ip_count = t10_ip_count
        trace.key = key
        return trace


def _get_files(folder, suffix):
    res = []
    for subdir, dirs, files in os.walk(folder):
        for file in files:
            f = os.path.join(subdir, file)
            if f.endswith(suffix):
                res.append(f)
    return res


def parse_set(name, folder_dir, file_ext=".txt"):
    if not os.path.exists(folder_dir):
        raise Exception(f"not found: {folder_dir}")

    # files = glob.glob(glob.escape(folder_dir) + "/**/*" + file_ext)
    files = _get_files(folder_dir, file_ext)
    print(files)

    traces = []
    for f in files:
        traces.append(parse_generic_trace(f))

    b = Benchmark(traces, name)
    return b


def parse_single(name, path):
    t = parse_generic_trace(path)
    traces = [t]
    b = Benchmark(traces, name)
    return b


if __name__ == '__main__':
    enc = '/home/b/2.5bay/mthesis-unsync/projects/trusted-periph/assets/benchmarking/23-04-18_enccuda/'
    ns = '/home/b/2.5bay/mthesis-unsync/projects/trusted-periph/assets/benchmarking/23-04-19_ns/ns/arm_cca_trace/'
    devmem = '/home/b/2.5bay/mthesis-unsync/projects/trusted-periph/assets/benchmarking/devmem/arm_cca_trace/'

    print(parse_single('srad1 cpu/ns', ns + 'trace-19-04-2023_11-55-38_ns_srad1.txt').get_ip_count())
    print(parse_single('srad1 cpu/enc',enc + 'trace-19-04-2023_09-02-50_srad1_enc.txt').get_ip_count())
    print(parse_single('srad1 cpu/devmem',
                       devmem + 'trace-19-04-2023_17-27-52_devmem_srad1.txt').get_ip_count())

    print(parse_single('backprop cpu/ns', ns + 'trace-19-04-2023_11-58-23_ns_backprop.txt').get_ip_count())
    print(parse_single('backprop cpu/enc', enc + 'trace-19-04-2023_02-46-50.backprop_enc.txt').get_ip_count())
    print(parse_single('backprop cpu/devmem', devmem + 'trace-19-04-2023_17-50-21_devmem_backprop.txt').get_ip_count())



    print(parse_single('bfs cpu/ns', ns + 'trace-19-04-2023_11-59-16_ns_bfs.txt').get_ip_count())
    print(parse_single('bfs cpu/enc', enc + 'trace-19-04-2023_02-52-21.bfs_enc.txt').get_ip_count())
    print(parse_single('bfs cpu/devmem', devmem + 'trace-19-04-2023_17-52-54_devmem_bfs.txt').get_ip_count())




