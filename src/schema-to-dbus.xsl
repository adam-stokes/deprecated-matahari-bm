<?xml version="1.0" encoding="ISO-8859-1"?>
<xsl:stylesheet version="1.0"
                xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
                xmlns:exsl="http://exslt.org/common"
                extension-element-prefixes="exsl">

<!-- This xsl transformation takes schema.xml and converts it to multiple
     DBus interfaces, one per each class.

     The DBus files will be name org.matahariproject.class_name.xml and will
     be created in current directory. Description of the properties/methods
     will be added as a comment for future processing.
-->

<xsl:template match="/">
    <xsl:variable name="package">
        <xsl:value-of select="schema/@package" />
    </xsl:variable>
    <xsl:variable name="filename" select="concat($package, '.xml')" />
    <exsl:document method="xml" indent="yes" version="1.0" encoding="utf-8" href="{$filename}">
        <node>
            <xsl:for-each select="schema/class">
                <interface>
                    <xsl:attribute name="name">
                        <xsl:value-of select="concat($package, '.', @name)" />
                    </xsl:attribute>
                    <annotation name="org.freedesktop.DBus.GLib.CSymbol">
                        <xsl:attribute name="value">
                            <xsl:value-of select="@name"/>
                        </xsl:attribute>
                    </annotation>
                    <xsl:for-each select="property|statistic">
                        <!-- What with doc? Add as comment now. -->
                        <xsl:if test="@desc">
                            <xsl:comment>
                                <xsl:value-of select="@desc" />
                            </xsl:comment>
                        </xsl:if>
                        <property>
                            <xsl:attribute name="name">
                                <xsl:value-of select="@name" />
                            </xsl:attribute>

                            <!-- Tranform data types -->
                            <xsl:call-template name="type" />

                            <!-- Tranform access rights -->
                            <xsl:call-template name="access" />
                        </property>
                    </xsl:for-each>

                    <!-- Methods -->
                    <xsl:for-each select="method">
                        <!-- What with doc? Add as comment now. -->
                        <xsl:if test="@desc">
                            <xsl:comment>
                                <xsl:value-of select="@desc" />
                            </xsl:comment>
                        </xsl:if>
                        <method>
                            <xsl:attribute name="name">
                                <xsl:value-of select="@name" />
                            </xsl:attribute>
                            <xsl:if test="not(arg[@dir='O'])">
                                <annotation name="org.freedesktop.DBus.GLib.NoReply" value="yes" />
                            </xsl:if>
                            <annotation name="org.freedesktop.DBus.GLib.Async" value="" />
                            <xsl:for-each select="arg">
                                <!-- DBus doesn't support in-out arguments, so two separate
                                arguments will be created, one for IN, second for OUT -->
                                <arg>
                                <xsl:attribute name="name">
                                    <xsl:choose>
                                        <xsl:when test="@dir='IO'">
                                            <xsl:value-of select="concat(@name, '_in')" />
                                        </xsl:when>
                                        <xsl:otherwise>
                                            <xsl:value-of select="@name" />
                                        </xsl:otherwise>
                                    </xsl:choose>
                                </xsl:attribute>

                                <!-- Tranform data types -->
                                <xsl:call-template name="type" />

                                <!-- Parameter direction-->
                                <xsl:call-template name="dir" />

                                </arg>

                                <!-- Add output argument if direction is IO -->
                                <xsl:if test="@dir='IO'">
                                    <arg>
                                        <xsl:attribute name="name">
                                            <xsl:value-of select="concat(@name, '_out')" />
                                        </xsl:attribute>

                                        <xsl:call-template name="type" />

                                        <xsl:attribute name="direction">out</xsl:attribute>
                                    </arg>
                                </xsl:if>
                            </xsl:for-each>
                        </method>
                    </xsl:for-each>
                </interface>
            </xsl:for-each>
            <!-- Get and set function will be used for accessing parameters.
                    It enables to obtain authorization via Polkit -->
            <interface name="org.freedesktop.DBus.Properties">
                <method name="Get">
                    <annotation name="org.freedesktop.DBus.GLib.Async" value=""/>
                    <arg name="interface" direction="in" type="s"/>
                    <arg name="property" direction="in" type="s"/>
                    <arg name="value" direction="out" type="v"/>
                </method>
                <method name="Set">
                    <annotation name="org.freedesktop.DBus.GLib.Async" value=""/>
                    <arg name="interface" direction="in" type="s"/>
                    <arg name="property" direction="in" type="s"/>
                    <arg name="value" direction="in" type="v"/>
                </method>
            </interface>
        </node>
    </exsl:document>
