<?xml version="1.0" encoding="ISO-8859-1"?>
<xsl:stylesheet version="1.0"
                xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

<xsl:output method="text" indent="no" />
<xsl:strip-space elements="*" />

<xsl:template match="/">
    <xsl:for-each select="node/interface">
        <xsl:call-template name="enum" />
        <xsl:text>

</xsl:text>
        <xsl:call-template name="struct" />
    </xsl:for-each>
</xsl:template>

<xsl:variable name="smallcase" select="'abcdefghijklmnopqrstuvwxyz'" />
<xsl:variable name="uppercase" select="'ABCDEFGHIJKLMNOPQRSTUVWXYZ'" />

<xsl:template name="struct">
    <xsl:text>typedef struct {
    enum Prop prop;
    gchar *name, *nick, *desc;
    GParamFlags flags;
    char type;
} Property;

</xsl:text>
    <xsl:value-of select="concat('Property properties_', substring-after(substring-after(@name, '.'), '.'))" />
    <xsl:text>[] = {&#10;</xsl:text>
    <xsl:for-each select="property">
        <xsl:text>    { </xsl:text>
        <xsl:value-of select="concat('PROP_', translate(@name, $smallcase, $uppercase))" />, "<xsl:value-of select="@name" />", "<xsl:value-of select="@name" />
        <xsl:text>", "</xsl:text>
        <xsl:value-of select="preceding::comment()[1]" />
        <xsl:text>", </xsl:text>
        <xsl:choose>
            <xsl:when test="@access='RW'">
                <xsl:text>G_PARAM_WRITABLE</xsl:text>
            </xsl:when>
            <xsl:otherwise>
                <xsl:text>G_PARAM_READABLE</xsl:text>
            </xsl:otherwise>
        </xsl:choose>
        <xsl:text>, '</xsl:text><xsl:value-of select="@type" /><xsl:text>'</xsl:text>
        <xsl:text> },&#10;</xsl:text>
    </xsl:for-each>
    <xsl:text>    { PROP_0, NULL, NULL, NULL, 0, '\0' },&#10;</xsl:text>
    <xsl:text>};</xsl:text>
    <xsl:apply-templates />
</xsl:template>

<xsl:template name="enum">
    <xsl:text>enum Prop {
    PROP_0,
    </xsl:text>
    <xsl:for-each select="property">
        <xsl:value-of select="concat('PROP_', translate(@name, $smallcase, $uppercase))" />,
    </xsl:for-each>};
</xsl:template>
</xsl:stylesheet>