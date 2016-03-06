const char *SOURCE_COLOR_FRAGMENT =
"uniform float u_IsTex;\n"
"uniform sampler2D u_Tex;\n"
"uniform vec3 u_Color;\n"
"varying vec4 v_Color;\n"
"void main()\n"
"{\n"
"	vec4 Color;\n"
"	if(u_IsTex == 1.0)\n"
"		Color = texture2D(u_Tex, gl_TexCoord[0].st);\n"
"	else\n"
"		Color = v_Color;\n"
"	gl_FragColor = vec4(u_Color.rgb, Color.a);\n"
"}\n";