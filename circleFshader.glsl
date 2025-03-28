#version 430 core
out vec4 FragColor;
uniform vec2 circleOffset;
uniform float circleRadius;
uniform bool colorSwap;

void main()
{
    vec2 fragCoord = (gl_FragCoord.xy / vec2(700.0, 700.0)) * 2.0 - 1.0;
    float dist = distance(fragCoord, circleOffset);
    
    if (dist < circleRadius) {
        float gradient = dist / circleRadius;
        if (colorSwap) {
            // Red border, green center when moving AND touching line
            FragColor = mix(vec4(0.0, 1.0, 0.0, 1.0), vec4(1.0, 0.0, 0.0, 1.0), gradient);
        } else {
            // Green border, red center (default)
            FragColor = mix(vec4(0.0, 1.0, 0.0, 1.0), vec4(1.0, 0.0, 0.0, 1.0), 1.0 - gradient);
        }
    } else {
        discard;
    }
}
