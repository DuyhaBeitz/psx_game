float color_levels = 32;

void fragment() {
    vec3 color = FetchColor(PIXCOORD);

    float gamma = 2.2;

    color = pow(color, vec3(1.0 / gamma));

    if (color_levels > 0.0) {
        color = floor(color * color_levels) / (color_levels - 1.0);
    }

    color = pow(color, vec3(gamma));
    COLOR = clamp(color, 0.0, 1.0);
}