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


class T10IpCount(TypedDict):
    core: int
    count: int


class T15IpMarker(TypedDict):
    core: int
    count: int


class Benchmark:
    def __init__(self, traces, name=''):
        self.traces = traces
        self.name = name

    def get_n(self):
        return len(self.traces)

    def get_ip_count(self):
        # <core> = mean of core
        core_map = {}
        for trace in self.traces:
            for ip_count in trace.t10_ip_count:
                core = ip_count['core']

                core_count = core_map.get(core, 0)
                core_count += ip_count['count']
                core_map[core] = core_count

        # mean
        # XXX: Do we want mean over number of trace files
        # or number of entries per core?
        # currently: number of trace files (=n)
        for key in core_map.keys():
            core_map[key] /= self.get_n()

        return core_map

    def get_ip_marker(self):
        marker_map = {}

    def get_event_marker(self):
        event_map: dict[str, dict[int, int]] = {}  # [marker] = [core]=count
        for t in self.traces:
            for event in t.t20_event_marker:
                event_name = event['event']
                core = event['core']
                count = event['count']

                event_entry = event_map.get(event_name, {

                })  # [core] = count
                event_entry[core] = event_entry.get(core, 0) + count
                event_map[event_name] = event_entry

        return event_map


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

                    t20_event_marker.append({
                        'event': marker,
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


def parse_set(name, folder_dir, file_ext=".txt"):
    if not os.path.exists(folder_dir):
        raise Exception(f"not found: {folder_dir}")

    files = glob.glob(glob.escape(folder_dir) + "/**/*" + file_ext)

    traces = []
    for f in files:
        traces.append(parse_generic_trace(f))

    b = Benchmark(traces, name)
    return b


if __name__ == '__main__':
    bench = parse_set('test', script_dir + '/data')

    print(bench.get_n())
    print(bench.get_ip_count())
    print(bench.get_event_marker())
