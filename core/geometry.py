from vec3 import Vec3, vec3_minus, vec3_cross
from airfoil import Airfoil
from xml.etree.ElementTree import ElementTree,Element
import  xml.dom.minidom
import sqlite3 as lite

import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D

import os
import re
import math
import string

from global_def import data,options

class Geometry:

	def __init__(self):

		self.__expanded_area_coef = 1			# (Ae/A0)/(C/D)

		self.__profile_radius  = []				# 2*radius/D
		self.__profile_chord   = []				# chord / nominal chord
		self.__profile_pitch   = []				# pitch / nominal pitch
		self.__profile_skew    = []				# in degree
		self.__profile_rake    = []				# in degree
		self.__profile_airfoil = []				# title of airfoil

		self.__profile_thickness = []			# relative thickness of airfoil
		self.__profile_xmaxthick = []			# relative chordlength of airfoil

		self.__leading_edge_coords = []
		self.__trailing_edge_coords = []
		self.__blade_reference_line_coords = []

		self.__face_airfoil_coords = {}
		self.__back_airfoil_coords = {}

	def setProfile(self, title):
		self.__AirfoilSampleCount = 20

		con = None
		try:
			con = lite.connect(options['resource-path']+'/propeller.db')
			cur = con.cursor()

			sql = 'select Context from Blade where Name=\"%s\"' % title
			cur.execute(sql)
			result = cur.fetchone()

			f = open('blade.xml', 'w')
			f.write(result[0])
			f.close()
			self.__loadProfile('blade.xml')

		except lite.Error, e:
			print 'Error %s:' % e.args[0]

		finally:
			if con:
				con.close()

	def setupGeometry(self):
		Diameter = data.getValue('diameter')
		Chord = data.getValue('ref-chord')
		Pitch = data.getValue('pitch')
		AddPitchAngle = data.getValue('add-pitch-angle') * math.pi / 180.0

		rotateOrigin = Vec3(0,0,0)
		rotateAxis = Vec3(0,1,0)

		self.__leading_edge_coords = []
		self.__trailing_edge_coords = []
		self.__blade_reference_line_coords = []
		self.__face_airfoil_coords = {}
		self.__back_airfoil_coords = {}

		n = len(self.__profile_radius)
		for i in range(0,n):
			radius = self.__profile_radius[i] * Diameter/2.0
			chord  = self.__profile_chord[i] * Chord
			pitch  = self.__profile_pitch[i] * Pitch
			skew   = self.__profile_skew[i]
			rake   = self.__profile_rake[i]

			theta_r = rake*math.pi/180
			theta_s = skew*math.pi/180
			theta_p = math.atan(pitch/2/math.pi/radius) 
			psi = chord/radius

			theta_le = theta_s + psi/2*math.cos(theta_p)
			theta_te = theta_s - psi/2*math.cos(theta_p)

			x_m = - radius * math.tan(theta_r)
			x_le = x_m + chord/2*math.sin(theta_p)
			x_te = x_m - chord/2*math.sin(theta_p)

			vm = Vec3(x=x_m, radius=radius, theta=theta_s, coordinate='cylindrical')
			vm1 = vm.rotate(origin=rotateOrigin, axis=rotateAxis, theta=AddPitchAngle)

			self.__blade_reference_line_coords.append(Vec3(x=x_m, radius=radius, theta=theta_s, coordinate='cylindrical').rotate(origin=rotateOrigin,axis=rotateAxis,theta=AddPitchAngle))
			self.__leading_edge_coords.append(Vec3(x=x_le, radius=radius, theta=theta_le, coordinate='cylindrical').rotate(origin=rotateOrigin,axis=rotateAxis,theta=AddPitchAngle))
			self.__trailing_edge_coords.append(Vec3(x=x_te, radius=radius, theta=theta_te, coordinate='cylindrical').rotate(origin=rotateOrigin,axis=rotateAxis,theta=AddPitchAngle))

			airfoil = Airfoil(self.__profile_airfoil[i])

			Naf = self.__AirfoilSampleCount
			r = 1.2
			kk = np.linspace(0,Naf,Naf+1)
			t1 = (np.power(r,kk)-1)/(r-1)
			t2 = t1/np.max(t1)
			t = math.pi * (1-t2)
			# t = np.linspace(180,0,Naf+1) * math.pi / 180
			xi = (1+np.cos(t))/2.0
			(eta_u,eta_l) = airfoil.evaluate(xi)
			eta = [eta_u,eta_l]

			for ispace in range(0,2):
				airfoil_coords = []
				for j in range(0,Naf+1):
					p = xi[j]
					xc = (1-p) * x_le + p * x_te
					tc = (1-p) * theta_le + p * theta_te

					x = xc + eta[ispace][j]*chord*math.cos(theta_p)
					theta = tc - eta[ispace][j]*chord/radius*math.sin(theta_p)

					airfoil_coords.append(Vec3(x=x, radius=radius, theta=theta, coordinate='cylindrical').rotate(origin=rotateOrigin,axis=rotateAxis,theta=AddPitchAngle))

				if ispace==0:
					self.__face_airfoil_coords[radius] = airfoil_coords
				else:
					self.__back_airfoil_coords[radius] = airfoil_coords
	
	def calcPitch(self):
		Diameter = data.getValue('diameter')
		PitchRatio = data.getValue('pitch-ratio')

		data.setValue('pitch', Diameter*PitchRatio)

	def calcChord(self):
		Diameter = data.getValue('diameter')
		Ae_A0 = data.getValue('expanded-ratio')
		C = Ae_A0 * Diameter / self.__expanded_area_coef
		data.setValue('ref-chord', C)

	def calcArea(self):
		Diameter = data.getValue('diameter')
		Chord = data.getValue('ref-chord')
		Nblade = data.getValue('blade-count')

		# Disc area
		A0 = 0.25*math.pi*Diameter*Diameter
		
		# Projected outline area
		Ap = 0
		nsec = len(self.__leading_edge_coords)
		for isec in range(1,nsec):
			V_LE0 = self.__leading_edge_coords[isec-1]
			V_LE1 = self.__leading_edge_coords[isec]
			V_TE0 = self.__trailing_edge_coords[isec-1]
			V_TE1 = self.__trailing_edge_coords[isec]
			dtheta = 0.5 * ( math.fabs(V_LE0.getCylinderTheta()-V_TE0.getCylinderTheta()) + math.fabs(V_LE1.getCylinderTheta()-V_TE1.getCylinderTheta()) )
			dr = math.fabs(V_LE1.getCylinderR() - V_LE0.getCylinderR())
			r = 0.5 * (V_LE1.getCylinderR()+V_LE0.getCylinderR())
			Ap = Ap + r*dtheta*dr
		Ap = Ap * Nblade

		# Expanded outline area
		Ae = 0
		for isec in range(1,nsec):
			r1 = self.__profile_radius[isec] * Diameter/2.0
			r0 = self.__profile_radius[isec-1] * Diameter/2.0
			c1 = self.__profile_chord[isec] * Chord
			c0 = self.__profile_chord[isec-1] * Chord
			Ae = Ae + 0.5 * (c1+c0) * math.fabs(r1-r0)
		Ae = Ae * Nblade

		# Save the results
		data.setValue('area-disc', A0)
		data.setValue('area-projected', Ap)
		data.setValue('area-expanded', Ae)
		data.setValue('projected-ratio', Ap/A0)

	def showBlade(self):
		curves = {}
		curves['leading edge'] = self.__leading_edge_coords
		curves['trailing edge'] = self.__trailing_edge_coords

		for radius in self.__face_airfoil_coords:
			curves["face airfoil@r=%s" % radius] = self.__face_airfoil_coords[radius]

		for radius in self.__back_airfoil_coords:
			curves["back airfoil@r=%s" % radius] = self.__back_airfoil_coords[radius]

		self.draw3dCurves(curves, title="blade shape")

	def exportBlade(self):

		curves = {}
		curves['leading edge'] = self.__leading_edge_coords
		curves['trailing edge'] = self.__trailing_edge_coords

		for radius in self.__face_airfoil_coords:
			curves["face airfoil@r=%s" % radius] = self.__face_airfoil_coords[radius]

		for radius in self.__back_airfoil_coords:
			curves["back airfoil@r=%s" % radius] = self.__back_airfoil_coords[radius]

		self.export3dCurves(curves, 'blade')

	def exportBladeToP3D(self, filename):

		Nblade = int(data.getValue('blade-count'))
		Nblock = 2*Nblade + 4

		f = open(filename, 'w')
		f.write('%s\n' % Nblock )
		M = len(self.__leading_edge_coords)
		N = self.__AirfoilSampleCount + 1

		for iblade in range(0,Nblade):
			f.write('%s,%s,1\n' % (N,M))
			f.write('%s,%s,1\n' % (N,M))

		f.write('%s,%s,1\n' % (37,11))
		f.write('%s,%s,1\n' % (37,11))
		f.write('%s,%s,1\n' % (37,11))
		f.write('%s,%s,1\n' % (37,11))

		for iblade in range(0,Nblade):

			for ispace in range(0,2):

				if ispace==0:
					airfoils = self.__face_airfoil_coords
				else:
					airfoils = self.__back_airfoil_coords

				radius = airfoils.keys()
				radius.sort()

				strs = []
				for r in radius:
					af = airfoils[r]
					for v in af:
						strs.append('%s' % v.x())
				line = ','.join(strs)
				f.write(line+'\n')

				strs = []
				for r in radius:
					af = airfoils[r]
					for v in af:
						y = v.y() * math.cos(math.pi/2*iblade) - v.z() * math.sin(math.pi/2*iblade)
						strs.append('%s' % y)
				line = ','.join(strs)
				f.write(line+'\n')

				strs = []
				for r in radius:
					af = airfoils[r]
					for v in af:
						z = v.y() * math.sin(math.pi/2*iblade) + v.z() * math.cos(math.pi/2*iblade)
						strs.append('%s' % z)
				line = ','.join(strs)
				f.write(line+'\n')


		Diameter = data.getValue('diameter')
		alpha = data.getValue('hub-cone-angle') * math.pi / 180.0

		for ispace in range(0,2):

			strs_x = []
			strs_y = []
			strs_z = []
			for isec in range(0,11):

				p = isec/10.0
				x = data.getValue('hub-length') * (0.5-p)

				if ispace==0:
					d = data.getValue('hub-diameter-ratio') * Diameter - x * math.tan(alpha/2) * 2
				else:
					d = data.getValue('shaft-diameter-ratio') * Diameter

				for ia in range(0,37):
					theta = math.pi * ia / 18.0
					y = d/2.0 * math.cos(theta)
					z = d/2.0 * math.sin(theta)

					strs_x.append('%s' % x)
					strs_y.append('%s' % y)
					strs_z.append('%s' % z)

			f.write(','.join(strs_x)+'\n')
			f.write(','.join(strs_y)+'\n')
			f.write(','.join(strs_z)+'\n')

		for ispace in range(0,2):

			strs_x = []
			strs_y = []
			strs_z = []

			for isec in range(0,11):

				if ispace==0:
					x = data.getValue('hub-length') * 0.5
				else:
					x = data.getValue('hub-length') * (-0.5)

				p = isec/10.0
				d = data.getValue('shaft-diameter-ratio') * Diameter * (1-p) + ( data.getValue('hub-diameter-ratio') * Diameter - x * math.tan(alpha/2) * 2 ) * p

				for ia in range(0,37):
					theta = math.pi * ia / 18.0
					y = d/2.0 * math.cos(theta)
					z = d/2.0 * math.sin(theta)

					strs_x.append('%s' % x)
					strs_y.append('%s' % y)
					strs_z.append('%s' % z)

			f.write(','.join(strs_x)+'\n')
			f.write(','.join(strs_y)+'\n')
			f.write(','.join(strs_z)+'\n')

		f.close()


	def exportBladeToSTL(self, filename):

		f = open(filename, 'w')
		f.write('solid blade\n')

		M = len(self.__leading_edge_coords)
		N = self.__AirfoilSampleCount

		for ispace in range(0,2):
			
			if ispace==0:
				airfoils = self.__face_airfoil_coords
			else:
				airfoils = self.__back_airfoil_coords

			radius = airfoils.keys()
			radius.sort()

			M = len(radius)

			for m in range(1,M):
				af0 = airfoils[radius[m-1]]
				af1 = airfoils[radius[m]]
				for n in range(1,N):
					v00 = af0[n-1]
					v01 = af0[n]
					v10 = af1[n-1]
					v11 = af1[n]

					for k in range(0,2):
						if k==0:
							a = v00
							b = v11
							c = v01
						else:
							a = v00
							b = v10
							c = v11

						d1 = vec3_minus(b,a)
						d2 = vec3_minus(c,a)
						n = vec3_cross(d1,d2)
						n.normalize()

						f.write('facet normal %s %s %s\n' % (n.x(), n.y(), n.z()))
						f.write('\touter loop\n')
						f.write('\t\tvertex %s %s %s\n' % (a.x(), a.y(), a.z()))
						f.write('\t\tvertex %s %s %s\n' % (b.x(), b.y(), b.z()))
						f.write('\t\tvertex %s %s %s\n' % (c.x(), c.y(), c.z()))
						f.write('\tendloop\n')
						f.write('endfacet\n')

		f.write('endsolid blade\n')

		f.close()

	def draw3dCurves(self, curves, title):

		fig = plt.figure()
		ax = fig.gca(projection='3d', adjustable='box')
		ax.set_title(title)
		ax.set_xlabel('x')
		ax.set_ylabel('y')
		ax.set_zlabel('z')
		ax.set_aspect('equal')

		minXs = []	
		maxXs = []
		minYs = []	
		maxYs = []
		minZs = []	
		maxZs = []

		for key in curves:
			xx = []
			yy = []
			zz = []
			points = curves[key]
			for vec in points:
				xx.append(vec.x())
				yy.append(vec.y())
				zz.append(vec.z())

			figure = ax.plot(xx,yy,zz, label=key)

			minXs.append(min(xx))
			maxXs.append(max(xx))
			minYs.append(min(yy))
			maxYs.append(max(yy))
			minZs.append(min(zz))
			maxZs.append(max(zz))

		minX = min(minXs)
		maxX = max(maxXs)
		minY = min(minYs)
		maxY = max(maxYs)
		minZ = min(minZs)
		maxZ = max(maxZs)

		midX = (minX+maxX)/2.0
		midY = (minY+maxY)/2.0
		midZ = (minZ+maxZ)/2.0
		
		radius = math.sqrt((maxX-minX)*(maxX-minX)+(maxY-minY)*(maxY-minY)+(maxZ-minZ)*(maxZ-minZ))/2.0

		ax.set_xlim3d(midX-radius, minX+radius)
		ax.set_ylim3d(midY-radius, minY+radius)
		ax.set_zlim3d(midZ-radius, minZ+radius)

		#plt.legend(loc='upper left')
		plt.show()

	def export3dCurves(self, curves, dirname):
		if not os.path.exists(dirname):
			os.mkdir(dirname)

		for key in curves:
			f = open(dirname+'/'+key+'.dat', 'w')
			points = curves[key]
			for vec in points:
				line = "%s\t%s\t%s\n" % (vec.x(), vec.y(), vec.z())
				f.write(line)
			f.close()

	# For blunt tip propeller
	def generateGripCode(self, filename):

		f = open(filename, 'w')

		# Declarition

		line = 'ENTITY/P_LE(%s)\n' % (len(self.__leading_edge_coords))
		f.write(line)
		line = 'ENTITY/P_TE(%s)\n' % (len(self.__trailing_edge_coords))
		f.write(line)
		line = 'ENTITY/SPLNEG(2)\n'
		f.write(line)
		line = 'ENTITY/SURF(4)\n'
		f.write(line)
		line = 'ENTITY/BLADE1\n'
		f.write(line)

		i = 1
		for radius in self.__face_airfoil_coords:
			line = 'ENTITY/PR%sU(%s)\n' % (i, len(self.__face_airfoil_coords[radius]))
			f.write(line)
			line = 'ENTITY/PR%sL(%s)\n' % (i, len(self.__back_airfoil_coords[radius]))
			f.write(line)
			i = i + 1

		line = "ENTITY/SPLNRU(%s),SPLNRL(%s)\n" % (len(self.__face_airfoil_coords), len(self.__back_airfoil_coords))
		f.write(line)

		# Leading & trailing edge 

		i = 1
		for vec in self.__leading_edge_coords:
			line = 'P_LE(%s) = POINT/%s,%s,%s\n' % (i, vec.x()*1000, vec.y()*1000, vec.z()*1000)
			f.write(line)
			i = i + 1

		i = 1
		for vec in self.__trailing_edge_coords:
			line = 'P_TE(%s) = POINT/%s,%s,%s\n' % (i, vec.x()*1000, vec.y()*1000, vec.z()*1000)
			f.write(line)
			i = i + 1

		line = 'SPLNEG(1) = SPLINE/P_LE\n'
		f.write(line)
		line = 'SPLNEG(2) = SPLINE/P_TE\n'
		f.write(line)

		# RADIUS

		rs = self.__face_airfoil_coords.keys()
		rs.sort()

		i = 1
		for radius in rs:
			curve_u = self.__face_airfoil_coords[radius]
			curve_l = self.__back_airfoil_coords[radius]
			j = 1
			for vec in curve_u:
				line = 'PR%sU(%s) = POINT/%s,%s,%s\n' % (i, j, vec.x()*1000, vec.y()*1000, vec.z()*1000)
				f.write(line)
				j = j + 1
			j = 1
			for vec in curve_l:
				line = 'PR%sL(%s) = POINT/%s,%s,%s\n' % (i, j, vec.x()*1000, vec.y()*1000, vec.z()*1000)
				f.write(line)
				j = j + 1

			line = 'SPLNRU(%s) = SPLINE/PR%sU\n' % (i,i)
			f.write(line)
			line = 'SPLNRL(%s) = SPLINE/PR%sL\n' % (i,i)
			f.write(line)

			i = i + 1

		# SURFACES

		f.write('SURF(1) = BSURF/MESH,SPLNRU,WITH,SPLNEG\n')
		f.write('SURF(2) = BSURF/MESH,SPLNRL,WITH,SPLNEG\n')
		f.write('SURF(3) = RLDSRF/SPLNRU(1),,SPLNRL(1)\n')
		f.write('SURF(4) = RLDSRF/SPLNRU(%s),,SPLNRL(%s)\n' % (len(self.__face_airfoil_coords), len(self.__back_airfoil_coords)))

		# BODIES

		f.write('BLADE1 = SEW/SURF\n')

		f.write('HALT\n')
		f.close()

	# For sharp tip propeller
	def generateGripCode0(self, filename):

		f = open(filename, 'w')

		# Declarition

		line = 'ENTITY/P_LE(%s)\n' % (len(self.__leading_edge_coords))
		f.write(line)
		line = 'ENTITY/P_TE(%s)\n' % (len(self.__trailing_edge_coords))
		f.write(line)
		line = 'ENTITY/SPLNEG(2)\n'
		f.write(line)
		line = 'ENTITY/SURF(3)\n'
		f.write(line)
		line = 'ENTITY/BLADE1\n'
		f.write(line)

		i = 1
		for radius in self.__face_airfoil_coords:
			line = 'ENTITY/PR%sU(%s)\n' % (i, len(self.__face_airfoil_coords[radius]))
			f.write(line)
			line = 'ENTITY/PR%sL(%s)\n' % (i, len(self.__back_airfoil_coords[radius]))
			f.write(line)
			i = i + 1

		line = "ENTITY/SPLNRU(%s),SPLNRL(%s)\n" % (len(self.__face_airfoil_coords), len(self.__back_airfoil_coords))
		f.write(line)

		# Leading & trailing edge 

		i = 1
		for vec in self.__leading_edge_coords:
			line = 'P_LE(%s) = POINT/%s,%s,%s\n' % (i, vec.x()*1000, vec.y()*1000, vec.z()*1000)
			f.write(line)
			i = i + 1

		i = 1
		for vec in self.__trailing_edge_coords:
			line = 'P_TE(%s) = POINT/%s,%s,%s\n' % (i, vec.x()*1000, vec.y()*1000, vec.z()*1000)
			f.write(line)
			i = i + 1

		line = 'SPLNEG(1) = SPLINE/P_LE\n'
		f.write(line)
		line = 'SPLNEG(2) = SPLINE/P_TE\n'
		f.write(line)

		# RADIUS

		rs = self.__face_airfoil_coords.keys()
		rs.sort()

		i = 1
		for radius in rs:
			curve_u = self.__face_airfoil_coords[radius]
			curve_l = self.__back_airfoil_coords[radius]
			if i<len(rs):
				j = 1
				for vec in curve_u:
					line = 'PR%sU(%s) = POINT/%s,%s,%s\n' % (i, j, vec.x()*1000, vec.y()*1000, vec.z()*1000)
					f.write(line)
					j = j + 1
				j = 1
				for vec in curve_l:
					line = 'PR%sL(%s) = POINT/%s,%s,%s\n' % (i, j, vec.x()*1000, vec.y()*1000, vec.z()*1000)
					f.write(line)
					j = j + 1
			else:
				for j in range(0,len(curve_u)):
					vec_u = curve_u[j]
					vec_l = curve_l[j]
					line = 'PR%sU(%s) = POINT/%s,%s,%s\n' % (i, j+1, (vec_u.x()+vec_l.x())*500, (vec_u.y()+vec_l.y())*500, (vec_u.z()+vec_l.z())*500)
					f.write(line)
				for j in range(0,len(curve_u)):
					vec_u = curve_u[j]
					vec_l = curve_l[j]
					line = 'PR%sL(%s) = POINT/%s,%s,%s\n' % (i, j+1, (vec_u.x()+vec_l.x())*500, (vec_u.y()+vec_l.y())*500, (vec_u.z()+vec_l.z())*500)
					f.write(line)

			line = 'SPLNRU(%s) = SPLINE/PR%sU\n' % (i,i)
			f.write(line)
			line = 'SPLNRL(%s) = SPLINE/PR%sL\n' % (i,i)
			f.write(line)

			i = i + 1

		# SURFACES

		f.write('SURF(1) = BSURF/MESH,SPLNRU,WITH,SPLNEG\n')
		f.write('SURF(2) = BSURF/MESH,SPLNRL,WITH,SPLNEG\n')
		f.write('SURF(3) = RLDSRF/SPLNRU(1),,SPLNRL(1)\n')

		# BODIES

		f.write('BLADE1 = SEW/SURF\n')

		f.write('HALT\n')
		f.close()

	def showOutline(self):
		Diameter = data.getValue('diameter')
		Chord = data.getValue('ref-chord')
		
		hdr = data.getValue('hub-diameter-ratio')
		hl  = data.getValue('hub-length')
		hca = data.getValue('hub-cone-angle') * math.pi / 180.0

		hd = Diameter * hdr

		# TRANSVERSE VIEW
		#-------------------------------------------

		# Projected outline
		xi = []
		et = []
		for vec in self.__leading_edge_coords:
			xi.append(-vec.z())
			et.append(vec.y())
		for vec in self.__trailing_edge_coords:
			xi.insert(0, -vec.z())
			et.insert(0, vec.y())
		plt.plot(xi, et, 'b', linewidth=2.0, label='Projected outline')

		xi = []
		et = []
		for vec in self.__blade_reference_line_coords:
			xi.append(-vec.z())
			et.append(vec.y())
		plt.plot(xi, et, 'b--')

		nsec = len(self.__blade_reference_line_coords)
		for radius in self.__face_airfoil_coords:
			xi = []
			et = []
			airfoil = self.__face_airfoil_coords[radius]
			for vec in airfoil:
				xi.append(-vec.z())
				et.append(vec.y())
			airfoil = self.__back_airfoil_coords[radius]
			for vec in airfoil:
				xi.insert(0,-vec.z())
				et.insert(0,vec.y())
			plt.plot(xi, et, 'b--')

		# Developed outline
		xi = []
		et = []
		nsec = len(self.__blade_reference_line_coords)
		
		for ispace in range(0,2):
			for i in range(0,nsec):
				vec_m = self.__blade_reference_line_coords[i]
				if ispace==0:
					vec_e = self.__leading_edge_coords[i]
				else:
					vec_e = self.__trailing_edge_coords[i]

				dx = vec_e.x() - vec_m.x()
				dz = vec_e.z() - vec_m.z()
				dl = math.sqrt(dx*dx+dz*dz)

				if ispace==0:
					xi.append(-vec_m.z()-dl)
					et.append(vec_e.y())
				else:
					xi.insert(0,-vec_m.z()+dl)
					et.insert(0,vec_e.y())

		plt.plot(xi, et, 'b--',  label='Developed outline')

		# Auxillary lines

		r0 = Diameter / 2.0 * hdr - math.tan(hca/2) * hl/2.0
		r1 = Diameter / 2.0 * hdr + math.tan(hca/2) * hl/2.0
		t = np.linspace(0,360,37) * math.pi / 180
		plt.plot(r0*np.cos(t), r0*np.sin(t), 'b', lw=2.0)
		plt.plot(r1*np.cos(t), r1*np.sin(t), 'b', lw=2.0)

		xi = [-r1*1.2, r1*1.2]
		et = [0,0]
		plt.plot(xi,et, 'b--', lw=0.75)

		xi = [0,0]
		et = [-r1*1.2, Diameter/2.0+r1*0.2]
		plt.plot(xi,et, 'b--', lw=0.75)

		# EXPANDED VIEW
		#-------------------------------------------
		offset_X = Diameter * 0.5

		# Expanded outline
		xi = []
		et = []
		for vec in self.__blade_reference_line_coords:
			xi.append(offset_X-vec.z())
			et.append(vec.getCylinderR())
		plt.plot(xi, et, 'b--', label='Blade reference line')

		xi = []
		et = []
		nsec = len(self.__blade_reference_line_coords)
		for ispace in range(0,2):
			for i in range(0,nsec):
				vec_m = self.__blade_reference_line_coords[i]
				chord = self.__profile_chord[i] * Chord
				if ispace==0:
					xi.append(offset_X-vec_m.z()-chord/2.0)
					et.append(vec_m.getCylinderR())
				else:
					xi.insert(0, offset_X-vec_m.z()+chord/2.0)
					et.insert(0, vec_m.getCylinderR())
		plt.plot(xi, et, 'b--', label='Expanded outline')

		xi = []
		et = []
		for isec in range(0,nsec):
			vec_m = self.__blade_reference_line_coords[isec]
			chord = self.__profile_chord[isec] * Chord
			xmaxt = self.__profile_xmaxthick[isec] * chord
			xi.append(offset_X-vec_m.z()-chord/2.0+xmaxt)
			et.append(vec_m.getCylinderR())
		plt.plot(xi,et, 'b--')

		# Airfoils at extended outline plot
		nsec = len(self.__blade_reference_line_coords)
		for i in range(0,nsec):
			xi = []
			et = []

			vec_m = self.__blade_reference_line_coords[i]
			chord = self.__profile_chord[i] * Chord
			airfoil = Airfoil(self.__profile_airfoil[i])

			Naf = self.__AirfoilSampleCount
			t = np.linspace(180,0,Naf+1) * math.pi / 180
			xx = (1+np.cos(t))/2.0
			(eta_u,eta_l) = airfoil.evaluate(xx)
			eta = [eta_u,eta_l]

			x0 = offset_X-vec_m.z()-chord/2.0
			y0 = vec_m.getCylinderR()
			for ispace in range(0,2):
				for k in range(0,Naf):
					if ispace==0:
						xi.append(x0+chord*xx[k])
						et.append(y0+chord*eta[ispace][k])
					else:
						xi.insert(0, x0+chord*xx[k])
						et.insert(0, y0+chord*eta[ispace][k])
			plt.plot(xi,et, 'b', lw=2.0)

		# PROFILE VIEW
		#-------------------------------------------
		offset_X = -0.5 * Diameter

		# Side view
		nsec = len(self.__blade_reference_line_coords)
		xi = []
		et = []
		for vec in self.__blade_reference_line_coords:
			xi.append(offset_X-vec.x())
			et.append(vec.y())
		plt.plot(xi, et, 'b', lw=2.0, label='Blade reference line')

		xi = []
		et = []
		for vec in self.__leading_edge_coords:
			xi.append(offset_X-vec.x())
			et.append(vec.y())
		for vec in self.__trailing_edge_coords:
			xi.insert(0, offset_X-vec.x())
			et.insert(0, vec.y())
		plt.plot(xi, et, 'b--')

		# Auxillary lines

		# Hub
		r0 = Diameter / 2.0 * hdr - math.tan(hca/2) * hl/2.0
		r1 = Diameter / 2.0 * hdr + math.tan(hca/2) * hl/2.0

		xi = [offset_X+hl/2, offset_X+hl/2, offset_X-hl/2, offset_X-hl/2, offset_X+hl/2]
		et = [r1, -r1, -r0, r0, r1]
		plt.plot(xi,et, 'b', lw=2.0)

		xi = [offset_X-hl/2*1.2, offset_X+hl/2*1.2]
		et = [0,0]
		plt.plot(xi,et, 'b--', lw=0.75)

		# GLOBAL SETTINGS
		#-------------------------------------------
		plt.gca().set_aspect('equal', adjustable='box')
		#plt.legend(loc='upper left')
		plt.show()

	def showGeometryFeature(self):

		ax = plt.subplot('221')
		ax.set_xlabel('$r/R$')
		ax.set_ylabel('$C/C_0$')
		ax.plot(self.__profile_radius, self.__profile_chord)

		ax = plt.subplot('222')
		ax.set_xlabel('$r/R$')
		ax.set_ylabel('$P/P_0$')
		ax.plot(self.__profile_radius, self.__profile_pitch)

		ax = plt.subplot('223')
		ax.set_xlabel('$r/R$')
		ax.set_ylabel('$\\theta_s (^\circ)$')
		ax.plot(self.__profile_radius, self.__profile_skew)

		ax = plt.subplot('224')
		ax.set_xlabel('$r/R$')
		ax.set_ylabel('$\\theta_r (^\circ)$')
		ax.plot(self.__profile_radius, self.__profile_rake)

		plt.show()

	def showHydrodynamicFeature(self):

		Va = data.getValue('offdesign-advance-speed') * 1.852 / 3.6		# in m/s
		N = data.getValue('offdesign-rotate-frequency') / 60.0			# in rev/s

		Diameter = data.getValue('diameter')
		Chord = data.getValue('ref-chord')
		Pitch = data.getValue('pitch')
		AddPitchAngle = data.getValue('add-pitch-angle') * math.pi / 180.0

		AoAs = []
		nsec = len(self.__profile_radius)
		for isec in range(0,nsec):
			radius = self.__profile_radius[isec] * Diameter / 2.0
			pitch = self.__profile_pitch[isec] * Pitch

			theta_p = math.atan(pitch/2/math.pi/radius) + AddPitchAngle
			theta_v = math.atan(Va/2/math.pi/N/radius)

			aoa = (theta_p - theta_v) * 180.0 / math.pi
			AoAs.append(aoa)

		plt.plot(self.__profile_radius, AoAs)
		plt.show()

	def __loadProfile(self, filename):
		self.__profile_radius  = []
		self.__profile_chord   = []
		self.__profile_pitch   = []
		self.__profile_skew    = []
		self.__profile_rake    = []
		self.__profile_airfoil = []

		dom = xml.dom.minidom.parse(filename)
		root = dom.documentElement
		profile = root.getElementsByTagName('profile')[0]
		sections = profile.getElementsByTagName('section')

		for sec in sections:
			self.__profile_radius.append(string.atof(sec.getAttribute('radius')))
			self.__profile_chord.append(string.atof(sec.getAttribute('chord')))
			self.__profile_pitch.append(string.atof(sec.getAttribute('pitch')))
			self.__profile_skew.append(string.atof(sec.getAttribute('skew')))
			self.__profile_rake.append(string.atof(sec.getAttribute('rake')))
			self.__profile_airfoil.append(sec.firstChild.data)

		Naf = self.__AirfoilSampleCount*10
		nsec = len(self.__profile_radius)
		for isec in range(0,nsec):
			airfoil = Airfoil(self.__profile_airfoil[isec])
			t = np.linspace(180,0,Naf+1) * math.pi / 180
			xi = (1+np.cos(t))/2.0
			(eta_u,eta_l) = airfoil.evaluate(xi)
			thick = eta_u-eta_l
			maxthick = np.amax(thick) 
			imax = thick.argmax()
			self.__profile_xmaxthick.append(xi[imax])
			self.__profile_thickness.append(maxthick)

		self.__expanded_area_coef = string.atof(root.getElementsByTagName('expanded-area-coef')[0].firstChild.data)

	def __loadProfileFromDat(self, filename):
		self.__profile_radius  = []
		self.__profile_chord   = []
		self.__profile_pitch   = []
		self.__profile_skew    = []
		self.__profile_rake    = []
		self.__profile_airfoil = []

		f = open(filename)
		line = f.readline()
		while line:
			if line.startswith('#'):
				line = f.readline()
				continue

			p = re.compile(r'\s+')
			strs = p.split(line)
			if (len(strs)>5):
				self.__profile_radius.append(string.atof(strs[0]))
				self.__profile_chord.append(string.atof(strs[1]))
				self.__profile_pitch.append(string.atof(strs[2]))
				self.__profile_skew.append(string.atof(strs[3]))
				self.__profile_rake.append(string.atof(strs[4]))
				self.__profile_airfoil.append(strs[5])
			line = f.readline()
		f.close()

		Naf = self.__AirfoilSampleCount*10
		nsec = len(self.__profile_radius)
		for isec in range(0,nsec):
			airfoil = Airfoil(self.__profile_airfoil[isec])
			t = np.linspace(180,0,Naf+1) * math.pi / 180
			xi = (1+np.cos(t))/2.0
			(eta_u,eta_l) = airfoil.evaluate(xi)
			thick = eta_u-eta_l
			maxthick = np.amax(thick) 
			imax = thick.argmax()
			self.__profile_xmaxthick.append(xi[imax])
			self.__profile_thickness.append(maxthick)


