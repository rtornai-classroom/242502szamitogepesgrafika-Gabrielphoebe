#version 430 core
out vec4 FragColor;
uniform vec4 color;
uniform vec2 circleOffset;
uniform float circleRadius = 0.1;
uniform vec2 lineOffset;
uniform float lineYAxis;

void main()
{
    vec2 fragCoord = gl_FragCoord.xy / 700.0 * 2.0 - 1.0;
    float dist = distance(fragCoord, circleOffset);

    if (dist < circleRadius) {
        float gradient = dist / circleRadius;
        FragColor = mix(vec4(0.0, 1.0, 0.0, 1.0), vec4(1.0, 0.0, 0.0, 1.0), gradient);

        // Check if the circle intersects the line
        if (abs(circleOffset.y - lineYAxis) < circleRadius && abs(circleOffset.x) < 0.25) {
            FragColor = mix(vec4(1.0, 0.0, 0.0, 1.0), vec4(0.0, 1.0, 0.0, 1.0), gradient);
        }
    } else {
        discard; // Discard fragments outside the circle
    }
}