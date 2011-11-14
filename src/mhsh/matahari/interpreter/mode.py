"""
Module for defining interpreter shell modes.

To define a new mode, create a subclass of the Mode class. Customise the
shell prompt by overriding the prompt() method. Add commands to the mode by
concatenating them with the += operator.
"""

import types
import time
import command


class Mode(object):
    """
    A interpreter shell mode. Each mode may have a distinctive prompt and its
    own set of commands, and may in addition store whatever state it likes.
    """

    def __init__(self, *commands):
        """Initialise with an initial set of commands."""
        self.shell = None
        self.commands = {}
        for m in dir(type(self)):
            try:
                c = getattr(self, m)
            except AttributeError:
                pass
            else:
                if (isinstance(c, command.CommandHandler)):
                    self += c
        for c in commands:
            self += c

    def activate(self, interpreter):
        """Method called by the interpreter when the mode becomes active."""
        self.shell = interpreter

    def deactivate(self, interpreter):
        """Method called by the interpreter when the mode becomes inactive."""
        if self.shell == interpreter:
            self.shell = None

    def prompt(self):
        """
        Return any additions to the command prompt that are to be appended in
        this mode. This method may be overridden by subclasses.
        """
        return ''

    def write(self, *lines):
        """Write one or more lines to the console."""
        for line in lines:
            self.shell.stdout.write(line + '\n')

    def __iadd__(self, cmd):
        """Add a command."""
        n = cmd.name
        c = self.commands.get(n)
        self.commands[n] = c + cmd
        return self

    def __getitem__(self, key):
        return self.commands[key]

    def __iter__(self):
        return iter(self.commands)

    def iterkeys(self):
        return iter(self)

    @command.Command('sleep', float)
    def sleep(self, kw_sleep, seconds):
        """Sleep for the specified number of seconds"""
        time.sleep(seconds)

    @command.Command('quit')
    def quit(self, kw_quit):
        """Exit the interpreter"""
        return True
