uniform float u_color_levels;

uniform float u_gamma;

void fragment() {
    vec3 color = FetchColor(PIXCOORD);

    float gamma = (u_gamma <= 0.0) ? 1.0 : u_gamma;

    color = pow(color, vec3(1.0 / gamma));

    if (u_color_levels > 0.0) {
        color = floor(color * u_color_levels) / (u_color_levels - 1.0);
    }

    color = pow(color, vec3(gamma));
    COLOR = clamp(color, 0.0, 1.0);
}