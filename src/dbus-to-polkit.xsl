<?xml version="1.0" encoding="ISO-8859-1"?>
<xsl:stylesheet version="1.0"
                xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

<xsl:output method="xml"
            indent="yes"
            doctype-public="-//freedesktop//DTD PolicyKit Policy Configuration 1.0//EN"
            doctype-system="http://www.freedesktop.org/standards/PolicyKit/1.0/policyconfig.dtd" />

<xsl:template match="/">
    <policyconfig>
    <vendor>Matahari</vendor>
    <vendor_url>https://fedorahosted.org/matahari/</vendor_url>
    <!-- TODO: icon <icon_name>matahari</icon_name> -->
    <xsl:variable name="interface">
        <xsl:value-of select="node/interface/@name" />
    </xsl:variable>
    <xsl:for-each select="node/interface/property|node/interface/statistics|node/interface/method">
        <action>
            <xsl:attribute name="id">
                <xsl:value-of select="concat($interface, '.', @name)" />
            </xsl:attribute>
            <!-- TODO: description -->
            <!-- TODO: message -->
            <defaults>
                <allow_any>no</allow_any>
                <allow_inactive>no</allow_inactive>
                <allow_active>
                    <xsl:choose>
                        <xsl:when test="@access='read'">
                            <xsl:text>yes</xsl:text>
                        </xsl:when>
                        <xsl:otherwise>
                            <xsl:text>auth_admin</xsl:text>
                        </xsl:otherwise>
                    </xsl:choose>
                </allow_active>
            </defaults>
        </action>
    </xsl:for-each>
    </policyconfig>
</xsl:template>

</xsl:stylesheet>
