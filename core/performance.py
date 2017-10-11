from global_def import data,options
from xml.dom import minidom
import re, string
import numpy as np
from scipy import interpolate
import math
import sqlite3 as lite

class Performance:

	def __init__(self):

		self.__name = 'Default performance'

		self.__SB_OffDesignPoint_SameDelta = {}
		self.__PD_OffDesignPoint_SameDelta = {}

		self.__SB_OffDesignPoint_SameEta_U = {}
		self.__PD_OffDesignPoint_SameEta_U = {}
		self.__SB_OffDesignPoint_SameEta_L = {}
		self.__PD_OffDesignPoint_SameEta_L = {}


	def setAtlas(self, title):

		con = None
		try:
			con = lite.connect(options['resource-path']+'/propeller.db')
			cur = con.cursor()

			sql = 'select Context from Atlas where Name=\"%s\"' % title
			cur.execute(sql)
			result = cur.fetchone()

			f = open('atlas.xml', 'w')
			f.write(result[0])
			f.close()
			self.__load('atlas.xml')

		except lite.Error, e:
			print 'Error %s:' % e.args[0]

		finally:
			if con:
				con.close()

	def __load(self, filename):

		doc = minidom.parse(filename)
		root = doc.documentElement

		splitter = re.compile(r'[, ]+')

		for node in root.getElementsByTagName('design-point'):
			for curveNode in node.getElementsByTagName('p-curve'):
				param = curveNode.getAttribute('t')

				if param=='eta':

					strs = splitter.split(curveNode.getElementsByTagName('t')[0].firstChild.data)
					self.__ET_DesignPoint_ParamEta = self.__createArrayFromStrings(strs)

					strs = splitter.split(curveNode.getElementsByTagName('x')[0].firstChild.data)
					self.__SB_DesignPoint_ParamEta = self.__createArrayFromStrings(strs)

					strs = splitter.split(curveNode.getElementsByTagName('y')[0].firstChild.data)
					self.__PD_DesignPoint_ParamEta = self.__createArrayFromStrings(strs)

				elif param=='delta':

					strs = splitter.split(curveNode.getElementsByTagName('t')[0].firstChild.data)
					self.__DT_DesignPoint_ParamDelta = self.__createArrayFromStrings(strs)

					strs = splitter.split(curveNode.getElementsByTagName('x')[0].firstChild.data)
					self.__SB_DesignPoint_ParamDelta = self.__createArrayFromStrings(strs)

					strs = splitter.split(curveNode.getElementsByTagName('y')[0].firstChild.data)
					self.__PD_DesignPoint_ParamDelta = self.__createArrayFromStrings(strs)

		for node in root.getElementsByTagName('offdesign-point'):

			for curvesNode in node.getElementsByTagName('curves'):

				if curvesNode.getAttribute('key-var') == 'delta':

					for curveNode in curvesNode.getElementsByTagName('curve'):
						key = string.atof(curveNode.getAttribute('key'))

						strs = splitter.split(curveNode.getElementsByTagName('x')[0].firstChild.data)
						self.__SB_OffDesignPoint_SameDelta[key] = self.__createArrayFromStrings(strs)

						strs = splitter.split(curveNode.getElementsByTagName('y')[0].firstChild.data)
						self.__PD_OffDesignPoint_SameDelta[key] = self.__createArrayFromStrings(strs)

				elif curvesNode.getAttribute('key-var') == 'eta':

					if curvesNode.getAttribute('position') == 'upper':

						for curveNode in curvesNode.getElementsByTagName('curve'):
							key = string.atof(curveNode.getAttribute('key'))

							strs = splitter.split(curveNode.getElementsByTagName('x')[0].firstChild.data)
							self.__SB_OffDesignPoint_SameEta_U[key] = self.__createArrayFromStrings(strs)

							strs = splitter.split(curveNode.getElementsByTagName('y')[0].firstChild.data)
							self.__PD_OffDesignPoint_SameEta_U[key] = self.__createArrayFromStrings(strs)

					elif curvesNode.getAttribute('position') == 'lower':
						
						for curveNode in curvesNode.getElementsByTagName('curve'):
							key = string.atof(curveNode.getAttribute('key'))

							strs = splitter.split(curveNode.getElementsByTagName('x')[0].firstChild.data)
							self.__SB_OffDesignPoint_SameEta_L[key] = self.__createArrayFromStrings(strs)

							strs = splitter.split(curveNode.getElementsByTagName('y')[0].firstChild.data)
							self.__PD_OffDesignPoint_SameEta_L[key] = self.__createArrayFromStrings(strs)

	def __createArrayFromStrings(self, strs):
		array = np.zeros(len(strs))
		for i in range(0, len(strs)):
			array[i] = string.atof(strs[i])
		return array

	def design_atlas_bestEfficiency(self):

		# Grab used input data
		P = data.getValue('design-power')
		Vs = data.getValue('design-ship-speed')
		rpm = data.getValue('design-rotate-frequency')
		N = rpm / 60.0

		coef_wc = data.getValue('water-coefficient')
		coef_dt = data.getValue('thrust-deduction-coefficient')
		eta_rt = data.getValue('rotate-efficiency')
		eta_dt = data.getValue('drivetrain-efficiency')

		ro = data.getValue('water-density')

		# Power on propeller, in horse power
		Pd = P * 1360 * eta_rt * eta_dt

		# Advanced speed, in kts
		Va = Vs * (1-coef_wc)

		# Bp and sqrt(Bp)
		Bp = rpm * math.sqrt(Pd) / math.pow(Va,2.5)
		sqrtBp = math.sqrt(Bp)

		# Interpolate P/D, eta, delta by sqrt(Bp), default by MAU4-55

		interp_PD = interpolate.interp1d(self.__SB_DesignPoint_ParamEta, self.__PD_DesignPoint_ParamEta)
		interp_ET = interpolate.interp1d(self.__SB_DesignPoint_ParamEta, self.__ET_DesignPoint_ParamEta)
		interp_DT = interpolate.interp1d(self.__SB_DesignPoint_ParamDelta, self.__DT_DesignPoint_ParamDelta)

		delta = interp_DT(sqrtBp)
		eta   = interp_ET(sqrtBp)
		PD    = interp_PD(sqrtBp)

		J = 60.0 * 1.852 / 3.6 / delta
		sqrtKQ = Bp / 33.3 * math.pow(J,2.5) 
		KQ = sqrtKQ * sqrtKQ
		KT = eta * KQ * 2 * math.pi / J

		D = delta * Va / rpm

		# Save the results

		data.setValue('diameter', D)
		data.setValue('pitch', PD*D)
		data.setValue('pitch-ratio', PD)

		data.setValue('design-advance-speed', Va)
		data.setValue('design-advance-coefficient', 60.0 * 1.852 / 3.6 / delta )
		data.setValue('design-efficiency', eta)
		data.setValue('design-thrust-coefficient', KT)
		data.setValue('design-torque-coefficient', KQ)

		data.setValue('design-torque', KQ * ro * math.pow(N,2) * math.pow(D,5) / 1000 )
		data.setValue('design-thrust', KT * ro * math.pow(N,2) * math.pow(D,4) / 1000 )
		data.setValue('design-thrust-installed', KT * ro * math.pow(N,2) * math.pow(D,4) * (1-coef_dt) / 1000 )
		data.setValue('design-atlas-bp', Bp)

		return 0

	def design_atlas_fixDiameter(self):

		P = data.getValue('design-power')
		Vs = data.getValue('design-ship-speed')
		rpm = data.getValue('design-rotate-frequency')
		N = rpm / 60.0

		D = data.getValue('diameter')

		coef_wc = data.getValue('water-coefficient')
		coef_dt = data.getValue('thrust-deduction-coefficient')
		eta_rt = data.getValue('rotate-efficiency')
		eta_dt = data.getValue('drivetrain-efficiency')

		ro = data.getValue('water-density')

		# Power on propeller, in horse power
		Pd = P * 1360 * eta_rt * eta_dt

		# Advanced speed, in kts
		Va = Vs * (1-coef_wc)

		# Bp and sqrt(Bp)
		Bp = rpm * math.sqrt(Pd) / math.pow(Va,2.5)
		sqrtBp = math.sqrt(Bp)

		# delta
		delta = rpm * D / Va

        # Interpolate P/D by delta and sqrt(Bp), default by MAU4-55

		key = math.floor(delta/2) * 2

		try:

			self.__SB_OffDesignPoint_SameDelta

			ip0 = interpolate.interp1d(self.__SB_OffDesignPoint_SameDelta[key], self.__PD_OffDesignPoint_SameDelta[key])
			PD0 = ip0(sqrtBp)

			ip1 = interpolate.interp1d(self.__SB_OffDesignPoint_SameDelta[key+2], self.__PD_OffDesignPoint_SameDelta[key+2])
			PD1 = ip1(sqrtBp)

			fp = (delta-key)/2.0
			PD = (1-fp) * PD0 + fp * PD1            

		except:
			print "Error: failed to find atlas data for delta=%s\nExiting..." % delta
			return 1

		# Interpolate eta by delta and sqrt(Bp), default by MAU4-55

		ipo_pd = interpolate.interp1d(self.__SB_DesignPoint_ParamEta, self.__PD_DesignPoint_ParamEta)
		ipo_et = interpolate.interp1d(self.__SB_DesignPoint_ParamEta, self.__ET_DesignPoint_ParamEta)
		PD_opt = ipo_pd(sqrtBp)
		ET_opt = ipo_et(sqrtBp)

		if PD<PD_opt:
			keys = self.__SB_OffDesignPoint_SameEta_L.keys()
			keys.sort()
			kk = np.array([])
			yy = np.array([])
			for key in keys:
				try:
					ip_i = interpolate.interp1d(self.__SB_OffDesignPoint_SameEta_L[key], self.__PD_OffDesignPoint_SameEta_L[key])
					PD_i = ip_i(sqrtBp)
					kk = np.append(kk, [key])
					yy = np.append(yy, [PD_i])
				except:
					pass

			kk = np.append(kk, [ET_opt])
			yy = np.append(yy, [PD_opt])

			ip_eta = interpolate.interp1d(yy,kk)
			eta = ip_eta(PD)

		else:
			keys = self.__SB_OffDesignPoint_SameEta_U.keys()
			keys.sort()
			kk = np.array([])
			xx = np.array([])
			for key in keys:
				try:
					ip_i = interpolate.interp1d(self.__PD_OffDesignPoint_SameEta_U[key], self.__SB_OffDesignPoint_SameEta_U[key])
					SB_i = ip_i(PD)
					kk = np.append(kk, [key])
					xx = np.append(xx, [SB_i])
				except:
					pass

			ip_eta = interpolate.interp1d(xx,kk)
			eta = ip_eta(sqrtBp)

		# Finally calculation

		J = 60.0 * 1.852 / 3.6 / delta
		sqrtKQ = Bp / 33.3 * math.pow(J,2.5) 
		KQ = sqrtKQ * sqrtKQ
		KT = eta * KQ * 2 * math.pi / J

		# Save the results

		data.setValue('pitch', PD*D)
		data.setValue('pitch-ratio', PD)

		data.setValue('design-advance-speed', Va)
		data.setValue('design-advance-coefficient', 60.0 * 1.852 / 3.6 / delta )
		data.setValue('design-efficiency', eta)
		data.setValue('design-thrust-coefficient', KT)
		data.setValue('design-torque-coefficient', KQ)

		data.setValue('design-torque', KQ * ro * math.pow(N,2) * math.pow(D,5) / 1000 )
		data.setValue('design-thrust', KT * ro * math.pow(N,2) * math.pow(D,4) / 1000 )
		data.setValue('design-thrust-installed', KT * ro * math.pow(N,2) * math.pow(D,4) * (1-coef_dt) / 1000 )
		data.setValue('design-atlas-bp', Bp)

		return 0

	def estimate_atlas_offdesign(self):

		# Grab used input data
		D   = data.getValue('diameter')
		PD  = data.getValue('pitch-ratio')
		Vs  = data.getValue('offdesign-ship-speed')
		rpm = data.getValue('offdesign-rotate-frequency')
		N   = rpm / 60.0

		ro      = data.getValue('water-density')
		coef_wc = data.getValue('water-coefficient')
		coef_dt = data.getValue('thrust-deduction-coefficient')
		eta_rt  = data.getValue('rotate-efficiency')
		eta_dt  = data.getValue('drivetrain-efficiency')

		# Calculate delta
		Va = Vs * (1-coef_wc)
		delta = rpm * D / Va


		# Interpolate sqrt(Bp) by delta and P/D, default by MAU4-55

		key = math.floor(delta/2) * 2

		try:
			print PD
			print self.__PD_OffDesignPoint_SameDelta[key]
			print self.__SB_OffDesignPoint_SameDelta[key]

			ip0 = interpolate.interp1d(self.__PD_OffDesignPoint_SameDelta[key], self.__SB_OffDesignPoint_SameDelta[key])
			sqrtBp0 = ip0(PD)


			ip1 = interpolate.interp1d(self.__PD_OffDesignPoint_SameDelta[key+2], self.__SB_OffDesignPoint_SameDelta[key+2])
			sqrtBp1 = ip1(PD)

			fp = (delta-key)/2.0
			sqrtBp = (1-fp) * sqrtBp0 + fp * sqrtBp1 

		except:
			print "Error: failed to find atlas data for delta=%s\nExiting..." % delta
			print self.__PD_OffDesignPoint_SameDelta.keys()
			return 1

		Bp = sqrtBp * sqrtBp

		# Interpolate eta and P/D by sqrt(Bp), default by MAU4-55

		ipo_pd = interpolate.interp1d(self.__SB_DesignPoint_ParamEta, self.__PD_DesignPoint_ParamEta)
		ipo_et = interpolate.interp1d(self.__SB_DesignPoint_ParamEta, self.__ET_DesignPoint_ParamEta)
		PD_opt = ipo_pd(sqrtBp)
		ET_opt = ipo_et(sqrtBp)

		if PD<PD_opt:
			keys = self.__SB_OffDesignPoint_SameEta_L.keys()
			keys.sort()
			kk = np.array([])
			yy = np.array([])
			for key in keys:
				try:
					ip_i = interpolate.interp1d(self.__SB_OffDesignPoint_SameEta_L[key], self.__PD_OffDesignPoint_SameEta_L[key])
					PD_i = ip_i(sqrtBp)
					kk = np.append(kk, [key])
					yy = np.append(yy, [PD_i])
				except:
					pass

			kk = np.append(kk, [ET_opt])
			yy = np.append(yy, [PD_opt])

			ip_eta = interpolate.interp1d(yy,kk)
			eta = ip_eta(PD)

		else:
			keys = self.__SB_OffDesignPoint_SameEta_U.keys()
			keys.sort()
			kk = np.array([])
			xx = np.array([])
			for key in keys:
				try:
					ip_i = interpolate.interp1d(self.__PD_OffDesignPoint_SameEta_U[key], self.__SB_OffDesignPoint_SameEta_U[key])
					SB_i = ip_i(PD)
					kk = np.append(kk, [key])
					xx = np.append(xx, [SB_i])
				except:
					print "Failed for key=%s" % key

			print sqrtBp
			print 'xx=', xx
			print 'yy=', kk

			ip_eta = interpolate.interp1d(xx,kk)
			eta = ip_eta(sqrtBp)

		# Finally calculation

		sqrtPd = Bp * math.pow(Va,2.5) / rpm
		Pd = sqrtPd * sqrtPd
		Power = Pd / 1360 / eta_rt / eta_dt

		J = 60.0 * 1.852 / 3.6 / delta
		sqrtKQ = Bp / 33.3 * math.pow(J,2.5) 
		KQ = sqrtKQ * sqrtKQ
		KT = eta * KQ * 2 * math.pi / J

		# Save the results

		data.setValue('offdesign-power', Power)
		data.setValue('offdesign-advance-speed', Va)
		data.setValue('offdesign-advance-coefficient', J)
		data.setValue('offdesign-efficiency', eta)
		data.setValue('offdesign-thrust-coefficient', KT)
		data.setValue('offdesign-torque-coefficient', KQ)

		data.setValue('offdesign-torque', KQ * ro * math.pow(N,2) * math.pow(D,5) / 1000 )
		data.setValue('offdesign-thrust', KT * ro * math.pow(N,2) * math.pow(D,4) / 1000 )
		data.setValue('offdesign-thrust-installed', KT * ro * math.pow(N,2) * math.pow(D,4) * (1-coef_dt) / 1000 )
		data.setValue('offdesign-atlas-bp', Bp)

		return 0
