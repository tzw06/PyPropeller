#!/usr/bin/python

import os,sys

from propeller import Propeller
from global_def import data, options

propeller = Propeller()

methods = []
if '-method' in sys.argv:
    i = sys.argv.index('-method')
    methods = data.readMethods(sys.argv[i+1])


(inputs, outputs) = propeller.getInputAndOutputVarLabels(methods)

f = open('vars.txt', 'w')

unusedVarIDs = data.getAllVarIDs()

inputVarIDs = []
for key in inputs:
    ID = data.getVarID(key)
    inputVarIDs.append(ID)
    if ID in unusedVarIDs:
        unusedVarIDs.remove(ID)

outputVarIDs = []
for key in outputs:
    ID = data.getVarID(key)
    outputVarIDs.append(ID)
    if ID in inputVarIDs:
        inputVarIDs.remove(ID)
    if ID in unusedVarIDs:
        unusedVarIDs.remove(ID)


f.write('input = ')
f.write(', '.join(inputVarIDs))
f.write('\n')

f.write('output = ')
f.write(', '.join(outputVarIDs))
f.write('\n')

f.write('unused = ')
f.write(', '.join(unusedVarIDs))
f.write('\n')

f.close()


