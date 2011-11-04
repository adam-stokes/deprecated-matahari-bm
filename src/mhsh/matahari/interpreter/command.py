
class Command(object):
    """Decorator to define the argument list for a command handler."""
    def __init__(self, *args):
        """Initialise with a list of command arguments."""
        self.args = [_parse(a) for a in args]

    def __call__(self, func):
        """Decorate the command handler."""
        return CommandHandler(func, self.args)

class InvalidArgumentException(Exception):
    pass

def _collate_args(args, values):
    for a in args:
        if not values:
            return

        if isinstance(a, OptionalArguments):
            try:
                _collate_args(a.args, values[:])
                yield list(_collate_args(a.args, values))
            except InvalidArgumentException:
                yield [None] * len(a.args)
        elif isinstance(a, RepeatedArguments):
            while values:
                yield list(_collate_args(a.args, values))
        elif a == values[0]:
            yield values.pop(0)
        elif isinstance(a, Parameter):
            yield a(values.pop(0))
        else:
            raise InvalidArgumentException

def _collate(args, values):
    v = list(values)
    r = list(_collate_args(args, v))
    if v:
        raise InvalidArgumentException("Invalid arguments (remaining: %s)" % v)
    return r

class CommandHandler(object):
    def __init__(self, func, args):
        self.do = func
        self.args = args
        self.__name__ = self.do.__name__
        self.__doc__ = self.help()

    def help(self):
        """Return the help string for the command."""
        doc = _trim(self.do.__doc__)
        return ' > %s\n\n%s' % (str(self), doc)

    def __call__(self, commandline):
        """Do the command"""
        args = _collate(self.args, commandline.split())
        self.do(*args)

    def __repr__(self):
        return '@Command(%s)' % ', '.join(repr(a) in self.args)

    def __str__(self):
        return ' '.join(str(a) for a in self.args)

class Parameter(object):
    """A parameter passed to the command by the user"""
    def __init__(self, param):
        self.param = param

    def __repr__(self):
        if callable(self.param):
            return self.param.__name__
        return str(self.param)

    def __str__(self):
        return repr(self).upper()

    def __call__(self, arg):
        if callable(self.param):
            return self.param(arg)
        return arg

class OptionalArguments(object):
    """A list of arguments that may be omitted when the command is called"""
    def __init__(self, *args):
        self.args = args

    def __repr__(self):
        contents = ', '.join(repr(a) for a in self.args)
        if len(self.args) == 1:
            contents += ','
        return '(%s)' % contents

    def __str__(self):
        return '(%s)' % ' '.join(str(a) for a in self.args)

class RepeatedArguments(object):
    """A list of arguments that may be repeated"""
    def __init__(self, *args):
        self.args = args

    def __repr__(self):
        contents = ', '.join(repr(a) for a in self.args)
        return '[%s]' % contents

    def __str__(self):
        return '[%s]' % ' '.join(str(a) for a in self.args)

def _parse(arg):
    """Parse an argument definition to determine its type."""
    if isinstance(arg, tuple):
        return OptionalArguments(*[_parse(a) for a in arg])
    if isinstance(arg, list):
        return RepeatedArguments(*[_parse(a) for a in arg])
    if isinstance(arg, basestring) and arg.upper() == arg:
        return Parameter(arg)
    if callable(arg):
        return Parameter(arg)
    return arg

def _trim(docstring):
    """Trim whitespace from a docstring."""
    if not docstring:
        return ''
    # Convert tabs to spaces (following the normal Python rules)
    # and split into a list of lines:
    lines = docstring.expandtabs().splitlines()
    # Determine minimum indentation (first line doesn't count):
    indent = None
    for line in lines[1:]:
        stripped = line.lstrip()
        if stripped:
            depth = len(line) - len(stripped)
            indent = indent is None and depth or min(indent, depth)
    # Remove indentation (first line is special):
    trimmed = [lines[0].strip()]
    if indent:
        trimmed.extend(l[indent:].rstrip() for l in lines[1:])
    # Strip off trailing and leading blank lines:
    while trimmed and not trimmed[-1]:
        trimmed.pop()
    while trimmed and not trimmed[0]:
        trimmed.pop(0)
    # Return a single string:
    return '\n'.join(trimmed)


if __name__ == '__main__':
    def time(arg):
        try:
            return int(arg)
        except TypeError:
            raise InvalidArgumentException('Invalid time interval "%s"')

    @Command('sleep', time)
    def sleep(slpkw, time):
        """Sleep for TIME seconds."""
        print 'Sleeping for %ds' % time

    @Command('class', ('package', 'PACKAGE'), 'CLASS')
    def classpkg(clskw, (pkgkw, package), classname):
        """
        Select a class of objects to act on, optionally specifying a package
        name. If not specified, the package name defaults to
        org.matahariproject.
        """
        if pkgkw is None:
            package = 'org.matahariproject'
        print 'Setting class "%s" in package "%s"' % (classname, package)

    print '### sleep'
    print 'Help:'
    print sleep.__doc__
    print 'Output:'
    sleep('sleep 5')

    print '### classpkg'
    print 'Help:'
    print classpkg.__doc__
    print 'Output:'
    classpkg('class package org.matahariproject Host')
    classpkg('class Network')
