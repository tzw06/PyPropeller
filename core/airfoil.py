from scipy.stats import binom
import numpy as np
import math
import sqlite3 as lite
import re
import string
import copy

from global_def import data,options

class Airfoil:

	def __init__(self, title):

		self.__title = title
		self.__cst_N = []
		self.__cst_A = []


		con = None
		try:
			con = lite.connect(options['resource-path']+'/propeller.db')
			cur = con.cursor()

			sql = 'select CoefU,CoefL from Airfoil where Name=\"%s\"' % title
			cur.execute(sql)
			result = cur.fetchone()

			for ispace in range(0,2):
				p = re.compile(', ')
				strs = p.split(result[ispace])
				N = len(strs)
				self.__cst_N.append(N-2)
				self.__cst_A.append(np.zeros((N)))
				for i in range(0,N):
					self.__cst_A[ispace][i] = string.atof(strs[i])

		except lite.Error, e:
			print 'Error %s:' % e.args[0]

		finally:
			if con:
				con.close()

	def setParameter(self, order, A):
		self.__cst_N = order
		self.__cst_A = A

	def evaluate(self, x):
		M = len(x)
		y = np.zeros(M)

		for ispace in range(0,2):
			N = self.__cst_N[ispace]
			for j in range(0,M):
				xj = x[j]
				fj = math.sqrt(xj) * (1-xj)
				yj = xj * self.__cst_A[ispace][N+1]
				for i in range(0,N+1):
					gij = binom.pmf(i,N,xj)
					yj = yj + fj * gij * self.__cst_A[ispace][i]
				y[j] = yj


			if ispace==0:
				yu = copy.deepcopy(y)
			else:
				yl = copy.deepcopy(y)

		return (yu, yl)