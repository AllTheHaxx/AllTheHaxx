const char *SOURCE_STEREOSCOPE_FRAGMENT =
"#version 150\n"
"in vec3 ex_Color;\n"
"in vec2 ex_TexCoord;\n"
"out vec4 out_Color;\n"
"uniform sampler2D Left;\n"
"uniform sampler2D Right;\n"
"void main()\n"
"{\n"
"	if(mod(trunc(gl_FragCoord.y), 2.0) < 0.5)\n"
"		out_Color = texture2D(Left, vec2(ex_TexCoord));\n"
"	else\n"
"		out_Color = texture2D(Right, vec2(ex_TexCoord));\n"
"}\n";