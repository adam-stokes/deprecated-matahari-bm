<?xml version="1.0" encoding="ISO-8859-1"?>
<xsl:stylesheet version="1.0"
                xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

<xsl:output method="text" indent="no" />
<xsl:strip-space elements="*" />

<!-- This xsl file is for tranforming dbus interface file to
     C code that can be used by glib-dbus for automated generating
     of dbus properties.

     It create enum named Prop with list of properties for internal
     identification.

     Then it create struct Property which contains enum of property, its name,
     nick, description, flags (readable, readwrite) and type. Type is then
     used to automaticly create the property from dbus.

     Last step is to create array of Property structs with info about all
     properties.
-->

<xsl:template match="/">
    <!-- Do not generate anything for interface Properies -->
    <xsl:if test="/node/interface/@name != 'org.freedesktop.DBus.Properties'">
        <!-- Create enum of the properties -->
        <xsl:call-template name="enum" />
        <!-- Define Property struct and create array of it with properties
        info -->
        <xsl:call-template name="struct" />
    </xsl:if>
</xsl:template>

<!-- This is needed for converting properties names to uppercase.
     The dash is always converted to underscore, so C compiler doesn't
     interpret is as minus. -->
<xsl:variable name="smallcase" select="'abcdefghijklmnopqrstuvwxyz-'" />
<xsl:variable name="uppercase" select="'ABCDEFGHIJKLMNOPQRSTUVWXYZ_'" />

<xsl:template name="enum">
    <!-- Create enum of the properties and first property with id 0 -->
    <xsl:text>enum Prop {
    PROP_0,
    </xsl:text>
    <!-- Iterate through all properties -->
    <xsl:for-each select="/node/interface/property">
        <xsl:value-of select="translate(concat('PROP_', ../annotation/@value, '_', @name), $smallcase, $uppercase)" />,
    </xsl:for-each>
    <xsl:text>};&#10;</xsl:text>
</xsl:template>

<xsl:template name="struct">
    <!-- Create the array of Property structs -->
    <xsl:text>Property properties[] = {&#10;</xsl:text>
    <!-- Iterate through all properties -->
    <xsl:for-each select="/node/interface/property">
        <xsl:text>    { </xsl:text>
        <!-- Enum of the item -->
        <xsl:value-of select="translate(concat('PROP_', ../annotation/@value, '_', @name), $smallcase, $uppercase)" />
        <xsl:text>, "</xsl:text>

        <!-- Name of the item -->
        <xsl:value-of select="@name" />
        <xsl:text>", "</xsl:text>

        <!-- Nickname - same as the name -->
        <xsl:value-of select="@name" />
        <xsl:text>", "</xsl:text>

        <!-- Description is taken from the comment that is preceding the item -->
        <xsl:value-of select="preceding::comment()[1]" />
        <xsl:text>", </xsl:text>

        <!-- Flags - readable or readwrite -->
        <xsl:choose>
            <xsl:when test="@access='readwrite'">
                <xsl:text>G_PARAM_READWRITE</xsl:text>
            </xsl:when>
            <xsl:otherwise>
                <xsl:text>G_PARAM_READABLE</xsl:text>
            </xsl:otherwise>
        </xsl:choose>
        <xsl:text>, '</xsl:text>

        <!-- Type of the property - some must be converted -->
        <xsl:choose>
            <!-- Convert dict to type 'e' -->
            <xsl:when test="@type='a{sv}'">
                <xsl:text>e</xsl:text>
            </xsl:when>
            <xsl:otherwise>
                <xsl:value-of select="@type" />
            </xsl:otherwise>
            </xsl:choose>
        <xsl:text>'</xsl:text>
        <xsl:text> },&#10;</xsl:text>
    </xsl:for-each>
    <!-- This item is used as sentinel node -->
    <xsl:text>    { PROP_0, NULL, NULL, NULL, 0, '\0' },&#10;</xsl:text>
    <xsl:text>};</xsl:text>
</xsl:template>

</xsl:stylesheet>
