import sys
import math
import numpy as np
from scipy import interpolate

from global_def import data,options
from geometry import Geometry
from performance import Performance

class Propeller:

	def __init__(self):

		self.__name = 'Default Propeller'

		self.geometry = Geometry()
		self.performance = Performance()

		self.__initMethodRepository()


	def __initMethodRepository(self):

		self.__methods = {}

		self.__methods['design_atlas_best']			= self.__design_atlas_bestEfficiency
		self.__methods['design_atlas_fixD'] 		= self.__design_atlas_fixDiameter
		self.__methods['atlas_offdesign'] 			= self.__estimate_atlas_offdesign
		self.__methods['blade_pitch'] 				= self.__blade_pitch
		self.__methods['blade_profile'] 			= self.__blade_profile
		self.__methods['blade_chord'] 				= self.__blade_chord
		self.__methods['blade_area'] 				= self.__blade_area
		self.__methods['show_outline'] 				= self.__show_outline
		self.__methods['show_geometry_feature'] 	= self.__show_geometry_feature
		self.__methods['show_hydrodynamic_feature'] = self.__show_hydrodynamic_feature
		self.__methods['generate_grip_code'] 		= self.__generate_grip_code
		self.__methods['export_to_stl'] 			= self.__export_to_stl


	def getInputAndOutputVarLabels(self, methods):

		inputs = []
		outputs = []

		for method in methods:
			inputs = inputs + self.__method_inputVarIDs[method]
			outputs = outputs + self.__method_outputVarIDs[method]

		return (inputs, outputs)

	def prepare(self):
		self.geometry.setProfile(options['blade-profile'])
		self.performance.setAtlas(options['atlas-title'])

	def estimate(self, method):
		print "<b>%s</b>" % method
		print "<p>%s</p>" % data.getMethodComment(method)
		sys.stdout.flush()
		return self.__methods[method]()

	def __design_atlas_bestEfficiency(self):
		return self.performance.design_atlas_bestEfficiency()

	def __design_atlas_fixDiameter(self):
		return self.performance.design_atlas_fixDiameter()

	def __estimate_atlas_offdesign(self):
		return self.performance.estimate_atlas_offdesign()

	def __blade_pitch(self):
		self.geometry.calcPitch()
		return 0

	def __blade_profile(self):
		self.geometry.setupGeometry()
		return 0

	def __blade_chord(self):
		self.geometry.calcChord()
		return 0

	def __blade_area(self):
		self.geometry.calcArea()
		return 0

	def __show_outline(self):
		self.geometry.showOutline()
		return 0

	def __show_geometry_feature(self):
		self.geometry.showGeometryFeature()
		return 0

	def __show_hydrodynamic_feature(self):
		self.geometry.showHydrodynamicFeature()
		return 0

	def __generate_grip_code(self):
		self.geometry.generateGripCode('blade.grs')
		return 0

	def __export_to_stl(self):
		self.geometry.exportBladeToSTL('blade.stl')
		return 0

