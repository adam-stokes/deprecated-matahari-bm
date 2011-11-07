"""
Module for defining the syntax of CLI commands.

To define a command, create a handler function and wrap it with the Command
decorator, like so:

    @Command('test', 'PARAM')
    def test(kw_test, param):
        '''
        A test command

        Prints the parameter passed in.
        '''
        assert kw_test == 'test'
        print param

The parameters to the Command constructor are the arguments to the command.
Arguments that are not in all caps are fixed keywords.

Arguments in all caps (e.g. 'PARAM') are variable parameters. Additionally,
an argument that is a function will be treated as a type conversion and
validation function for a variable parameter. For example, pass the builtin
int class to convert the argument to an integer. The function should throw
a ValueError exception if the argument cannot be converted.

A group of optional parameters can be passed as a tuple. For example:

    @Command('test2', ('optional-keyword', 'PARAM'), 'MANDATORY')
    def test2(kw_test, (kw_optkw, param), mandatory):
        '''A test command with optional parameters'''
        if kw_optkw is not None:
            print 'Optional:', param
        print 'Mandatory:', mandatory

Note that if the optional paramters are not supplied, they will each have
the value None when passed to the handler function. This is done (instead of
passing an empty list) so that tuple unpacking can be used as shown in the
example.

A variable argument list can be implemented by passing a list as the final
parameter, thus:

    @Command('test3', ['PARAMS'])
    def test3(kw_test, *params):
        '''A test command with variable arguments'''
        print 'Params:' params

A least one argument must be supplied for a variable argument list. To allow
a list with zero items, make it optional by wrapping it in a tuple.
"""

class Command(object):
    """
    Decorator to define the argument list for a command handler.

    Arguments may be keywords (lower-case strings), string parameters
    (upper-case strings), or parameters of different types (functions that
    validate and convert the parameters from strings).

    A series of arguments can be made optional by enclosing them in a tuple,
    or repeated by enclosing in a list.
    """

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
        if isinstance(a, OptionalArguments):
            try:
                if not values:
                    raise InvalidArgumentException
                list(_collate_args(a.args, values[:]))
                yield list(_collate_args(a.args, values))
            except InvalidArgumentException:
                yield [None] * len(a.args)
        elif not values:
            return
        elif isinstance(a, RepeatedArguments):
            while values:
                for c in _collate_args(a.args, values):
                    yield c
        elif a == values[0]:
            yield values.pop(0)
        elif isinstance(a, Parameter):
            yield a(values.pop(0))
        else:
            raise InvalidArgumentException

def _collate(args, values):
    v = list(values)
    r = list(_collate_args(args, v))
    if len(r) < len(args):
        raise InvalidArgumentException("Missing arguments")
    if v:
        raise InvalidArgumentException("Excess arguments %s" % v)
    return r

class CommandHandler(object):
    """A handler for a command"""
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
            try:
                return self.param(arg)
            except ValueError, e:
                raise InvalidArgumentException('Invalid argument value (%s)' % e)
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


import unittest

