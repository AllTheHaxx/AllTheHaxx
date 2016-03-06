const char *SOURCE_MOTION_BLUR_VERTEX =
"varying vec4 v_Color;\n"
"void main()\n"
"{\n"
"	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
"	gl_TexCoord[0] = gl_MultiTexCoord0;\n"
"	v_Color = gl_Color;\n"
"}\n";