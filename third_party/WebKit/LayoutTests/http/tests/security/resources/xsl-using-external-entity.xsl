<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE doc [ <!ENTITY ent SYSTEM "http://localhost:8000/security/resources/target.xml"> ]>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<xsl:template match="*">
<html>
<body>
<script>
if (window.layoutTestController)
  layoutTestController.dumpAsText();
</script>
<div>This test includes a cross-origin external entity.  It passes if the load
fails and thus there is no text below this line.</div>
<div>&ent;</div>
</body>
</html>
</xsl:template>
</xsl:stylesheet>