class CommandTest(unittest.TestCase):
    class CallCounter(object):
        def __init__(self, func):
            self.func = func
            self.__name__ = func.__name__
            self.calls = 0

        def __call__(self, *args, **kwargs):
            self.calls += 1
            return self.func(*args, **kwargs)


    def setUp(self):
        def nullCmd(*args, **kwargs):
            self.fail('Command handler called')
        self.nullCmd = nullCmd

    def test_simple(self):
        @self.CallCounter
        def foo(arg):
            self.assertEqual(arg, 'foo')
        handler = Command('foo')(foo)
        handler('foo')
        self.assertEqual(foo.calls, 1)

    def test_simple_bad(self):
        handler = Command('foo')(self.nullCmd)
        self.assertRaises(InvalidArgumentException, handler, 'bar')

    def test_param(self):
        @self.CallCounter
        def foo(kwfoo, param):
            self.assertEqual(kwfoo, 'foo')
            self.assertEqual(param, 'bar')
        handler = Command('foo', 'PARAM')(foo)
        handler('foo bar')
        self.assertEqual(foo.calls, 1)

    def test_param_missing(self):
        handler = Command('foo', 'PARAM')(self.nullCmd)
        self.assertRaises(InvalidArgumentException, handler, 'foo')

    def test_param_extra(self):
        handler = Command('foo')(self.nullCmd)
        self.assertRaises(InvalidArgumentException, handler, 'foo bar')

    def test_param_validator(self):
        @self.CallCounter
        def foo(kwfoo, intarg):
            self.assertEqual(kwfoo, 'foo')
            self.assertIsInstance(intarg, int)
            self.assertEqual(intarg, 42)
        handler = Command('foo', int)(foo)
        handler('foo 42')
        self.assertEqual(foo.calls, 1)

    def test_param_validator_invalid(self):
        @self.CallCounter
        def foo(kwfoo, intarg):
            self.fail('Command handler called')
        handler = Command('foo', int)(foo)
        self.assertRaises(InvalidArgumentException, handler, 'foo bar')

    def test_opt_args(self):
        @self.CallCounter
        def foo(kwfoo, (kwbar, param1), param2):
            self.assertEqual(kwfoo, 'foo')
            self.assertEqual(kwbar, 'bar')
            self.assertEqual(param1, 'baz')
            self.assertEqual(param2, 'blarg')
        handler = Command('foo', ('bar', 'PARAM1'), 'PARAM2')(foo)
        handler('foo bar baz blarg')
        self.assertEqual(foo.calls, 1)

    def test_opt_args_missing(self):
        @self.CallCounter
        def foo(kwfoo, optargs, param2):
            self.assertEqual(kwfoo, 'foo')
            self.assertEqual(optargs, [None, None])
            self.assertEqual(param2, 'wibble')
        handler = Command('foo', ('bar', 'PARAM1'), 'PARAM2')(foo)
        handler('foo wibble')
        self.assertEqual(foo.calls, 1)

    def test_opt_args_partial(self):
        handler = Command('foo', ('bar', 'PARAM1'), 'PARAM2')(self.nullCmd)
        self.assertRaises(InvalidArgumentException, handler, 'foo bar')

    def test_opt_args_end_missing(self):
        @self.CallCounter
        def foo(kwfoo, optargs):
            self.assertEqual(kwfoo, 'foo')
            self.assertSequenceEqual(optargs, [None])
        handler = Command('foo', ('bar',))(foo)
        handler('foo')
        self.assertEqual(foo.calls, 1)

    def test_opt_kw(self):
        @self.CallCounter
        def foo(kwfoo, (kwbar,), param):
            self.assertEqual(kwfoo, 'foo')
            self.assertEqual(kwbar, 'bar')
            self.assertEqual(param, 'baz')
        handler = Command('foo', ('bar',), 'PARAM')(foo)
        handler('foo bar baz')
        self.assertEqual(foo.calls, 1)

    def test_opt_kw_missing(self):
        @self.CallCounter
        def foo(kwfoo, (kwbar,), param):
            self.assertEqual(kwfoo, 'foo')
            self.assertIs(kwbar, None)
            self.assertEqual(param, 'quux')
        handler = Command('foo', ('bar',), 'PARAM')(foo)
        handler('foo quux')
        self.assertEqual(foo.calls, 1)

    def test_opt_kw_partial(self):
        handler = Command('foo', ('bar',), 'PARAM')(self.nullCmd)
        self.assertRaises(InvalidArgumentException, handler, 'foo bar')

    def test_opt_kw_missing(self):
        @self.CallCounter
        def foo(kwfoo, (kwbar,), param):
            self.assertEqual(kwfoo, 'foo')
            self.assertIs(kwbar, None)
            self.assertEqual(param, 'quux')
        handler = Command('foo', ('bar',), 'PARAM')(foo)
        handler('foo quux')
        self.assertEqual(foo.calls, 1)

    def test_list(self):
        @self.CallCounter
        def foo(kwfoo, param, *args):
            self.assertEqual(kwfoo, 'foo')
            self.assertEqual(param, 'bar')
            self.assertSequenceEqual(args, ['baz', 'blarg', 'wibble'])
        handler = Command('foo', 'bar', ['PARAMS'])(foo)
        handler('foo bar baz blarg wibble')
        self.assertEqual(foo.calls, 1)

    def test_list_single(self):
        @self.CallCounter
        def foo(kwfoo, param, *args):
            self.assertEqual(kwfoo, 'foo')
            self.assertEqual(param, 'bar')
            self.assertSequenceEqual(args, ['baz'])
        handler = Command('foo', 'bar', ['PARAMS'])(foo)
        handler('foo bar baz')
        self.assertEqual(foo.calls, 1)

    def test_list_empty(self):
        handler = Command('foo', 'bar', ['PARAMS'])(self.nullCmd)
        self.assertRaises(InvalidArgumentException, handler, 'foo bar')

    def test_list_optional(self):
        @self.CallCounter
        def foo(kwfoo, param, args):
            self.assertEqual(kwfoo, 'foo')
            self.assertEqual(param, 'bar')
            self.assertSequenceEqual(args, [42, 43])
        handler = Command('foo', 'bar', ([int],))(foo)
        handler('foo bar 42 43')
        self.assertEqual(foo.calls, 1)

    def test_list_optional_empty(self):
        @self.CallCounter
        def foo(kwfoo, param, args):
            self.assertEqual(kwfoo, 'foo')
            self.assertEqual(param, 'bar')
            self.assertIs(args[0], None)
        handler = Command('foo', 'bar', ([int],))(foo)
        handler('foo bar')
        self.assertEqual(foo.calls, 1)

class SyntaxTest(unittest.TestCase):
    def setUp(self):
        def nullCmd(*args, **kwargs):
            self.fail('Command handler called')
        self.nullCmd = nullCmd

    def test_simple(self):
        foo = Command('foo')(self.nullCmd)
        self.assertEqual(str(foo), 'foo')

    def test_param(self):
        foo = Command('foo', 'PARAM')(self.nullCmd)
        self.assertEqual(str(foo), 'foo PARAM')

    def test_param_validator(self):
        foo = Command('foo', int)(self.nullCmd)
        self.assertEqual(str(foo), 'foo INT')

    def test_opt_args(self):
        foo = Command('foo', ('bar', 'PARAM1'), 'PARAM2')(self.nullCmd)
        self.assertEqual(str(foo), 'foo (bar PARAM1) PARAM2')

    def test_opt_kw(self):
        foo = Command('foo', ('bar',), 'PARAM')(self.nullCmd)
        self.assertEqual(str(foo), 'foo (bar) PARAM')

    def test_list(self):
        foo = Command('foo', ['PARAMS'])(self.nullCmd)
        self.assertEqual(str(foo), 'foo [PARAMS]')

    def test_opt_list(self):
        foo = Command('foo', (['PARAMS'],))(self.nullCmd)
        self.assertEqual(str(foo), 'foo ([PARAMS])')

def suite():
    suite = unittest.TestSuite()
    suite.addTest(CommandTest)
    suite.addTest(SyntaxTest)
    return suite
