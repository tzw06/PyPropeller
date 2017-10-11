#!/usr/bin/python

import os,sys
import re

from propeller import Propeller
from global_def import data, options

inputfile = ""
outputfile = ""
respath = sys.path[0] + '/res'
curpath = sys.path[0] + '/../workspace'

options['resource-path'] = respath


try:
    i = sys.argv.index('-input')
    inputfile = sys.argv[i+1]
except:
    print "failed to assign input file"
    exit()

try:
    i = sys.argv.index('-output')
    outputfile = sys.argv[i+1]
except:
    print "failed to assign output file"
    exit()

data.open(inputfile)


propeller = Propeller()
propeller.prepare()
geometry = propeller.geometry



if '-batch' in sys.argv:
    print 'batch mode'
    output = open('log.txt', 'w')

    for diameter in [2.7,2.8,2.9,3.0,3.1,3.2,3.3,3.4,3.5,3.6,3.7]:
        print "diameter=%s" % diameter
        geometry.setValue('diameter', diameter)
        propeller.estimate('atlas_fixD')
        line = "%s\t%s\t%s\t%s\t%s\n" % (geometry.getValue('diameter'), geometry.getValue('pitchRatio'), requirement.getValue('efficiency'), requirement.getValue('thrust'), requirement.getValue('torque'))
        output.write(line)

    output.close()

else:

    methods = []
    if '-method' in sys.argv:
        i = sys.argv.index('-method')
        methods = data.readMethods(sys.argv[i+1])

    for method in methods:
        exitCode = propeller.estimate(method)
        if exitCode>0:
            sys.exit(exitCode)

data.save(inputfile,outputfile)
propeller.geometry.exportBladeToP3D('blade.xyz')
