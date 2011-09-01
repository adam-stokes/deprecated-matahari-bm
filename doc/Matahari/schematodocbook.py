#!/usr/bin/env python
'''
Convert QMF schema to docbook.

Copyright (C) 2011, Red Hat, Inc.
Russell Bryant <rbryant@redhat.com>
'''

import sys
import libxml2


class Schema(object):
    def __init__(self):
        self.package = None
        self.event_args = {}
        self.events = []
        self.classes = []

    def write_docbook(self, f):
        f.write("<para>Package: <literal>%s</literal></para>\n" % self.package)
        if len(self.events):
            for e in self.events:
                e.write_docbook(f)
        if len(self.classes):
            for c in self.classes:
                c.write_docbook(f)


class Event(object):
    def __init__(self, schema, name, args):
        self.name = name
        self.args = []
        for arg_name in args.split(","):
            self.args.append(schema.event_args[arg_name])

    def write_docbook(self, f):
        f.write("    <section>\n")
        f.write("      <title>Event: <literal>%s</literal></title>\n" % self.name)
        f.write("      <table>\n")
        f.write("        <title>Arguments for Event: <literal>%s</literal></title>\n"
                % self.name)
        f.write("        <tgroup cols=\"2\">\n")
        f.write("          <thead>\n")
        f.write("            <row>\n")
        f.write("              <entry>Name</entry>\n")
        f.write("              <entry>Type</entry>\n")
        f.write("            </row>\n")
        f.write("          </thead>\n")
        f.write("          <tbody>\n")
        for a in self.args:
            a.write_docbook(f)
        f.write("          </tbody>\n")
        f.write("        </tgroup>\n")
        f.write("      </table>\n")
        f.write("    </section>\n")


class EventArgument(object):
    def __init__(self, name, type):
        self.name = name
        self.type = type

    def write_docbook(self, f):
        f.write("            <row>\n")
        f.write("              <entry><para><literal>%s</literal></para></entry>\n" % self.name)
        f.write("              <entry><para><literal>%s</literal></para></entry>\n" % self.type)
        f.write("            </row>\n")


class Class(object):
    def __init__(self, name):
        self.name = name
        self.properties = []
        self.statistics = []
        self.methods = []

    def write_docbook(self, f):
        f.write("    <section>\n")
        f.write("      <title>Class: %s</title>\n" % self.name)

        if len(self.properties):
            f.write("      <table>\n")
            f.write("        <title>Properties for Class: <literal>%s</literal></title>\n"
                    % self.name)
            f.write("        <tgroup cols=\"5\">\n")
            f.write("          <thead>\n")
            f.write("            <row>\n")
            f.write("              <entry>Name</entry>\n")
            f.write("              <entry>Type</entry>\n")
            f.write("              <entry>Access</entry>\n")
            f.write("              <entry>Description</entry>\n")
            f.write("              <entry>Comments</entry>\n")
            f.write("            </row>\n")
            f.write("          </thead>\n")
            f.write("          <tbody>\n")
            for p in self.properties:
                p.write_docbook(f)
            f.write("          </tbody>\n")
            f.write("        </tgroup>\n")
            f.write("      </table>\n")

        if len(self.statistics):
            f.write("      <table>\n")
            f.write("        <title>Statistics for Class: <literal>%s</literal></title>\n"
                    % self.name)
            f.write("        <tgroup cols=\"4\">\n")
            f.write("          <thead>\n")
            f.write("            <row>\n")
            f.write("              <entry>Name</entry>\n")
            f.write("              <entry>Type</entry>\n")
            f.write("              <entry>Unit</entry>\n")
            f.write("              <entry>Description</entry>\n")
            f.write("            </row>\n")
            f.write("          </thead>\n")
            f.write("          <tbody>\n")
            for s in self.statistics:
                s.write_docbook(f)
            f.write("          </tbody>\n")
            f.write("        </tgroup>\n")
            f.write("      </table>\n")

        if len(self.methods):
            f.write("      <section>\n")
            f.write("      <title>Methods</title>\n")
            for m in self.methods:
                m.write_docbook(f)
            f.write("      </section>\n")

        f.write("    </section>\n")


class Property(object):
    def __init__(self, name, type, access, desc, more_info):
        self.name = name
        self.type = type
        self.access = access
        self.desc = desc
        self.more_info = more_info

    def write_docbook(self, f):
        f.write("            <row>\n")
        f.write("              <entry><para><literal>%s</literal></para></entry>\n" % self.name)
        f.write("              <entry><para><literal>%s</literal></para></entry>\n" % self.type)
        f.write("              <entry><para><literal>%s</literal></para></entry>\n" % self.access)
        f.write("              <entry><para>%s</para></entry>\n" % self.desc)
        if self.more_info:
            f.write("              <entry>%s</entry>\n" % self.more_info)
        else:
            f.write("              <entry><para></para></entry>\n")
        f.write("            </row>\n")


class Statistic(object):
    def __init__(self, name, type, desc):
        self.name = name
        self.type = type
        self.desc = desc
        self.unit = ""

    def write_docbook(self, f):
        f.write("            <row>\n")
        f.write("              <entry><para><literal>%s</literal></para></entry>\n" % self.name)
        f.write("              <entry><para><literal>%s</literal></para></entry>\n" % self.type)
        f.write("              <entry><para><literal>%s</literal></para></entry>\n" % self.unit)
        f.write("              <entry><para>%s</para></entry>\n" % self.desc)
        f.write("            </row>\n")


