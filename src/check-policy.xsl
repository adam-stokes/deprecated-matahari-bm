<?xml version="1.0" encoding="ISO-8859-1"?>
<xsl:stylesheet version="1.0"
                xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

<xsl:output method="text" indent="no" />

<!--
This XSL transformation takes XML schema for QMF and check if for each
property|statistics|method is defined PolicyKit action in
"$schema.$class.policy" file.

Missing action as well as other errors (like missing .policy file) will be
written to standard error output.
-->

<xsl:template match="/">
    <xsl:for-each select="schema">
        <xsl:variable name="schema">
            <xsl:value-of select="@package"/>
        </xsl:variable>
        <xsl:for-each select="class">
            <xsl:variable name="class">
                <xsl:value-of select="@name"/>
            </xsl:variable>
            <xsl:for-each select="property|statistics|method">
                <xsl:variable name="action">
                    <xsl:value-of select="concat($schema, '.', $class, '.', @name)"/>
                </xsl:variable>
                <xsl:choose>
                    <xsl:when test="document(concat($schema, '.', $class, '.policy'))/policyconfig/action/@id = $action">
                        <!-- OK, action exists -->
                    </xsl:when>
                    <xsl:otherwise>
                        <xsl:message><xsl:value-of select="$action"/></xsl:message>
                    </xsl:otherwise>
                </xsl:choose>
            </xsl:for-each>
        </xsl:for-each>
    </xsl:for-each>
</xsl:template>

</xsl:stylesheet>
