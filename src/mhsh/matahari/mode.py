"""
Modes for adding specific routines
"""

from interpreter import mode, command
from core import Manager

class RootMode(mode.Mode):
    def __init__(self):
        self.manager = Manager()
        super(RootMode, self).__init__()

    @command.Command('show', 'hosts')
    def show_hosts(self, kw_show, kw_hosts):
        """ shows connected hosts """
        print 'Connected Hosts:\n'
        for h in self.manager.hosts():
            print "Host: %s\n" % (h,)

    @command.Command('select', 'host', ['HOST'])
    def select(self, kw_host, *arg_hosts):
        """ Select host based on existing hostnames """
        for h in self.manager.hosts():
            if arg_hosts[1] in str(h):
                self.shell.set_mode(FilteredMode(h))

    @command.Command('class', ('package', 'PACKAGE'), 'CLASS')
    def package_class(self, kw_pkg, param, klass):
        self.shell.set_mode(ClassMode(param[1], klass))

    @command.Command('clear', 'subscriptions')
    def clear(self, kw_clear, kw_subscriptions):
        pass

class FilteredMode(RootMode):
    def __init__(self, host):
        self.manager = Manager()
        self.selected_host = host
        super(FilteredMode, self).__init__()

    @command.Command('clear', 'selection')
    def clear_selection(self, kw_clear, kw_selection):
        pass

    @command.Command('show', 'selection')
    def show_selection(self, kw_show, kw_selection):
        print "agents:\n"
        print "\t\n".join(str(a) for a in self.manager.agents(self.selected_host))

    def prompt(self):
        return ' (%s) ' % (self.selected_host,)

class ClassMode(RootMode, Manager):
    def __init__(self, package, klass):
        self.package = package
        self.klass = klass
        super(ClassMode, self).__init__()

    @command.Command('clear', 'class')
    def clear_class(self, kw_clear, kw_class):
        self.shell.set_mode(RootMode())

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

    def prompt(self):
        return ' (%s:%s) ' % (self.package, self.klass)

class FilteredClassMode(FilteredMode, ClassMode):
    pass
