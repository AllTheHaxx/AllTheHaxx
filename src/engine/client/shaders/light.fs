const char *SOURCE_LIGHT_FRAGMENT =
"uniform float u_IsTex;\n"
"uniform sampler2D u_Tex;\n"
"uniform vec2 u_Resolution;\n"
"varying vec4 v_Color;\n"
"uniform vec2 u_Mouse;\n"

"void main()\n"
"{\n"
"	vec3 Res = vec3(u_Resolution, u_Resolution.x / u_Resolution.y);\n" //res.z = aspect ratio
"	vec4 color = texture2D(u_Tex, gl_TexCoord[0].xy);\n"
"	float d = distance(vec2(gl_TexCoord[0].x, gl_TexCoord[0].y/Res.z), vec2(0.5 + 1/Res.x*u_Mouse.x , (0.5 - 1/Res.y*u_Mouse.y)/Res.z));\n"
"	color.rgb *= smoothstep(0.15, 0.001, d) + 0.005;\n"
"	gl_FragColor = color;\n"
"}\n";