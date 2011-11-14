"""
Module defining a modal command interpreter.

The commands supported by the interpreter are supplied by the current mode,
which can be changed by passing a new mode to the set_mode() method.
"""

import cmd

import mode
from command import InvalidCommandException, InvalidArgumentException


class Interpreter(cmd.Cmd):
    """A modal command interpreter."""

    def __init__(self, prompt, mode=mode.Mode()):
        """Initialise the interpreter in the initial mode."""
        cmd.Cmd.__init__(self)
        self.name = prompt
        self.mode = None
        self.set_mode(mode)
        self.doc_header = 'Commands:'
        self.misc_header = 'Help topics:'
        self.undoc_header = ''

    def set_mode(self, mode):
        """Set the shell mode."""
        if self.mode is not None:
            self.mode.deactivate(self)
        self.mode = mode
        self.mode.activate(self)
        self.prompt = self.name + mode.prompt() + '> '

    def completenames(self, *args):
        # Tab-complete should move to the next argument
        return [s + ' ' for s in cmd.Cmd.completenames(self, *args)]

    def complete_help(self, *args):
        # Don't suggest multiple arguments
        if len(args[1].split()) > (args[1][-1] == ' ' and 1 or 2):
            return []
        # Explicitly call superclass's completenames() method, to avoid adding
        # spaces (and thus duplicate output when combined with the topic list)
        commands = set(cmd.Cmd.completenames(self, *args))
        topics = set(a[5:] for a in self.get_names()
                     if a.startswith('help_' + args[0]))
        return list(commands | topics)

    def emptyline(self):
        # Don't do anything for an empty command
        pass

    def do_EOF(self, line):
        """Type Ctrl-D to exit the shell"""
        self.stdout.write('\n')
        # Exit the interpreter on ^D
        return True

    def onecmd(self, line):
        try:
            return cmd.Cmd.onecmd(self, line)
        except Exception, e:
            data = '\n'.join('% ' + l for l in str(e).splitlines())
            self.stdout.write(data + '\n\n')

    def cmdloop(self, *args, **kwargs):
        while True:
            try:
                cmd.Cmd.cmdloop(self, *args, **kwargs)
            except KeyboardInterrupt:
                self.stdout.write('\n')
                # Abort the current command and continue on ^C
                continue
            break

    def runscript(self, script):
        """Run the specified file as a script."""
        for line in script:
            if not line.strip().startswith('#'):
                self.onecmd(line)

    def help_help(self):
        # You've got to be kidding me
        self.stdout.write('Type "help <topic>" for help on commands\n')

    def __getattr__(self, name):
        """
        Get the do_ or complete_ method for a command from the current mode.
        The help_ methods should not be needed, as commands can provide help
        via their docstrings.
        """

        action, u, command_name = name.partition('_')
        if command_name:
            try:
                command = self.mode[command_name]
            except KeyError:
                pass
            else:
                if action == 'do':
                    return command
                if action == 'complete':
                    return command.complete

        raise AttributeError("%r object has no attribute %r" %
                             (type(self).__name__, name))

    def get_names(self):
        return (filter(lambda n: n != 'do_EOF', cmd.Cmd.get_names(self)) +
                ['%s_%s' % (p, c) for p in ['do', 'complete', 'help']
                                  for c in self.mode])


if __name__ == '__main__':
    mode = mode.Mode()
    shell = Interpreter('mhsh', mode)
    shell.cmdloop()

