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
a ValueError exception if the argument cannot be converted. If the function
has an attribute named "complete", this will be called to provide suggested
values for tab-completion. The complete method is passed the current
fragment that the user is trying to tab-complete, and should return a list
of matching allowable values.

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

The docstring of the command handler function will be used to display help
about the command to the user on request.
"""


import types
from itertools import *


class Command(object):
    """
    Decorator to define the argument list for a command handler.

    Arguments may be keywords (lower-case strings), string parameters
    (upper-case strings), or parameters of different types (functions that
    validate and/or convert the parameters from strings).

    A series of arguments can be made optional by enclosing them in a tuple,
    or repeated by enclosing in a list.
    """

    def __init__(self, name, *args):
        """Initialise with a list of command arguments."""
        self.name = Keyword(name)
        self.args = [Argument(a) for a in args]

    def __call__(self, func):
        """Decorate the command handler."""
        return CommandHandler(func, self.name, self.args)


class InvalidArgumentException(Exception):
    """
    An Exception raised when the value of an argument passed by the user is not
    valid in the current context.
    """
    pass

class InvalidCommandException(Exception):
    """
    An Exception raised when a command executed by the user is invalid.
    """

    def __init__(self, line, candidates=[], message=None):
        self.line = line
        self.candidates = candidates
        if message is None:
            message = 'Invalid Command: "%s"' % self.line

        if self.candidates:
            message += ('\nCandidates are:\n' +
                        '\n'.join('    ' + str(c) for c, e in self.candidates))

        super(InvalidCommandException, self).__init__(message)


class CommandHandler(object):
    """A handler for a command."""

    def __init__(self, func, name, args):
        """Initialise with a handler function and a list of arguments."""
        self.do = func
        self.name = str(name)
        self.args = [name] + args
        self.arg_graph = ArgGraph(self.args)
        self.__name__ = self.do.__name__
        self.__doc__ = self.help()

    def help(self):
        """Return the help string for the command."""
        doc = self._trimdoc(self.do.__doc__)
        return ' > %s\n\n%s\n' % (str(self), doc)

    def complete(self, text, line, begidx, endidx):
        """Return a list of tab-completion options for the command."""
        values = line.split()
        if values[-1] != text:
            values.append(text)

        match = lambda (a, v): a and a.complete(v) or []

        def completions(arglist):
            argvals = takewhile(lambda (a, v): v is not None,
                                izip_longest(arglist, values, fillvalue=None))
            for m in imap(match, argvals):
                if not m:
                    return []
            return m

        candidates = chain.from_iterable(imap(completions, self.arg_graph))

        return list(set(candidates))

    def __get__(self, obj, objtype=None):
        # Allow the handler to be bound to objects as a method
        boundfunc = types.MethodType(self.do, obj, objtype)
        return CommandHandler(boundfunc, self.args[0], self.args[1:])

    def __call__(self, line):
        """Do the command."""
        vals = [self.name] + line.split()
        args = tuple(Argument.collate(self.args, vals))

        if len(args) < len(self.args):
            miss = ' '.join(str(a) for a in self.args[len(args):])
            raise InvalidCommandException(line,
                message="Missing arguments: '%s'" % miss)
        if vals:
            extra = ' '.join(vals)
            raise InvalidCommandException(line,
                message="Excess arguments: '%s'" % extra)

        return self.do(*args)

    def __repr__(self):
        return '@Command(%s)' % ', '.join(repr(a) for a in self.args)

    def __str__(self):
        return ' '.join(str(a) for a in self.args)

    def __add__(self, other):
        if other is None:
            return self
        return CommandGroupHandler(self.name, self, other)

    def __radd__(self, other):
        if other is None:
            return self
        return CommandGroupHandler(self.name, other, self)

    @staticmethod
    def _trimdoc(docstring):
        """Trim whitespace from a docstring."""
        if not docstring:
            return ''
        # Convert tabs to spaces (following the normal Python rules)
        # and split into a list of lines:
        lines = docstring.expandtabs().splitlines()
        # Determine minimum indentation (first line doesn't count):
        nonblank = lambda l: l and not l.isspace()
        indents = [len(l) - len(l.lstrip()) for l in lines[1:] if nonblank(l)]
        indent = indents and reduce(min, indents) or 0
        # Remove indentation (first line is special):
        trimmed = [lines[0].strip()] + [l[indent:].rstrip() for l in lines[1:]]
        # Strip off trailing and leading blank lines:
        while trimmed and not trimmed[-1]:
            trimmed.pop()
        while trimmed and not trimmed[0]:
            trimmed.pop(0)
        # Return a single string:
        return '\n'.join(trimmed)


class CommandGroupHandler(object):
    """A handler for a group of commands with the same name."""

    def __init__(self, *args):
        """
        Initialise with an optional command name and a series of command
        handlers.
        """
        if isinstance(args[0], basestring):
            self.name = args[0]
            args = args[1:]
        else:
            self.name = args[0].name
        self.cmds = []
        self.__doc__ = None
        for c in args:
            self += c
        self.__doc__ = self.help()

    def help(self):
        """Return the help string for the command."""
        return '\n'.join(c.help() for c in self.cmds)

    def complete(self, text, line, begidx, endidx):
        """Return a list of tab-completion options for the command."""
        completions = lambda c: c.complete(text, line, begidx, endidx)
        return list(set().union(*map(completions, self.cmds)))

    def _check_add(self, other):
        if not isinstance(other, CommandHandler):
            raise TypeError("%s is not a CommandHandler" % type(other))
        if other.name != self.name:
            raise ValueError('Command name "%s" does not match group "%s"' %
                             (other.name, self.name))

    def __add__(self, other):
        """Create a new group including an additional command or group."""
        if other is None:
            return self
        if isinstance(other, CommandGroupHandler):
            cmds = other.cmds
            if other.name != self.name:
                raise ValueError('Command names "%s" and "%s" do not match' %
                                 (self.name, other.name))
        else:
            self._check_add(other)
            cmds = [other]
        return CommandGroupHandler(self.name, *(self.cmds + cmds))

    def __iadd__(self, other):
        """Add a new command to the group."""
        self._check_add(other)
        self.cmds.append(other)
        if self.__doc__ is not None:
            self.__doc__ = self.help()
        return self

    def __repr__(self):
        return 'CommandGroupHandler(%s)' % repr(self.cmds)

    def __str__(self):
        return '\n'.join('    ' + str(c) for c in self.cmds)

    def __call__(self, line):
        """Do the command."""
        failures = []

        for c in self.cmds:
            try:
                return c(line)
            except InvalidCommandException, e:
                failures.extend(e.candidates or [(c, None)])
            except InvalidArgumentException, e:
                failures.append((c, e))

        line = ' '.join((self.name, line))
        raise InvalidCommandException(line.strip(), candidates=failures)


class ArgGraph(object):
    """A representation of the allowable sequences of command arguments in the
    form of a directed acyclic graph."""

    def __init__(self, args):
        """Initialise with a list of arguments"""
        self.edges = {}
        roots, leaves = self._addargs(args)
        self.first = roots[0]

    def __getitem__(self, key):
        if key is None:
            return self.first
        return self.edges[key]

    def __iter__(self):
        """
        Return a generator for all of the allowed sequences of arguments.
        """
        return self._getpaths()

    def _getpaths(self, visited=None):
        """
        Return a generator for all of the allowed sequences of agruments
        beginning with the given subpath.
        """
        children = self[visited and visited[-1]]

        isleaf = lambda n: n not in self.edges
        pathto = lambda n: (visited or []) + [n]

        # Paths that end with the start node's children
        leaf_paths = imap(lambda leaf: pathto(leaf),
                          ifilter(isleaf, children))

        # Treat each Argument as an iterator and chain their output to pick up
        # internal members of e.g. RepeatedArguments
        arg_chains = imap(chain.from_iterable, leaf_paths)

        getpaths = lambda n: self._getpaths(pathto(n))
        nonleaf_children = ifilterfalse(isleaf, children)

        # Paths that are recursively obtained via non-leaf children
        child_arg_chains = chain.from_iterable(imap(getpaths,
                                                    nonleaf_children))

        return chain(arg_chains, child_arg_chains)

    def _addargs(self, args):
        """Add a list of arguments to the Graph"""
        roots_found = False
        roots = []
        leaves = []

        optional = lambda n: isinstance(n, OptionalArguments)

        def handle_opt_args(opt_args):
            r, l = self._addargs(opt_args)
            self._addedges(leaves, *r)
            if not roots_found:
                roots.extend(r)
            leaves.extend(l)

        def handle_arg(arg):
            self._addedges(leaves, arg)
            if not roots_found:
                roots.append(arg)
            leaves[:] = [arg]

        for a in args:
            if optional(a):
                handle_opt_args(a)
            else:
                handle_arg(a)
                roots_found = True

        return roots, leaves

    def _addedges(self, srcs, *dests):
        for s in srcs:
            l = self.edges.get(s, [])
            l.extend(dests)
            self.edges[s] = l


class Argument(object):
    """The definition of an argument to a command"""

    def __new__(cls, arg):
        """Create an argument of the correct type from its definition."""
        if cls != Argument:
            return super(Argument, cls).__new__(cls)

        if isinstance(arg, tuple):
            argclass = OptionalArguments
        elif isinstance(arg, list):
            argclass = RepeatedArguments
        elif callable(arg):
            argclass = Parameter
        elif isinstance(arg, basestring):
            if arg.upper() == arg:
                argclass = Parameter
            else:
                argclass = Keyword
        else:
            return arg
        return argclass(arg)

    @staticmethod
    def collate(args, values):
        """
        Match values to arguments and return an argument list suitable for
        passing to the handler function.
        """
        arglist = chain.from_iterable(imap(lambda a: a.match(values), args))
        return takewhile(lambda p: p is not None, arglist)

    def __iter__(self):
        yield self


class Parameter(Argument):
    """A parameter passed to the command by the user"""
    def __init__(self, param):
        self.param = param

    def __repr__(self):
        if callable(self.param):
            if hasattr(self.param, '__name__'):
                return self.param.__name__
            else:
                return type(self.param).__name__
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

    def complete(self, value):
        if hasattr(self.param, 'complete') and callable(self.param.complete):
            return self.param.complete(value)
        try:
            if value:
                parsed = self(value)
            return ['']
        except InvalidArgumentException:
            return []

    def match(self, values):
        if not values:
            yield None
            return
        yield self(values.pop(0))


class Keyword(Argument):
    """A fixed keyword that must be present in the command"""
    def __init__(self, kw):
        self.kw = kw

    def __str__(self):
        return self.kw

    def __eq__(self, other):
        return self.kw == other

    def __ne__(self, other):
        return self.kw != other

    def complete(self, value):
        if self.kw.startswith(value):
            return [self.kw + ' ']
        else:
            return []

    def match(self, values):
        if not values:
            yield None
            return
        val = values[0]
        if self != val:
            raise InvalidArgumentException("Invalid keyword '%s'" % val)
        yield values.pop(0)


class ArgumentList(Argument):
    """Abstract base class for lists of arguments"""

    def __init__(self, args):
        self.args = [Argument(a) for a in args]

    def __iter__(self):
        return iter(self.args)

    def __repr__(self):
        return ', '.join(repr(a) for a in self.args)

    def __str__(self):
        return ' '.join(str(a) for a in self.args)


class OptionalArguments(ArgumentList):
    """A list of arguments that may be omitted when the command is called"""

    def __repr__(self):
        contents = ArgumentList.__repr__(self)
        if len(self.args) == 1:
            contents += ','
        return '(%s)' % contents

    def __str__(self):
        return '(%s)' % ArgumentList.__str__(self)

    def match(self, values):
        args = [None] * len(self.args)
        if values:
            values_copy = list(values)
            try:
                args = list(self.collate(self.args, values_copy))
                values[:] = values_copy
            except InvalidArgumentException:
                pass
        yield args


class RepeatedArguments(ArgumentList):
    """A list of arguments that may be repeated"""

    def __iter__(self):
        return cycle(self.args)

    def __repr__(self):
        return '[%s]' % ArgumentList.__repr__(self)

    def __str__(self):
        return '[%s]' % ArgumentList.__str__(self)

    def match(self, values):
        if not values:
            raise InvalidArgumentException("Empty variadic argument list")

        values_remain = lambda c: values

        iterations = takewhile(values_remain,
                               starmap(self.collate,
                                       repeat((self.args, values))))
        return chain.from_iterable(iterations)


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
            self.fail('Command handler erroneously called')
        self.nullCmd = nullCmd

    def test_simple(self):
        @self.CallCounter
        def foo(arg):
            self.assertEqual(arg, 'foo')
        handler = Command('foo')(foo)
        handler('')
        self.assertEqual(foo.calls, 1)

    def test_simple_bad(self):
        handler = Command('foo')(self.nullCmd)
        self.assertRaises(InvalidCommandException, handler, 'bar')

    def test_param(self):
        @self.CallCounter
        def foo(kwfoo, param):
            self.assertEqual(kwfoo, 'foo')
            self.assertEqual(param, 'bar')
        handler = Command('foo', 'PARAM')(foo)
        handler('bar')
        self.assertEqual(foo.calls, 1)

    def test_param_missing(self):
        handler = Command('foo', 'PARAM')(self.nullCmd)
        self.assertRaises(InvalidCommandException, handler, '')

    def test_param_extra(self):
        handler = Command('foo')(self.nullCmd)
        self.assertRaises(InvalidCommandException, handler, 'bar')

    def test_param_validator(self):
        @self.CallCounter
        def foo(kwfoo, intarg):
            self.assertEqual(kwfoo, 'foo')
            self.assertIsInstance(intarg, int)
            self.assertEqual(intarg, 42)
        handler = Command('foo', int)(foo)
        handler('42')
        self.assertEqual(foo.calls, 1)

    def test_param_validator_invalid(self):
        @self.CallCounter
        def foo(kwfoo, intarg):
            self.fail('Command handler called')
        handler = Command('foo', int)(foo)
        self.assertRaises(InvalidArgumentException, handler, 'bar')

    def test_opt_args(self):
        @self.CallCounter
        def foo(kwfoo, (kwbar, param1), param2):
            self.assertEqual(kwfoo, 'foo')
            self.assertEqual(kwbar, 'bar')
            self.assertEqual(param1, 'baz')
            self.assertEqual(param2, 'blarg')
        handler = Command('foo', ('bar', 'PARAM1'), 'PARAM2')(foo)
        handler('bar baz blarg')
        self.assertEqual(foo.calls, 1)

    def test_opt_args_missing(self):
        @self.CallCounter
        def foo(kwfoo, optargs, param2):
            self.assertEqual(kwfoo, 'foo')
            self.assertEqual(optargs, [None, None])
            self.assertEqual(param2, 'wibble')
        handler = Command('foo', ('bar', 'PARAM1'), 'PARAM2')(foo)
        handler('wibble')
        self.assertEqual(foo.calls, 1)

    def test_opt_args_partial(self):
        handler = Command('foo', ('bar', 'PARAM1'), 'PARAM2')(self.nullCmd)
        self.assertRaises(InvalidCommandException, handler, 'bar')

    def test_opt_args_end_missing(self):
        @self.CallCounter
        def foo(kwfoo, optargs):
            self.assertEqual(kwfoo, 'foo')
            self.assertSequenceEqual(optargs, [None])
        handler = Command('foo', ('bar',))(foo)
        handler('')
        self.assertEqual(foo.calls, 1)

    def test_opt_kw(self):
        @self.CallCounter
        def foo(kwfoo, (kwbar,), param):
            self.assertEqual(kwfoo, 'foo')
            self.assertEqual(kwbar, 'bar')
            self.assertEqual(param, 'baz')
        handler = Command('foo', ('bar',), 'PARAM')(foo)
        handler('bar baz')
        self.assertEqual(foo.calls, 1)

    def test_opt_kw_missing(self):
        @self.CallCounter
        def foo(kwfoo, (kwbar,), param):
            self.assertEqual(kwfoo, 'foo')
            self.assertIs(kwbar, None)
            self.assertEqual(param, 'quux')
        handler = Command('foo', ('bar',), 'PARAM')(foo)
        handler('quux')
        self.assertEqual(foo.calls, 1)

    def test_opt_kw_partial(self):
        handler = Command('foo', ('bar',), 'PARAM')(self.nullCmd)
        self.assertRaises(InvalidCommandException, handler, 'bar')

    def test_opt_kw_missing(self):
        @self.CallCounter
        def foo(kwfoo, (kwbar,), param):
            self.assertEqual(kwfoo, 'foo')
            self.assertIs(kwbar, None)
            self.assertEqual(param, 'quux')
        handler = Command('foo', ('bar',), 'PARAM')(foo)
        handler('quux')
        self.assertEqual(foo.calls, 1)

    def test_list(self):
        @self.CallCounter
        def foo(kwfoo, param, *args):
            self.assertEqual(kwfoo, 'foo')
            self.assertEqual(param, 'bar')
            self.assertSequenceEqual(args, ['baz', 'blarg', 'wibble'])
        handler = Command('foo', 'bar', ['PARAMS'])(foo)
        handler('bar baz blarg wibble')
        self.assertEqual(foo.calls, 1)

    def test_list_single(self):
        @self.CallCounter
        def foo(kwfoo, param, *args):
            self.assertEqual(kwfoo, 'foo')
            self.assertEqual(param, 'bar')
            self.assertSequenceEqual(args, ['baz'])
        handler = Command('foo', 'bar', ['PARAMS'])(foo)
        handler('bar baz')
        self.assertEqual(foo.calls, 1)

    def test_list_empty(self):
        handler = Command('foo', 'bar', ['PARAMS'])(self.nullCmd)
        self.assertRaises(InvalidArgumentException, handler, 'bar')

    def test_list_optional(self):
        @self.CallCounter
        def foo(kwfoo, param, args):
            self.assertEqual(kwfoo, 'foo')
            self.assertEqual(param, 'bar')
            self.assertSequenceEqual(args, [42, 43])
        handler = Command('foo', 'bar', ([int],))(foo)
        handler('bar 42 43')
        self.assertEqual(foo.calls, 1)

    def test_list_optional_empty(self):
        @self.CallCounter
        def foo(kwfoo, param, args):
            self.assertEqual(kwfoo, 'foo')
            self.assertEqual(param, 'bar')
            self.assertIs(args[0], None)
        handler = Command('foo', 'bar', ([int],))(foo)
        handler('bar')
        self.assertEqual(foo.calls, 1)

    def test_list_optional_kw_err(self):
        handler = Command('foo', ('bar', ['PARAMS']))(self.nullCmd)
        self.assertRaises(InvalidCommandException, handler, 'bar')


class CompletionTest(unittest.TestCase):
    def handler(self, *args):
        @Command(*args)
        def nullCmd(*args, **kwargs):
            self.fail('Command handler called')
        return nullCmd

    def assertCompletion(self, handler, line, expected):
        actual = self.complete(handler, line)
        err = '\n' + '\n'.join(('Command:      "%s"' % handler,
                                'Typed:        "%s"' % line,
                                'Completions:  %s' % actual,
                                'Expected:     %s' % expected))
        self.assertItemsEqual(actual, expected, err)

    def complete(self, handler, line):
        parts = line.rpartition(' ')
        text = parts[2]
        begidx = len(parts[0]) + len(parts[1])
        endidx = len(line)
        return handler.complete(text, line, begidx, endidx)

    def test_kw(self):
        h = self.handler('foo', 'bar')
        self.assertCompletion(h, 'foo ', ['bar '])
        self.assertCompletion(h, 'foo b', ['bar '])
        self.assertCompletion(h, 'foo bar ', [])

    def test_kw_space(self):
        h = self.handler('foo', 'bar')
        self.assertCompletion(h, 'foo', ['foo '])
        self.assertCompletion(h, 'foo bar', ['bar '])

    def test_multiple_kw(self):
        h = self.handler('foo', 'bar', 'baz')
        self.assertCompletion(h, 'foo b', ['bar '])
        self.assertCompletion(h, 'foo bar ', ['baz '])

    def test_param(self):
        h = self.handler('foo', 'PARAM')
        self.assertCompletion(h, 'foo ', [''])
        self.assertCompletion(h, 'foo b', [''])
        self.assertCompletion(h, 'foo bar ', [])

    def test_param_validator(self):
        h = self.handler('foo', int)
        self.assertCompletion(h, 'foo ', [''])
        self.assertCompletion(h, 'foo b', [])
        self.assertCompletion(h, 'foo 1', [''])
        self.assertCompletion(h, 'foo 123 ', [])

    def test_param_suggestor(self):
        candidates = ['bar ', 'baz ', 'blarg ']
        class Param(object):
            def __call__(self):
                pass
            def complete(self, arg):
                return [a for a in candidates if a.startswith(arg)]

        h = self.handler('foo', Param())
        self.assertCompletion(h, 'foo ', candidates)
        self.assertCompletion(h, 'foo b', candidates)
        self.assertCompletion(h, 'foo ba', ['bar ', 'baz '])
        self.assertCompletion(h, 'foo bar', ['bar '])
        self.assertCompletion(h, 'foo bar ', [])
        self.assertCompletion(h, 'foo quux', [])

    def test_multiple_param(self):
        h = self.handler('foo', 'PARAM', 'PARAM2')
        self.assertCompletion(h, 'foo ', [''])
        self.assertCompletion(h, 'foo b', [''])
        self.assertCompletion(h, 'foo bar ', [''])
        self.assertCompletion(h, 'foo bar baz', [''])
        self.assertCompletion(h, 'foo bar baz ', [])

    def test_opt_args(self):
        h = self.handler('foo', ('bar', 'PARAM1'), 'PARAM2')
        self.assertCompletion(h, 'foo ', ['bar ', ''])
        self.assertCompletion(h, 'foo b', ['bar ', ''])
        self.assertCompletion(h, 'foo bar ', [''])
        self.assertCompletion(h, 'foo quux', [''])
        self.assertCompletion(h, 'foo quux ', [])

    def test_multiple_opt_args(self):
        h = self.handler('foo', ('bar', 'PARAM1'), ('baz', 'PARAM2'))
        self.assertCompletion(h, 'foo', ['foo '])
        self.assertCompletion(h, 'foo ', ['bar ', 'baz '])
        self.assertCompletion(h, 'foo b', ['bar ', 'baz '])
        self.assertCompletion(h, 'foo bar ', [''])
        self.assertCompletion(h, 'foo baz ', [''])
        self.assertCompletion(h, 'foo baz quux ', [])

    def test_opt_kw(self):
        h = self.handler('foo', ('bar',), 'PARAM')
        self.assertCompletion(h, 'foo ', ['bar ', ''])
        self.assertCompletion(h, 'foo b', ['bar ', ''])
        self.assertCompletion(h, 'foo bar ', [''])
        self.assertCompletion(h, 'foo bar quux', [''])
        self.assertCompletion(h, 'foo bar quux ', [])
        self.assertCompletion(h, 'foo quux', [''])
        self.assertCompletion(h, 'foo quux ', [])

    def test_multiple_opt_kw_first(self):
        h = self.handler('foo', ('bar',), ('baz',))
        self.assertCompletion(h, 'foo', ['foo '])
        self.assertCompletion(h, 'foo ', ['bar ', 'baz '])
        self.assertCompletion(h, 'foo b', ['bar ', 'baz '])
        self.assertCompletion(h, 'foo bar ', ['baz '])
        self.assertCompletion(h, 'foo baz ', [])
        self.assertCompletion(h, 'foo quux ', [])
        self.assertCompletion(h, 'foo bar quux ', [])

    def test_multiple_opt_kw(self):
        h = self.handler('foo', 'wibble', ('bar',), ('baz',))
        self.assertCompletion(h, 'foo', ['foo '])
        self.assertCompletion(h, 'foo ', ['wibble '])
        self.assertCompletion(h, 'foo wibble ', ['bar ', 'baz '])
        self.assertCompletion(h, 'foo wibble b', ['bar ', 'baz '])
        self.assertCompletion(h, 'foo wibble bar ', ['baz '])
        self.assertCompletion(h, 'foo wibble baz ', [])
        self.assertCompletion(h, 'foo wibble quux ', [])
        self.assertCompletion(h, 'foo wibble bar quux ', [])

    def test_list(self):
        h = self.handler('foo', 'bar', ['PARAMS'])
        self.assertCompletion(h, 'foo ', ['bar '])
        self.assertCompletion(h, 'foo bar', ['bar '])
        self.assertCompletion(h, 'foo bar ', [''])
        self.assertCompletion(h, 'foo bar baz', [''])
        self.assertCompletion(h, 'foo bar baz ', [''])
        self.assertCompletion(h, 'foo bar baz blarg', [''])
        self.assertCompletion(h, 'foo bar baz blarg ', [''])
        self.assertCompletion(h, 'foo bar baz blarg wibble', [''])
        self.assertCompletion(h, 'foo bar baz blarg wibble ', [''])

    def test_list_optional(self):
        h = self.handler('foo', 'bar', (['PARAMS'],))
        self.assertCompletion(h, 'foo ', ['bar '])
        self.assertCompletion(h, 'foo bar', ['bar '])
        self.assertCompletion(h, 'foo bar ', [''])
        self.assertCompletion(h, 'foo bar baz', [''])
        self.assertCompletion(h, 'foo bar baz ', [''])
        self.assertCompletion(h, 'foo bar baz blarg', [''])
        self.assertCompletion(h, 'foo bar baz blarg ', [''])
        self.assertCompletion(h, 'foo bar baz blarg wibble', [''])
        self.assertCompletion(h, 'foo bar baz blarg wibble ', [''])

    def test_list_kw_optional(self):
        h = self.handler('foo', 'bar', ('baz', ['PARAMS'],))
        self.assertCompletion(h, 'foo ', ['bar '])
        self.assertCompletion(h, 'foo bar', ['bar '])
        self.assertCompletion(h, 'foo bar ', ['baz '])
        self.assertCompletion(h, 'foo bar baz', ['baz '])
        self.assertCompletion(h, 'foo bar baz ', [''])
        self.assertCompletion(h, 'foo bar baz blarg', [''])
        self.assertCompletion(h, 'foo bar baz blarg ', [''])
        self.assertCompletion(h, 'foo bar baz blarg wibble', [''])
        self.assertCompletion(h, 'foo bar baz blarg wibble ', [''])


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
    suite.addTest(CompletionTest)
    suite.addTest(SyntaxTest)
    return suite
