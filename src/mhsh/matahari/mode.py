"""
Modes for adding specific routines
"""

from interpreter import mode, command

class RootMode(mode.Mode):
    @command.Command('select host', host)
    def select(self, kw_host, arg_host):
        """ Select host based on existing hostnames """
        pass

    @command.Command('class', package, klass)
    def package_class(self, kw_klass, *klass_args):
        pass

class FilteredMode(RootMode):
    pass

class ClassMode(RootMode):
    pass

class FilteredClassMode(FilteredMode, ClassMode):
    pass
