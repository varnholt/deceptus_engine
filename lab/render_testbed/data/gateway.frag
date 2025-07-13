uniform float time;
uniform float alpha;
uniform float radius;
uniform vec2 resolution;
uniform vec2 center;

void main()
{
    // snap to pixelated coordinate
    const float pixelSize = 3.0; // pixel size in screen pixels
    vec2 pixelCoord = floor(gl_FragCoord.xy / pixelSize) * pixelSize;
    vec2 uv = pixelCoord / resolution;
    
    // plasma
    float c = 0.0;
    c += sin(uv.x * 10.0 + time);
    c += sin((uv.y * 10.0 + time) / 2.0);
    c += sin((uv.x + uv.y) * 10.0 + time);
    c += sin(sqrt(uv.x * uv.x + uv.y * uv.y) * 10.0 + time);
    c = c / 4.0;

    vec3 color = vec3(0.5 + 0.5 * sin(3.0 + c * 2.0));

    float dist = distance(gl_FragCoord.xy, center);

    // radial falloff
    float mask = smoothstep(radius * 0.5, radius, dist);  
    float final_alpha = alpha * (1.0 - mask);             
    vec4 final_color = vec4(color, final_alpha);
    vec4 red = vec4(1,0,0,1);

    gl_FragColor = final_color;
}
