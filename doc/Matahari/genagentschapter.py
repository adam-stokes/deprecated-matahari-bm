#!/usr/bin/env python
'''
Generate en-US/Agents.xml chapter.

Copyright (C) 2011, Red Hat, Inc.
Russell Bryant <rbryant@redhat.com>
'''

import sys
from schematodocbook import SchemaParser


AGENTS = [
    ("../../src/host/schema.xml", "Host"),
    ("../../src/service/schema.xml", "Service"),
    ("../../src/sysconfig/schema.xml", "Sysconfig"),
    ("../../src/network/schema.xml", "Network")
]


def main(argv=None):
    f = open("en-US/Agents.xml", "w")
    f.write(open("en-US/Agents.xml.begin", "r").read())
    for a in AGENTS:
        parser = SchemaParser(a[0])
        f.write("<section>\n")
        f.write("  <title>Agent: <literal>%s</literal></title>\n" % a[1])
        parser.write_docbook(f)
        f.write("</section>\n")
    f.write(open("en-US/Agents.xml.end", "r").read())
    f.close()

    return 0


if __name__ == "__main__":
    sys.exit(main())
