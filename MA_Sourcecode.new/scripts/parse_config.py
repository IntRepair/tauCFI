import xml.sax
from xml.sax.handler import ContentHandler

class TypeShieldConfigReader(ContentHandler):
	def __init__(self):
		self.config = {}
		self._stack = []

	def startElement(self, name, attrs):
		self._stack += [name]

		prefix = ""
		for level in self._stack:
			prefix += level + "."

		for attrs_key in attrs.keys():
			config_key = prefix + attrs_key
			assert(config_key not in self.config.keys())
			self.config[config_key] = attrs[attrs_key]
	
	def endElement(self, name):
		assert(self._stack[len(self._stack) - 1] == name)
		self._stack = self._stack[:-1]

class TestSourcesConfigReader(ContentHandler):
	def __init__(self):
		self.config = {}
		self.config["programs"] = []
		self._stack = []

	def startElement(self, name, attrs):
		self._stack += [name]
		
		if name == "program":
			program = {}
			program["name"] = attrs["name"]
			program["src"] = attrs["src"]
			program["method"] = attrs["method"]
			program["binary_path"] = attrs["binary_path"]
			program["sourcedir_binary"] = attrs["sourcedir_binary"]
			program["archive"] = attrs["archive"]
			self.config["programs"] += [program]

	def endElement(self, name):
		assert(self._stack[len(self._stack) - 1] == name)
		self._stack = self._stack[:-1]

def parse_config(config_type, config_file):
	config = {}

	with open(config_file) as cfg_file:
		if config_type == "type_shield":
			parser = TypeShieldConfigReader()
		elif config_type == "test_sources":
			parser = TestSourcesConfigReader()
		xml.sax.parse(cfg_file, parser)
		config = parser.config

	return config

