
spikePrefix = "__spk_"

# classes, functions, singletons (void, false, true) and variables
# with external linkage
externalPrefix = spikePrefix + "x_"

# metaclass instances -- although these objects have external linkage,
# they cannot be named from Spike code
metaPrefix = spikePrefix + "meta_"

# literals of various types
symPrefix   = spikePrefix + "sym_"
intPrefix   = spikePrefix + "int_"
floatPrefix = spikePrefix + "float_"
charPrefix  = spikePrefix + "char_"
strPrefix   = spikePrefix + "str_"

# code (non-object text entry points)
codePrefix  = spikePrefix + "code_"

