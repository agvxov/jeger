class RegexPrinter:
	def __init__(self, val):
		self.val = val
	def to_string(self):
		# Init
		s = "{"
		#   Regular shit
		s += "accepting_state = " + str(self.val['accepting_state']) + ", str = " + str(self.val['str']) + ",\n"
		#   Delta
		delta_t_ptr_ptr_t = gdb.lookup_type("delta_t").pointer().pointer()
		dt = self.val['delta_table']
		s += "delta_table = {\n"
		d0 = 0
		for i in range(0, dt['element_count']):
			s += "\t"
			s += (
					str(
						(
							dt['data'].cast(delta_t_ptr_ptr_t)
								+
							i
						).dereference().dereference()
					)
				)
			s += ",\n"
		s = s[:-2]
		s += "\n  },\n"
		#   Offshoot
		offshoot_t_ptr_ptr = gdb.lookup_type("offshoot_t").pointer().pointer()
		dt = self.val['catch_table']
		s += "offshoot_table = { \n"
		for i in range(0, dt['element_count']):
			s += "\t"
			s += (
					str(
						(
							dt['data'].cast(offshoot_t_ptr_ptr)
								+
							i
						).dereference().dereference()
					)
				)
			s += ",\n"
		s = s[:-2]
		s += "\n  }\n"
		# Closour
		s += "}"
		return s

def regex_lookup(val):
	if str(val.type) == 'regex_t' or str(val.type) == 'const regex_t':
		return RegexPrinter(val)
	return None

gdb.pretty_printers.append(regex_lookup)
