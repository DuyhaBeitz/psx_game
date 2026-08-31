uniform vec2 u_target_resolution;

const float bayer4x4[16] = float[16](
     0.0/16.0 - 0.5,  8.0/16.0 - 0.5,  2.0/16.0 - 0.5, 10.0/16.0 - 0.5,
    12.0/16.0 - 0.5,  4.0/16.0 - 0.5, 14.0/16.0 - 0.5,  6.0/16.0 - 0.5,
     3.0/16.0 - 0.5, 11.0/16.0 - 0.5,  1.0/16.0 - 0.5,  9.0/16.0 - 0.5,
    15.0/16.0 - 0.5,  7.0/16.0 - 0.5, 13.0/16.0 - 0.5,  5.0/16.0 - 0.5
);

void fragment() {
    vec3 color = FetchColor(PIXCOORD);

    ivec2 dither_coord = ivec2(TEXCOORD * u_target_resolution);

    int x = dither_coord.x % 4;
    int y = dither_coord.y % 4;
    float dither = bayer4x4[y * 4 + x];

    const float COLOR_STEPS = 32.0;
    color += dither * (1.0 / COLOR_STEPS);
    color = floor(color * (COLOR_STEPS - 1.0) + 0.5) / (COLOR_STEPS - 1.0);

    COLOR = clamp(color, 0.0, 1.0);
}