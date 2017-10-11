import math

class Vec3:

	def __init__(self, x=0, y=0, z=0, radius=1, theta=0, coordinate='cartisian'):
		self.__x = x
		if coordinate=='cylindrical':
			self.__y = radius * math.cos(theta)
			self.__z = radius * math.sin(theta)
		else:
			self.__y = y
			self.__z = z

	def x(self):
		return self.__x

	def y(self):
		return self.__y

	def z(self):
		return self.__z

	def length(self):
		return math.sqrt(self.__x*self.__x+self.__y*self.__y+self.__z*self.__z)

	def normalize(self):
		r = self.length()
		self.__x = self.__x / r
		self.__y = self.__y / r
		self.__z = self.__z / r

	def getCylinderR(self):
		return math.sqrt(self.__y*self.__y + self.__z*self.__z)

	def getCylinderTheta(self):
		return math.atan2(self.__z, self.__y)

	def rotate(self, origin, axis, theta):

		cost = math.cos(theta)
		sint = math.sin(theta)

		ux = axis.x()
		uy = axis.y()
		uz = axis.z()

		R00 = ux*ux + (1-ux*ux)*cost
		R01 = ux*uy*(1-cost) - uz*sint
		R02 = uz*ux*(1-cost) + uy*sint
		R10 = ux*uy*(1-cost) + uz*sint
		R11 = uy*uy + (1-uy*uy)*cost
		R12 = uy*uz*(1-cost) - ux*sint
		R20 = uz*ux*(1-cost) - uy*sint
		R21 = uy*uz*(1-cost) + ux*sint
		R22 = uz*uz + (1-uz*uz)*cost

		x0 = origin.x()
		y0 = origin.y()
		z0 = origin.z()
		dx = self.__x - x0
		dy = self.__y - y0
		dz = self.__z - z0

		x1 = x0 + R00*dx + R01*dy + R02*dz
		y1 = y0 + R10*dx + R11*dy + R12*dz
		z1 = z0 + R20*dx + R21*dy + R22*dz

		return Vec3(x=x1,y=y1,z=z1,coordinate='cartisian')

def vec3_plus(v1, v2):
	return Vec3(v1.x()+v2.x(), v1.y()+v2.y(), v1.z()+v2.z())

def vec3_minus(v1, v2):
	return Vec3(v1.x()-v2.x(), v1.y()-v2.y(), v1.z()-v2.z())

def vec3_dot(v1, v2):
	return v1.x()*v2.x() + v1.y()*v2.y() + v1.z()*v2.z()

def vec3_cross(v1, v2):
	x = v1.y()*v2.z() - v2.y()*v1.z()
	y = v1.z()*v2.x() - v2.z()*v1.x()
	z = v1.x()*v2.y() - v2.x()*v1.y()
	return Vec3(x,y,z)