</xsl:template>

<xsl:template name="type">
    <!-- Transform data types -->
    <xsl:choose>
        <xsl:when test="@type='list'">
            <!-- Convert list to list of strings -->
            <xsl:attribute name="type">as</xsl:attribute>
        </xsl:when>

        <xsl:when test="@type='int8'">
            <xsl:attribute name="type">n</xsl:attribute>
        </xsl:when>
        <xsl:when test="@type='int16'">
            <xsl:attribute name="type">n</xsl:attribute>
        </xsl:when>
        <xsl:when test="@type='int32'">
            <xsl:attribute name="type">i</xsl:attribute>
        </xsl:when>
        <xsl:when test="@type='int64'">
            <xsl:attribute name="type">x</xsl:attribute>
        </xsl:when>
        <xsl:when test="@type='uint8'">
            <xsl:attribute name="type">y</xsl:attribute>
        </xsl:when>
        <xsl:when test="@type='uint16'">
            <xsl:attribute name="type">q</xsl:attribute>
        </xsl:when>
        <xsl:when test="@type='uint32'">
            <xsl:attribute name="type">u</xsl:attribute>
        </xsl:when>
        <xsl:when test="@type='uint64'">
            <xsl:attribute name="type">t</xsl:attribute>
        </xsl:when>
        <xsl:when test="@type='bool'">
            <xsl:attribute name="type">b</xsl:attribute>
        </xsl:when>
        <xsl:when test="@type='double'">
            <xsl:attribute name="type">d</xsl:attribute>
        </xsl:when>
        <xsl:when test="@type='float'">
            <xsl:attribute name="type">d</xsl:attribute>
        </xsl:when>
        <xsl:when test="@type='sstr'">
            <xsl:attribute name="type">s</xsl:attribute>
        </xsl:when>
        <xsl:when test="@type='lstr'">
            <xsl:attribute name="type">s</xsl:attribute>
        </xsl:when>

        <xsl:when test="@type='list'">
            <!-- Convert list to list of strings -->
            <xsl:attribute name="type">as</xsl:attribute>
        </xsl:when>

        <!-- Warning: Uncompatible -->
        <xsl:when test="@type='absTime'">
            <xsl:attribute name="type">i</xsl:attribute>
        </xsl:when>
        <xsl:when test="@type='deltaTime'">
            <xsl:attribute name="type">i</xsl:attribute>
        </xsl:when>

        <xsl:when test="@type='uuid'">
            <xsl:attribute name="type">x</xsl:attribute>
        </xsl:when>
        <xsl:when test="@type='map'">
            <xsl:attribute name="type">a{sv}</xsl:attribute>
        </xsl:when>

        <!-- TODO: check if compatible -->
        <xsl:when test="@type='objId'">
            <xsl:attribute name="type">o</xsl:attribute>
        </xsl:when>
    </xsl:choose>
</xsl:template>

<xsl:template name="access">
    <xsl:choose>
        <xsl:when test="@access='RW'">
            <xsl:attribute name="access">readwrite</xsl:attribute>
        </xsl:when>
        <xsl:when test="@access='RO'">
            <xsl:attribute name="access">read</xsl:attribute>
        </xsl:when>
        <!-- No Read-Create in DBus -->
        <xsl:when test="@access='RC'">
            <xsl:attribute name="access">readwrite</xsl:attribute>
        </xsl:when>
        <xsl:otherwise>
            <xsl:attribute name="access">read</xsl:attribute>
        </xsl:otherwise>
    </xsl:choose>
</xsl:template>

<xsl:template name="dir">
    <xsl:choose>
        <xsl:when test="@dir='O'">
            <xsl:attribute name="direction">out</xsl:attribute>
        </xsl:when>
        <xsl:otherwise>
            <xsl:attribute name="direction">in</xsl:attribute>
        </xsl:otherwise>
    </xsl:choose>
</xsl:template>

</xsl:stylesheet>
