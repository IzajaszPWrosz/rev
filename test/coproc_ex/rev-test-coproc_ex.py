#
# Copyright (C) 2017-2023 Tactical Computing Laboratories, LLC
# All Rights Reserved
# contact@tactcomplabs.com
#
# See LICENSE in the top level directory for licensing details
#
# rev-test-ex1.py
#

import os
import sst

# Define SST core options
sst.setProgramOption("timebase", "1ps")
sst.setProgramOption("stopAtCycle", "0s")

# Tell SST what statistics handling we want
sst.setStatisticLoadLevel(4)

max_addr_gb = 1

# Define the simulation components
comp_cpu = sst.Component("cpu", "revcpu.RevCPU")
comp_cpu.addParams({
	"verbose" : 6,                                # Verbosity
        "numCores" : 1,                               # Number of cores
	"clock" : "1.0GHz",                           # Clock
        "memSize" : 1024*1024*1024,                   # Memory size in bytes
        "machine" : "[0:RV32I]",                      # Core:Config; RV32I for core 0
        "enableCoProc" : 1,
        "startAddr" : "[0:0x00000000]",               # Starting address for core 0
        "memCost" : "[0:1:10]",                       # Memory loads required 1-10 cycles
        "program" : os.getenv("REV_EXE", "ex1.exe"),  # Target executable
        "splash" : 1                                  # Display the splash message
})
comp_cpu.enableAllStatistics()

comp_coProc = comp_cpu.setSubComponent("co_proc", "revcpu.RevSimpleCoProc")
comp_coProc.addParams({
    "clock" : "1.0GHz",
    "verbose" : 6
    })

sst.setStatisticOutput("sst.statOutputCSV")
sst.enableAllStatisticsForAllComponents()

# EOF
