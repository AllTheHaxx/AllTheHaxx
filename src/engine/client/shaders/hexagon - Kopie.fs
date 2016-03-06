// Created by inigo quilez - iq/2014
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

const char *SOURCE_HEXAGON_FRAGMENT =

"uniform float u_Time;\n"
"uniform vec2 u_Resolution;\n"
"uniform float u_IsTex;\n"
"uniform sampler2D u_Tex;\n"
"varying vec4 v_Color;\n"

"vec4 hexagon(vec2 p)\n"
"{\n"
"	vec2 q = vec2( p.x*2.0*0.5773503, p.y + p.x*0.5773503 );\n"
"	vec2 pi = floor(q);\n"
"	vec2 pf = fract(q);\n"
"	float v = mod(pi.x + pi.y, 3.0);\n"
"	float ca = step(1.0,v);\n"
"	float cb = step(2.0,v);\n"
"	vec2  ma = step(pf.xy,pf.yx);\n"
// distance to borders
"	float e = dot( ma, 1.0-pf.yx + ca*(pf.x+pf.y-1.0) + cb*(pf.yx-2.0*pf.xy) );\n"
"	// distance to center	\n"
"	p = vec2( q.x + floor(0.5+p.y/1.5), 4.0*p.y/3.0 )*0.5 + 0.5;\n"
"	float f = length( (fract(p) - 0.5)*vec2(1.0,0.85) );		\n"
"	return vec4( pi + ca - cb*ma, e, f );\n"
"}\n"

"float hash1( vec2  p ) { float n = dot(p,vec2(127.1,311.7) ); return fract(sin(n)*43758.5453); }\n"

"float noise( in vec3 x )\n"
"{\n"
"    vec3 p = floor(x);\n"
"    vec3 f = fract(x);\n"
"	f = f*f*(3.0-2.0*f);\n"
"	vec2 uv = (p.xy+vec2(37.0,17.0)*p.z) + f.xy;\n"
"	vec2 rg = texture2D( u_Tex, (uv+0.5)/256.0, -100.0 ).yx;\n"
"	return mix( rg.x, rg.y, f.z );\n"
"}\n"

"void main( void ) \n"
"{\n"
"    vec2 uv = gl_FragCoord.xy/u_Resolution.xy;\n"
"	vec2 pos = (-u_Resolution.xy + 2.0*gl_FragCoord.xy)/u_Resolution.y;\n"
// distort
"	pos *= 1.0 - 0.2*length(pos);\n"
// gray
"	vec4 h = hexagon(8.0*pos + 0.5*u_Time);\n"
"	float n = noise( vec3(0.3*h.xy+u_Time*0.1,u_Time) );\n"
"	vec3 col = 0.15 + 0.15*hash1(h.xy+1.2)*vec3(1.0);\n"
"	col *= smoothstep( 0.10, 0.11, h.z );\n"
"	col *= smoothstep( 0.10, 0.11, h.w );\n"
"	col *= 1.0 + 0.15*sin(40.0*h.z);\n"
"	col *= 0.75 + 0.5*h.z*n;\n"
// red
"	h = hexagon(6.0*pos + 0.6*u_Time);\n"
"	n = noise( vec3(0.3*h.xy+u_Time*0.1,u_Time) );\n"
"	vec3 colb = 0.9 + 0.8*sin( hash1(h.xy)*1.5 + 2.0 + vec3(0.0,1.0,1.0) );\n"
"	colb *= smoothstep( 0.10, 0.11, h.z );\n"
"	colb *= 1.0 + 0.15*sin(40.0*h.z);\n"
"	colb *= 0.75 + 0.5*h.z*n;\n"
"	h = hexagon(6.0*(pos+0.1*vec2(-1.3,1.0)) + 0.6*u_Time);\n"
"   col *= 1.0-0.8*smoothstep(0.45,0.451,noise( vec3(0.3*h.xy+u_Time*0.1,u_Time) ));\n"
"	col = mix( col, colb, smoothstep(0.45,0.451,n) );\n"
"	col *= pow( 16.0*uv.x*(1.0-uv.x)*uv.y*(1.0-uv.y), 0.1 );\n"
"	gl_FragColor = vec4( col, 1.0 );\n"
"}\n";