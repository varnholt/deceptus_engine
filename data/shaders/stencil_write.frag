#version 330 compatibility

uniform sampler2D u_texture_sampler;
uniform float     u_alpha_threshold;

in vec2  interpolated_uv;
out vec4 fragment_color;

void main()
{
    vec4 sampled_color = texture(u_texture_sampler, interpolated_uv);

    // emulate alpha test: discard fragments with alpha below/equal threshold
    if (sampled_color.a <= u_alpha_threshold)
    {
        discard;
    }

    // color output is ignored when stencilOnly=true, but keep it valid
    fragment_color = sampled_color;
}
