const char *SOURCE_HEXAGON_VERTEX =

"varying vec4 Vertex_UV;\n"
"varying vec4 v_Color;\n"
"void main()\n"
"{\n"
"	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n"
"	Vertex_UV = gl_TexCoord[0] = gl_MultiTexCoord0;\n"
"	v_Color = gl_Color;\n"
"}\n";