import cmd

class Interpreter(cmd.Cmd):
    def __init__(self, prompt):
        cmd.Cmd.__init__(self)
        self.prompt = prompt + '> '
        self.doc_header = 'Commands:'
        self.misc_header = 'Help topics:'
        self.undoc_header = ''

    def completenames(self, *args, **kwargs):
        # Tab-complete should move to the next argument
        return [s + ' ' for s in cmd.Cmd.completenames(self, *args, **kwargs)]

    def emptyline(self):
        # Don't do anything for an empty command
        pass

    def do_EOF(self, line):
        """Type Ctrl-D to exit the shell"""
        self.stdout.write('\n')
        # Exit the interpreter on ^D
        return True

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
        for line in script:
            if not line.strip().startswith('#'):
                self.onecmd(line)

    def help_help(self):
        # You've got to be kidding me
        self.stdout.write('Type "help <topic>" for help on commands\n')

    ### A test command
    def do_test(self, c):
        """A test command"""
        print 'Testing', ', '.join(c.split())

    def complete_test(self, text, line, begidx, endidx):
        params = ['foo', 'bar', 'baz', 'blarg', 'wibble']
        previous = frozenset(line.split())
        return [p + ' ' for p in params if p not in previous and p.startswith(text)]


if __name__ == '__main__':
    shell = Interpreter('mhsh')
    shell.cmdloop()

