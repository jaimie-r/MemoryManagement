#! /usr/bin/env python3
import subprocess
import statistics
import os
import math
from tabulate import tabulate

utilization_target = 75.00
performance_target = 1400

def get_num_ops(trace_file):
    f = open(trace_file, "r")
    num_ops = int(f.readlines()[1])
    return num_ops

def performance_check(trace_file):
    N = 20
    total_time = 0
    num_ops = get_num_ops(trace_file)
    for i in range(0, N):
        performance = subprocess.run(["./performance", trace_file], universal_newlines=True, stdout=subprocess.PIPE)
        if 'Success' not in performance.stdout:
            return -1
        total_time += int(performance.stdout.split()[1])
    return (num_ops / (total_time // N)) * 1000

def utilization_check(trace_file):
    utilization = subprocess.run(["./runner", '-ru', trace_file], universal_newlines=True, stdout=subprocess.PIPE,stderr=subprocess.PIPE)
    if utilization.returncode != 0:
        return -1
    return_array = utilization.stdout.split('\n')
    utilization_percentage = float(return_array[4].split()[3])
    return utilization_percentage

def correctness_check(trace_file):
    correctness = subprocess.run(["./runner", '-r', trace_file], universal_newlines=True, stdout=subprocess.PIPE,stderr=subprocess.PIPE)
    if correctness.returncode != 0:
        return False
    return 'umalloc package passed correctness check.' in correctness.stdout

trace_correctness = []
trace_utilization = []
trace_performance = []
table = []

def run_trace(trace_file):
    global trace_correctness
    global trace_utilization
    global trace_performance
    global table
    passed = correctness_check(trace_file)
    trace_correctness += [passed]
    util = -1
    perf = -1
    if passed:
        util = utilization_check(trace_file)
        perf = performance_check(trace_file)
        trace_utilization += [util]
        trace_performance += [perf]
    correct = 'Yes' if passed else 'No' 
    table += [[trace_file, correct, util, perf]]

os.system("make clean; make all")
for file in os.listdir("./traces"):
    if file.endswith(".rep") and 'short' not in file:
        run_trace(os.path.join("./traces", file))
utilization_average = sum(trace_utilization) / (1 if len(trace_utilization) == 0 else len(trace_utilization))
performance_average = sum(trace_performance) / (1 if len(trace_performance) == 0 else len(trace_performance))
correctness_average = sum(trace_correctness) / (1 if len(trace_correctness) == 0 else len(trace_correctness))
table += [["Average", "{:.2f}".format(correctness_average * 100), "{:.2f}".format(utilization_average), "{:.2f}".format(performance_average)]]
print (tabulate(table, headers=["Trace", "Passed", "Utilization", "Performance (Operations per millisecond)"]))

# Score calculation
utilization_score = 50 * (utilization_average / utilization_target)
if utilization_score < 35:
    print(f'Your utilization was {utilization_score:.2f}, which is below the minimum of 35 required to receive utilization credit.')
    utilization_score = 0
if utilization_score > 55:
    utilization_score = 55

performance_score = 20 * (performance_average / performance_target)
if performance_score > 25:
    performance_score = 25

correctness_average = sum(trace_correctness) / len(trace_correctness)
correctness_score = 15 * correctness_average
scale_factor = 1
if correctness_average < 1.0:
    scale_factor = 0
print (f"Correct: {correctness_score}/15, perf: {performance_score}/20, util: {utilization_score}/50")
print ("Score " + str(math.ceil(correctness_score + ((performance_score + utilization_score) * scale_factor))) + " / 85")
print ("The other fifteen points come from the style check and writeup after the assignment is turned in.")

