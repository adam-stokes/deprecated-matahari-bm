"""
Modes for adding specific routines
"""

from interpreter import mode, command

class RootMode(mode.Mode):
    @command.Command('select', 'host', ['HOST'])
    def select(self, kw_host, arg_host):
        """ Select host based on existing hostnames """
        pass

    @command.Command('class', ('package', 'PACKAGE'), 'CLASS')
    def package_class(self, kw_class, *class_args):
        pass

    @command.Command('clear', 'subscriptions')
    def clear(self, kw_clear, kw_subscriptions):
        pass

class FilteredMode(RootMode):
    @command.Command('clear', 'selection')
    def clear_selection(self, kw_clear, kw_selection):
        pass

    @command.Command('show', 'selection')
    def show_selection(self, kw_show, kw_selection):
        pass

class ClassMode(RootMode):
    @command.Command('clear', 'class')
    def clear_class(self, kw_clear, kw_class):
        pass

    @command.Command('select', 'property', 'PROPERTY', 'VALUE')
    def select_property(self, kw_select, kw_property, arg_property, arg_value):
        pass

    @command.Command('call', 'method', ['ARGS'], {})
    def call_method(self, kw_call, kw_method, *args, **kwargs):
        pass

    @command.Command('get', 'PROPERTY')
    def get_property(self, kw_get, arg_property):
        pass

    @command.Command('set', 'PROPERTY', 'VALUE')
    def set_property(self, kw_set, arg_property, arg_value):
        pass

    @command.Command('get', 'STATISTIC')
    def get_statistic(self, kw_get, arg_statistic):
        pass

    @command.Command('subscribe', 'EVENT')
    def subscribe_event(self, kw_subscribe, arg_event):
        pass

class FilteredClassMode(FilteredMode, ClassMode):
    pass