class Method(object):
    def __init__(self, name, desc, more_info):
        self.name = name
        self.desc = desc
        self.more_info = more_info
        self.args = []

    def write_docbook(self, f):
        f.write("        <section>\n")
        f.write("        <title>Method: <literal>%s</literal></title>\n" % self.name)
        f.write("        <para>%s</para>\n" % self.desc)
        if self.more_info:
            f.write("%s\n" % self.more_info)
        if len(self.args):
            f.write("        <table>\n")
            f.write("          <title>Arguments for Method: <literal>%s</literal></title>\n"
                    % self.name)
            f.write("          <tgroup cols=\"3\">\n")
            f.write("            <thead>\n")
            f.write("              <row>\n")
            f.write("                <entry>Name</entry>\n")
            f.write("                <entry>Direction</entry>\n")
            f.write("                <entry>Type</entry>\n")
            f.write("              </row>\n")
            f.write("            </thead>\n")
            f.write("            <tbody>\n")
            for a in self.args:
                a.write_docbook(f)
            f.write("            </tbody>\n")
            f.write("          </tgroup>\n")
            f.write("        </table>\n")
        f.write("        </section>\n")


class MethodArgument(object):
    def __init__(self, name, dir, type):
        self.name = name
        self.dir = dir
        self.type = type

    def write_docbook(self, f):
        f.write("              <row>\n")
        f.write("                <entry><para><literal>%s</literal></para></entry>\n" % self.name)
        f.write("                <entry><para><literal>%s</literal></para></entry>\n" % self.dir)
        f.write("                <entry><para><literal>%s</literal></para></entry>\n" % self.type)
        f.write("              </row>\n")


class SchemaCallbacks(object):
    def __init__(self, parser):
        self.parser = parser
        self.cur_schema = None
        self.cur_class = None
        self.cur_method = None
        self.in_event_args = False
        self.last_comment = None

    def __handle_arg(self, attrs):
        if self.in_event_args:
            event_arg = EventArgument(attrs["name"], attrs["type"])
            self.cur_schema.event_args[attrs["name"]] = event_arg
        elif self.cur_method:
            method_arg = MethodArgument(attrs["name"], attrs["dir"],
                                        attrs["type"])
            self.cur_method.args.append(method_arg)
        else:
            sys.stderr.write("Unexpected arg with attrs: %s\n" % attrs)

    def startElement(self, tag, attrs):
        tag = tag.lower()

        if tag == "schema":
            self.cur_schema = Schema()
            self.parser.schemas.append(self.cur_schema)
            self.cur_schema.package = attrs["package"]
        elif tag == "eventarguments":
            self.in_event_args = True
        elif tag == "arg":
            self.__handle_arg(attrs)
        elif tag == "event":
            self.cur_schema.events.append(Event(self.cur_schema, attrs["name"],
                                                attrs["args"]))
        elif tag == "class":
            self.cur_class = Class(attrs["name"])
            self.cur_schema.classes.append(self.cur_class)
        elif tag == "method":
            self.cur_method = Method(attrs["name"], attrs["desc"],
                                     self.last_comment)
            self.cur_class.methods.append(self.cur_method)
        elif tag == "property":
            self.cur_class.properties.append(Property(attrs["name"],
                                attrs["type"], attrs["access"], attrs["desc"],
                                self.last_comment))
        elif tag == "statistic":
            stat = Statistic(attrs["name"], attrs["type"], attrs["desc"])
            self.cur_class.statistics.append(stat)
            if "unit" in attrs:
                stat.unit = attrs["unit"]
        else:
            sys.stderr.write("Unexpected startElement %s %s\n" % (tag, attrs))

        self.last_comment = None

    def endElement(self, tag):
        tag = tag.lower()

        if tag == "schema":
            self.cur_schema = None
        elif tag == "eventarguments":
            self.in_event_args = False
        elif tag == "class":
            self.cur_class = None
        elif tag == "method":
            self.cur_method = None

    def comment(self, msg):
        self.last_comment = msg

    def warning(self, msg):
        sys.stderr.write("warning: %s\n" % (msg))

    def error(self, msg):
        sys.stderr.write("error: %s\n" % (msg))

    def fatalError(self, msg):
        sys.stderr.write("fatalError: %s\n" % (msg))


class SchemaParser(object):
    def __init__(self, fn):
        self.schemas = []
        self.fn = fn
        self.__parse_schema()

    def __parse_schema(self):
        # XXX I'm sure there is a way to do this without pulling it into memory
        ctxt = libxml2.createPushParser(SchemaCallbacks(self), "", 0, self.fn)
        schema = open(self.fn, "r").read()
        ctxt.parseChunk(schema, len(schema), 1)

    def write_docbook(self, f):
        for s in self.schemas:
            s.write_docbook(f)


def main(argv=None):
    if argv is None:
        argv = sys.argv

    if len(argv) != 2:
        sys.stderr.write("Usage: %s <schema.xml>\n" % argv[0])
        return 1

    SchemaParser(argv[1]).write_docbook(sys.stdout)

    return 0


if __name__ == "__main__":
    sys.exit(main())
