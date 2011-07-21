<?xml version="1.0" encoding="ISO-8859-1"?>
<!-- Edited with XML Spy v2007 (http://www.altova.com) -->
<xsl:stylesheet version="1.0"
xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<xsl:output method='text' version='1.0' encoding='UTF-8' indent='yes'/>

<xsl:template match="property">
  <xsl:text>      </xsl:text>
  <xsl:value-of select="@name"/>
  <xsl:text> (</xsl:text>
  <xsl:apply-templates select="@type" />
  <xsl:text>) 		</xsl:text>
  <xsl:value-of select="@access"/>
  <xsl:text> - </xsl:text>
  <xsl:value-of select="@desc"/>
</xsl:template>

<xsl:template match="statistic">
  <xsl:text>      </xsl:text>
  <xsl:value-of select="@name"/>
  <xsl:text> (</xsl:text>
  <xsl:apply-templates select="@type" />
  <xsl:text>) 	</xsl:text>
  <xsl:text>- </xsl:text>
  <xsl:value-of select="@desc"/>
</xsl:template>

<xsl:template match="method">
  <xsl:text>      </xsl:text>
  <xsl:value-of select="@desc"/>
  <xsl:text>
           - </xsl:text>
  <xsl:value-of select="@name"/>
  <xsl:text>(</xsl:text>
   <xsl:for-each select="arg">
     <xsl:value-of select="@type" />
     <xsl:text> </xsl:text>
     <xsl:apply-templates select="@dir" />
     <xsl:value-of select="@name"/>     
     <xsl:text>, </xsl:text>
     <xsl:if test="position() = 5">
       <xsl:text>
              </xsl:text>
     </xsl:if>
     <xsl:if test="position() = 10">
       <xsl:text>
              </xsl:text>
     </xsl:if>
   </xsl:for-each>
  <xsl:text>)
</xsl:text>
</xsl:template>

<xsl:template name="print_event_args">
  <xsl:param name="arglist"/>
  
   <xsl:for-each select="/schema/eventArguments/arg">
       <!--xsl:text>Searching </xsl:text>
       <xsl:value-of select="$arglist"/>     
       <xsl:text>for </xsl:text>
       <xsl:value-of select="@name"/>     
       <xsl:text>
</xsl:text-->
     <xsl:if test="contains($arglist, @name)"> 
       <xsl:text>
           - </xsl:text>
       <xsl:value-of select="@name"/>     
       <xsl:text> (</xsl:text>
       <xsl:apply-templates select="@type" />
       <xsl:text>)</xsl:text>
     </xsl:if>
   </xsl:for-each>
</xsl:template>

<xsl:template match="event">
  <xsl:text>         </xsl:text>
  <xsl:value-of select="@name"/>
  <xsl:text> 		</xsl:text>
      <xsl:call-template name="print_event_args">
	<xsl:with-param name="arglist"><xsl:value-of select="@args"/></xsl:with-param>
      </xsl:call-template>
  <xsl:text>
</xsl:text>
</xsl:template>

<xsl:template match="class">
  <xsl:value-of select="/schema/@package"/>.<xsl:value-of select="@name"/>:
  <xsl:apply-templates/>
  <xsl:text>    Events:
</xsl:text>
  <xsl:apply-templates select="/schema/event" />
  <xsl:text>
</xsl:text>
</xsl:template>

<xsl:template match="eventArguments"/>

<xsl:template match="@type">
  <xsl:choose>
    <xsl:when test=". = 'sstr'">
      <xsl:text>Short string</xsl:text>
    </xsl:when>
    <xsl:when test=". = 'absTime'">
      <xsl:text>Date/time</xsl:text>
    </xsl:when>
    <xsl:when test=". = 'uint32'">
      <xsl:text>32-bit Integer</xsl:text>
    </xsl:when>
    <xsl:when test=". = 'uint64'">
      <xsl:text>64-bit Integer</xsl:text>
    </xsl:when>
    <xsl:when test=". = 'list'">
      <xsl:text>List</xsl:text>
    </xsl:when>
    <xsl:when test=". = 'map'">
      <xsl:text>Map</xsl:text>
    </xsl:when>
    <xsl:otherwise>
      <xsl:value-of select="."/>
    </xsl:otherwise>    
  </xsl:choose>
</xsl:template>

<xsl:template match="comment()">
  <xsl:value-of select="."/>
</xsl:template>
  
<xsl:template match="@dir">
  <xsl:if test=". = 'IO'">
    <xsl:text>&amp;</xsl:text>
  </xsl:if>
  <xsl:if test=". = 'O'">
    <xsl:text>*</xsl:text>
  </xsl:if>
  <xsl:if test=". = 'I'">
    <xsl:text></xsl:text>
  </xsl:if>
</xsl:template>

<xsl:template match="schema">
  <xsl:apply-templates select="class" />
</xsl:template>

<xsl:template match="/">
  <xsl:apply-templates/>
</xsl:template>

<xsl:template match="*">
  <xsl:text>Unknown Object:</xsl:text>
  <xsl:text> </xsl:text>
  <xsl:value-of select="name()"/>
  <xsl:apply-templates select="@*"/>
  <xsl:apply-templates select="node()" />
</xsl:template>

<xsl:template match="@*">
  <xsl:value-of select="name()"/>
  <xsl:text>=</xsl:text>
  <xsl:value-of select="."/>
  <xsl:text> </xsl:text>
</xsl:template>

</xsl:stylesheet>
