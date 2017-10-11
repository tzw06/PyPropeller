from xml.etree.ElementTree import ElementTree,Element
import  xml.dom.minidom
import string
import sys

# GLOBAL INSTANCE

options = {}

class PropellerData:

	def __init__(self):
		
		self.values = {}
		
		self.__OptionIDs = {}
		self.__OptionIDs['atlas-title']		= 'options/atlas-title'
		self.__OptionIDs['blade-profile']	= 'options/blade-profile'

		filename = sys.path[0] + '/res/setting.xml'
		dom = xml.dom.minidom.parse(filename)
		root = dom.documentElement

		self.__IDs = {}
		items = root.getElementsByTagName('data')[0].getElementsByTagName('var')
		for vi in items:
			self.__IDs[vi.getAttribute("name")] = vi.firstChild.data

		self.__methodComments = {}
		items = root.getElementsByTagName('methods')[0].getElementsByTagName('method')
		for mi in items:
			self.__methodComments[mi.getAttribute('title')] = mi.getAttribute('comment')

	def getValue(self, var):
		return self.values[var]

	def setValue(self, var, value):
		self.values[var] = value

	def getVarID(self, key):
		return self.__IDs[key]

	def getAllVarIDs(self):
		return self.__IDs.values()

	def getMethodComment(self, key):
		return self.__methodComments[key]

	def open(self, filename):
		self.__loadFromFile(filename, self.__IDs)
		self.__loadOptionFromFile(filename, self.__OptionIDs)

	def save(self, template, filename):
		self.__saveToFile(template, filename, self.__IDs)

	def __loadFromFile(self, filename, nodeIDs):
		tree = ElementTree()
		tree.parse(filename)
		for key in nodeIDs:
			self.values[key] = self.__getValueFromTree(tree, nodeIDs[key])

	def __saveToFile(self, template, filename, nodeIDs):
		tree = ElementTree()
		tree.parse(template)

		for key in nodeIDs:
			text = "%s" % self.values[key]
			nodelist = tree.findall(nodeIDs[key])
			for node in nodelist:
				node.text = text
						
		tree.write(filename, encoding="utf-8",xml_declaration=True)

	def __loadOptionFromFile(self, filename, optionIDs):
		tree = ElementTree()
		tree.parse(filename)
		for key in optionIDs:
			options[key] = self.__getStringFromTree(tree, optionIDs[key])

	def __getValueFromTree(self, tree, key):
		try:
			node = tree.find(key)
			return string.atof(node.text)
		except:
			print "failed to find \"%s\"" % key

	def __getStringFromTree(self, tree, key):
		try:
			node = tree.find(key)
			return node.text
		except:
			print "failed to find \"%s\"" % key


	def readMethods(self, filename):
		dom = xml.dom.minidom.parse(filename)
		root = dom.documentElement
		items = root.getElementsByTagName('item')

		methods = []
		for mi in items:
			methods.append(mi.firstChild.data)

		return methods


data = PropellerData()


# GLOBAL FUNCTIONS
